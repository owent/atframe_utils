/**
 * @file compiler_message.h
 * @brief 编译器适配
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2014.12.15
 *
 * @history
 *
 *
 */

#ifndef UTIL_COMMON_COMPILER_MESSAGE_H
#define UTIL_COMMON_COMPILER_MESSAGE_H

// 目测主流编译器都支持且有优化， gcc 3.4 and upper, vc, clang, c++ builder xe3, intel c++ and etc.
#pragma once

// 宏展开在预处理之后，仅作为参考，不可使用

#define COMPILER_MSG_STR(x) #x
#define COMPILER_MSG_DEFER(M, ...) M(__VA_ARGS__)

#if defined(__clang__) || defined(__GNUC__)
#  define COMPILER_PRAGMA(x) _Pragma(#  x)

#  define COMPILER_MSG_INFO(...) COMPILER_PRAGMA(message(__VA_ARGS__))
#  define COMPILER_MSG_WARN(...) COMPILER_PRAGMA(GCC warning __VA_ARGS__)
#  define COMPILER_MSG_ERROR(...) COMPILER_PRAGMA(GCC error __VA_ARGS__)

#elif defined(_MSC_VER)
// VC 2008 以上才支持动态的pragma
#  if _MSC_VER >= 1500
#    define COMPILER_PRAGMA(...) __pragma(__VA_ARGS__)

#    define COMPILER_MSG_CONTENT(TYPE, ...) \
      __FILE__ ":" COMPILER_MSG_DEFER(COMPILER_MSG_STR, __LINE__) ": " TYPE __VA_ARGS__

#    define COMPILER_MSG_INFO(...) __pragma(message(COMPILER_MSG_CONTENT("[info]: ", __VA_ARGS__)))
#    define COMPILER_MSG_WARN(...) __pragma(message(COMPILER_MSG_CONTENT("[warn]: ", __VA_ARGS__)))
#    define COMPILER_MSG_ERROR(...) \
      __pragma(message(COMPILER_MSG_CONTENT("[error]: ", __VA_ARGS__))) __pragma(message(false))

// #undef COMPILER_MSG_CONTENT
// #define COMPILER_MSG_ERROR(...) #error __VA_ARGS__
#  else
#    define COMPILER_PRAGMA(...)

#    define COMPILER_MSG_INFO(...)
#    define COMPILER_MSG_WARN(...)
#    define COMPILER_MSG_ERROR(...)
#  endif
#elif defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
#  define COMPILER_PRAGMA(...) __pragma(__VA_ARGS__)

#  define COMPILER_MSG_CONTENT(TYPE, ...) \
    __FILE__ ":" COMPILER_MSG_DEFER(COMPILER_MSG_STR, __LINE__) ": " TYPE __VA_ARGS__

#  define COMPILER_MSG_INFO(...) __pragma(message(COMPILER_MSG_CONTENT("[info]: ", __VA_ARGS__)))
#  define COMPILER_MSG_WARN(...) __pragma(message(COMPILER_MSG_CONTENT("[warn]: ", __VA_ARGS__)))
#  define COMPILER_MSG_ERROR(...) \
    __pragma(message(COMPILER_MSG_CONTENT("[error]: ", __VA_ARGS__))) __pragma(message(false))

// #undef COMPILER_MSG_CONTENT
// #define COMPILER_MSG_ERROR(...) #error __VA_ARGS__

#else
// 默认尝试GCC方案, 一些编译器会做gcc适配，如果编译不过请直接重定义为空
#  define COMPILER_PRAGMA(x) _Pragma(#  x)

#  define COMPILER_MSG_INFO(...) COMPILER_PRAGMA(message(__VA_ARGS__))
#  define COMPILER_MSG_WARN(...) COMPILER_PRAGMA(GCC warning __VA_ARGS__)
#  define COMPILER_MSG_ERROR(...) COMPILER_PRAGMA(GCC error __VA_ARGS__)

#endif

#define COMPILER_UNUSED(x) ((void)(x)) /* to avoid warnings */

#endif /* _UTIL_COMMON_COMPILER_MESSAGE_H_ */
