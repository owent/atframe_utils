#!/usr/bin/env python3
"""Run deterministic CLI integration checks: test_tool.py EXE SCRATCH_DIRECTORY."""
import pathlib
import json
import os
import re
import subprocess
import shutil
import sys
import tempfile
import xml.etree.ElementTree as ET


FORMATS = ("dot", "mermaid", "plantuml", "d2", "nomnoml", "graphml")
GRAPHML = "{http://graphml.graphdrawing.org/xmlns}"
YWORKS = "{http://www.yworks.com/xml/graphml}"


def windows_console_worker(executable, image, section, code_page, result_path):
    """Read actual console cells; a stdout pipe does not exercise CRT console conversion."""
    import ctypes
    import ctypes.wintypes as wintypes

    class Coord(ctypes.Structure):
        _fields_ = [("X", ctypes.c_short), ("Y", ctypes.c_short)]

    kernel = ctypes.WinDLL("kernel32", use_last_error=True)
    kernel.GetStdHandle.argtypes = [wintypes.DWORD]
    kernel.GetStdHandle.restype = wintypes.HANDLE
    kernel.SetConsoleOutputCP.argtypes = [wintypes.UINT]
    kernel.SetConsoleOutputCP.restype = wintypes.BOOL
    kernel.GetConsoleOutputCP.restype = wintypes.UINT
    kernel.SetConsoleScreenBufferSize.argtypes = [wintypes.HANDLE, Coord]
    kernel.SetConsoleScreenBufferSize.restype = wintypes.BOOL
    kernel.ReadConsoleOutputCharacterW.argtypes = [wintypes.HANDLE, wintypes.LPWSTR, wintypes.DWORD,
                                                  Coord, ctypes.POINTER(wintypes.DWORD)]
    kernel.ReadConsoleOutputCharacterW.restype = wintypes.BOOL
    handle = kernel.GetStdHandle(-11 & 0xffffffff)  # STD_OUTPUT_HANDLE
    width, height = 512, 256
    assert kernel.SetConsoleOutputCP(int(code_page)), ctypes.get_last_error()
    assert kernel.SetConsoleScreenBufferSize(handle, Coord(width, height)), ctypes.get_last_error()
    with open("CONOUT$", "wb", buffering=0) as console:
        completed = subprocess.run([executable, "print", "--input", image, "--section", section],
                                   stdout=console, stderr=subprocess.PIPE)
    screen = ctypes.create_unicode_buffer(width * height)
    count = wintypes.DWORD()
    assert kernel.ReadConsoleOutputCharacterW(handle, screen, len(screen) - 1, Coord(0, 0),
                                              ctypes.byref(count)), ctypes.get_last_error()
    result = {"returncode": completed.returncode, "stderr": completed.stderr.decode("utf-8", "backslashreplace"),
              "code_page": kernel.GetConsoleOutputCP(), "screen": screen[:count.value].rstrip()}
    pathlib.Path(result_path).write_text(json.dumps(result, ensure_ascii=True), encoding="utf-8")


def check_windows_console(executable, image, section, code_page, root):
    result_path = root / "console-{}-{}.json".format(section, code_page)
    startup = subprocess.STARTUPINFO()
    startup.dwFlags = subprocess.STARTF_USESHOWWINDOW
    startup.wShowWindow = 0  # SW_HIDE: never open a visible helper window.
    subprocess.run([sys.executable, str(pathlib.Path(__file__).resolve()), "--console-worker",
                    executable, str(image), section, str(code_page), str(result_path)],
                   startupinfo=startup, creationflags=subprocess.CREATE_NEW_CONSOLE, check=True, timeout=30)
    result = json.loads(result_path.read_text(encoding="utf-8"))
    assert result["returncode"] == 0, result
    assert result["stderr"] == "", result["stderr"]
    assert result["code_page"] == code_page, result["code_page"]
    return result["screen"]


