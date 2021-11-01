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
template <std::size_t Len, class... Types>
struct max_alignof_size_helper;

template <std::size_t Len, class First, class... Types>
struct max_alignof_size_helper<Len, First, Types...> {
  static constexpr const std::size_t alignment_value = alignof(First) >
                                                               max_alignof_size_helper<Len, Types...>::alignment_value
                                                           ? alignof(First)
                                                           : max_alignof_size_helper<Len, Types...>::alignment_value;
};

template <std::size_t Len>
struct max_alignof_size_helper<Len> {
  static constexpr const std::size_t alignment_value = Len;
};

template <std::size_t Len, class... Types>
struct max_sizeof_size_helper;

template <std::size_t Len, class First, class... Types>
struct max_sizeof_size_helper<Len, First, Types...> {
  static constexpr const std::size_t value = sizeof(First) > max_sizeof_size_helper<Len, Types...>::value
                                                 ? sizeof(First)
                                                 : max_sizeof_size_helper<Len, Types...>::value;
};

template <std::size_t Len>
struct max_sizeof_size_helper<Len> {
  static constexpr const std::size_t value = Len;
};

// alignas is available from gcc 4.8, clang 3.0 and MSVC 19.0(2015)
// alignof is available from gcc 4.5, clang 2.9 and MSVC 19.0(2015)
template <std::size_t Len, class... Types>
struct LIBATFRAME_UTILS_API_HEAD_ONLY aligned_union {
  static constexpr const std::size_t alignment_value = max_alignof_size_helper<0, Types...>::alignment_value;

  struct type {
    alignas(sizeof(unsigned char[alignment_value])) unsigned char _s[max_sizeof_size_helper<0, Types...>::value];
  };
};
#endif

}  // namespace nostd
}  // namespace util
