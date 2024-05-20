// Copyright 2024 atframework

#pragma once

// UTIL_HAVE_BUILTIN&UTIL_HAVE_FEATURE
//
// Checks whether the compiler supports a Clang Feature Checking Macro, and if
// so, checks whether it supports the provided builtin function "x" where x
// is one of the functions noted in
// https://clang.llvm.org/docs/LanguageExtensions.html
//
// Note: Use this macro to avoid an extra level of #ifdef __has_builtin check.
// http://releases.llvm.org/3.3/tools/clang/docs/LanguageExtensions.html
#if !defined(UTIL_HAVE_BUILTIN)
#  ifdef __has_builtin
#    define UTIL_HAVE_BUILTIN(x) __has_builtin(x)
#  else
#    define UTIL_HAVE_BUILTIN(x) 0
#  endif
#endif

#if !defined(UTIL_HAVE_FEATURE)
#  ifdef __has_feature
#    define UTIL_HAVE_FEATURE(f) __has_feature(f)
#  else
#    define UTIL_HAVE_FEATURE(f) 0
#  endif
#endif

// ================ has feature ================
// UTIL_HAVE_ATTRIBUTE
//
// A function-like feature checking macro that is a wrapper around
// `__has_attribute`, which is defined by GCC 5+ and Clang and evaluates to a
// nonzero constant integer if the attribute is supported or 0 if not.
//
// It evaluates to zero if `__has_attribute` is not defined by the compiler.
//
// GCC: https://gcc.gnu.org/gcc-5/changes.html
// Clang: https://clang.llvm.org/docs/LanguageExtensions.html
#if !defined(UTIL_HAVE_ATTRIBUTE)
#  ifdef __has_attribute
#    define UTIL_HAVE_ATTRIBUTE(x) __has_attribute(x)
#  else
#    define UTIL_HAVE_ATTRIBUTE(x) 0
#  endif
#endif

// UTIL_HAVE_CPP_ATTRIBUTE
//
// A function-like feature checking macro that accepts C++11 style attributes.
// It's a wrapper around `__has_cpp_attribute`, defined by ISO C++ SD-6
// (https://en.cppreference.com/w/cpp/experimental/feature_test). If we don't
// find `__has_cpp_attribute`, will evaluate to 0.
#if !defined(UTIL_HAVE_CPP_ATTRIBUTE)
#  if defined(__cplusplus) && defined(__has_cpp_attribute)
// NOTE: requiring __cplusplus above should not be necessary, but
// works around https://bugs.llvm.org/show_bug.cgi?id=23435.
#    define UTIL_HAVE_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#  else
#    define UTIL_HAVE_CPP_ATTRIBUTE(x) 0
#  endif
#endif

// ================ branch prediction information ================
#if !defined(UTIL_LIKELY_IF) && defined(__cplusplus)
// GCC 9 has likely attribute but do not support declare it at the beginning of statement
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
#if !defined(UTIL_LIKELY_CONDITION) && defined(__cplusplus)
// GCC 9 has likely attribute but do not support declare it at the beginning of statement
#  if defined(__has_cpp_attribute) && (defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 9)
#    if __has_cpp_attribute(likely)
#      define UTIL_LIKELY_CONDITION(__C) (__C) [[likely]]
#    endif
#  endif
#endif
#if !defined(UTIL_LIKELY_CONDITION) && (defined(__clang__) || defined(__GNUC__))
#  define UTIL_LIKELY_CONDITION(__C) (__builtin_expect(!!(__C), true))
#endif
#ifndef UTIL_LIKELY_CONDITION
#  define UTIL_LIKELY_CONDITION(__C) (__C)
#endif

