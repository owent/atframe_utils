// Copyright 2026 atframework

#pragma once

// Export macros for the atframework test framework library (atframework.test.library).
//
// The macros follow the same pattern as ATFRAMEWORK_UTILS_API:
// - ATFRAMEWORK_TEST_API_NATIVE is defined by the build of atframework.test.library itself.
// - ATFRAMEWORK_TEST_API_DLL is defined (on both the library and its consumers) when the library is
//   built as a shared library.
// - When neither is defined (static library or source inclusion of test/frame/*.cpp into a test
//   executable, as before), ATFRAMEWORK_TEST_API degrades to plain default visibility, so existing
//   source-inclusion users keep compiling unchanged.

#if defined(_MSC_VER)
// The exported classes have STL members (std::string, std::unordered_set, ...). C4251 warns about
// STL types crossing the DLL boundary, which is harmless here: the test framework library and its
// consumers are always built in the same tree with the same toolchain and STL. The same pattern is
// used by config/compiler/protobuf_prefix.h.
#  pragma warning(disable : 4251)
#endif

#include <config/atframe_utils_build_feature.h>

#if defined(ATFRAMEWORK_TEST_API_NATIVE) && ATFRAMEWORK_TEST_API_NATIVE
#  if defined(ATFRAMEWORK_TEST_API_DLL) && ATFRAMEWORK_TEST_API_DLL
#    define ATFRAMEWORK_TEST_API ATFW_UTIL_SYMBOL_EXPORT
#  else
#    define ATFRAMEWORK_TEST_API ATFW_UTIL_SYMBOL_VISIBLE
#  endif
#else
#  if defined(ATFRAMEWORK_TEST_API_DLL) && ATFRAMEWORK_TEST_API_DLL
#    define ATFRAMEWORK_TEST_API ATFW_UTIL_SYMBOL_IMPORT
#  else
#    define ATFRAMEWORK_TEST_API ATFW_UTIL_SYMBOL_VISIBLE
#  endif
#endif
#define ATFRAMEWORK_TEST_API_HEAD_ONLY ATFW_UTIL_SYMBOL_VISIBLE
