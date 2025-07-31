include_guard(GLOBAL)

# Check mkstemp/_mktemp/_mktemp_s for file system
include(CheckCXXSourceCompiles)
include(CheckCSourceCompiles)
include(CMakeDependentOption)
include(CMakePushCheckState)

include("${CMAKE_CURRENT_LIST_DIR}/FetchToolset.cmake")

include("${ATFRAMEWORK_CMAKE_TOOLSET_DIR}/Import.cmake")
include(IncludeDirectoryRecurse)
include(EchoWithColor)

# 默认配置选项
# ######################################################################################################################
math(EXPR ATFRAMEWORK_UTILS_API_LEVEL "1000 * ${PROJECT_VERSION_MAJOR} + ${PROJECT_VERSION_MINOR}"
     OUTPUT_FORMAT DECIMAL)
set(ATFRAMEWORK_UTILS_ABI_TAG
    "v${ATFRAMEWORK_UTILS_API_LEVEL}"
    CACHE STRING "ABI tag for libatframe_utils.")

if(ANDROID
   OR CMAKE_HOST_APPLE
   OR WIN32
   OR MINGW
   OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS "4.9"))
  # There is a BUG in gcc 4.6-4.8 and fixed in gcc 4.9 https://gcc.gnu.org/bugzilla/show_bug.cgi?id=58016
  # https://gcc.gnu.org/gcc-4.9/changes.html
  option(ATFRAMEWORK_UTILS_LIBUNWIND_ENABLED "Enable using libunwind." OFF)
else()
  option(ATFRAMEWORK_UTILS_LIBUNWIND_ENABLED "Enable using libunwind." ON)
endif()

option(ATFRAMEWORK_UTILS_LOG_ENABLE_LUA_SUPPORT "Enable lua support." ON)
option(ATFRAMEWORK_UTILS_LOG_WRAPPER_CHECK_LUA "Check lua support." ON)
set(ATFRAMEWORK_UTILS_LOG_MAX_SIZE_PER_LINE
    "2097152"
    CACHE STRING "Max size in one log line.")
set(ATFRAMEWORK_UTILS_LOG_CATEGORIZE_SIZE
    "16"
    CACHE STRING "Default log categorize number.")
option(ATFRAMEWORK_UTILS_ENABLE_NETWORK "Enable network support." ON)
option(ATFRAMEWORK_UTILS_ENABLE_CXX_GSL "Enable C++ Core Guideline: The Guideline Support Library." ON)
option(ATFRAMEWORK_UTILS_ENABLE_CXX_GSL_STD_STRING_VIEW "Enable alias gsl::string_view=std::string_view." ON)

option(ATFRAMEWORK_UTILS_ENABLE_LEGACY_ALIAS "Enable legacy alias for atframework utils" ON)

# Check pthread
set(THREADS_PREFER_PTHREAD_FLAG TRUE)
find_package(Threads)
if(CMAKE_USE_PTHREADS_INIT OR ATFRAMEWORK_CMAKE_TOOLSET_TEST_FLAG_PTHREAD)
  set(ATFRAMEWORK_UTILS_THREAD_TLS_USE_PTHREAD 1)
endif()
if(TARGET Threads::Threads)
  list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES Threads::Threads)
endif()

cmake_push_check_state()
set(CMAKE_REQUIRED_LIBRARIES ${ATFRAMEWORK_CMAKE_TOOLSET_SYSTEM_LINKS})
if(WIN32)
  check_cxx_source_compiles(
    "
    #include <io.h>
    #include <stdio.h>
    int main() {
        char buffer[32] = \"abcdefgXXXXXX\";
        const char* f = _mktemp(buffer);
        puts(f);
        return 0;
    }
    "
    ATFRAMEWORK_UTILS_TEST_WINDOWS_MKTEMP)
  if(ATFRAMEWORK_UTILS_TEST_WINDOWS_MKTEMP)
    set(ATFRAMEWORK_UTILS_ENABLE_WINDOWS_MKTEMP TRUE)
  endif()