#if !defined(UTIL_UNLIKELY_IF) && defined(__cplusplus)
// GCC 9 has likely attribute but do not support declare it at the beginning of statement
#  if defined(__has_cpp_attribute) && (defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 9)
#    if __has_cpp_attribute(likely)
#      define UTIL_UNLIKELY_IF(...) if (__VA_ARGS__) [[unlikely]]
#    endif
#  endif
#endif
#if !defined(UTIL_UNLIKELY_IF) && (defined(__clang__) || defined(__GNUC__))
#  define UTIL_UNLIKELY_IF(...) if (__builtin_expect(!!(__VA_ARGS__), false))
#endif
#ifndef UTIL_UNLIKELY_IF
#  define UTIL_UNLIKELY_IF(...) if (__VA_ARGS__)
#endif
#if !defined(UTIL_UNLIKELY_CONDITION) && defined(__cplusplus)
// GCC 9 has likely attribute but do not support declare it at the beginning of statement
#  if defined(__has_cpp_attribute) && (defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 9)
#    if __has_cpp_attribute(likely)
#      define UTIL_UNLIKELY_CONDITION(__C) (__C) [[unlikely]]
#    endif
#  endif
#endif
#if !defined(UTIL_UNLIKELY_CONDITION) && (defined(__clang__) || defined(__GNUC__))
#  define UTIL_UNLIKELY_CONDITION(__C) (__builtin_expect(!!(__C), false))
#endif
#ifndef UTIL_UNLIKELY_CONDITION
#  define UTIL_UNLIKELY_CONDITION(__C) (__C)
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
#    if __cplusplus >= 201103L
#      define UTIL_FORCEINLINE [[gnu::always_inline]] inline
#    else
#      define UTIL_FORCEINLINE __attribute__((always_inline)) inline
#    endif
#  elif defined(__GNUC__) && __GNUC__ > 3
#    if __cplusplus >= 201103L && (__GNUC__ * 100 + __GNUC_MINOR__) >= 408
#      define UTIL_FORCEINLINE [[gnu::always_inline]] inline
#    else
#      define UTIL_FORCEINLINE __attribute__((always_inline)) inline
#    endif
#  elif defined(_MSC_VER)
#    define UTIL_FORCEINLINE __forceinline
#  else
#    define UTIL_FORCEINLINE inline
#  endif
#endif

#ifndef UTIL_NOINLINE_NOCLONE
#  if defined(__clang__)
#    if __cplusplus >= 201103L
#      define UTIL_NOINLINE_NOCLONE [[gnu::noinline]]
#    else
#      define UTIL_NOINLINE_NOCLONE __attribute__((noinline))
#    endif
#  elif defined(__GNUC__) && __GNUC__ > 3
#    if __cplusplus >= 201103L && (__GNUC__ * 100 + __GNUC_MINOR__) >= 408
#      define UTIL_NOINLINE_NOCLONE [[gnu::noinline, gnu::noclone]]
#    else
#      define UTIL_NOINLINE_NOCLONE __attribute__((noinline, noclone))
#    endif
#  elif defined(_MSC_VER)
#    define UTIL_NOINLINE_NOCLONE __declspec(noinline)
#  else
#    define UTIL_NOINLINE_NOCLONE
#  endif
#endif

#ifndef UTIL_CONST_INIT
#  if defined(__cpp_constinit) && __cpp_constinit >= 201907L
#    if defined(_MSC_VER)
#      define UTIL_CONST_INIT
#    else
#      define UTIL_CONST_INIT constinit
#    endif
#  elif UTIL_HAVE_CPP_ATTRIBUTE(clang::require_constant_initialization)
#    define UTIL_CONST_INIT [[clang::require_constant_initialization]]
#  else
#    define UTIL_CONST_INIT
#  endif
#endif