if os.name == "nt" and sys.argv[1:2] == ["--console-worker"]:
    windows_console_worker(*sys.argv[2:])
    sys.exit(0)


def topology(path, fmt):
    text = path.read_text(encoding="utf-8")
    if fmt == "graphml":
        graph = ET.fromstring(text).find(GRAPHML + "graph")
        assert graph is not None and graph.attrib["edgedefault"] == "directed"
        nodes = {int(n.attrib["id"][1:]) for n in graph.findall(GRAPHML + "node")}
        edges = set()
        for edge in graph.findall(GRAPHML + "edge"):
            fields = {d.attrib["key"]: d.text for d in edge.findall(GRAPHML + "data")}
            kind = fields["kind"]
            assert edge.find(".//" + YWORKS + "LineStyle").attrib["type"] == ("dashed" if kind == "fail" else "line")
            assert edge.find(".//" + YWORKS + "BendStyle").attrib["smoothed"] == "false"
            edges.add((int(edge.attrib["source"][1:]), int(edge.attrib["target"][1:]), kind))
        return nodes, edges
    node_patterns = {
        "dot": r'^\s*n(\d+) \[label="node ',
        "mermaid": r'^\s*n(\d+)\["node ',
        "plantuml": r'^state "node .*" as n(\d+)$',
        "d2": r'^n(\d+): "node ',
        "nomnoml": r'^\[<state id=n(\d+)>node ',
    }
    nodes = {int(n) for n in re.findall(node_patterns[fmt], text, re.MULTILINE)}
    edges = set()
    for line in text.splitlines():
        if fmt == "dot":
            match = re.match(r'\s*n(\d+) -> n(\d+) \[', line)
            failure = "style=dashed" in line
        elif fmt == "mermaid":
            match = re.match(r'\s*n(\d+) (?:-->.*|-. fail .->) n(\d+)$', line)
            failure = "-. fail .->" in line
        elif fmt == "plantuml":
            match = re.match(r'n(\d+) (?:-->|\.\.>) n(\d+)', line)
            failure = "..>" in line
        elif fmt == "d2":
            match = re.match(r'n(\d+) -> n(\d+): ', line)
            failure = "stroke-dash" in line
        else:
            match = re.match(r'\[n(\d+)\] .*? (?:-->|->) \[n(\d+)\]$', line)
            failure = " --> " in line
        if match is not None:
            edges.add((int(match[1]), int(match[2]), "fail" if failure else "trie"))
    return nodes, edges


def render_small_graph(path, fmt):
    if fmt == "d2" and shutil.which("d2"):
        subprocess.run(["d2", "validate", str(path)], check=True, capture_output=True)
        svg = path.with_suffix(".svg")
        subprocess.run(["d2", "--layout", "dagre", str(path), str(svg)], check=True, capture_output=True)
        ET.parse(svg)
    module = os.environ.get("AC_AUTOMATION_NOMNOML_MODULE")
    if fmt == "nomnoml" and module is not None:
        code = ("const fs=require('fs'),n=require(process.argv[1]);"
                "fs.writeFileSync(process.argv[3],n.renderSvg(fs.readFileSync(process.argv[2],'utf8')));")
        svg = path.with_suffix(".svg")
        subprocess.run(["node", "-e", code, module, str(path), str(svg)], check=True, capture_output=True)
        ET.parse(svg)