else()
  check_c_source_compiles(
    "
    #include <stdlib.h>
    #include <stdio.h>
    #include <unistd.h>
    int main() {
        char buffer[32] = \"/tmp/abcdefgXXXXXX\";
        int f = mkstemp(buffer);
        close(f);
        return 0;
    }
    "
    ATFRAMEWORK_UTILS_TEST_POSIX_MKSTEMP)
  if(ATFRAMEWORK_UTILS_TEST_POSIX_MKSTEMP)
    set(ATFRAMEWORK_UTILS_ENABLE_POSIX_MKSTEMP TRUE)
  endif()
endif()
cmake_pop_check_state()

if(ANDROID)
  # Android发现偶现_Unwind_Backtrace调用崩溃,默认禁用掉这个功能。 可以用adb logcat | ./ndk-stack -sym $PROJECT_PATH/obj/local/armeabi 代替
  option(ATFRAMEWORK_UTILS_LOG_WRAPPER_ENABLE_STACKTRACE "Try to enable stacktrace for log." OFF)
else()
  option(ATFRAMEWORK_UTILS_LOG_WRAPPER_ENABLE_STACKTRACE "Try to enable stacktrace for log." ON)
endif()
option(ATFRAMEWORK_UTILS_LOCK_DISABLE_MT "Disable multi-thread support lua support." OFF)
set(ATFRAMEWORK_UTILS_LOG_STACKTRACE_MAX_STACKS
    "510"
    CACHE STRING "Max stacks when stacktracing.")

# 内存混淆int
set(ATFRAMEWORK_UTILS_ENABLE_MIXEDINT_MAGIC_MASK
    0
    CACHE STRING "Integer mixed magic mask")
if(NOT ATFRAMEWORK_UTILS_ENABLE_MIXEDINT_MAGIC_MASK)
  set(ATFRAMEWORK_UTILS_ENABLE_MIXEDINT_MAGIC_MASK 0)
endif()

# third_party

if(ATFRAMEWORK_UTILS_LOG_ENABLE_LUA_SUPPORT AND ATFRAMEWORK_UTILS_LOG_WRAPPER_CHECK_LUA)
  project_third_party_include_port("lua/lua.cmake")
endif()

if(ATFRAMEWORK_UTILS_LOG_WRAPPER_ENABLE_STACKTRACE)
  if(NOT ANDROID AND NOT CMAKE_HOST_APPLE)
    # project_third_party_include_port("jemalloc/jemalloc.cmake")
    if(ATFRAMEWORK_UTILS_LIBUNWIND_ENABLED)
      project_third_party_include_port("libunwind/libunwind.cmake")
    endif()
  endif()
endif()

if(NOT CMAKE_CROSSCOMPILING AND UNIX)
  set(ATFRAMEWORK_UTILS_TEST_SYSLOG_BACKUP_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
  set(CMAKE_REQUIRED_LIBRARIES ${ATFRAMEWORK_CMAKE_TOOLSET_SYSTEM_LINKS})

  check_cxx_source_compiles(
    "
  #include <syslog.h>
  int main() {
      openlog(\"test\", LOG_NDELAY | LOG_PID, LOG_USER);
      syslog(LOG_INFO, \"%s\", \"Hello World!\");
      return 0;
  }
  "
    ATFRAMEWORK_UTILS_TEST_SYSLOG)
  cmake_dependent_option(ATFRAMEWORK_UTILS_LOG_SINK_ENABLE_SYSLOG_SUPPORT "Enable syslog sink for log." ON
                         "ATFRAMEWORK_UTILS_TEST_SYSLOG" OFF)
  set(CMAKE_REQUIRED_LIBRARIES ${ATFRAMEWORK_UTILS_TEST_SYSLOG_BACKUP_CMAKE_REQUIRED_LIBRARIES})
  unset(ATFRAMEWORK_UTILS_TEST_SYSLOG_BACKUP_CMAKE_REQUIRED_LIBRARIES)
else()
  option(ATFRAMEWORK_UTILS_LOG_SINK_ENABLE_SYSLOG_SUPPORT "Enable syslog sink for log." OFF)
endif()

include("${PROJECT_ATFRAME_UTILS_SOURCE_DIR}/log/log_configure.cmake")

if(NOT ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_CRYPTO_DISABLED)
  project_third_party_include_port("ssl/port.cmake")
endif()

if(ATFRAMEWORK_UTILS_ENABLE_NETWORK)
  project_third_party_include_port("libuv/libuv.cmake")
  project_third_party_include_port("compression/import.cmake")
  find_package(c-ares QUIET)
  find_package(OpenSSL QUIET)
  find_package(MbedTLS QUIET)
  find_package(Libnghttp3 QUIET)
  find_package(Libngtcp2 QUIET)
  find_package(Libnghttp2 QUIET)
  project_third_party_include_port("libcurl/libcurl.cmake")
endif()

if(NOT DEFINED ATFRAMEWORK_UTILS_ENABLE_LIBUUID AND NOT DEFINED CACHE{ATFRAMEWORK_UTILS_ENABLE_LIBUUID})
  find_package(Libuuid)
  if(TARGET libuuid)
    echowithcolor(COLOR YELLOW "-- Dependency(${PROJECT_NAME}): uuid generator with libuuid.(Target: libuuid)")
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES libuuid)
    set(ATFRAMEWORK_UTILS_ENABLE_LIBUUID TRUE)
    project_build_tools_patch_default_imported_config(libuuid)
  elseif(Libuuid_FOUND)
    echowithcolor(
      COLOR YELLOW
      "-- Dependency(${PROJECT_NAME}): uuid generator with libuuid.(${Libuuid_INCLUDE_DIRS}:${Libuuid_LIBRARIES})")
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_INCLUDE_DIRS ${Libuuid_INCLUDE_DIRS})
    if(Libuuid_LIBRARIES)
      list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES ${Libuuid_LIBRARIES})
    endif()
    set(ATFRAMEWORK_UTILS_ENABLE_LIBUUID TRUE)
  else()
    set(ATFRAMEWORK_UTILS_ENABLE_LIBUUID FALSE)
  endif()
