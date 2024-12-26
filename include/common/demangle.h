/**
 * @file demangle.h
 * @brief demangle symbol
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2020.08.04
 *
 * @history
 *
 *
 */

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compiler_features.h>
#include <design_pattern/noncopyable.h>

#include <cstring>
#include <string>

// __has_include is currently supported by GCC and Clang. However GCC 4.9 may have issues and
// returns 1 for 'defined( __has_include )', while '__has_include' is actually not supported:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63662
#if defined(__has_include) && \
    (!(defined(UTIL_CONFIG_COMPILER_IS_GNU) && UTIL_CONFIG_COMPILER_IS_GNU) || (__GNUC__ + 0) >= 5)
#  if __has_include(<cxxabi.h>)
#    define ATFRAMEWORK_UTILS_DEMANGLE_USING_CXX_ABI
#  endif
#elif defined(__GLIBCXX__) || defined(__GLIBCPP__)
#  define ATFRAMEWORK_UTILS_DEMANGLE_USING_CXX_ABI
#endif

#if !defined(ATFRAMEWORK_UTILS_DEMANGLE_USING_CXX_ABI) && defined(_WIN32)
#  define ATFRAMEWORK_UTILS_DEMANGLE_USING_WINDOWS
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN

ATFRAMEWORK_UTILS_API const char *demangle_alloc(const char *name) noexcept;
ATFRAMEWORK_UTILS_API void demangle_free(const char *name) noexcept;
ATFRAMEWORK_UTILS_API std::string demangle(const char *name);

class scoped_demangled_name {
 private:
  const char *m_p;
  UTIL_DESIGN_PATTERN_NOCOPYABLE(scoped_demangled_name)

 public:
  ATFRAMEWORK_UTILS_API explicit scoped_demangled_name(const char *name) noexcept;

  ATFRAMEWORK_UTILS_API ~scoped_demangled_name() noexcept;

  ATFRAMEWORK_UTILS_API scoped_demangled_name(scoped_demangled_name &&other) noexcept;
  ATFRAMEWORK_UTILS_API scoped_demangled_name &operator=(scoped_demangled_name &&other) noexcept;

  UTIL_FORCEINLINE const char *get() const noexcept { return m_p; }
};

ATFRAMEWORK_UTILS_NAMESPACE_END
