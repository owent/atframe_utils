---
name: build
description: "Use when: configuring or building atframe_utils with CMake, enabling samples/tests/tools, or selecting crypto/unwind backends."
---

# Build (atframe_utils)

This repo uses **CMake (>= 3.24)**.

## Typical build flow

- Resolve `<BUILD_DIR>` first: read the nearest `.vscode/settings.json` for `cmake.buildDirectory`; if absent, infer from
  clangd `--compile-commands-dir=...` or an existing configured build tree; if no user setting is readable, use `build`.
- Keep all build output and agent-generated scratch/log/temp files under `<BUILD_DIR>/...`; use
  `<BUILD_DIR>/_agent_tmp/...` for agent scratch.
- Configure (enable unit tests/samples/tools as needed):
  - `cmake -S . -B <BUILD_DIR> -DPROJECT_ENABLE_SAMPLE=YES -DPROJECT_ENABLE_UNITTEST=YES -DPROJECT_ENABLE_TOOLS=ON`
- Build:
  - Linux/macOS: `cmake --build <BUILD_DIR>`
  - Windows (MSVC): `cmake --build <BUILD_DIR> --config RelWithDebInfo`

## Run tests via CTest

- `ctest --test-dir <BUILD_DIR> -V`

## Key CMake options

- `PROJECT_ENABLE_SAMPLE` (NO/YES)
- `PROJECT_ENABLE_UNITTEST` (NO/YES)
- `PROJECT_ENABLE_TOOLS` (NO/YES)
- `LIBUNWIND_ENABLED` (NO/YES)
- `CRYPTO_DISABLED` (NO/YES)
- `CRYPTO_USE_OPENSSL` / `CRYPTO_USE_MBEDTLS`
