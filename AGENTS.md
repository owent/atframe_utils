# atframe_utils Agent Guide

This is the canonical, self-contained cross-agent guide for this repository. Keep it short: put repeatable workflows in
`.agents/skills/*/SKILL.md`, keep `CLAUDE.md` as a lightweight bridge, and avoid redundant tool-specific prompt copies.
This repository manages its own AI agent prompts and skills; it must not depend on a parent or sibling repository guide.

**atframe_utils** is a C++ utility library for algorithms, crypto, logging, memory, networking, and common runtime
helpers.

- **Repository**: <https://github.com/atframework/atframe_utils>
- **Languages**: C++ (C++14 minimum, C++17/C++20/C++23 features used when available)

## Project Map

- `include/`: public headers (`algorithm`, `common`, `log`, `memory`, `network`, `nostd`, `time`, etc.).
- `src/`: implementation files.
- `test/`: private unit test framework and test cases.
- `sample/`, `tools/`: examples and utilities.
- `.agents/skills/`: engineering, build, testing, and AI-agent maintenance playbooks.

## Always-On Rules

- Respect the user's dirty workspace: inspect current file contents before editing and avoid unrelated reformatting.
- Start with the current task, nearest instructions, Skill index, and capabilities actually exposed by the active agent
  harness. Load full Skill bodies or tool-specific directories only when the task routes there; do not assume or install
  absent workflows, tools, modes, or extensions.
- Before a nontrivial plan or edit, inspect the relevant code, configs, docs, generated sources, tests, and current
  official docs for mutable external behavior. Separate verified facts from assumptions, then state the smallest plan
  and verification path.
- For test work, derive cases from verified behavior and risk. Exercise a contract-valid workflow with deterministic
  controls, then cover the highest-value boundaries and failures. Do not invent interfaces, fields, environments, or
  root causes; do not shape data only to make a test pass or add redundant cases for counts. When a check fails, fix
  the implementation or the case design; never weaken, skip, or loosen an existing assertion, add a retry, or widen a
  timeout merely to force green. If scope or prerequisites remain unclear, state assumptions and coverage gaps instead
  of claiming completeness.
- Match process to risk: use the shortest verified path for small changes; read `change-workflow` for defects and for
  cross-module behavior, public API/ABI, data model/migration, security, or deployment changes. Keep their scope and
  acceptance in one existing authoritative artifact or active task plan; do not initialize a methodology for ceremony.
- Never unconditionally `touch` or same-content overwrite code/resources consumed by `add_custom_command`,
  `add_custom_target`, `add_executable`, `add_library`, `target_sources`, or another dependency edge, whether generated,
  copied, or non-handwritten. Use content-stable writes and accurate `OUTPUT`/`BYPRODUCTS`/`DEPENDS`/`DEPFILE`; only
  dedicated non-consumed stamp files may be touched.
- Resolve `<BUILD_DIR>` before creating build trees or temporary files: read the nearest `.vscode/settings.json` for
  `cmake.buildDirectory`; if absent, infer from clangd `--compile-commands-dir=...` or an existing configured build
  tree; if no user setting is readable, use `build`.
- Put all CMake build trees, AI scratch files, script output/logs, and temporary data under `<BUILD_DIR>/...`; for agent
  scratch use `<BUILD_DIR>/_agent_tmp/...`. Never create ad-hoc temp files in the repository root.
- Read the matching `.agents/skills/*/SKILL.md` before C++ edit/review, build, or test work.
- After C++ edits, run `clang-format -i <file>` and verify with `clang-format --dry-run --Werror <file>` when practical.

## C++ Conventions

1. **Namespace**: public code is under `atfw::util`.
2. **Include guards**: use `#pragma once`.
3. **Header/API visibility**: public non-template APIs must use `ATFRAMEWORK_UTILS_API` (or the matching `*_API` macro)
   or `ATFW_UTIL_FORCEINLINE`; public template functions defined in headers may use `ATFW_UTIL_SYMBOL_VISIBLE`. Read
   `engineering-guidelines` for the ODR and internal-only rules.
4. **Exported ABI**: keep non-template implementations covered by `ATFRAMEWORK_UTILS_API` or another `*_API` macro in
   `.cpp` files so ABI stays stable across compilers and build options.
5. **Naming**: classes/functions use `snake_case`; constants use `UPPER_SNAKE_CASE`; member variables use trailing `_`.
6. **Error handling**: use return codes or `nostd::result` types as existing code does.
7. **Anonymous namespace + static**: in `.cpp` files, file-local functions should be inside an anonymous namespace **and**
   keep the `static` keyword.

   ```cpp
   namespace {
   static void my_helper() { /* ... */ }
   }  // namespace
   ```

## Skill Routing

Read the matching `.agents/skills/*/SKILL.md` before specialized work:

| Skill | Use when |
| --- | --- |
| `engineering-guidelines` | Writing/reviewing C++, header template visibility, or exported API ABI |
| `change-workflow` | Diagnosing defects or delivering nontrivial/high-risk changes with a reviewable contract |
| `build` | Configuring or building with CMake |
| `testing` | Running or writing private test-framework cases |
| `ai-agent-maintenance` | Auditing or optimizing AI agent prompts, bridge files, and skills |

## Agent File Compatibility

- `AGENTS.md` is canonical for tools that support hierarchical agent instructions.
- `.agents/skills/` is the portable project skill location; keep each `SKILL.md` focused and self-contained.
- Do not maintain `.github/copilot-instructions.md` copies when `AGENTS.md` and `.agents/skills/` cover the same rules.
- `CLAUDE.md` exists only to point Claude-compatible tools at this guide and `.agents/skills/`.
- Do not make this repository depend on root, sibling, or vendored-submodule prompt files.
- Keep skill folder names and frontmatter `name` values identical; descriptions are the discovery surface.
