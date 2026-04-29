---
name: testing
description: "Use when: running or writing atframe_utils unit tests with the private test framework, filtering cases, or fixing Windows DLL/PATH test startup issues."
---

# Unit testing (atframe_utils)

This repo uses a **private unit testing framework** (not GoogleTest). The framework is located in `test/frame/`.

## Run tests

The test executable is `atframe_utils_unit_test`.

Common commands:

- Run all tests: `./atframe_utils_unit_test`
- List tests: `./atframe_utils_unit_test -l` / `./atframe_utils_unit_test --list-tests`
- Run a group/case: `./atframe_utils_unit_test -r <group>` or `./atframe_utils_unit_test -r <group>.<case>`
- Filter: `./atframe_utils_unit_test -f "pattern*"` / `./atframe_utils_unit_test --filter "pattern*"`
- Help/version: `./atframe_utils_unit_test -h`, `./atframe_utils_unit_test -v`

## Windows: DLL lookup via PATH

On Windows, running `atframe_utils_unit_test.exe` (or samples/tools) from a build directory may fail if dependent DLLs are not discoverable.

Preferred approach: **prepend those folders to `PATH`** for the current session.

Typical DLL directories in the monorepo/toolset layout:

- `<BUILD_DIR>\\publish\\bin\\<Config>`
- `<REPO_ROOT>\\third_party\\install\\windows-amd64-msvc-19\\bin`

Example (PowerShell):

- `$buildDir = "<BUILD_DIR>"`
- `$cfg = "Debug"`
- `$env:PATH = "$buildDir\\publish\\bin\\$cfg;$buildDir\\publish\\bin;${PWD}\\third_party\\install\\windows-amd64-msvc-19\\bin;" + $env:PATH`
- `Set-Location "$buildDir\\_deps\\atframe_utils\\test\\$cfg"`
- `./atframe_utils_unit_test.exe -r test_manager`

## Writing tests

Test files are under `test/case/`.

Minimal example:

- Include: `frame/test_macros.h`
- Use: `CASE_TEST(group, case)` and `CASE_EXPECT_*` assertions
