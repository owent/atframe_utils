/**
 * @file explicit_declare.h
 * @brief 导入继承关系约束<br />
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT, owt5008137@live.com
 * @date 2013-12-25
 *
 * @history
 *
 */

#pragma once

#include <cstdlib>
#include <utility>

// ============================================================
// 公共包含部分
// C++0x/11/14/17 显示申明
// ============================================================

#ifndef PARAM_IN
#  if defined(_MSC_VER) && _MSC_VER >= 1700  // vs 2012 or higher
#    define PARAM_IN _In_
#  else
#    define PARAM_IN
#  endif
#endif

#ifndef PARAM_OUT
#  if defined(_MSC_VER) && _MSC_VER >= 1700  // vs 2012 or higher
#    define PARAM_OUT _Out_
#  else
#    define PARAM_OUT
#  endif
#endif

#ifndef PARAM_INOUT
#  if defined(_MSC_VER) && _MSC_VER >= 1700  // vs 2012 or higher
#    define PARAM_INOUT _Inout_
#  else
#    define PARAM_INOUT
#  endif
#endif

/**
 * @brief deprecated, 标记为不推荐使用
 * usage:
 *   EXPLICIT_DEPRECATED_ATTR int a;
 *   class EXPLICIT_DEPRECATED_ATTR a;
 *   EXPLICIT_DEPRECATED_ATTR int a();
 * usage:
 *   EXPLICIT_DEPRECATED_MSG("there is better choose") int a;
 *   class DEPRECATED_MSG("there is better choose") a;
 *   EXPLICIT_DEPRECATED_MSG("there is better choose") int a();
 */
#if defined(__cplusplus) && __cplusplus >= 201402L
#  define EXPLICIT_DEPRECATED_ATTR [[deprecated]]
#elif defined(__clang__)
#  define EXPLICIT_DEPRECATED_ATTR __attribute__((deprecated))
#elif defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#  define EXPLICIT_DEPRECATED_ATTR __attribute__((deprecated))
#elif defined(_MSC_VER) && _MSC_VER >= 1400  // vs 2005 or higher
#  if _MSC_VER >= 1910 && defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#    define EXPLICIT_DEPRECATED_ATTR [[deprecated]]
#  else
#    define EXPLICIT_DEPRECATED_ATTR __declspec(deprecated)
#  endif
#else
#  define EXPLICIT_DEPRECATED_ATTR
#endif

#if defined(__cplusplus) && __cplusplus >= 201402L
#  define EXPLICIT_DEPRECATED_MSG(msg) [[deprecated(msg)]]
#elif defined(__clang__)
#  define EXPLICIT_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#elif defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#  define EXPLICIT_DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER) && _MSC_VER >= 1400  // vs 2005 or higher
#  if _MSC_VER >= 1910 && defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#    define EXPLICIT_DEPRECATED_MSG(msg) [[deprecated(msg)]]
#  else
#    define EXPLICIT_DEPRECATED_MSG(msg) __declspec(deprecated(msg))
#  endif
#else
#  define EXPLICIT_DEPRECATED_MSG(msg)
#endif

/**
 * @brief nodiscard, 标记禁止忽略返回值
 * usage:
 *   EXPLICIT_NODISCARD_ATTR int a;
 *   class EXPLICIT_NODISCARD_TYPE_ATTR a;
 *   EXPLICIT_NODISCARD_ATTR int a();
 */
#if defined(__cplusplus) && __cplusplus >= 201703L
#  define EXPLICIT_NODISCARD_ATTR [[nodiscard]]
#  define EXPLICIT_NODISCARD_TYPE_ATTR [[nodiscard]]
#elif defined(__clang__)
#  define EXPLICIT_NODISCARD_ATTR __attribute__((warn_unused_result))
#  define EXPLICIT_NODISCARD_TYPE_ATTR
#elif defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#  define EXPLICIT_NODISCARD_ATTR __attribute__((warn_unused_result))
#  define EXPLICIT_NODISCARD_TYPE_ATTR
#elif defined(_MSC_VER) && _MSC_VER >= 1700  // vs 2012 or higher
#  if _MSC_VER >= 1910 && defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#    define EXPLICIT_NODISCARD_ATTR [[nodiscard]]
#    define EXPLICIT_NODISCARD_TYPE_ATTR [[nodiscard]]
#  else
#    define EXPLICIT_NODISCARD_ATTR _Check_return_
#    define EXPLICIT_NODISCARD_TYPE_ATTR
#  endif
#else
#  define EXPLICIT_NODISCARD_ATTR
#  define EXPLICIT_NODISCARD_TYPE_ATTR
#endif

