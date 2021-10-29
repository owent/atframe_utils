// Copyright 2021 atframework
// Created by owent on 2021-10-29

#pragma once

#include <algorithm>
#include <type_traits>

#ifdef max
#  undef max
#endif

namespace util {
namespace nostd {
#if ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
template <std::size_t Len, class... Types>
using aligned_union = std::aligned_union<Len, Types...>;
#else
// alignas is available from gcc 4.8, clang 3.0 and MSVC 19.0(2015)
// alignof is available from gcc 4.5, clang 2.9 and MSVC 19.0(2015)
template <std::size_t Len, class... Types>
struct LIBATFRAME_UTILS_API_HEAD_ONLY aligned_union {
  static constexpr std::size_t alignment_value = std::max({alignof(Types)...});

  struct type {
    alignas(alignment_value) unsigned char _s[std::max({Len, sizeof(Types)...})];
  };
};
#endif

}  // namespace nostd
}  // namespace util
