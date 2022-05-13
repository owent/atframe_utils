// Copyright 2022 atframework

#pragma once

// ================ branch prediction information ================
#if !defined(UTIL_LIKELY_IF) && defined(__cplusplus)
#  if defined(__has_cpp_attribute) && (defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 9)
#    if __has_cpp_attribute(likely)
#      define UTIL_LIKELY_IF(...) if (__VA_ARGS__) [[likely]]
#    endif
#  endif
#endif
#if !defined(UTIL_LIKELY_IF) && (defined(__clang__) || defined(__GNUC__))
#  define UTIL_LIKELY_IF(...) if (__builtin_expect(!!(__VA_ARGS__), true))
#endif
#ifndef UTIL_LIKELY_IF
#  define UTIL_LIKELY_IF(...) if (__VA_ARGS__)
#endif

#if !defined(UTIL_UNLIKELY_IF) && defined(__cplusplus)
#  if defined(__has_cpp_attribute) && (defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 9)
#    if __has_cpp_attribute(likely)
#      define COPP_UNLIKELY_IF(...) if (__VA_ARGS__) [[unlikely]]
#    endif
#  endif
#endif
#if !defined(UTIL_UNLIKELY_IF) && (defined(__clang__) || defined(__GNUC__))
#  define UTIL_UNLIKELY_IF(...) if (__builtin_expect(!!(__VA_ARGS__), false))
#endif
#ifndef UTIL_UNLIKELY_IF
#  define UTIL_UNLIKELY_IF(...) if (__VA_ARGS__)
#endif

// ---------------- branch prediction information ----------------

// ================ import/export ================
// @see https://gcc.gnu.org/wiki/Visibility
// @see http://releases.llvm.org/9.0.0/tools/clang/docs/AttributeReference.html
// 不支持 borland/sunpro_cc/xlcpp

// ================ import/export: for compilers ================
#if defined(__GNUC__) && !defined(__ibmxl__)
//  GNU C++/Clang
//
// Dynamic shared object (DSO) and dynamic-link library (DLL) support
//
#  if __GNUC__ >= 4
#    if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
// All Win32 development environments, including 64-bit Windows and MinGW, define
// _WIN32 or one of its variant spellings. Note that Cygwin is a POSIX environment,
// so does not define _WIN32 or its variants.
#      ifndef UTIL_SYMBOL_EXPORT
#        define UTIL_SYMBOL_EXPORT __attribute__((__dllexport__))
#      endif
#      ifndef UTIL_SYMBOL_IMPORT
#        define UTIL_SYMBOL_IMPORT __attribute__((__dllimport__))
#      endif

#    else

#      ifndef UTIL_SYMBOL_EXPORT
#        define UTIL_SYMBOL_EXPORT __attribute__((visibility("default")))
#      endif
#      ifndef UTIL_SYMBOL_IMPORT
#        define UTIL_SYMBOL_IMPORT __attribute__((visibility("default")))
#      endif
#      ifndef UTIL_SYMBOL_VISIBLE
#        define UTIL_SYMBOL_VISIBLE __attribute__((visibility("default")))
#      endif
#      ifndef UTIL_SYMBOL_LOCAL
#        define UTIL_SYMBOL_LOCAL __attribute__((visibility("hidden")))
#      endif

#    endif

#  else
// config/platform/win32.hpp will define UTIL_SYMBOL_EXPORT, etc., unless already defined
#    ifndef UTIL_SYMBOL_EXPORT
#      define UTIL_SYMBOL_EXPORT
#    endif

#    ifndef UTIL_SYMBOL_IMPORT
#      define UTIL_SYMBOL_IMPORT
#    endif
#    ifndef UTIL_SYMBOL_VISIBLE
#      define UTIL_SYMBOL_VISIBLE
#    endif
#    ifndef UTIL_SYMBOL_LOCAL
#      define UTIL_SYMBOL_LOCAL
#    endif

#  endif

#elif defined(_MSC_VER)
//  Microsoft Visual C++
//
//  Must remain the last #elif since some other vendors (Metrowerks, for
//  example) also #define _MSC_VER
#else
#endif
// ---------------- import/export: for compilers ----------------

// ================ import/export: for platform ================
//  Default defines for UTIL_SYMBOL_EXPORT and UTIL_SYMBOL_IMPORT
//  If a compiler doesn't support __declspec(dllexport)/__declspec(dllimport),
//  its boost/config/compiler/ file must define UTIL_SYMBOL_EXPORT and
//  UTIL_SYMBOL_IMPORT
#if !defined(UTIL_SYMBOL_EXPORT) && (defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__))

#  ifndef UTIL_SYMBOL_EXPORT
#    define UTIL_SYMBOL_EXPORT __declspec(dllexport)
#  endif
#  ifndef UTIL_SYMBOL_IMPORT
#    define UTIL_SYMBOL_IMPORT __declspec(dllimport)
#  endif
#endif
// ---------------- import/export: for platform ----------------

#ifndef UTIL_SYMBOL_EXPORT
#  define UTIL_SYMBOL_EXPORT
#endif
#ifndef UTIL_SYMBOL_IMPORT
#  define UTIL_SYMBOL_IMPORT
#endif
#ifndef UTIL_SYMBOL_VISIBLE
#  define UTIL_SYMBOL_VISIBLE
#endif
#ifndef UTIL_SYMBOL_LOCAL
#  define UTIL_SYMBOL_LOCAL
#endif
#ifndef UTIL_SYMBOL_NONE
#  define UTIL_SYMBOL_NONE
#endif

// ---------------- import/export ----------------

// ================ __cdecl ================
#if defined(__GNUC__) || defined(__GNUG__)
#  ifndef __cdecl
// see https://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Function-Attributes.html
// Intel x86 architecture specific calling conventions
#    ifdef _M_IX86
#      define __cdecl __attribute__((__cdecl__))
#    else
#      define __cdecl
#    endif
#  endif
#endif
// ---------------- __cdecl ----------------

// ================ always inline ================

#ifndef UTIL_FORCEINLINE
#  if defined(__clang__)
#    if __has_attribute(always_inline)
#      define UTIL_FORCEINLINE __attribute__((always_inline))
#    else
#      define UTIL_FORCEINLINE inline
#    endif

#  elif defined(__GNUC__) && __GNUC__ > 3
#    define UTIL_FORCEINLINE __attribute__((always_inline))
#  elif defined(_MSC_VER)
#    define UTIL_FORCEINLINE __forceinline
#  else
#    define UTIL_FORCEINLINE inline
#  endif
#endif

#ifndef UTIL_NOINLINE
#  if defined(_MSC_VER)
#    define UTIL_NOINLINE __declspec(noinline)
#  elif defined(__GNUC__) && __GNUC__ > 3
// Clang also defines __GNUC__ (as 4)
#    if defined(__CUDACC__)
// nvcc doesn't always parse __noinline__,
// see: https://svn.boost.org/trac/boost/ticket/9392
#      define UTIL_NOINLINE __attribute__((noinline))
#    else
#      define UTIL_NOINLINE __attribute__((__noinline__))
#    endif
#  else
#    define UTIL_NOINLINE
#  endif
#endif
