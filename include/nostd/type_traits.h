// Copyright 2021 atframework
// Created by owent on 2021-10-29

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#ifdef max
#  undef max
#endif

#ifndef UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR
#  if __cplusplus >= 202002L
#    define UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR constexpr
#  else
#    define UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR
#  endif
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

#if ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)) && \
    defined(__cpp_lib_is_invocable)
template <class F, class... ArgTypes>
using invoke_result = ::std::invoke_result<F, ArgTypes...>;

using ::std::invoke_result_t;
#else
template <class F, class... ArgTypes>
using invoke_result = ::std::result_of<F(ArgTypes...)>;

template <class F, class... ArgTypes>
using invoke_result_t = typename invoke_result<F, ArgTypes...>::type;
#endif

#if ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)) && \
    defined(__cpp_lib_invoke)

using ::std::invoke;

#else

namespace details {

// Used by result_of, invoke etc. to unwrap a reference_wrapper.
template <typename _Tp, typename _Up = remove_cvref_t<_Tp>>
struct UTIL_SYMBOL_VISIBLE __inv_unwrap {
  using type = _Tp;
};

template <typename _Tp, typename _Up>
struct UTIL_SYMBOL_VISIBLE __inv_unwrap<_Tp, ::std::reference_wrapper<_Up>> {
  using type = _Up&;
};

struct UTIL_SYMBOL_VISIBLE __invoke_memfun_ref {};
struct UTIL_SYMBOL_VISIBLE __invoke_memfun_deref {};
struct UTIL_SYMBOL_VISIBLE __invoke_memobj_ref {};
struct UTIL_SYMBOL_VISIBLE __invoke_memobj_deref {};
struct UTIL_SYMBOL_VISIBLE __invoke_other {};

template <typename _Tp, typename _Up = typename __inv_unwrap<_Tp>::type>
UTIL_SYMBOL_VISIBLE inline UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR _Up&& __invfwd(
    typename ::std::remove_reference<_Tp>::type& __t) noexcept {
  return static_cast<_Up&&>(__t);
}

template <typename R, typename _Fn, typename... _Args>
UTIL_SYMBOL_VISIBLE inline UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR R __invoke_impl(__invoke_other, _Fn&& __f,
                                                                              _Args&&... __args) {
  return ::std::forward<_Fn>(__f)(::std::forward<_Args>(__args)...);
}

template <typename R, typename _MemFun, typename _Tp, typename... _Args>
UTIL_SYMBOL_VISIBLE inline UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR R __invoke_impl(__invoke_memfun_ref, _MemFun&& __f,
                                                                              _Tp&& __t, _Args&&... __args) {
  return (__invfwd<_Tp>(__t).*__f)(::std::forward<_Args>(__args)...);
}

template <typename R, typename _MemFun, typename _Tp, typename... _Args>
UTIL_SYMBOL_VISIBLE inline UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR R __invoke_impl(__invoke_memfun_deref, _MemFun&& __f,
                                                                              _Tp&& __t, _Args&&... __args) {
  return ((*::std::forward<_Tp>(__t)).*__f)(::std::forward<_Args>(__args)...);
}

template <typename R, typename _MemPtr, typename _Tp>
UTIL_SYMBOL_VISIBLE inline UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR R __invoke_impl(__invoke_memobj_ref, _MemPtr&& __f,
                                                                              _Tp&& __t) {
  return __invfwd<_Tp>(__t).*__f;
}

template <typename R, typename _MemPtr, typename _Tp>
UTIL_SYMBOL_VISIBLE inline UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR R __invoke_impl(__invoke_memobj_deref, _MemPtr&& __f,
                                                                              _Tp&& __t) {
  return (*::std::forward<_Tp>(__t)).*__f;
}

template <bool, bool, typename _Functor, typename... _ArgTypes>
struct __result_of_tag_impl {
  using type = __invoke_other;
};

template <typename _MemPtr, typename _Clazz, typename... _ArgTypes>
struct __result_of_tag_impl<true, false, _MemPtr, _Clazz, _ArgTypes...> {
  using type =
      typename ::std::conditional<::std::is_pointer<_Clazz>::value, __invoke_memobj_deref, __invoke_memobj_ref>::type;
};

template <typename _MemPtr, typename _Clazz, typename... _ArgTypes>
struct __result_of_tag_impl<false, true, _MemPtr, _Clazz, _ArgTypes...> {
  using type =
      typename ::std::conditional<::std::is_pointer<_Clazz>::value, __invoke_memfun_deref, __invoke_memfun_ref>::type;
};

// __invoke_result (std::invoke_result for C++11)
template <typename _Functor, typename... _ArgTypes>
struct __invoke_tag : public __result_of_tag_impl<
                          ::std::is_member_object_pointer<typename ::std::remove_reference<_Functor>::type>::value,
                          ::std::is_member_function_pointer<typename ::std::remove_reference<_Functor>::type>::value,
                          _Functor, _ArgTypes...> {};

template <typename F, typename... ArgTypes>
UTIL_SYMBOL_VISIBLE inline UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR invoke_result_t<F, ArgTypes...> __invoke(
    F&& __fn, ArgTypes&&... __args) {
  using __tag = typename __invoke_tag<F, ArgTypes...>::type;
  return __invoke_impl<invoke_result_t<F, ArgTypes...>>(__tag{}, std::forward<F>(__fn),
                                                        std::forward<ArgTypes>(__args)...);
}
}  // namespace details

template <typename F, typename... ArgTypes>
UTIL_SYMBOL_VISIBLE inline UTIL_NOSTD_INVOKE_RESULT_CONSTEXPR invoke_result_t<F, ArgTypes...> invoke(
    F&& __fn, ArgTypes&&... __args) {
  return details::__invoke(std::forward<F>(__fn), std::forward<ArgTypes>(__args)...);
}

#endif

// is_function()
//
// Determines whether the passed type `T` is a function type.
//
// This metafunction is designed to be a drop-in replacement for the C++11
// `std::is_function()` metafunction for platforms that have incomplete C++11
// support (such as libstdc++ 4.x).
//
// This metafunction works because appending `const` to a type does nothing to
// function types and reference types (and forms a const-qualified type
// otherwise).
template <class T>
struct is_function : ::std::integral_constant<bool, !(::std::is_reference<T>::value ||
                                                      ::std::is_const<typename ::std::add_const<T>::type>::value)> {};

// GCC 4.8 do not support variable template

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
