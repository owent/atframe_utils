#!/usr/bin/env python3
# Copyright 2026 atframework
"""Generate pinned AC corpora in a build directory and validate ac_automation_benchmark."""
import argparse
import hashlib
import io
import json
import os
from pathlib import Path
import re
import subprocess
import sys
import time
import urllib.request


REVISION = "fc2fe8cf3d7b523efd66353fb3b697b4cfc8172e"
BASE_URL = "https://raw.githubusercontent.com/yosupo06/library-checker-problems/" + REVISION + "/"
# Upstream files are small; generated inputs and reference outputs are never committed.
UPSTREAM = {
    "random.h": ("common/random.h", "d0ae9470c3a04425e1ab8c39f376a0e707507659b8f5aa3ca06c5b47f193613c"),
    "LICENSE": ("LICENSE", "629e9441670a58ce23132259eaaedfc5d364253b72d9d5c6f75046d3c8b23b77"),
    "info.toml": ("string/aho_corasick/info.toml", "52eef2e729c82b52acc10bf0c6085b897b711fa85e22749b9cc2010989dadb28"),
    "hash.json": ("string/aho_corasick/hash.json", "0ce31383b36a77886ccb283af8157ffacea420e2ac7f0b866e3b5d6a42919506"),
    "gen/bfs.cpp": ("string/aho_corasick/gen/bfs.cpp", "62c29ecc1a294723f0c50e43c4f5fd6acd4c3acc5ca3848b5f4b9723f7712891"),
    "gen/random.cpp": ("string/aho_corasick/gen/random.cpp", "a28f4f31dc8d5ea3a9c1680dcb008092b418e117565bb49999e85cd297a474fc"),
    "gen/short_period.cpp": ("string/aho_corasick/gen/short_period.cpp", "f676cc9d3388184d5c497f2d1652cf7abc355424cc072213c73457783e3dc5d7"),
    "gen/fibonacci.cpp": ("string/aho_corasick/gen/fibonacci.cpp", "fff060cce010aeb5d1ee3199efecdf6ec9135b017b633387cffd4e69b813ef17"),
    "gen/almost_single.cpp": ("string/aho_corasick/gen/almost_single.cpp", "f00376bbe41fde1dddafce24582c3b8938028e65e92c0dfd6fa19d20a962c071"),
}
FAMILIES = (("bfs", 4), ("random", 24), ("short_period", 16), ("fibonacci", 16), ("almost_single", 8))
KUHN_FIXTURE = Path("test/case/ac_automation_v2/UTF-8-test.txt")
KUHN_SHA256 = "b51cfe9a8d2689c90b10a13a3624092d546e0837c6ff835b6e5d713c5749c8c6"
ORACLE_SHA256 = "c5c506ba099c526b5c981322748adb17abde7d7274998551d3e3fdbfba4a3274"
ORACLE_RECORDS = 3363077
PROCESS_TIMEOUT = 180


def digest(data):
    return hashlib.sha256(data).hexdigest()


def write_changed(path, data):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_bytes() != data:
        path.write_bytes(data)


def fetch(path, url, expected):
    if path.exists():
        data = path.read_bytes()
    else:
        request = urllib.request.Request(url, headers={"User-Agent": "atframe-utils-ac-stress"})
        with urllib.request.urlopen(request, timeout=30) as response:
            data = response.read()
    if digest(data) != expected:
        raise ValueError("SHA-256 mismatch: " + str(path))
    write_changed(path, data)
    return data


def execute(command, work, name, output=None):
    stderr = work / "logs" / (name + ".stderr.txt")
    stderr.parent.mkdir(parents=True, exist_ok=True)
    with stderr.open("wb") as errors:
        if output is None:
            result = subprocess.run(command, stdout=subprocess.PIPE, stderr=errors, timeout=PROCESS_TIMEOUT)
        else:
            with output.open("wb") as destination:
                result = subprocess.run(command, stdout=destination, stderr=errors, timeout=PROCESS_TIMEOUT)
    if result.returncode != 0:
        detail = stderr.read_text(encoding="utf-8", errors="replace")[-4096:]
        raise RuntimeError("{} exited {}: {}".format(name, result.returncode, detail))
    return result.stdout if output is None else None


