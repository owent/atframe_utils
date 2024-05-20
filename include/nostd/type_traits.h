// Copyright 2021 atframework
// Created by owent on 2021-10-29

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <algorithm>
#include <cstddef>
#include <type_traits>

#ifdef max
#  undef max
#endif

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace nostd {

template <class...>
using void_t = void;

template <typename T>
using add_const_t = typename ::std::add_const<T>::type;

template <typename T>
using remove_const_t = typename ::std::remove_const<T>::type;

template <typename T>
using add_volatile_t = typename ::std::add_volatile<T>::type;

template <typename T>
using remove_volatile_t = typename ::std::remove_volatile<T>::type;

template <typename T>
using add_cv_t = typename ::std::add_cv<T>::type;

template <typename T>
using remove_cv_t = typename ::std::remove_cv<T>::type;

template <typename T>
using add_pointer_t = typename ::std::add_pointer<T>::type;

template <typename T>
using remove_pointer_t = typename ::std::remove_pointer<T>::type;

template <typename T>
using remove_reference_t = typename ::std::remove_reference<T>::type;

template <typename T>
using remove_cvref_t = remove_cv_t<remove_reference_t<T>>;

template <typename T>
using decay_t = typename ::std::decay<T>::type;

template <class... T>
using common_type_t = typename ::std::common_type<T...>::type;

template <class T>
using underlying_type_t = typename ::std::underlying_type<T>::type;

template <class T>
using remove_extent_t = typename ::std::remove_extent<T>::type;

template <class T>
using remove_all_extents_t = typename ::std::remove_all_extents<T>::type;

template <bool B, class T = void>
using enable_if_t = typename ::std::enable_if<B, T>::type;

template <class T, class U>
constexpr bool is_same_v = ::std::is_same<T, U>::value;

template <class Base, class Derived>
constexpr bool is_base_of_v = ::std::is_base_of<Base, Derived>::value;

template <class From, class To>
constexpr bool is_convertible_v = ::std::is_convertible<From, To>::value;

template <class T>
constexpr ::std::size_t rank_v = ::std::rank<T>::value;

template <class T, unsigned N = 0>
constexpr ::std::size_t extent_v = ::std::extent<T, N>::value;

// std::aligned_union is deprecated in C++23, which will be warned by MSVC with C++20 only
#if ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)) && \
    !((defined(__cplusplus) && __cplusplus >= 202004L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202004L))
template <std::size_t Len, class... Types>
using aligned_union = std::aligned_union<Len, Types...>;

template <std::size_t Len, std::size_t Align = alignof(std::max_align_t)>
using aligned_storage = std::aligned_storage<Len, Align>;

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

// We should align to at least 16 bytes, @see https://wiki.osdev.org/System_V_ABI for more details
#  if (defined(__cplusplus) && __cplusplus >= 201103L) || \
      (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || (defined(_MSC_VER) && _MSC_VER >= 1900)
template <std::size_t Len, std::size_t Align = alignof(max_align_t)>
#  else
template <std::size_t Len, std::size_t Align = 16>
#  endif
struct LIBATFRAME_UTILS_API_HEAD_ONLY aligned_storage {
  struct type {
    alignas(Align) unsigned char _s[Len];
  };
};
#endif

}  // namespace nostd
LIBATFRAME_UTILS_NAMESPACE_END
