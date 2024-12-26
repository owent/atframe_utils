// Copyright 2023 atframework
// Created by owent on 2024-09-06.
//

#include <config/atframe_utils_build_feature.h>

#include <common/demangle.h>

#if defined(ATFRAMEWORK_UTILS_DEMANGLE_USING_CXX_ABI)
#  include <cxxabi.h>
// For some archtectures (mips, mips64, x86, x86_64) cxxabi.h in Android NDK is implemented by gabi++ library
// (https://android.googlesource.com/platform/ndk/+/master/sources/cxx-stl/gabi++/), which does not implement
// abi::__cxa_demangle(). We detect this implementation by checking the include guard here.
#  if defined(__GABIXX_CXXABI_H__)
#    undef ATFRAMEWORK_UTILS_DEMANGLE_USING_CXX_ABI
#  else
#    include <cstddef>
#    include <cstdlib>
#  endif

#  include <memory>
#  include <string>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN

ATFRAMEWORK_UTILS_API const char *demangle_alloc(const char *name) noexcept {
  if (nullptr == name) {
    return nullptr;
  }
  int status = 0;
  std::size_t size = 0;
  return abi::__cxa_demangle(name, nullptr, &size, &status);
}

ATFRAMEWORK_UTILS_API void demangle_free(const char *name) noexcept { std::free(const_cast<char *>(name)); }

ATFRAMEWORK_UTILS_API std::string demangle(const char *name) {
  scoped_demangled_name demangled_name(name);
  const char *p = demangled_name.get();
  if (!p) {
    p = name;
  }

  return p;
}

ATFRAMEWORK_UTILS_NAMESPACE_END

#endif
