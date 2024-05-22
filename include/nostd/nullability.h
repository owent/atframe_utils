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
template <class, class = void>
struct __is_nullability_compatible : ::std::false_type {};

// Allow custom to supoort nullability by define nullability_compatible_type as void
template <class T>
struct __is_nullability_compatible<T, void_t<typename T::nullability_compatible_type>> : ::std::true_type {};

template <class T>
struct __is_nullability_support {
  UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = __is_nullability_compatible<T>::value;
};

template <class T>
struct __is_nullability_support<T*> {
  UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = true;
};

template <class T, class U>
struct __is_nullability_support<T U::*> {
  UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = true;
};

template <class T, class... Deleter>
struct __is_nullability_support<std::unique_ptr<T, Deleter...>> {
  UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = true;
};

template <class T>
struct __is_nullability_support<std::shared_ptr<T>> {
  UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = true;
};

template <class T>
struct __enable_nullable {
  static_assert(__is_nullability_support<remove_cv_t<T>>::value,
                "Template argument must be a raw or supported smart pointer "
                "type. See nostd/nullability.h.");
  using type = T;
};

template <class T>
struct __enable_nonnull {
  static_assert(__is_nullability_support<remove_cv_t<T>>::value,
                "Template argument must be a raw or supported smart pointer "
                "type. See nostd/nullability.h.");
  using type = T;
};

template <class T>
struct __enable_nullability_unknown {
  static_assert(__is_nullability_support<remove_cv_t<T>>::value,
                "Template argument must be a raw or supported smart pointer "
                "type. See nostd/nullability.h.");
  using type = T;
};

template <class T, class = typename __enable_nullable<T>::type>
using nullable
#if UTIL_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nullable")]]
#endif
    = T;

template <class T, class = typename __enable_nonnull<T>::type>
using nonnull
#if UTIL_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nonnull")]]
#endif
    = T;

template <class T, class = typename __enable_nullability_unknown<T>::type>
using nullability_unknown
#if UTIL_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nullability_Unspecified")]]
#endif
    = T;

}  // namespace nostd
LIBATFRAME_UTILS_NAMESPACE_END
