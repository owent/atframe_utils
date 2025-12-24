// Copyright 2024 atframework
// Created by owent on 2024-05-20

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <memory>
#include <type_traits>

#include "nostd/type_traits.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace nostd {
template <class, class = void>
struct __is_nullability_compatible : ::std::false_type {};

// Allow custom to support nullability by define nullability_compatible_type as void
template <class T>
struct __is_nullability_compatible<T, void_t<typename T::nullability_compatible_type>> : ::std::true_type {};

template <class T>
struct __is_nullability_support {
  ATFW_UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = __is_nullability_compatible<T>::value;
};

template <class T>
struct __is_nullability_support<T*> {
  ATFW_UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = true;
};

template <class T, class U>
struct __is_nullability_support<T U::*> {
  ATFW_UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = true;
};

template <class T, class... Deleter>
struct __is_nullability_support<std::unique_ptr<T, Deleter...>> {
  ATFW_UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = true;
};

template <class T>
struct __is_nullability_support<std::shared_ptr<T>> {
  ATFW_UTIL_MACRO_INLINE_VARIABLE static constexpr const bool value = true;
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
#if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nullable")]]
#endif
    = T;

template <class T, class = typename __enable_nonnull<T>::type>
using nonnull
#if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nonnull")]]
#endif
    = T;

template <class T, class = typename __enable_nullability_unknown<T>::type>
using nullability_unknown
#if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(clang::annotate)
    [[clang::annotate("Nullability_Unspecified")]]
#endif
    = T;

}  // namespace nostd
ATFRAMEWORK_UTILS_NAMESPACE_END

// This macro specifies that all unannotated pointer types within the given
// file are designated as nonnull (instead of the default "unknown"). This macro
// exists as a standalone statement and applies default nonnull behavior to all
// subsequent pointers; as a result, place this macro as the first non-comment,
// non-`#include` line in a file.
//
// Example:
//
//     #include "nostd/nullability.h"
//
//     void FillMessage(Message *m);                // implicitly non-null
//     T* ATFW_UTIL_MACRO_NULLABLE GetNullablePtr();           // explicitly nullable
//     T* ATFW_UTIL_MACRO_NULLABILITY_UNKNOWN GetUnknownPtr(); // explicitly unknown
//
// The macro can be safely used in header files – it will not affect any files
// that include it.
//
// In files with the macro, plain `T*` syntax means `T* ATFW_UTIL_MACRO_NONNULL`, and the
// exceptions (`ATFW_UTIL_MACRO_NULLABLE` and `ATFW_UTIL_MACRO_NULLABILITY_UNKNOWN`) must be marked
// explicitly. The same holds, correspondingly, for smart pointer types.
//
// For comparison, without the macro, all unannotated pointers would default to
// unknown, and otherwise require explicit annotations to change this behavior:
//
//     #include "nostd/nullability.h"
//
//     void FillMessage(Message* ATFW_UTIL_MACRO_NONNULL m);  // explicitly non-null
//     T* ATFW_UTIL_MACRO_NULLABLE GetNullablePtr();          // explicitly nullable
//     T* GetUnknownPtr();                         // implicitly unknown
//
// No-op except for being a human readable signal.
#if defined(__clang__) && !defined(__OBJC__) && ATFW_UTIL_HAVE_FEATURE(nullability_on_classes)
// ATFW_UTIL_MACRO_NONNULL (default with `ABSL_POINTERS_DEFAULT_NONNULL`)
//
// The indicated pointer is never null. It is the responsibility of the provider
// of this pointer across an API boundary to ensure that the pointer is never
// set to null. Consumers of this pointer across an API boundary may safely
// dereference the pointer.
//
// Example:
//
// // `employee` is designated as not null.
// void PaySalary(Employee* ATFW_UTIL_MACRO_NONNULL employee) {
//   pay(*employee);  // OK to dereference
// }
#  define ATFW_UTIL_MACRO_NONNULL _Nonnull

// ATFW_UTIL_MACRO_NULLABLE
//
// The indicated pointer may, by design, be either null or non-null. Consumers
// of this pointer across an API boundary should perform a `nullptr` check
// before performing any operation using the pointer.
//
// Example:
//
// // `employee` may  be null.
// void PaySalary(Employee* ATFW_UTIL_MACRO_NULLABLE employee) {
//   if (employee != nullptr) {
//     Pay(*employee);  // OK to dereference
//   }
// }
#  define ATFW_UTIL_MACRO_NULLABLE _Nullable

// ATFW_UTIL_MACRO_NULLABILITY_UNKNOWN  (default without `ABSL_POINTERS_DEFAULT_NONNULL`)
//
// The indicated pointer has not yet been determined to be definitively
// "non-null" or "nullable." Providers of such pointers across API boundaries
// should, over time, annotate such pointers as either "non-null" or "nullable."
// Consumers of these pointers across an API boundary should treat such pointers
// with the same caution they treat currently unannotated pointers. Most
// existing code will have "unknown"  pointers, which should eventually be
// migrated into one of the above two nullability states: `ATFW_UTIL_MACRO_NONNULL` or
//  `ATFW_UTIL_MACRO_NULLABLE`.
//
// NOTE: For files that do not specify `ABSL_POINTERS_DEFAULT_NONNULL`,
// because this annotation is the global default state, unannotated pointers are
// are assumed to have "unknown" semantics. This assumption is designed to
// minimize churn and reduce clutter within the codebase.
//
// Example:
//
// // `employee`s nullability state is unknown.
// void PaySalary(Employee* ATFW_UTIL_MACRO_NULLABILITY_UNKNOWN employee) {
//   Pay(*employee); // Potentially dangerous. API provider should investigate.
// }
//
// Note that a pointer without an annotation, by default, is assumed to have the
// annotation `NullabilityUnknown`.
//
// // `employee`s nullability state is unknown.
// void PaySalary(Employee* employee) {
//   Pay(*employee); // Potentially dangerous. API provider should investigate.
// }
#  define ATFW_UTIL_MACRO_NULLABILITY_UNKNOWN _Null_unspecified
#else
// No-op for non-Clang compilers or Objective-C.
#  define ATFW_UTIL_MACRO_NONNULL
// No-op for non-Clang compilers or Objective-C.
#  define ATFW_UTIL_MACRO_NULLABLE
// No-op for non-Clang compilers or Objective-C.
#  define ATFW_UTIL_MACRO_NULLABILITY_UNKNOWN
#endif

// ATFW_UTIL_MACRO_NULLABILITY_COMPATIBLE
//
// Indicates that a class is compatible with nullability annotations.
//
// For example:
//
// struct ATFW_UTIL_MACRO_NULLABILITY_COMPATIBLE MyPtr {
//   ...
// };
//
// Note: Compilers that don't support the `nullability_on_classes` feature will
// allow nullability annotations to be applied to any type, not just ones marked
// with `ATFW_UTIL_MACRO_NULLABILITY_COMPATIBLE`.
#if ATFW_UTIL_HAVE_FEATURE(nullability_on_classes)
#  define ATFW_UTIL_MACRO_NULLABILITY_COMPATIBLE _Nullable
#else
#  define ATFW_UTIL_MACRO_NULLABILITY_COMPATIBLE
#endif
