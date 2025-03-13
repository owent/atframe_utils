// Copyright 2025 atframework
// Licensed under the MIT licenses.
// Created by owent on 2025-03-04
// https://en.cppreference.com/w/cpp/feature_test

#include <iostream>

// Import the C++20 feature-test macros
#ifdef __has_include
#  if __has_include(<version>)
#    include <version>
#  endif
#elif defined(_MSC_VER) && \
    ((defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L))
#  if _MSC_VER >= 1922
#    include <version>
#  endif
#endif

int main() {
#if defined(_MSVC_LANG) && _MSVC_LANG > 1
  std::cout << "#define ATFRAMEWORK_UTILS_WITH_CXX_STANDARD " << _MSVC_LANG << "\n";
#else
  std::cout << "#define ATFRAMEWORK_UTILS_WITH_CXX_STANDARD " << __cplusplus << "\n";
#endif

  // stacktrace
  std::cout << "#define ATFRAMEWORK_UTILS_ENABLE_SOURCE_LOCATION " <<
#if defined(__cpp_lib_stacktrace) && __cpp_lib_stacktrace >= 202011L
      __cpp_lib_stacktrace
#else
      0
#endif
            << "\n";

  // stacktrace
  std::cout << "#define ATFRAMEWORK_UTILS_ENABLE_SOURCE_LOCATION " <<
#if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L
      __cpp_lib_source_location
#else
      0
#endif
            << "\n";

  return 0;
}