# atframe_utils - Copilot Instructions

## Project Overview

**atframe_utils** is a C++ utility library providing common functionality for building high-performance applications. It includes utilities for algorithms, cryptography, logging, memory management, networking, and more.

- **Repository**: https://github.com/atframework/atframe_utils
- **License**: MIT
- **Languages**: C++ (C++14 minimum, C++17/C++20/C++23 features used when available)

## Skills (How-to playbooks)

Operational, copy/paste-friendly guides live in `.github/skills/`:

- Entry point: `.github/skills/README.md`

## Build System

This project uses **CMake** (minimum version 3.24.0).

Build steps and common configuration options are documented in:

- `.github/skills/build.md`

## Directory Structure

```
atframe_utils/
├── include/           # Public headers
│   ├── algorithm/     # Hash, CRC, Base64, SHA, XXTEA, etc.
│   ├── cli/           # Command-line interface utilities
│   ├── common/        # File system, string operations, demangle
│   ├── config/        # Build configuration headers
│   ├── data_structure/# Data structures (WAL, etc.)
│   ├── design_pattern/# Design patterns (singleton, etc.)
│   ├── gsl/           # Guidelines Support Library integration
│   ├── lock/          # Lock primitives and spinlocks
│   ├── log/           # Logging system
│   ├── memory/        # Smart pointers, allocators, LRU cache
│   ├── network/       # Network utilities
│   ├── nostd/         # Backports of C++ standard features
│   ├── random/        # Random number generators
│   ├── string/        # String utilities
│   └── time/          # Time utilities
├── src/               # Implementation files
├── test/              # Unit tests
│   ├── case/          # Test case implementations
│   └── frame/         # Private test framework
├── sample/            # Sample applications
└── tools/             # Utility tools
```

## Unit Testing Framework

This project uses a **private unit testing framework** (not Google Test). The framework is located in `test/frame/`.

### Test Framework Macros

```cpp
// Define a test case
CASE_TEST(test_group_name, test_case_name) {
    // Test implementation
}

// Assertions
CASE_EXPECT_TRUE(condition)
CASE_EXPECT_FALSE(condition)
CASE_EXPECT_EQ(expected, actual)
CASE_EXPECT_NE(val1, val2)
CASE_EXPECT_LT(val1, val2)
CASE_EXPECT_LE(val1, val2)
CASE_EXPECT_GT(val1, val2)
CASE_EXPECT_GE(val1, val2)
CASE_EXPECT_ERROR(message)

// Logging during tests
CASE_MSG_INFO() << "Info message";

### Running and writing tests

See `.github/skills/testing.md`.
    CASE_EXPECT_EQ(32, result.size());
    CASE_MSG_INFO() << "SHA256 hash computed successfully";
}
```

## Key Components

### Algorithm (`include/algorithm/`)

- `base64.h` - Base64 encoding/decoding
- `sha.h` - SHA-1/SHA-256/SHA-384/SHA-512
- `crc.h` - CRC checksums
- `xxtea.h` - XXTEA encryption
- `crypto_cipher.h` - Symmetric ciphers (AES, ChaCha20, etc.)
- `crypto_dh.h` - Diffie-Hellman key exchange
- `crypto_hmac.h` - HMAC authentication

### Logging (`include/log/`)

- `log_wrapper.h` - Main logging interface
- `log_formatter.h` - Log message formatting
- `log_sink_file_backend.h` - File output backend

### Memory (`include/memory/`)

- `rc_ptr.h` - Reference-counted smart pointer
- `lru_map.h` - LRU cache map
- `lru_object_pool.h` - LRU object pool
- `allocator_ptr.h` - Custom allocator support

### Common (`include/common/`)

- `file_system.h` - File system operations
- `string_oprs.h` - String operations
- `demangle.h` - C++ symbol demangling

## Code Formatting

This project uses **clang-format** for code formatting. The `.clang-format` file is located at the project root.

- Style: Based on Google style
- Column limit: 120
- Run formatting: `clang-format -i <file>`

## Coding Conventions

1. **Namespace**: All code is in `atfw::util` namespace
2. **Include guards**: Use `#pragma once`
3. **C++ Standard**: Minimum C++14, with conditional C++17/C++20/C++23 features
4. **Naming**:
   - Classes/structs: `snake_case`
   - Functions: `snake_case`
   - Constants: `UPPER_SNAKE_CASE`
   - Member variables: `snake_case_` (trailing underscore)
5. **Error handling**: Use return codes or `nostd::result` types

## Compiler Support

| Compiler    | Minimum Version |
| ----------- | --------------- |
| GCC         | 7.1+            |
| Clang       | 7+              |
| Apple Clang | 12.0+           |
| MSVC        | VS2022+         |

## Dependencies

- **OpenSSL** or **MbedTLS** (optional, for crypto support)
- **libunwind** (optional, for stack traces)
- **Lua** (optional, for log Lua binding)
