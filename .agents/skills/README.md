# Skills (Agent Playbooks)

This folder contains subproject workflows that agents load on demand. Keep `AGENTS.md` small; put task-specific steps,
commands, caveats, and examples here.

## Contents

| Skill | Description |
| --- | --- |
| `build/` | Configure and build atframe_utils with CMake |
| `testing/` | Run and write private-framework unit tests |
| `ai-agent-maintenance/` | Audit and optimize AI agent prompts, bridge files, and skills |

## When to read what

- If you want to **build**: start with `build/SKILL.md`.
- If you want to **run or write unit tests**: start with `testing/SKILL.md`.
- If you are updating AI agent prompts or skills: see `ai-agent-maintenance/SKILL.md`.

## Maintenance rules

- Folder name and frontmatter `name` must match.
- `description` is the discovery surface: start with `Use when:` and include concrete trigger words.
- Keep each `SKILL.md` focused; move bulky examples or reference material into sibling files when needed.