endif()

set(ATFRAMEWORK_UTILS_ENABLE_UUID_WINRPC FALSE)
if(NOT ATFRAMEWORK_UTILS_ENABLE_LIBUUID AND WIN32)
  include(CheckCXXSourceCompiles)
  set(ATFRAMEWORK_UTILS_TEST_UUID_BACKUP_CMAKE_REQUIRED_LIBRARIES ${CMAKE_REQUIRED_LIBRARIES})
  set(CMAKE_REQUIRED_LIBRARIES Rpcrt4 ${ATFRAMEWORK_CMAKE_TOOLSET_SYSTEM_LINKS})
  check_cxx_source_compiles(
    "
    #include <rpc.h>
    int main() {
        UUID Uuid;
        UuidCreate(&Uuid);
        return 0;
    }
    "
    ATFRAMEWORK_UTILS_TEST_UUID)

  set(CMAKE_REQUIRED_LIBRARIES ${ATFRAMEWORK_UTILS_TEST_UUID_BACKUP_CMAKE_REQUIRED_LIBRARIES})
  unset(ATFRAMEWORK_UTILS_TEST_UUID_BACKUP_CMAKE_REQUIRED_LIBRARIES)
  if(ATFRAMEWORK_UTILS_TEST_UUID)
    set(ATFRAMEWORK_UTILS_ENABLE_UUID_WINRPC TRUE)
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES Rpcrt4)
    echowithcolor(COLOR YELLOW "-- Dependency(${PROJECT_NAME}): uuid generator with Windows Rpcrt4.")
  endif()
endif()

if(NOT ATFRAMEWORK_UTILS_ENABLE_LIBUUID AND NOT ATFRAMEWORK_UTILS_ENABLE_UUID_WINRPC)
  set(ATFRAMEWORK_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT TRUE)
  echowithcolor(COLOR YELLOW "-- Dependency(${PROJECT_NAME}): uuid generator with internal implement.")
else()
  set(ATFRAMEWORK_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT FALSE)
endif()

