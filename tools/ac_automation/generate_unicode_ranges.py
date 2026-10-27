#!/usr/bin/env python3
"""Generate conservative built-in skip ranges and independent display properties.

Usage: python generate_unicode_ranges.py UnicodeData.txt DerivedCoreProperties.txt output.h
       [--prop-list PropList.txt]
PropList.txt defaults to the UnicodeData.txt directory. All inputs must belong to
the same UCD release. The Unicode license must accompany the generated tables.
"""
import argparse
import collections
import functools
import hashlib
import pathlib
import re


CODEPOINT_COUNT = 0x110000
WHITE_SPACE = 1
PUNCTUATION = 2
SYMBOL = 4
INVISIBLE = 8
EXTRA_SYNTAX = {0x005f}
EXTRA_FORMAT_CONTROLS = {0x00ad, 0x200b, 0x2060, 0xfeff}
PROTECTED_PROPERTIES = (
    "Alphabetic", "Ideographic", "Unified_Ideograph", "Radical", "Diacritic", "Extender",
    "Grapheme_Extend", "Grapheme_Link", "Join_Control", "Variation_Selector",
    "Prepended_Concatenation_Mark", "IDS_Unary_Operator", "IDS_Binary_Operator",
    "IDS_Trinary_Operator", "ID_Compat_Math_Start", "ID_Compat_Math_Continue",
)
UnicodeRecord = collections.namedtuple("UnicodeRecord", "category decomposition numeric")


def codepoint_range(text):
    """Parse an inclusive code-point range, including UCD surrogate rows."""
    limits = text.strip().split("..")
    first, last = int(limits[0], 16), int(limits[-1], 16)
    if len(limits) > 2 or first > last or first < 0 or last >= CODEPOINT_COUNT:
        raise ValueError("invalid code-point range: " + text)
    return range(first, last + 1)


def read_properties(path, kind):
    """Read explicit UCD property sets and the release declared in their header."""
    content = path.read_text(encoding="utf-8")
    version = re.search(r"^# " + re.escape(kind) + r"-(\d+\.\d+\.\d+)\.txt\s*$", content, re.MULTILINE)
    if version is None:
        raise ValueError("missing UCD version header: " + str(path))
    properties = collections.defaultdict(set)
    for line in content.splitlines():
        fields = line.split("#", 1)[0].strip().split(";")
        if len(fields) >= 2:
            properties[fields[1].strip()].update(codepoint_range(fields[0]))
    return version[1], properties


def read_unicode_data(path):
    """Expand UnicodeData First/Last records without treating range labels as text."""
    records = {}
    pending = None
    previous = -1
    for line in path.read_text(encoding="utf-8").splitlines():
        fields = line.split(";")
        if len(fields) < 9:
            raise ValueError("short UnicodeData record")
        cp = int(fields[0], 16)
        if cp <= previous or cp >= CODEPOINT_COUNT:
            raise ValueError("unordered/out-of-range UnicodeData record")
        previous = cp
        pieces = fields[5].split()
        if pieces and pieces[0].startswith("<"):
            pieces = pieces[1:]
        mapping = tuple(int(value, 16) for value in pieces)
        if any(value < 0 or value >= CODEPOINT_COUNT for value in mapping):
            raise ValueError("out-of-range UnicodeData decomposition")
        record = UnicodeRecord(fields[2], mapping, any(fields[6:9]))
        if fields[1].endswith(", First>"):
            if pending is not None:
                raise ValueError("nested UnicodeData range")
            pending = (cp, fields[1][:-len(", First>")], record)
            continue
        if fields[1].endswith(", Last>"):
            if pending is None or pending[1] != fields[1][:-len(", Last>")] or pending[2] != record:
                raise ValueError("unmatched UnicodeData range")
            first = pending[0]
            pending = None
        else:
            if pending is not None:
                raise ValueError("unterminated UnicodeData range")
            first = cp
        for value in range(first, cp + 1):
            records[value] = record
    if pending is not None:
        raise ValueError("unterminated UnicodeData range")
    return records


