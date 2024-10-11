// Copyright 2024 atframework
// Created by owent on 2024-10-11

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <string>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace nostd {

#if ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))

using ::std::data;
using ::std::size;

#else

template <class TCONTAINER>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr inline auto size(const TCONTAINER& container) -> decltype(container.size()) {
  return container.size();
}

template <class T, size_t SIZE>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr inline size_t size(const T (&)[SIZE]) noexcept {
  return SIZE;
}

template <class TCONTAINER>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr inline auto data(TCONTAINER& container) -> decltype(container.data()) {
  return container.data();
}

template <class TCONTAINER>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr inline auto data(const TCONTAINER& container) -> decltype(container.data()) {
  return container.data();
}

template <class T, size_t SIZE>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr inline T* data(T (&array_value)[SIZE]) noexcept {
  return array_value;
}

template <class TELEMENT>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr inline const TELEMENT* data(std::initializer_list<TELEMENT> l) noexcept {
  return l.begin();
}

#endif
}  // namespace nostd
LIBATFRAME_UTILS_NAMESPACE_END
