---
name: testing
description: "Use when: designing, writing, reviewing, or running atframe_utils private-framework unit tests, filtering cases, or diagnosing Windows test startup/PATH."
---

# Unit testing (atframe_utils)

The test executable is `atframe_utils_unit_test`; the private framework lives in `test/frame/`, and cases live in
`test/case/`.

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
