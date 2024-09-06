// Copyright 2023 atframework
// Created by owent on 2024-09-06.
//

#include <config/atframe_utils_build_feature.h>

#include <common/demangle.h>

#include <memory>
#include <string>

LIBATFRAME_UTILS_NAMESPACE_BEGIN

LIBATFRAME_UTILS_API scoped_demangled_name::scoped_demangled_name(const char *name) noexcept
    : m_p(demangle_alloc(name)) {}

LIBATFRAME_UTILS_API scoped_demangled_name::~scoped_demangled_name() noexcept {
  if (nullptr != m_p) {
    demangle_free(m_p);
  }
}

LIBATFRAME_UTILS_API scoped_demangled_name::scoped_demangled_name(scoped_demangled_name &&other) noexcept
    : m_p(other.m_p) {
  other.m_p = nullptr;
}

LIBATFRAME_UTILS_API scoped_demangled_name &scoped_demangled_name::operator=(scoped_demangled_name &&other) noexcept {
  const char *tmp = m_p;
  m_p = other.m_p;
  other.m_p = tmp;
  return *this;
}

#if !defined(UTIL_COMMON_DEMANGLE_USING_CXX_ABI) && !defined(LIBATFRAME_UTILS_DEMANGLE_USING_WINDOWS)
LIBATFRAME_UTILS_API const char *demangle_alloc(const char *name) noexcept { return name; }
LIBATFRAME_UTILS_API void demangle_free(const char *) noexcept {}
LIBATFRAME_UTILS_API std::string demangle(const char *name) { return name; }
#endif

LIBATFRAME_UTILS_NAMESPACE_END
