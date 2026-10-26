# AC v2 competitive programming cases

`library-checker/in/example_00.in` through `example_03.in` are the four public sample inputs from
[Library Checker: Aho Corasick](https://judge.yosupo.jp/problem/aho_corasick), obtained from
[revision fc2fe8cf3d7b523efd66353fb3b697b4cfc8172e](https://github.com/yosupo06/library-checker-problems/tree/fc2fe8cf3d7b523efd66353fb3b697b4cfc8172e/string/aho_corasick/gen).
The upstream Apache-2.0 license is retained in `LICENSE.library-checker`.

The samples cover duplicate patterns, repeated prefixes, overlapping suffixes and single-pattern failure links.
The tests compare failure links by prefix bytes because the judge uses insertion IDs and AC v2 uses breadth-first IDs.
Matching expectations use independent substring comparison, including original byte positions and keyword IDs,
before and after serialization. Repeated, periodic and Fibonacci families in the test source are independently
constructed cases inspired by the upstream generator categories.

The C++ test reads `example_00.in` through `example_03.in` relative to its own `__FILE__`, using the same source-file
lookup pattern as `evptests.txt`. It does not depend on the process working directory.

The full competitive-programming and Unicode stress corpora are optional local data and are excluded by
`.gitignore`; ordinary tests need only the four sample files and require no downloads. CMake excludes this data
directory from its source glob so locally downloaded upstream generators are not compiled into the unit tests.

The `AC automation stress` CI workflow runs `ci/ac_automation_v2_stress.py` with the existing
`ac_automation_benchmark`. It regenerates all 72 official cases from pinned, SHA-256-verified sources in the build
directory and compares the graph output hashes with the upstream manifest. It also generates an independent
Unicode tokenizer oracle and runs fixed-seed malformed-dump validation. No generated pressure corpus is versioned.

[UVa 1449: Dominating Patterns](https://onlinejudge.org/external/14/1449.pdf) is also useful for occurrence-count
checks. Its counting contract includes overlaps, so AC v2 must use `allow_overlap = true`. UVa 10679 tests substring
existence. Public problem samples do not establish access to the judges' hidden tests; those hidden tests are not
part of this corpus.
