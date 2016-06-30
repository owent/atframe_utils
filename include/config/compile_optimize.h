#ifndef _UTIL_CONFIG_COMPILE_OPTIMIZE_H_
#define _UTIL_CONFIG_COMPILE_OPTIMIZE_H_

#pragma once


// ================ branch prediction information ================
#ifndef likely
#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#else
#define likely(x) !!(x)
#endif
#endif

#ifndef unlikely
#ifdef __GNUC__
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define unlikely(x) !!(x)
#endif
#endif

#ifndef unreachable
#ifdef __GNUC__
#ifdef __clang__
#if __has_builtin(__builtin_unreachable)
#define unreachable() __builtin_unreachable()
#else
#define unreachable() abort()
#endif
#else
#if (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 6))
#define unreachable() __builtin_unreachable()
#else
#define unreachable() abort()
#endif
#endif
#else
#define unreachable() abort()
#endif
#endif
// ---------------- branch prediction information ----------------

#endif