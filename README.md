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

## Clangd 配置参考

本仓库默认使用 `build_jobs_cmake_tools` 作为编译数据库目录（由 CMake 生成 `compile_commands.json`）。建议保持与当前设置一致：

**.clangd（可选）**

```yaml
CompileFlags:
	CompilationDatabase: build_jobs_cmake_tools

Index:
	Background: Build

Diagnostics:
	UnusedIncludes: Strict
```

**VS Code settings**

```json
{
	"C_Cpp.intelliSenseEngine": "disabled",
	"clangd.enable": true,
	"clangd.arguments": [
		"--compile-commands-dir=${workspaceFolder}/build_jobs_cmake_tools",
		"--background-index",
		"--clang-tidy",
		"--completion-style=detailed",
		"--header-insertion=iwyu",
		"-j=8"
	]
}
```

**MSVC + clangd --query-driver 推荐配置**

当使用 MSVC 工具链时，建议显式配置 `--query-driver` 以便 clangd 正确读取 MSVC 的系统头与内置宏：

```jsonc
{
	"clangd.arguments": [
		// 使用环境变量（推荐，VS 开发者命令行会注入 VCToolsInstallDir）
		"--query-driver=${env:VCToolsInstallDir}bin/Hostx64/x64/cl.exe",

		// 通配符版本（多个版本时会选择第一个匹配项）
		"--query-driver=C:/Program Files/Microsoft Visual Studio/*/Community/VC/Tools/MSVC/*/bin/Hostx64/x64/cl.exe"
	]
}
```

**显式指定 C++ 标准（CMake 配置）**

如果需要固定 `__cplusplus` 版本，可通过 CMake 统一指定标准（示例：C++20）：

```jsonc
{
	"cmake.configureSettings": {
		"CMAKE_CXX_STANDARD": "20",
		"CMAKE_CXX_STANDARD_REQUIRED": "ON"
	}
}
```

说明：只有在 MSVC 下 clangd 不识别 `-std:c++latest` 和 `/std:c++latest` 时，才需要使用以上方式固定标准。使用 `CMAKE_CXX_STANDARD` 通常兼容性更好；`--query-driver` 不影响单独打开的 `.h` 文件，仅影响带编译命令的翻译单元。两者可二选一使用。

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
