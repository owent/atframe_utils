// Copyright 2024 atframework
// Created by owent on 2024-05-20

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <memory>
#include <type_traits>

#include "nostd/type_traits.h"

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace nostd {
template <typename, typename = void>
struct __is_nullability_compatible : ::std::false_type {};

// Allow custom to supoort nullability by define nullability_compatible_type as void
template <typename T>
struct __is_nullability_compatible<T, void_t<typename T::nullability_compatible_type>> : std::true_type {};

template <class T>
inline constexpr bool __is_nullability_support_v = __is_nullability_compatible<T>::value;

template <typename T>
inline constexpr bool __is_nullability_support_v<T*> = true;

template <typename T, typename U>
inline constexpr bool __is_nullability_support_v<T U::*> = true;

template <typename T, typename... Deleter>
inline constexpr bool __is_nullability_support_v<std::unique_ptr<T, Deleter...>> = true;

template <typename T>
inline constexpr bool __is_nullability_support_v<std::shared_ptr<T>> = true;

template <typename T>
struct __enable_nullable {
  static_assert(__is_nullability_support_v<remove_cv_t<T>>,
                "Template argument must be a raw or supported smart pointer "
                "type. See nostd/nullability.h.");
  using type = T;
};

template <typename T>
struct __enable_nonnull {
  static_assert(__is_nullability_support_v<remove_cv_t<T>>,
                "Template argument must be a raw or supported smart pointer "
                "type. See nostd/nullability.h.");
  using type = T;
};

template <typename T>
struct __enable_nullability_unknown {
  static_assert(__is_nullability_support_v<remove_cv_t<T>>,
                "Template argument must be a raw or supported smart pointer "
                "type. See nostd/nullability.h.");
  using type = T;
};

template <typename T, typename = typename __enable_nullable<T>::type>
using nullable
#if UTIL_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nullable")]]
#endif
    = T;

template <typename T, typename = typename __enable_nonnull<T>::type>
using nonnull
#if UTIL_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nonnull")]]
#endif
    = T;

template <typename T, typename = typename __enable_nullability_unknown<T>::type>
using nullability_unknown
#if UTIL_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nullability_Unspecified")]]
#endif
    = T;

}  // namespace nostd
LIBATFRAME_UTILS_NAMESPACE_END
