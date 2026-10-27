#!/usr/bin/env python3
"""Run deterministic CLI integration checks: test_tool.py EXE SCRATCH_DIRECTORY."""
import pathlib
import subprocess
import shutil
import sys
import tempfile

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
    for fmt in ("dot", "mermaid", "plantuml"):
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
