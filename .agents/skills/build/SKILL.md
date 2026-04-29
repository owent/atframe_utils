---
name: build
description: "Use when: configuring or building atframe_utils with CMake, enabling samples/tests/tools, or selecting crypto/unwind backends."
---

# Build (atframe_utils)

This repo uses **CMake (>= 3.24)**.

## Typical build flow

- Configure (enable unit tests/samples/tools as needed):
  - `cmake .. -DPROJECT_ENABLE_SAMPLE=YES -DPROJECT_ENABLE_UNITTEST=YES -DPROJECT_ENABLE_TOOLS=ON`
- Build:
  - Linux/macOS: `cmake --build .`
  - Windows (MSVC): `cmake --build . --config RelWithDebInfo`

## Run tests via CTest

- `ctest . -V`

## Key CMake options

- `PROJECT_ENABLE_SAMPLE` (NO/YES)
- `PROJECT_ENABLE_UNITTEST` (NO/YES)
- `PROJECT_ENABLE_TOOLS` (NO/YES)
- `LIBUNWIND_ENABLED` (NO/YES)
- `CRYPTO_DISABLED` (NO/YES)
- `CRYPTO_USE_OPENSSL` / `CRYPTO_USE_MBEDTLS`