def make_tables(records, core, properties):
    """Keep writing components significant while retaining their display escaping."""
    for name in PROTECTED_PROPERTIES:
        if name not in core and name not in properties:
            raise ValueError("missing protected Unicode property: " + name)
    for name in ("White_Space", "Pattern_Syntax", "Bidi_Control"):
        if name not in properties:
            raise ValueError("missing Unicode property: " + name)
    if "Default_Ignorable_Code_Point" not in core:
        raise ValueError("missing Default_Ignorable_Code_Point property")
    protected = set()
    for name in PROTECTED_PROPERTIES:
        protected.update(core.get(name, ()))
        protected.update(properties.get(name, ()))

    @functools.lru_cache(maxsize=None)
    def decomposition(cp):
        record = records.get(cp)
        if record is None or not record.decomposition:
            return (cp,)
        return tuple(part for value in record.decomposition for part in decomposition(value))

    def writing_component(cp):
        record = records.get(cp)
        return record is None or record.category[0] in "LMN" or record.numeric or cp in protected

    controls = {cp for cp, record in records.items() if record.category == "Cc"}
    formats = properties["Bidi_Control"] | EXTRA_FORMAT_CONTROLS
    base = properties["Pattern_Syntax"] | properties["White_Space"] | controls | formats | EXTRA_SYNTAX
    base = {cp for cp in base if not writing_component(cp) and records[cp].category not in ("Cn", "Co", "Cs")}
    allowed = set(base)
    for cp, record in records.items():
        if record.category[0] in "PSZ" and record.decomposition:
            if all(value in base for value in decomposition(cp)):
                allowed.add(cp)
    allowed = {cp for cp in allowed if not writing_component(cp) and
               not any(writing_component(value) for value in decomposition(cp))}

    skips = bytearray(CODEPOINT_COUNT)
    display = bytearray(CODEPOINT_COUNT)
    for cp, record in records.items():
        if record.category in ("Cc", "Cf"):
            display[cp] |= INVISIBLE
    for cp in core["Default_Ignorable_Code_Point"]:
        display[cp] |= INVISIBLE
    for cp in properties["White_Space"]:
        display[cp] |= WHITE_SPACE
    for cp in allowed:
        category = records[cp].category
        if cp in properties["White_Space"]:
            skips[cp] |= WHITE_SPACE
        if category.startswith("P"):
            skips[cp] |= PUNCTUATION
        if category.startswith("S"):
            skips[cp] |= SYMBOL
        if category == "Cc" or cp in formats:
            skips[cp] |= INVISIBLE
        if skips[cp] == 0:
            raise ValueError("skip scalar has no option category: " + hex(cp))
    return skips, display


def append_ranges(lines, name, masks):
    """Emit sorted, disjoint, inclusive ranges with one property mask per row."""
    lines.append("static constexpr unicode_range " + name + "[] = {")
    first = 0
    while first < len(masks):
        mask = masks[first]
        end = first + 1
        while end < len(masks) and masks[end] == mask:
            end += 1
        if mask != 0:
            lines.append(f"    {{0x{first:x}, 0x{end - 1:x}, {mask}}},")
        first = end
    lines.append("};")


def generate(unicode_data, core_path, prop_path):
    """Generate deterministic header bytes; callers publish only changed content."""
    version, core = read_properties(core_path, "DerivedCoreProperties")
    prop_version, properties = read_properties(prop_path, "PropList")
    if version != prop_version:
        raise ValueError("Unicode property input versions do not match")
    skips, display = make_tables(read_unicode_data(unicode_data), core, properties)
    lines = [f"// Generated from Unicode {version} data. See UNICODE-LICENSE.txt."]
    for path in (unicode_data, core_path, prop_path):
        lines.append("// " + path.name + " SHA-256: " + hashlib.sha256(path.read_bytes()).hexdigest())
    lines += ["#pragma once", "", "#include <config/compile_optimize.h>", "", "#include <cstdint>", "",
              "namespace {", "", "struct ATFW_UTIL_SYMBOL_LOCAL unicode_range {",
              "  uint32_t first;", "  uint32_t last;", "  uint32_t mask;", "};", "",
              "// Bits: whitespace=1, punctuation=2, symbols=4, invisible=8.",
              "// Skip profile: conservative-pattern-syntax-v1. Writing components and their",
              "// compatibility equivalents stay significant; explicit --skip-file is separate.",
              "// clang-format off"]
    append_ranges(lines, "kUnicodeSkipRanges", skips)
    lines += ["", "// Display/trim properties only. Default-ignorables can be writing components:",
              "// escape their display, but never use this table to configure matching skips."]
    append_ranges(lines, "kUnicodeDisplayRanges", display)
    lines += ["// clang-format on", "", "}  // namespace", ""]
    return "\n".join(lines).encode("utf-8")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("unicode_data", type=pathlib.Path)
    parser.add_argument("derived_core", type=pathlib.Path)
    parser.add_argument("output", type=pathlib.Path)
    parser.add_argument("--prop-list", type=pathlib.Path)
    args = parser.parse_args()
    prop_path = args.prop_list if args.prop_list is not None else args.unicode_data.with_name("PropList.txt")
    try:
        content = generate(args.unicode_data, args.derived_core, prop_path)
        if not args.output.exists() or args.output.read_bytes() != content:
            args.output.write_bytes(content)
    except (OSError, ValueError) as error:
        parser.error(str(error))


if __name__ == "__main__":
    main()
