#ifndef UTIL_CONFIG_COMPILE_OPTIMIZE_H
#define UTIL_CONFIG_COMPILE_OPTIMIZE_H

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
#if __GNUC__ >= 4
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
// All Win32 development environments, including 64-bit Windows and MinGW, define
// _WIN32 or one of its variant spellings. Note that Cygwin is a POSIX environment,
// so does not define _WIN32 or its variants.
#define BOOST_HAS_DECLSPEC
#define UTIL_SYMBOL_EXPORT __attribute__((__dllexport__))
#define UTIL_SYMBOL_IMPORT __attribute__((__dllimport__))
#else
#define UTIL_SYMBOL_EXPORT __attribute__((__visibility__("default")))
#define UTIL_SYMBOL_IMPORT __attribute__((__visibility__("default")))
#define UTIL_SYMBOL_VISIBLE __attribute__((__visibility__("default")))
#define UTIL_SYMBOL_HIDDEN __attribute__((__visibility__("hidden")))
#endif
#else
// config/platform/win32.hpp will define UTIL_SYMBOL_EXPORT, etc., unless already defined
#define UTIL_SYMBOL_EXPORT
#define UTIL_SYMBOL_IMPORT
#define UTIL_SYMBOL_VISIBLE
#define UTIL_SYMBOL_HIDDEN
#endif

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
#define UTIL_SYMBOL_EXPORT __declspec(dllexport)
#define UTIL_SYMBOL_IMPORT __declspec(dllimport)
#endif
// ---------------- import/export: for platform ----------------

#ifndef UTIL_SYMBOL_EXPORT
#define UTIL_SYMBOL_EXPORT
#endif
#ifndef UTIL_SYMBOL_IMPORT
#define UTIL_SYMBOL_IMPORT
#endif
#ifndef UTIL_SYMBOL_VISIBLE
#define UTIL_SYMBOL_VISIBLE
#endif
#ifndef UTIL_SYMBOL_HIDDEN
#define UTIL_SYMBOL_HIDDEN
#endif

// ---------------- import/export ----------------

// ================ __cdecl ================
#if defined(__GNUC__)
#ifndef __cdecl
// see https://gcc.gnu.org/onlinedocs/gcc-4.0.0/gcc/Function-Attributes.html
// Intel x86 architecture specific calling conventions
#ifdef _M_IX86
#define __cdecl __attribute__((__cdecl__))
#else
#define __cdecl
#endif
#endif
#endif
// ---------------- __cdecl ----------------

#endif