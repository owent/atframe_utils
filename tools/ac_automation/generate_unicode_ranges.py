#!/usr/bin/env python3
"""Generate tool-only skip ranges from UnicodeData and DerivedCoreProperties.

Usage: python generate_unicode_ranges.py UnicodeData.txt DerivedCoreProperties.txt output.h
The Unicode license must accompany the generated table. No word list is embedded.
"""
import pathlib
import sys

data = [0] * 0x110000
start = None
for line in pathlib.Path(sys.argv[1]).read_text(encoding="utf-8").splitlines():
    fields = line.split(";")
    cp, name, category = int(fields[0], 16), fields[1], fields[2]
    if name.endswith(", First>"):
        start = cp
        continue
    begin = start if name.endswith(", Last>") else cp
    start = None
    mask = (2 if category.startswith("P") else 0) | (4 if category.startswith("S") else 0)
    mask |= 8 if category in ("Cc", "Cf") else 0
    for i in range(begin, cp + 1):
        data[i] |= mask
for line in pathlib.Path(sys.argv[2]).read_text(encoding="utf-8").splitlines():
    fields = line.split("#", 1)[0].strip().split(";")
    if len(fields) < 2 or fields[1].strip() != "Default_Ignorable_Code_Point":
        continue
    limits = fields[0].strip().split("..")
    for i in range(int(limits[0], 16), int(limits[-1], 16) + 1):
        data[i] |= 8
# White_Space property (25 code points), verified against Unicode 18 PropList.txt.
for begin, end in [(9, 13), (32, 32), (133, 133), (160, 160), (0x1680, 0x1680),
                   (0x2000, 0x200a), (0x2028, 0x2029), (0x202f, 0x202f),
                   (0x205f, 0x205f), (0x3000, 0x3000)]:
    for i in range(begin, end + 1):
        data[i] |= 1
lines = ["// Generated from Unicode 18.0.0 data. See UNICODE-LICENSE.txt.", "#pragma once",
         "#include <config/compile_optimize.h>", "#include <cstdint>", "namespace {", "struct ATFW_UTIL_SYMBOL_LOCAL unicode_range {",
         "  uint32_t first;", "  uint32_t last;", "  uint32_t mask;", "};",
         "static constexpr unicode_range kUnicodeSkipRanges[] = {"]
i = 0
while i < len(data):
    mask, j = data[i], i + 1
    while j < len(data) and data[j] == mask:
        j += 1
    if mask:
        lines.append(f"  {{0x{i:x}, 0x{j - 1:x}, {mask}}},")
    i = j
lines += ["};", "}  // namespace", ""]
output = pathlib.Path(sys.argv[3])
content = "\n".join(lines)
if not output.exists() or output.read_text(encoding="utf-8") != content:
    output.write_bytes(content.encode("utf-8"))
