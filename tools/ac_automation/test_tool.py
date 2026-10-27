#!/usr/bin/env python3
"""Run deterministic CLI integration checks: test_tool.py EXE SCRATCH_DIRECTORY."""
import pathlib
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
        result = subprocess.run([exe, *map(str, args)], capture_output=True, text=True)
        assert (result.returncode == 0) == ok, (args, result.returncode, result.stdout, result.stderr)
        return result.stdout
    source, image = root / "words.txt", root / "words.ac"
    source.write_text("\ufeff  ab\r\n\n\u3000BC\u3000\n#tag\nx,y\nquote\"slash/\nAB\n", encoding="utf-8")
    run("build", "--input", source, "--output", image, "--nocase", "--skip-space", "--skip-punctuation")
    original = image.read_bytes()
    assert "keywords=5" in run("info", "--input", image)
    run("verify", "--input", image, "--semantic")
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
    image.write_bytes(original + b"trailing")
    run("verify", "--input", image, ok=False)
    assert not list(root.glob("*.tmp.*"))
print("CLI integration checks passed")