# libuv
if(ATFRAMEWORK_UTILS_ENABLE_NETWORK)
  if(ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_LIBUV_LINK_NAME)
    set(ATFRAMEWORK_UTILS_NETWORK_EVPOLL_ENABLE_LIBUV 1)
    message(STATUS "libuv using target(${PROJECT_NAME}): ${ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_LIBUV_LINK_NAME}")
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES ${ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_LIBUV_LINK_NAME})
  else()
    message(STATUS "Libuv support disabled(${PROJECT_NAME})")
  endif()

  # curl
  if(TARGET CURL::libcurl)
    message(STATUS "Curl using target(${PROJECT_NAME}): CURL::libcurl")
    set(ATFRAMEWORK_UTILS_NETWORK_ENABLE_CURL 1)
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES CURL::libcurl)
    project_build_tools_patch_default_imported_config(CURL::libcurl)
  elseif(CURL_FOUND AND CURL_LIBRARIES)
    message(STATUS "Curl support enabled(${PROJECT_NAME})")
    set(ATFRAMEWORK_UTILS_NETWORK_ENABLE_CURL 1)
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_INCLUDE_DIRS ${CURL_INCLUDE_DIRS})
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES ${CURL_LIBRARIES})
  else()
    message(STATUS "Curl support disabled(${PROJECT_NAME})")
  endif()
endif()

if(ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_CRYPT_LINK_NAME)
  list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES ${ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_CRYPT_LINK_NAME})
  if(ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_CRYPTO_USE_LIBRESSL)
    message(STATUS "SSL(${PROJECT_NAME}): libressl")
    set(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL 1)
  elseif(ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_CRYPTO_USE_BORINGSSL)
    message(STATUS "SSL(${PROJECT_NAME}): boringssl")
    set(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL 1)
  elseif(ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_CRYPTO_USE_OPENSSL)
    message(STATUS "SSL(${PROJECT_NAME}): openssl")
    set(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL 1)
  elseif(ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_CRYPTO_USE_MBEDTLS)
    message(STATUS "SSL(${PROJECT_NAME}): mbedtls")
    set(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS 1)
  else()
    message(WARNING "SSL(${PROJECT_NAME}): unknown and will be disabled")
  endif()
else()
  message(STATUS "SSL(${PROJECT_NAME}): disabled")
endif()

if(NOT ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_CRYPTO_DISABLED)
  find_package(Libsodium)
  if(Libsodium_FOUND)
    set(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBSODIUM 1)
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_INCLUDE_DIRS ${Libsodium_INCLUDE_DIRS})
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES ${Libsodium_LIBRARIES})
  endif()
endif()

# MSVC 1929 - VS 2019 (14.29) has wrong argument type for some functions of std::format So we disable it for easier to
# use
if(MSVC AND MSVC_VERSION LESS 1930)
  set(ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT FALSE)
endif()

# Check fmtlib(https://fmt.dev/) or std::format
cmake_push_check_state()

