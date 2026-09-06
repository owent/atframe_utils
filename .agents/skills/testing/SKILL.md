---
name: testing
description: "Use when: designing, writing, reviewing, or running atframe_utils private-framework unit tests, filtering cases, or diagnosing Windows test startup/PATH."
---

# Unit testing (atframe_utils)

The test executable is `atframe_utils_unit_test`; the private framework lives in `test/frame/`, and cases live in
`test/case/`. The framework is built as `atframework::test::library` (frame sources) and
`atframework::test::main` (entry point, like `GTest::Main`) by `test/test.build_bin.cmake`;
`atframe_utils_unit_test` links `atframework::test::main` and compiles only `test/case/`. Other projects either link
these targets via `atframe_add_test_executable(... LINK_TEST_LIBRARY|LINK_TEST_MAIN)` or keep compiling
`frame/*.cpp` sources directly — both are supported; public frame symbols use `ATFRAMEWORK_TEST_API` from
`frame/test_framework_export.h`.

Read [test design and acceptance](references/test-design-and-acceptance.md) when planning, writing, or reviewing cases.
It is not needed merely to run a known test command.

## Repository-specific rules

- Inspect the public contract/implementation and the nearest test file before adding a case. Use
  `#include <frame/test_macros.h>` and the existing `CASE_TEST(group, case)` naming/fixture pattern; do not invent a
  GoogleTest-only API or a new fixture layer without a verified need.
- Most utility cases can be pure and in-process. Pass explicit inputs and timepoints to WAL, time, cache, formatter, and
  algorithm code when the API supports them. Use real wall-clock waiting only when wall-clock integration is itself the
  behavior; otherwise use an existing controllable seam or assert a clock-independent invariant.
- For randomness, crypto, filesystem, logging, and platform cases, assert stable public invariants and use current
  platform guards. Use a reproducible seed only through an existing API. Put temporary files and generated artifacts
  under the resolved `<BUILD_DIR>`, and clean up with existing test helpers.
- `CASE_EXPECT_*` is non-fatal. After a failed setup/precondition assertion, guard dependent work or return after
  required cleanup.
- Keep test code minimal, but do not hide decisive data/expected values inside helpers. Apply `engineering-guidelines`
  for C++/CMake style and `change-workflow` for defect RED-GREEN-REFACTOR evidence.

## Resource limits

The framework enforces default resource limits in every test process, applied at startup by
`setup_test_resource_limit()` (`frame/test_resource_limit.h`, included via `frame/test_macros.h`) for the
private runner and the GoogleTest/Boost.Test integrations alike:

- Memory: `min(90% of available physical memory at process start, 8GiB)`. Enforced with a job object on
  Windows (allocations fail once exceeded, i.e. system-level OOM); on other platforms a watchdog samples the
  resident set size every 100ms and kills the process (POSIX `RLIMIT_AS` also counts file mappings and would
  kill mmap-heavy cases spuriously). Ignored entirely under sanitizers (ASan/MSan/TSan/HWASan/DFSan).
- CPU: capped where the OS supports it — at least one logical core kept free on multi-core machines, 90% on
  single-core machines. Throttling never touches CPU affinity (it may conflict with the code under test):
  Windows uses a job object CPU rate hard cap, Linux/Android use a cgroup v2 `cpu.max` / v1
  `cpu.cfs_quota_us` limit applied to a self-created leaf cgroup. When no mechanism is available or the
  cgroup hierarchy is not writable/delegated, no CPU limit is applied and a note is logged.
- Time: 600s per case, 1800s per program; the watchdog kills the test process (exit code 137, with a
  `[ RESOURCE ] ... killing test process` line on stderr) when exceeded.

Overrides: `set_test_resource_limit` (program scope), `set_test_suite_resource_limit`,
`set_test_case_resource_limit` take `test_resource_limit_options`; a zero field keeps the default, a
negative value disables that limit for the scope. Ratio fields are integer permyriad (10000 = 100%), not
floating point. Fetch the stored options with `get_test_resource_limit` / `get_test_suite_resource_limit` /
`get_test_case_resource_limit` to change only some fields, or call
`set_current_test_suite_resource_limit` / `set_current_test_case_resource_limit` inside a running case to
adjust the current scope without naming it (timeout/memory changes apply on the next watchdog tick; CPU is
setup-time only). Per-suite/per-case memory caps are watchdog-only.
`set_test_resource_limit_enabled(false)` disables everything. When a case is killed by a limit, split it or
raise only that case's limit; do not disable limits globally to hide a regression.


## Run tests

Resolve `<BUILD_DIR>` as required by `AGENTS.md`, then prefer CTest for the registered target:

```bash
ctest --test-dir <BUILD_DIR> -R "^atframe_utils\.unit_test$" --output-on-failure
```

Add `-C <CONFIG>` for a verified multi-config generator.

The executable supports:

- List: `atframe_utils_unit_test -l` / `--list-tests`
- Run a group/case: `atframe_utils_unit_test -r <group>` or `-r <group>.<case>`
- Filter: `atframe_utils_unit_test -f "pattern*"` / `--filter "pattern*"`
- Help/version: `-h`, `-v`

Run the exact case first and confirm it was selected, then the registered CTest and broader coverage in proportion to
risk. Read exit status and case/skip counts; a skipped platform/dependency case is not passing coverage.

## Windows startup

Prefer the registered CTest command so the target and working directory match current CMake configuration. If CTest or a
direct run reports missing DLLs, locate the actual executable/DLL output in the current build tree, then prepend those
verified directories to the current process `PATH`. Do not hardcode a parent checkout's `_deps` layout or a third-party
install triplet that the current repository/configuration has not produced.