def generate_unicode(kuhn):
    result = io.BytesIO()
    count = 0

    def put(kind, token, valid, amount):
        nonlocal count
        result.write(bytes((kind, len(token), int(valid), amount)) + token)
        count += 1

    for code_point in range(0x110000):
        if 0xD800 <= code_point <= 0xDFFF:
            continue
        for kind, encoding in enumerate(("utf-8", "utf-16-le", "utf-16-be")):
            token = chr(code_point).encode(encoding)
            put(kind, token, True, len(token))
    for position in range(len(kuhn)):
        token = kuhn[position:position + 4]
        length = 0
        for end in range(1, len(token) + 1):
            try:
                scalar = token[:end].decode("utf-8", "strict")
            except UnicodeDecodeError:
                continue
            if len(scalar) == 1:
                length = end
                break
        put(0, token, length > 0, length or 1)
    for token in (b"\xc0\x80", b"\xed\xa0\x80", b"\xf4\x90\x80\x80", b"\xc2", b"\xe2\x82", b"\xf0\x90\x80"):
        put(0, token, False, 1)
    for kind, order in ((1, "little"), (2, "big")):
        for code_point in range(0xD800, 0xE000):
            put(kind, code_point.to_bytes(2, order), False, 2)
        put(kind, b"\0", False, 1)
    data = result.getvalue()
    if count != ORACLE_RECORDS or digest(data) != ORACLE_SHA256:
        raise ValueError("Independent Unicode oracle differs from its fixed expected corpus")
    return data