// UTIL_ATTRIBUTE_LIFETIME_BOUND indicates that a resource owned by a function
// parameter or implicit object parameter is retained by the return value of the
// annotated function (or, for a parameter of a constructor, in the value of the
// constructed object). This attribute causes warnings to be produced if a
// temporary object does not live long enough.
//
// When applied to a reference parameter, the referenced object is assumed to be
// retained by the return value of the function. When applied to a non-reference
// parameter (for example, a pointer or a class type), all temporaries
// referenced by the parameter are assumed to be retained by the return value of
// the function.
//
// See also the upstream documentation:
// https://clang.llvm.org/docs/AttributeReference.html#lifetimebound
#ifndef UTIL_ATTRIBUTE_LIFETIME_BOUND
#  if UTIL_HAVE_CPP_ATTRIBUTE(clang::lifetimebound)
#    define UTIL_ATTRIBUTE_LIFETIME_BOUND [[clang::lifetimebound]]
#  elif UTIL_HAVE_ATTRIBUTE(lifetimebound)
#    define UTIL_ATTRIBUTE_LIFETIME_BOUND __attribute__((lifetimebound))
#  else
#    define UTIL_ATTRIBUTE_LIFETIME_BOUND
#  endif
#endif

// UTIL_HAVE_MEMORY_SANITIZER
//
// MemorySanitizer (MSan) is a detector of uninitialized reads. It consists of
// a compiler instrumentation module and a run-time library.
#ifndef UTIL_HAVE_MEMORY_SANITIZER
#  if !defined(__native_client__) && UTIL_HAVE_FEATURE(memory_sanitizer)
#    define UTIL_HAVE_MEMORY_SANITIZER 1
#  else
#    define UTIL_HAVE_MEMORY_SANITIZER 0
#  endif
#endif

#if UTIL_HAVE_MEMORY_SANITIZER && UTIL_HAVE_ATTRIBUTE(no_sanitize_memory)
#  define UTIL_SANITIZER_NO_MEMORY __attribute__((no_sanitize_memory))  // __attribute__((no_sanitize("memory")))
#else
#  define UTIL_SANITIZER_NO_MEMORY
#endif

// UTIL_HAVE_THREAD_SANITIZER
//
// ThreadSanitizer (TSan) is a fast data race detector.
#ifndef UTIL_HAVE_THREAD_SANITIZER
#  if defined(__SANITIZE_THREAD__)
#    define UTIL_HAVE_THREAD_SANITIZER 1
#  elif UTIL_HAVE_FEATURE(thread_sanitizer)
#    define UTIL_HAVE_THREAD_SANITIZER 1
#  else
#    define UTIL_HAVE_THREAD_SANITIZER 0
#  endif
#endif

#if UTIL_HAVE_THREAD_SANITIZER && UTIL_HAVE_ATTRIBUTE(no_sanitize_thread)
#  define UTIL_SANITIZER_NO_THREAD __attribute__((no_sanitize_thread))  // __attribute__((no_sanitize("thread")))
#else
#  define UTIL_SANITIZER_NO_THREAD
#endif

// UTIL_HAVE_ADDRESS_SANITIZER
//
// AddressSanitizer (ASan) is a fast memory error detector.
#ifndef UTIL_HAVE_ADDRESS_SANITIZER
#  if defined(__SANITIZE_ADDRESS__)
#    define UTIL_HAVE_ADDRESS_SANITIZER 1
#  elif UTIL_HAVE_FEATURE(address_sanitizer)
#    define UTIL_HAVE_ADDRESS_SANITIZER 1
#  else
#    define UTIL_HAVE_ADDRESS_SANITIZER 0
#  endif
#endif

// UTIL_HAVE_HWADDRESS_SANITIZER
//
// Hardware-Assisted AddressSanitizer (or HWASAN) is even faster than asan
// memory error detector which can use CPU features like ARM TBI, Intel LAM or
// AMD UAI.
#ifndef UTIL_HAVE_HWADDRESS_SANITIZER
#  if defined(__SANITIZE_HWADDRESS__)
#    define UTIL_HAVE_HWADDRESS_SANITIZER 1
#  elif UTIL_HAVE_FEATURE(hwaddress_sanitizer)
#    define UTIL_HAVE_HWADDRESS_SANITIZER 1
#  else
#    define UTIL_HAVE_HWADDRESS_SANITIZER 0
#  endif
#endif