/**
 * @brief maybe_unused, 标记忽略unused警告
 * usage:
 *   EXPLICIT_UNUSED_ATTR int a;
 *   class EXPLICIT_UNUSED_ATTR a;
 *   EXPLICIT_UNUSED_ATTR int a();
 */
#if defined(__cplusplus) && __cplusplus >= 201703L
#  define EXPLICIT_UNUSED_ATTR [[maybe_unused]]
#elif defined(__clang__)
#  define EXPLICIT_UNUSED_ATTR __attribute__((unused))
#elif defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#  define EXPLICIT_UNUSED_ATTR __attribute__((unused))
#elif defined(_MSC_VER) && _MSC_VER >= 1700  // vs 2012 or higher
#  if _MSC_VER >= 1910 && defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#    define EXPLICIT_UNUSED_ATTR [[maybe_unused]]
#  else
#    define EXPLICIT_UNUSED_ATTR
#  endif
#else
#  define EXPLICIT_UNUSED_ATTR
#endif

/**
 * @brief fallthrough, 标记忽略switch内case的无break警告
 * usage:
 *   EXPLICIT_FALLTHROUGH int a;
 *   switch (xxx) {
 *      case XXX:
 *      EXPLICIT_FALLTHROUGH
 */
#if defined(__cplusplus) && __cplusplus >= 201703L
#  define EXPLICIT_FALLTHROUGH [[fallthrough]];
#elif defined(__clang__) && ((__clang_major__ * 100) + __clang_minor__) >= 309
#  if defined(__apple_build_version__)
#    define EXPLICIT_FALLTHROUGH
#  elif defined(__has_warning) && __has_feature(cxx_attributes) && __has_warning("-Wimplicit-fallthrough")
#    define EXPLICIT_FALLTHROUGH [[clang::fallthrough]];
#  else
#    define EXPLICIT_FALLTHROUGH
#  endif
#elif defined(__GNUC__) && (__GNUC__ >= 7)
#  define EXPLICIT_FALLTHROUGH [[gnu::fallthrough]];
#elif defined(_MSC_VER) && _MSC_VER >= 1700  // vs 2012 or higher
#  if _MSC_VER >= 1910 && defined(_MSVC_LANG) && _MSVC_LANG >= 201703L
#    define EXPLICIT_FALLTHROUGH [[fallthrough]];
#  else
#    define EXPLICIT_FALLTHROUGH
#  endif
#else
#  define EXPLICIT_FALLTHROUGH
#endif

/**
 * @brief may_alias, 标记允许类型别名(strict-aliasing)
 * usage:
 *   using target_type = EXPLICIT_MAY_ALIAS unsigned char[N];
 *   target_type a;
 */
#if defined(__clang__)
#  define EXPLICIT_MAY_ALIAS __attribute__((__may_alias__))
#elif defined(__GNUC__) && (__GNUC__ >= 4)
#  define EXPLICIT_MAY_ALIAS __attribute__((__may_alias__))
#else
#  define EXPLICIT_MAY_ALIAS
#endif

#if !defined(EXPLICIT_NORETURN_ATTR) && defined(__has_cpp_attribute)
#  if __has_cpp_attribute(noreturn)
#    define EXPLICIT_NORETURN_ATTR [[noreturn]]
#  endif
#endif
#ifndef EXPLICIT_NORETURN_ATTR
#  define EXPLICIT_NORETURN_ATTR
#endif

#ifndef EXPLICIT_UNREACHABLE
#  if defined(__cpp_lib_unreachable)
#    if __cpp_lib_unreachable
#      define EXPLICIT_UNREACHABLE() std::unreachable()
#    endif
#  endif
#  if !defined(EXPLICIT_UNREACHABLE) && defined(unreachable)
#    define EXPLICIT_UNREACHABLE() unreachable()
#  endif
#  if !defined(EXPLICIT_UNREACHABLE)
#    ifdef __GNUC__
#      ifdef __clang__
#        if __has_builtin(__builtin_unreachable)
#          define EXPLICIT_UNREACHABLE() __builtin_unreachable()
#        endif
#      else
#        if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#          define EXPLICIT_UNREACHABLE() __builtin_unreachable()
#        endif
#      endif
#    endif
#  endif
#endif
#if !defined(EXPLICIT_UNREACHABLE)
#  if defined(_DEBUG) || !defined(NDEBUG)
#    define EXPLICIT_UNREACHABLE() std::abort()
#  else
#    define EXPLICIT_UNREACHABLE()
#  endif
#endif