if(NOT DEFINED ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT)
  check_cxx_source_compiles(
    "
#include <format>
#include <iostream>
#include <string>
struct custom_object {
  int32_t x;
  std::string y;
};

// Some STL implement may have BUGs on some APIs, we need check it
template <class CharT>
struct std::formatter<custom_object, CharT> : std::formatter<std::basic_string_view<CharT>, CharT> {
  template <class FormatContext>
  auto format(const custom_object &vec, FormatContext &ctx) const {
    return std::vformat_to(ctx.out(), \"({},{})\", std::make_format_args(vec.x, vec.y));
  }
};
int main() {
  custom_object custom_obj;
  custom_obj.x = 43;
  custom_obj.y = \"44\";
  std::cout<< std::format(\"The answer is {}, custom object: {}.\", 42, custom_obj)<< std::endl;
  char buffer[64] = {0};
  const auto result = std::format_to_n(buffer, sizeof(buffer), \"{} {}: {}\", \"Hello\", \"World!\", 42);
  std::cout << \"Buffer: \" << buffer << \",Untruncated output size = \" << result.size << std::endl;
  return 0;
}"
    ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT)
  set(ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT
      ${ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT}
      CACHE BOOL "Using std format for log formatter")
endif()
cmake_pop_check_state()

if(NOT ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT)
  set(ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_TEST_STD_FORMAT OFF)
  project_third_party_include_port("fmtlib/fmtlib.cmake")
  if(ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_FMTLIB_LINK_NAME AND NOT DEFINED ATFRAMEWORK_UTILS_ENABLE_FMTLIB)
    cmake_push_check_state()
    set(CMAKE_REQUIRED_LIBRARIES ${ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_FMTLIB_LINK_NAME}
                                 ${ATFRAMEWORK_CMAKE_TOOLSET_SYSTEM_LINKS})
    check_cxx_source_compiles(
      "
#include <fmt/format.h>
#include <iostream>
#include <string>
int main() {
    std::cout<< fmt::format(\"The answer is {}.\", 42)<< std::endl;
    char buffer[64] = {0};
    const auto result = fmt::format_to_n(buffer, sizeof(buffer), \"{} {}: {}\", \"Hello\", \"World!\", 42);
    std::cout << \"Buffer: \" << buffer << \",Untruncated output size = \" << result.size << std::endl;
    return 0;
}
"
      ATFRAMEWORK_UTILS_ENABLE_FMTLIB)
    cmake_pop_check_state()

    set(ATFRAMEWORK_UTILS_ENABLE_FMTLIB
        ${ATFRAMEWORK_UTILS_ENABLE_FMTLIB}
        CACHE BOOL "Using fmt.dev for log formatter")
  endif()

  if(ATFRAMEWORK_UTILS_ENABLE_FMTLIB)
    list(APPEND PROJECT_ATFRAME_UTILS_PUBLIC_LINK_NAMES ${ATFRAMEWORK_CMAKE_TOOLSET_THIRD_PARTY_FMTLIB_LINK_NAME})
  endif()
endif()

cmake_push_check_state()

check_cxx_source_compiles(
  "
        #include <unordered_map>
        #include <unordered_set>
        #include <string>

        int main() {
            std::unordered_set<std::string> k1;
            k1.reserve(8);
            std::unordered_map<std::string, int> k2;
            k2.reserve(8);
            return 0;
        }"
  ATFRAMEWORK_UTILS_UNORDERED_MAP_SET_HAS_RESERVE)

check_cxx_source_compiles(
  "
  #include <iostream>
  #include <string_view>
  #include <source_location>

  void log(const std::string_view message,
           const std::source_location location =
                 std::source_location::current()) {
    std::cout << \"file: \"
              << location.file_name() << \"(\"
              << location.line() << \":\"
              << location.column() << \") `\"
              << location.function_name() << \"`: \"
              << message << std::endl;
  }

  template <typename T> void fun(T x) {
    log(x);
  }

  int main(int, char*[]) {
    log(\"Hello world!\");
    fun(\"Hello C++20!\");
    return 0;
  }"
  ATFRAMEWORK_UTILS_TEST_SOURCE_LOCATION)
set(ATFRAMEWORK_UTILS_ENABLE_SOURCE_LOCATION ${ATFRAMEWORK_UTILS_TEST_SOURCE_LOCATION})

check_cxx_source_compiles(
  "#include <iostream>
#include <stacktrace>

void func() { std::cout << std::stacktrace::current() << '\\n'; }

int main() {
  func();
  return 0;
}
"
  ATFRAMEWORK_UTILS_TEST_STACKTRACE)
set(ATFRAMEWORK_UTILS_ENABLE_STACKTRACE ${ATFRAMEWORK_UTILS_TEST_STACKTRACE})

cmake_pop_check_state()

set(ATFRAMEWORK_UTILS_ENABLE_RTTI ${COMPILER_OPTIONS_TEST_RTTI})
set(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION ${COMPILER_OPTIONS_TEST_EXCEPTION})
set(ATFRAMEWORK_UTILS_ENABLE_STD_EXCEPTION_PTR ${COMPILER_OPTIONS_TEST_STD_EXCEPTION_PTR})

# Test Configure
set(GTEST_ROOT
    ""
    CACHE STRING "GTest root directory")
set(BOOST_ROOT
    ""
    CACHE STRING "Boost root directory")
option(PROJECT_TEST_ENABLE_BOOST_UNIT_TEST "Enable boost unit test." OFF)

option(PROJECT_ENABLE_UNITTEST "Enable unit test" OFF)
option(PROJECT_ENABLE_SAMPLE "Enable sample" OFF)
if(CMAKE_CROSSCOMPILING)
  option(PROJECT_ENABLE_TOOLS "Enable sample" OFF)
else()
  option(PROJECT_ENABLE_TOOLS "Enable sample" ON)
endif()

option(ATFRAMEWORK_USE_DYNAMIC_LIBRARY "Build and linking with dynamic libraries." OFF)

if(ATFRAMEWORK_UTILS_ENABLE_CXX_GSL)
  include("${CMAKE_CURRENT_LIST_DIR}/GSLSupport.cmake")
endif()