#if UTIL_HAVE_ADDRESS_SANITIZER && UTIL_HAVE_ATTRIBUTE(no_sanitize_address)
#  define UTIL_SANITIZER_NO_ADDRESS __attribute__((no_sanitize_address))  // __attribute__((no_sanitize("address")))
#elif UTIL_HAVE_ADDRESS_SANITIZER && defined(_MSC_VER) && _MSC_VER >= 1928
#  define UTIL_SANITIZER_NO_ADDRESS __declspec(no_sanitize_address)
#elif UTIL_HAVE_HWADDRESS_SANITIZER && UTIL_HAVE_ATTRIBUTE(no_sanitize)
#  define UTIL_SANITIZER_NO_ADDRESS __attribute__((no_sanitize("hwaddress")))
#else
#  define UTIL_SANITIZER_NO_ADDRESS
#endif

// UTIL_HAVE_DATAFLOW_SANITIZER
//
// Dataflow Sanitizer (or DFSAN) is a generalised dynamic data flow analysis.
#ifndef UTIL_HAVE_DATAFLOW_SANITIZER
#  if defined(DATAFLOW_SANITIZER)
// GCC provides no method for detecting the presence of the standalone
// DataFlowSanitizer (-fsanitize=dataflow), so GCC users of -fsanitize=dataflow
// should also use -DDATAFLOW_SANITIZER.
#    define UTIL_HAVE_DATAFLOW_SANITIZER 1
#  elif UTIL_HAVE_FEATURE(dataflow_sanitizer)
#    define UTIL_HAVE_DATAFLOW_SANITIZER 1
#  else
#    define UTIL_HAVE_DATAFLOW_SANITIZER 0
#  endif
#endif

// UTIL_HAVE_LEAK_SANITIZER
//
// LeakSanitizer (or lsan) is a detector of memory leaks.
// https://clang.llvm.org/docs/LeakSanitizer.html
// https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer
//
// The macro UTIL_HAVE_LEAK_SANITIZER can be used to detect at compile-time
// whether the LeakSanitizer is potentially available. However, just because the
// LeakSanitizer is available does not mean it is active. Use the
// always-available run-time interface in //absl/debugging/leak_check.h for
// interacting with LeakSanitizer.
#ifndef UTIL_HAVE_LEAK_SANITIZER
#  if defined(LEAK_SANITIZER)
// GCC provides no method for detecting the presence of the standalone
// LeakSanitizer (-fsanitize=leak), so GCC users of -fsanitize=leak should also
// use -DLEAK_SANITIZER.
#    define UTIL_HAVE_LEAK_SANITIZER 1
// Clang standalone LeakSanitizer (-fsanitize=leak)
#  elif UTIL_HAVE_FEATURE(leak_sanitizer)
#    define UTIL_HAVE_LEAK_SANITIZER 1
#  elif defined(ABSL_HAVE_ADDRESS_SANITIZER)
// GCC or Clang using the LeakSanitizer integrated into AddressSanitizer.
#    define UTIL_HAVE_LEAK_SANITIZER 1
#  else
#    define UTIL_HAVE_LEAK_SANITIZER 0
#  endif
#endif

#ifndef UTIL_SANITIZER_NO_UNDEFINED
#  if UTIL_HAVE_ATTRIBUTE(no_sanitize_undefined)
#    define UTIL_SANITIZER_NO_UNDEFINED __attribute__((no_sanitize_undefined))
#  elif UTIL_HAVE_ATTRIBUTE(no_sanitize)
#    define UTIL_SANITIZER_NO_UNDEFINED __attribute__((no_sanitize("undefined")))
#  else
#    define UTIL_SANITIZER_NO_UNDEFINED
#  endif
#endif

#ifndef UTIL_MACRO_INLINE_VARIABLE
#  if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    define UTIL_MACRO_INLINE_VARIABLE inline
#  else
#    define UTIL_MACRO_INLINE_VARIABLE
#  endif
#endif
