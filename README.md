# atframe_utils

cxx utils code

[![ci-badge]][ci-link] [![codecov badge]][codecov status]

[ci-badge]: https://github.com/atframework/atframe_utils/actions/workflows/main.yml/badge.svg "Github action build status"
[ci-link]:  https://github.com/atframework/atframe_utils/actions/workflows/main.yml "Github action build status"
[codecov badge]: https://codecov.io/gh/owent/atframe_utils/branch/main/graph/badge.svg?token=S6MBY4242I
[codecov status]: https://codecov.io/gh/owent/atframe_utils

## CI Job Matrix

| Target System | Toolchain          | Note                  |
| ------------- | ------------------ | --------------------- |
| Linux         | GCC                | -                     |
| Linux         | GCC                | With MbedTLS          |
| Linux         | Clang              | With libc++           |
| MinGW64       | GCC                | Static linking        |
| MinGW64       | GCC                | Dynamic linking       |
| Windows       | Visual Studio 2022 | Static linking        |
| Windows       | Visual Studio 2022 | Dynamic linking       |
| macOS         | AppleClang         | With libc++           |

## Usage

+ require [cmake][cmake] 3.24.0 or upper
+ require gcc 7.1+/clang 7+/apple clang 12.0+/MSVC(VS2022+)

~~~~~~~~~~bash
# clone and make build directory
git clone --single-branch --depth=1 -b master https://github.com/atframework/atframe_utils.git
mkdir atframe_utils/build && cd atframe_utils/build

# run cmake
# cmake <atframe_utils dir> [options...]
cmake .. -DPROJECT_ENABLE_SAMPLE=YES -DPROJECT_ENABLE_UNITTEST=YES -DPROJECT_ENABLE_TOOLS=ON #  -DCMAKE_INSTALL_PREFIX=<install prefix>

# build
cmake --build . # using clang or gcc
# cmake --build . --config RelWithDebInfo # using MSVC

# run unit test
ctest . -V

# run unit test
cmake --build . --target INSTALL # using clang or gcc
# cmake --build . --config RelWithDebInfo --target INSTALL # using MSVC
~~~~~~~~~~

## Other options

| Option                                     | Description                                                                         |
| ------------------------------------------ | ----------------------------------------------------------------------------------- |
| BUILD\_SHARED\_LIBS=YES\|NO                | [default=NO] Build dynamic library                                                  |
| LIBUNWIND\_ENABLED=YES\|NO                 | [default=NO] Enable and using libunwind for callstack unwind                        |
| LOG\_WRAPPER\_ENABLE\_LUA\_SUPPORT=YES\|NO | [default=YES] Enable lua support for log system                                     |
| LOG\_WRAPPER\_CHECK\_LUA=YES\|NO           | [default=YES] Enable checking for lua support                                       |
| LOG\_WRAPPER\_ENABLE\_STACKTRACE=YES\|NO   | [default=YES] Enable stack trace for log system                                     |
| ENABLE\_MIXEDINT\_MAGIC\_MASK=0-8          | [default=0] Set mixed int mask                                                      |
| CRYPTO\_DISABLED=YES\|NO                   | [default=NO] Disable crypto and DH/ECDH support                                     |
| CRYPTO\_USE\_OPENSSL=YES\|NO               | [default=NO] Using openssl for crypto and DH/ECDH support, and close auto detection |
| CRYPTO\_USE\_MBEDTLS=YES\|NO               | [default=NO] Using mbedtls for crypto and DH/ECDH support, and close auto detection |

[cmake]: https://cmake.org/
