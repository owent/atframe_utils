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

#ifndef UTIL_COMMON_DEMANGLE_H
#define UTIL_COMMON_DEMANGLE_H

#include <config/compiler_features.h>
#include <design_pattern/noncopyable.h>

#include <cstring>
#include <string>

#pragma once

// __has_include is currently supported by GCC and Clang. However GCC 4.9 may have issues and
// returns 1 for 'defined( __has_include )', while '__has_include' is actually not supported:
// https://gcc.gnu.org/bugzilla/show_bug.cgi?id=63662
#if defined(__has_include) && \
    (!(defined(UTIL_CONFIG_COMPILER_IS_GNU) && UTIL_CONFIG_COMPILER_IS_GNU) || (__GNUC__ + 0) >= 5)
#  if __has_include(<cxxabi.h>)
#    define UTIL_COMMON_DEMANGLE_USING_CXX_ABI
#  endif
#elif defined(__GLIBCXX__) || defined(__GLIBCPP__)
#  define UTIL_COMMON_DEMANGLE_USING_CXX_ABI
#endif

#if defined(UTIL_COMMON_DEMANGLE_USING_CXX_ABI)
#  include <cxxabi.h>
// For some archtectures (mips, mips64, x86, x86_64) cxxabi.h in Android NDK is implemented by gabi++ library
// (https://android.googlesource.com/platform/ndk/+/master/sources/cxx-stl/gabi++/), which does not implement
// abi::__cxa_demangle(). We detect this implementation by checking the include guard here.
#  if defined(__GABIXX_CXXABI_H__)
#    undef UTIL_COMMON_DEMANGLE_USING_CXX_ABI
#  else
#    include <cstddef>
#    include <cstdlib>
#  endif
#endif

namespace util {

inline const char *demangle_alloc(const char *name) UTIL_CONFIG_NOEXCEPT;
inline void demangle_free(const char *name) UTIL_CONFIG_NOEXCEPT;

class scoped_demangled_name {
 private:
  const char *m_p;
  UTIL_DESIGN_PATTERN_NOCOPYABLE(scoped_demangled_name)

 public:
  explicit scoped_demangled_name(const char *name) UTIL_CONFIG_NOEXCEPT : m_p(demangle_alloc(name)) {}

  ~scoped_demangled_name() UTIL_CONFIG_NOEXCEPT {
    if (NULL != m_p) {
      demangle_free(m_p);
    }
  }

#if defined(UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES) && UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES
  scoped_demangled_name(scoped_demangled_name &&other) : m_p(other.m_p) { other.m_p = NULL; }
  scoped_demangled_name &operator=(scoped_demangled_name &&other) {
    const char *tmp = m_p;
    m_p = other.m_p;
    other.m_p = tmp;
    return *this;
  }
#endif

  const char *get() const UTIL_CONFIG_NOEXCEPT { return m_p; }
};

#if defined(UTIL_COMMON_DEMANGLE_USING_CXX_ABI)

inline const char *demangle_alloc(const char *name) UTIL_CONFIG_NOEXCEPT {
  if (NULL == name) {
    return NULL;
  }
  int status = 0;
  std::size_t size = 0;
  return abi::__cxa_demangle(name, NULL, &size, &status);
}

inline void demangle_free(const char *name) UTIL_CONFIG_NOEXCEPT { std::free(const_cast<char *>(name)); }

inline std::string demangle(const char *name) {
  scoped_demangled_name demangled_name(name);
  const char *p = demangled_name.get();
  if (!p) {
    p = name;
  }
  return p;
}

#else

inline const char *demangle_alloc(const char *name) UTIL_CONFIG_NOEXCEPT { return name; }
inline void demangle_free(const char *) UTIL_CONFIG_NOEXCEPT {}
inline std::string demangle(const char *name) { return name; }

#endif

}  // namespace util

#undef UTIL_COMMON_DEMANGLE_USING_CXX_ABI

#endif  // #ifndef UTIL_COMMON_DEMANGLE_H
