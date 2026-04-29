# atframe_utils Agent Guide

This is the canonical, cross-agent guide for this subproject. Keep it short: put repeatable workflows in
`.agents/skills/*/SKILL.md`, and keep `.github/copilot-instructions.md` / `CLAUDE.md` as lightweight bridges.

**atframe_utils** is a C++ utility library for algorithms, crypto, logging, memory, networking, and common runtime
helpers.

- **Repository**: <https://github.com/atframework/atframe_utils>
- **Languages**: C++ (C++14 minimum, C++17/C++20/C++23 features used when available)

## Project Map

- `include/`: public headers (`algorithm`, `common`, `log`, `memory`, `network`, `nostd`, `time`, etc.).
- `src/`: implementation files.
- `test/`: private unit test framework and test cases.
- `sample/`, `tools/`: examples and utilities.
- `.agents/skills/`: build, testing, and AI-agent maintenance playbooks.

## Always-On Rules

- Respect the user's dirty workspace: inspect current file contents before editing and avoid unrelated reformatting.
- Read the matching `.agents/skills/*/SKILL.md` before build or test work; skills contain commands and edge cases.
- After C++ edits, run `clang-format -i <file>` and verify with `clang-format --dry-run --Werror <file>` when practical.

## C++ Conventions

1. **Namespace**: public code is under `atfw::util`.
2. **Include guards**: use `#pragma once`.
3. **Naming**: classes/functions use `snake_case`; constants use `UPPER_SNAKE_CASE`; member variables use trailing `_`.
4. **Error handling**: use return codes or `nostd::result` types as existing code does.
5. **Anonymous namespace + static**: in `.cpp` files, file-local functions should be inside an anonymous namespace **and**
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
| `build` | Configuring or building with CMake |
| `testing` | Running or writing private test-framework cases |
| `ai-agent-maintenance` | Auditing or optimizing AI agent prompts, bridge files, and skills |

## Agent File Compatibility

- `AGENTS.md` is canonical for tools that support hierarchical agent instructions.
- `.github/copilot-instructions.md` exists only to point VS Code Copilot at this guide and `.agents/skills/`.
- `CLAUDE.md` exists only to point Claude-compatible tools at this guide and `.agents/skills/`.
- Keep skill folder names and frontmatter `name` values identical; descriptions are the discovery surface.