def run(arguments, summary):
    root = Path(__file__).resolve().parents[1]
    work = arguments.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    benchmark = arguments.benchmark.resolve(strict=True)
    upstream = work / "upstream"
    for name, (source, expected) in UPSTREAM.items():
        fetch(upstream / name, BASE_URL + source, expected)
    official = json.loads((upstream / "hash.json").read_text(encoding="utf-8"))
    write_changed(upstream / "params.h", b"#pragma once\nconst int MIN_N=1;\nconst int MAX_N=1000000;\n"
                  b"const int MIN_S_SUM=1;\nconst int MAX_S_SUM=1000000;\n")
    inputs = work / "in"
    binaries = work / "generators"
    outputs = work / "outputs"
    for directory in (inputs, binaries, outputs):
        directory.mkdir(exist_ok=True)
    samples = root / "test/case/ac_automation_v2/library-checker/in"
    cases = []
    for index in range(4):
        name = "example_{:02d}.in".format(index)
        path = inputs / name
        write_changed(path, (samples / name).read_bytes())
        cases.append(path)
    for family, count in FAMILIES:
        executable = binaries / (family + (".exe" if os.name == "nt" else ""))
        execute([arguments.upstream_cxx, "-std=c++17", "-O2", "-I" + str(upstream),
                 str(upstream / "gen" / (family + ".cpp")), "-o", str(executable)], work, "compile-" + family)
        for seed in range(count if arguments.profile == "full" else 1):
            path = inputs / ("{}_{:02d}.in".format(family, seed))
            execute([str(executable), str(seed)], work, path.stem + "-generate", path)
            cases.append(path)
    expected_count = 72 if arguments.profile == "full" else 9
    if len(cases) != expected_count:
        raise ValueError("Incomplete competitive corpus")
    actual = outputs / "current.out"
    for path in cases:
        summary["current_case"] = path.name
        raw = path.read_bytes().replace(b"\r\n", b"\n")
        if digest(raw) != official[path.name]:
            raise ValueError("Generated input differs from upstream: " + path.name)
        start = time.monotonic()
        execute([str(benchmark), "--graph", str(path)], work, path.stem, actual)
        if digest(actual.read_bytes().replace(b"\r\n", b"\n")) != official[path.stem + ".out"]:
            raise ValueError("AC graph differs from upstream: " + path.name)
        summary["cases"].append({"name": path.name, "seconds": round(time.monotonic() - start, 3)})
        print("{}/{} {} passed".format(len(summary["cases"]), len(cases), path.name), flush=True)
    # Use short random patterns for the original v1/v2 benchmark, avoiding a deep legacy trie.
    summary["current_case"] = "benchmark"
    performance_input = inputs / "random_02.in"
    if arguments.profile == "quick":
        executable = binaries / ("random.exe" if os.name == "nt" else "random")
        execute([str(executable), "2"], work, "benchmark-generate", performance_input)
    raw = performance_input.read_bytes().replace(b"\r\n", b"\n")
    if digest(raw) != official[performance_input.name]:
        raise ValueError("Benchmark input differs from upstream")
    words = work / "benchmark-words.utf8.txt"
    write_changed(words, raw.split(b"\n", 1)[1])
    performance = outputs / "benchmark.csv"
    execute([str(benchmark), str(words)], work, "benchmark", performance)
    summary["benchmark_source"] = performance_input.name
    summary["benchmark_report"] = str(performance.relative_to(work))
    print(performance.read_text(encoding="utf-8").strip(), flush=True)
    summary["current_case"] = "unicode"
    kuhn = (root / KUHN_FIXTURE).read_bytes()
    if digest(kuhn) != KUHN_SHA256:
        raise ValueError("SHA-256 mismatch: " + str(KUHN_FIXTURE))
    oracle = work / "tokenizers.bin"
    write_changed(oracle, generate_unicode(kuhn))
    result = execute([str(benchmark), "--unicode", str(oracle)], work, "unicode")
    if result.decode().strip() != "unicode_records=" + str(ORACLE_RECORDS):
        raise ValueError("Incomplete Unicode validation: " + result.decode(errors="replace"))
    summary["unicode_records"] = ORACLE_RECORDS
    print(result.decode().strip(), flush=True)
    summary["current_case"] = "fuzz"
    result = execute([str(benchmark), "--fuzz"], work, "fuzz")
    summary["fuzz"] = result.decode().strip()
    counts = re.fullmatch(r"mutations=(\d+) accepted=(\d+) rejected=(\d+)", summary["fuzz"])
    if counts is None or int(counts[1]) != 20000 or int(counts[2]) + int(counts[3]) != 20000 or \
            int(counts[2]) == 0 or int(counts[3]) == 0:
        raise ValueError("Incomplete malformed-image validation")
    print(summary["fuzz"], flush=True)
    summary["status"] = "passed"
    summary.pop("current_case", None)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--benchmark", type=Path, required=True)
    parser.add_argument("--work-dir", type=Path, required=True)
    parser.add_argument("--upstream-cxx", default="c++", help="GCC/Clang executable for the pinned C++17 generators")
    parser.add_argument("--profile", choices=("quick", "full"), default="full")
    arguments = parser.parse_args()
    root = Path(__file__).resolve().parents[1]
    arguments.work_dir = arguments.work_dir.resolve()
    if arguments.work_dir == root or (arguments.work_dir.is_relative_to(root) and
                                      not arguments.work_dir.relative_to(root).parts[0].startswith("build")):
        parser.error("--work-dir must be a dedicated build directory, outside the source directories")
    summary = {"revision": REVISION, "profile": arguments.profile, "status": "failed", "cases": []}
    try:
        run(arguments, summary)
    except Exception as error:
        summary["error"] = str(error)
        print(str(error), file=sys.stderr)
    finally:
        arguments.work_dir.mkdir(parents=True, exist_ok=True)
        write_changed(arguments.work_dir / "summary.json", (json.dumps(summary, indent=2) + "\n").encode())
    return 0 if summary["status"] == "passed" else 1


if __name__ == "__main__":
    sys.exit(main())
