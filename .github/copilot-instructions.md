# atframe_utils - Copilot Instructions

## Project Overview

**atframe_utils** is a C++ utility library providing common functionality for building high-performance applications. It includes utilities for algorithms, cryptography, logging, memory management, networking, and more.

- **Repository**: https://github.com/atframework/atframe_utils
- **License**: MIT
- **Languages**: C++ (C++14 minimum, C++17/C++20/C++23 features used when available)

## Build System

This project uses **CMake** (minimum version 3.24.0).

### Build Commands

```bash
# Clone and configure
git clone --single-branch --depth=1 -b main https://github.com/atframework/atframe_utils.git
mkdir atframe_utils/build && cd atframe_utils/build

# Configure with unit tests enabled
cmake .. -DPROJECT_ENABLE_SAMPLE=YES -DPROJECT_ENABLE_UNITTEST=YES -DPROJECT_ENABLE_TOOLS=ON

# Build
cmake --build .                          # Linux/macOS (GCC/Clang)
cmake --build . --config RelWithDebInfo  # Windows (MSVC)

# Run tests via CTest
ctest . -V
```

### Key CMake Options

| Option                    | Default | Description                     |
| ------------------------- | ------- | ------------------------------- |
| `BUILD_SHARED_LIBS`       | NO      | Build dynamic library           |
| `PROJECT_ENABLE_SAMPLE`   | NO      | Build sample applications       |
| `PROJECT_ENABLE_UNITTEST` | NO      | Build unit tests                |
| `PROJECT_ENABLE_TOOLS`    | NO      | Build tools                     |
| `LIBUNWIND_ENABLED`       | NO      | Enable libunwind for stacktrace |
| `CRYPTO_DISABLED`         | NO      | Disable crypto/DH/ECDH support  |
| `CRYPTO_USE_OPENSSL`      | NO      | Force OpenSSL for crypto        |
| `CRYPTO_USE_MBEDTLS`      | NO      | Force MbedTLS for crypto        |

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
CASE_MSG_ERROR() << "Error message";

// Test utilities
CASE_THREAD_SLEEP_MS(milliseconds)
CASE_THREAD_YIELD()
```

### Running Tests

The test executable is `atframe_utils_unit_test`.

```bash
# Run all tests
./atframe_utils_unit_test

# List all test cases
./atframe_utils_unit_test -l
./atframe_utils_unit_test --list-tests

# Run specific test group(s) or case(s)
./atframe_utils_unit_test -r <test_group_name>
./atframe_utils_unit_test -r <test_group_name>.<test_case_name>

# Run with filter pattern (supports wildcards)
./atframe_utils_unit_test -f "crypto*"
./atframe_utils_unit_test --filter "sha*"

# Show help
./atframe_utils_unit_test -h
./atframe_utils_unit_test --help

# Show version
./atframe_utils_unit_test -v
./atframe_utils_unit_test --version
```

### Windows: DLL lookup via PATH

On Windows, running `atframe_utils_unit_test.exe` (or samples/tools) from a build directory may fail if dependent DLLs are not discoverable. The DLLs are often under the build output directory (and sometimes a third-party `bin/` directory). The easiest fix is to **prepend those folders to `PATH`** for the current session.

Typical DLL directories:

- `<BUILD_DIR>\\publish\\bin\\<Config>` (project DLLs)
- `<REPO_ROOT>\\third_party\\install\\windows-amd64-msvc-19\\bin` (third-party DLLs in this monorepo/toolset layout)

Example (PowerShell):

```powershell
$buildDir = "<BUILD_DIR>"  # e.g. D:\\workspace\\...\\build_jobs_cmake_tools
$cfg = "Debug"

$env:PATH = "$buildDir\\publish\\bin\\$cfg;$buildDir\\publish\\bin;${PWD}\\third_party\\install\\windows-amd64-msvc-19\\bin;" + $env:PATH
Set-Location "$buildDir\\_deps\\atframe_utils\\test\\$cfg"
./atframe_utils_unit_test.exe -r test_manager
```

### Writing Test Cases

Test files are located in `test/case/`. Example:

```cpp
#include "frame/test_macros.h"
#include "algorithm/sha.h"

CASE_TEST(sha, sha256_basic) {
    std::string input = "hello";
    auto result = atfw::util::hash::sha256(input.data(), input.size());

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