exe = str(pathlib.Path(sys.argv[1]).resolve())
scratch = pathlib.Path(sys.argv[2])
scratch.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="ac-tool-", dir=scratch) as directory:
    root = pathlib.Path(directory)
    def run(*args, ok=True):
        result = subprocess.run([exe, *map(str, args)], capture_output=True, text=True, encoding="utf-8")
        assert (result.returncode == 0) == ok, (args, result.returncode, result.stdout, result.stderr)
        return result.stdout
    replacement_file = root / "replacements.txt"
    replacement_file.write_text("65 66\n67 68\nE5A3B9 E4B880\n78 65\n", encoding="ascii")
    replacement_words = root / "replacement-words.txt"
    replacement_words.write_text("abc\nac\naec\nafc\nahc\n壹\n-\n", encoding="utf-8")
    replacement_skips = root / "replacement-skips.txt"
    replacement_skips.write_text("b-", encoding="ascii")
    replacement_image = root / "replacement.ac"
    run("build", "--input", replacement_words, "--output", replacement_image,
        "--replacement-file", replacement_file, "--skip-file", replacement_skips)
    replacement_report = json.loads(run("print", "--input", replacement_image, "--format", "json"))
    assert replacement_report["config"]["version"] == "2.1"
    assert replacement_report["config"]["replacements"] is True
    assert replacement_report["config"]["nocase"] is False
    assert replacement_report["config"]["replacement_rules"] == 4
    assert replacement_report["folds"] == []
    assert [word["text"] for word in replacement_report["keywords"]] == ["abc", "aec", "ahc", "壹"]
    assert {rule["source"]["text"]: rule["target"]["text"] for rule in replacement_report["replacements"]} == {
        "e": "f", "g": "h", "壹": "一", "x": "f"}
    assert "version=2.1" in run("info", "--input", replacement_image)
    run("verify", "--input", replacement_image, "--semantic")
    printed_replacements = json.loads(run("print", "--input", replacement_image, "--section", "replacements",
                                         "--format", "json"))
    assert printed_replacements == {"replacements": replacement_report["replacements"]}
    replacement_symbols = {entry["id"]: entry for entry in replacement_report["symbols"]}
    for symbol in replacement_symbols.values():
        target = replacement_symbols[symbol["canonical_id"]]
        assert target["canonical_id"] == target["id"]
    replacement_dot = root / "replacement.dot"
    run("visualize", "--input", replacement_image, "--output", replacement_dot, "--format", "dot")
    graph_text = replacement_dot.read_text(encoding="utf-8")
    assert 'label="f"' in graph_text and 'label="e"' not in graph_text
    assert 'label="一"' in graph_text and 'label="壹"' not in graph_text
    cycle_words = root / "cycle-words.txt"
    cycle_words.write_text("agc\naec\nafc\nadc\n", encoding="ascii")
    replacement_file.write_text("67 65\n64 67\n65 66\n66 67\n", encoding="ascii")
    cycle_image = root / "cycle.ac"
    run("build", "--input", cycle_words, "--output", cycle_image, "--replacement-file", replacement_file)
    run("verify", "--input", cycle_image, "--semantic")
    cycle_report = json.loads(run("print", "--input", cycle_image, "--format", "json"))
    assert [word["text"] for word in cycle_report["keywords"]] == ["agc"]
    assert {rule["source"]["text"]: rule["target"]["text"] for rule in cycle_report["replacements"]} == {
        "d": "e", "f": "e", "g": "e"}
    cycle_symbols = {entry["id"]: entry for entry in cycle_report["symbols"]}
    for symbol in cycle_symbols.values():
        if symbol["text"] in ("d", "e", "f", "g"):
            assert cycle_symbols[symbol["canonical_id"]]["text"] == "e"
        assert cycle_symbols[symbol["canonical_id"]]["canonical_id"] == symbol["canonical_id"]
    for malformed in ("78 79\n78 7A\n", "78 ZZ\n", "78 7\n", "78\n", "78 79 7A\n"):
        replacement_file.write_text(malformed, encoding="ascii")
        rejected_image = root / "rejected-replacement.ac"
        run("build", "--input", replacement_words, "--output", rejected_image,
            "--replacement-file", replacement_file, ok=False)
        assert not rejected_image.exists()
    replacement_file.write_text("65 46\n66 67\n", encoding="ascii")
    replacement_words.write_text("E\nf\ng\n", encoding="ascii")
    composed_image = root / "composed.ac"
    run("build", "--input", replacement_words, "--output", composed_image,
        "--replacement-file", replacement_file, "--nocase")
    composed = json.loads(run("print", "--input", composed_image, "--format", "json"))
    assert len(composed["keywords"]) == 1 and composed["keywords"][0]["text"] == "E"
    assert len(composed["folds"]) == 26
    assert {rule["source"]["text"]: rule["target"]["text"] for rule in composed["replacements"]} == {
        "e": "g", "f": "g"}
    run("verify", "--input", composed_image, "--semantic")
    source, image = root / "words.txt", root / "words.ac"
    source.write_text("\ufeff  ab\r\n\n\u3000BC\u3000\n#tag\nx,y\nquote\"slash/\nAB\n", encoding="utf-8")
    run("build", "--input", source, "--output", image, "--nocase", "--skip-space", "--skip-punctuation")
    original = image.read_bytes()
    assert "keywords=5" in run("info", "--input", image)
    run("verify", "--input", image, "--semantic")
    # Printed records describe the stored bytes, not the normalized trie spelling.
    report = json.loads(run("print", "--input", image, "--format", "json"))
    assert set(report) == {"config", "keywords", "skips", "folds", "replacements", "symbols"}
    assert report["replacements"] == [] and report["config"]["replacements"] is False
    assert [entry["text"] for entry in report["keywords"]] == ["ab", "BC", "#tag", "x,y", 'quote"slash/']
    assert [bytes.fromhex(entry["hex"]) for entry in report["keywords"]] == [
        word.encode("utf-8") for word in ("ab", "BC", "#tag", "x,y", 'quote"slash/')]
    assert [entry["id"] for entry in report["keywords"]] == list(range(5))
    config = report["config"]
    assert config["nocase"] is True and config["version"] == "2.0"
    assert config["keywords"] == 5 and config["symbol_index_bytes"] == 2052
    assert config["skip_symbols"] == len(report["skips"]) > 0
    assert config["case_fold_rules"] == len(report["folds"]) == 26
    assert config["symbols"] == len(report["symbols"])
    symbols = {entry["id"]: entry for entry in report["symbols"]}
    assert [entry["id"] for entry in report["symbols"]] == list(range(1, config["symbols"] + 1))
    assert {entry["hex"] for entry in report["skips"]} >= {"20", "0A", "E38080", "2D"}
    assert {entry["hex"] for entry in report["skips"]} == {
        entry["hex"] for entry in report["symbols"] if entry["skipped"]}
    for entry in report["folds"]:
        assert entry["source"]["hex"] == symbols[entry["source_id"]]["hex"]
        assert entry["target"]["hex"] == symbols[entry["target_id"]]["hex"]
        assert symbols[entry["source_id"]]["folded_id"] == entry["target_id"]
    for section in report:
        selected = json.loads(run("print", "--input", image, "--section", section, "--format", "json"))
        assert selected == {section: report[section]}
    printed = root / "printed.txt"
    text = run("print", "--input", image)
    assert all("[{}]".format(section) in text for section in report)
    assert 'text="\\n"' in text and 'text="quote\\"slash/"' in text
    run("print", "--input", image, "--output", printed)
    assert printed.read_text(encoding="utf-8") == text
    run("print", "--input", image, "--output", printed, ok=False)
    assert printed.read_text(encoding="utf-8") == text
    for extra in (("--section", "bad"), ("--format", "dot"), ("--max-file-bytes", "1"),
                  ("--max-memory-bytes", "1"), ("--section", "keywords", "--section", "skips")):
        rejected = root / "rejected-print.json"
        run("print", "--input", image, "--output", rejected, *extra, ok=False)
        assert not rejected.exists()
    # Configuration-only models include symbols absent from every trie edge.
    empty_source = root / "empty.txt"
    empty_source.write_bytes(b"")
    skip_file = root / "skips.txt"
    skip_file.write_bytes(b"\x00\t\n \x1b" + "\u200b\U000e0001".encode("utf-8"))
    fold_file = root / "folds.txt"
    fold_file.write_text("FF 6100\n", encoding="ascii")
    configured = root / "configured.ac"
    run("build", "--input", empty_source, "--output", configured, "--nocase",
        "--skip-file", skip_file, "--case-fold-file", fold_file)
    opaque = json.loads(run("print", "--input", configured, "--format", "json"))
    assert opaque["keywords"] == [] and len(opaque["folds"]) == 27
    assert {entry["hex"] for entry in opaque["skips"]} == {"00", "09", "0A", "20", "1B", "E2808B", "F3A08081"}
    rule = next(entry for entry in opaque["folds"] if entry["source"]["hex"] == "FF")
    assert rule["target"]["hex"] == "6100" and rule["target"]["text"] == "a\x00"
    opaque_text = run("print", "--input", configured)
    assert "\x1b" not in opaque_text and "\U000e0001" not in opaque_text and "\\uDB40\\uDC01" in opaque_text
    plain = root / "empty.ac"
    run("build", "--input", empty_source, "--output", plain)
    empty = json.loads(run("print", "--input", plain, "--format", "json"))
    assert all(empty[section] == [] for section in ("keywords", "skips", "folds", "replacements", "symbols"))
    assert empty["config"]["nocase"] is False and empty["config"]["symbols"] == 0
    # Built-in profiles preserve writing components even when Unicode calls them
    # symbols/default-ignorables. Explicit --skip-file remains an exact user override.
    protected = set("Aa09αЖאاकア가一⼀⺀〆々㍿①é^`ᅟᅠㅤﾠ∂" + "\u0301\u034f\u200c\u200d\ufe0f\U000e0001")
    profiles = {
        "--skip-space": set(" \t\r\n\u3000"),
        "--skip-punctuation": set("*-/_,。？—"),
        "--skip-symbols": set("+$☀"),
        "--skip-invisible": set("\x00\t\n\u00ad\u200b\u2060\ufeff"),
    }
    profile_sets = []
    for option, required in profiles.items():
        profile_image = root / (option[2:] + ".ac")
        run("build", "--input", empty_source, "--output", profile_image, option)
        entries = json.loads(run("print", "--input", profile_image, "--section", "skips", "--format", "json"))["skips"]
        tokens = {bytes.fromhex(entry["hex"]).decode("utf-8") for entry in entries}
        assert required <= tokens, (option, required - tokens)
        assert not (tokens & protected), (option, tokens & protected)
        profile_sets.append(tokens)
    builtin = root / "builtin-safe.ac"
    run("build", "--input", empty_source, "--output", builtin, *profiles)
    entries = json.loads(run("print", "--input", builtin, "--section", "skips", "--format", "json"))["skips"]
    combined = {bytes.fromhex(entry["hex"]).decode("utf-8") for entry in entries}
    assert combined == set.union(*profile_sets)
    exact_file = root / "builtin-exact.txt"
    exact_file.write_bytes("".join(sorted(combined)).encode("utf-8"))
    explicit = root / "builtin-explicit.ac"
    run("build", "--input", empty_source, "--output", explicit, "--skip-file", exact_file)
    assert builtin.read_bytes() == explicit.read_bytes()
    visible_source = root / "writing-components.txt"
    visible_word = "\u115fa\u034f\u200c\ufe0fb\u115f"
    visible_source.write_text(visible_word + "\n", encoding="utf-8")
    visible_image = root / "writing-components.ac"
    run("build", "--input", visible_source, "--output", visible_image, *profiles)
    visible_report = run("print", "--input", visible_image, "--format", "json")
    assert all("\\u{:04X}".format(ord(char)) in visible_report for char in "\u115f\u034f\u200c\ufe0f")
    parsed_visible = json.loads(visible_report)
    assert parsed_visible["keywords"][0]["text"] == visible_word
    assert all(not entry["skipped"] for entry in parsed_visible["symbols"] if entry["text"] in protected)
    visible_graph = root / "writing-components.dot"
    run("visualize", "--input", visible_image, "--output", visible_graph, "--format", "dot")
    assert "0xE1 0x85 0x9F" in visible_graph.read_text(encoding="utf-8")
    fallback_source = root / "fallback.txt"
    fallback_word = "汉字😀𠀀\ue219"
    fallback_source.write_text(fallback_word + "\n", encoding="utf-8")
    fallback_image = root / "fallback.ac"
    run("build", "--input", fallback_source, "--output", fallback_image)
    fallback = run("print", "--input", fallback_image, "--section", "keywords", "--format", "json")
    assert "\\uD83D\\uDE00\\uD840\\uDC00\\uE219" in fallback
    fallback_record = json.loads(fallback)["keywords"][0]
    assert fallback_record["text"] == fallback_word
    assert bytes.fromhex(fallback_record["hex"]) == fallback_word.encode("utf-8")
    if os.name == "nt":
        # These are valid UTF-8; GBK console output previously garbled them and stopped mid-record.
        console_source = root / "console.txt"
        console_source.write_text("001工程\n007手机防盗软件任意显软件\n汉字😀𠀀\ue219\n" + "中" * 6000 + "\nlast\n",
                                  encoding="utf-8")
        console_skips = root / "console-skips.txt"
        console_skips.write_text("-*/⼀", encoding="utf-8")
        console_image = root / "console.ac"
        run("build", "--input", console_source, "--output", console_image, "--skip-file", console_skips)
        for code_page in (936, 65001):
            text = check_windows_console(exe, console_image, "keywords", code_page, root)
            assert 'text="001工程"' in text and 'text="007手机防盗软件任意显软件"' in text
            assert 'text="汉字\\uD83D\\uDE00\\uD840\\uDC00\\uE219"' in text and 'text="last"' in text
            assert text.count("{id=") == 5 and "中" * 6000 in text
            skips = check_windows_console(exe, console_image, "skips", code_page, root)
            assert skips.count("{id=") == 4 and 'text="⼀"' in skips
            assert all('text="{}"'.format(char) not in skips for char in "一中文工程")
    for fmt in FORMATS:
        output = root / ("graph." + fmt)
        run("visualize", "--input", image, "--output", output, "--format", fmt, "--utf8-labels")
        text = output.read_text(encoding="utf-8")
        assert "node 0" in text and "fail" in text
    raw = root / "raw.ac"
    run("build", "--input", source, "--output", raw, "--ignore-case")
    escaped = root / "escaped.dot"
    run("visualize", "--input", raw, "--output", escaped, "--format", "dot", "--label-format", "utf8")
    assert "0x22" in escaped.read_text(encoding="utf-8")
    if shutil.which("dot"):
        subprocess.run(["dot", "-Tsvg", str(escaped), "-o", str(root / "graph.svg")], check=True, capture_output=True)
    run("build", "--input", source, "--output", image, ok=False)
    assert image.read_bytes() == original
    for extra in (("--unknown",), ("--nocase", "garbage"), ("--input", source), ("--output=wrong",)):
        output = root / "must-not-exist.ac"
        run("build", "--input", source, "--output", output, *extra, ok=False)
        assert not output.exists()
    output = root / "limited.dot"
    run("visualize", "--input", image, "--output", output, "--format", "dot", "--max-nodes", "1", ok=False)
    assert not output.exists()
    run("visualize", "--input", image, "--output", output, "--format", "dot", "--max-depth", "0")
    assert "node 0" in output.read_text()
    subtree = root / "subtree.dot"
    run("visualize", "--input", image, "--output", subtree, "--format", "dot",
        "--root-id", "1", "--max-depth", "0", "--max-nodes", "1")
    assert "node 1" in subtree.read_text() and "node 0" not in subtree.read_text()
    for option, number in (("--max-nodes", "0"), ("--max-nodes", "18446744073709551616"),
                           ("--root-id", "4294967295"), ("--max-depth", "4294967296"),
                           ("--max-depth", "invalid")):
        rejected = root / "invalid-bound.dot"
        run("visualize", "--input", image, "--output", rejected, "--format", "dot", option, number, ok=False)
        assert not rejected.exists()
    # Independent prefix/suffix definitions exercise interleaved BFS subtrees in every syntax.
    words = ["aba", "abac", "abb", "abcd", "baa", "bab", "baca", "bc", "cab", "caba", "敏感", "感"]
    source.write_text("\n".join(words) + "\n", encoding="utf-8")
    branches = root / "branches.ac"
    run("build", "--input", source, "--output", branches)
    prefixes = {""} | {word[:end] for word in words for end in range(1, len(word) + 1)}
    ordered = sorted(prefixes, key=lambda word: (len(word), word.encode("utf-8")))
    ids = {word: index for index, word in enumerate(ordered)}
    full_depth = max(map(len, words))
    for index, (prefix, depth) in enumerate((("", full_depth), ("a", 1), ("ab", 2), ("b", full_depth), ("c", 0))):
        chosen = {word for word in prefixes if word.startswith(prefix) and len(word) <= len(prefix) + depth}
        expected_nodes = {ids[word] for word in chosen}
        expected_edges = {(ids[word[:-1]], ids[word], "trie") for word in chosen if word != prefix}
        for word in chosen - {""}:
            suffix = max((part for part in prefixes if part != word and word.endswith(part)), key=len)
            if suffix in chosen:
                expected_edges.add((ids[word], ids[suffix], "fail"))
        for fmt in FORMATS:
            output = root / ("branches-{}.{}".format(index, fmt))
            run("visualize", "--input", branches, "--output", output, "--format", fmt,
                "--root-id", ids[prefix], "--max-depth", depth)
            assert topology(output, fmt) == (expected_nodes, expected_edges), (prefix, depth, fmt)
            if index == 0:
                assert "敏" in output.read_text(encoding="utf-8")
                render_small_graph(output, fmt)
    hexadecimal = root / "hex.d2"
    run("visualize", "--input", branches, "--output", hexadecimal, "--format", "d2", "--label-format", "hex")
    assert "敏" not in hexadecimal.read_text(encoding="utf-8") and "0xE6" in hexadecimal.read_text(encoding="utf-8")
    # Unsafe punctuation, malformed display bytes and XML noncharacters remain literal hex labels.
    source.write_text('x"[]<>;&:#\\\n\u3000x\ufffey\uffffz\ufdd0\n', encoding="utf-8")
    special = root / "special.ac"
    run("build", "--input", source, "--output", special)
    special_print = json.loads(run("print", "--input", special, "--section", "keywords", "--format", "json"))
    assert [entry["text"] for entry in special_print["keywords"]] == ['x"[]<>;&:#\\', 'x\ufffey\uffffz\ufdd0']
    unicode_print = json.loads(run("print", "--input", branches, "--section", "keywords", "--format", "json"))
    assert [entry["text"] for entry in unicode_print["keywords"]] == words
    for fmt in FORMATS:
        output = root / ("special." + fmt)
        run("visualize", "--input", special, "--output", output, "--format", fmt)
        text = output.read_text(encoding="utf-8")
        assert "0x22" in text and "0xEF 0xBF 0xBE" in text and "\ufffe" not in text
        topology(output, fmt)
        render_small_graph(output, fmt)
    source.write_bytes(b"good\ninvalid\xff\n")
    output = root / "invalid.ac"
    run("build", "--input", source, "--output", output, ok=False)
    assert not output.exists()
    image.write_bytes(original[:-1])
    run("verify", "--input", image, ok=False)
    run("print", "--input", image, "--format", "json", ok=False)
    image.write_bytes(original + b"trailing")
    run("verify", "--input", image, ok=False)
    run("print", "--input", image, "--format", "json", ok=False)
    assert not list(root.glob("*.tmp.*"))
print("CLI integration checks passed")
