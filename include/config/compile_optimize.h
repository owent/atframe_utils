// Copyright 2025 atframework

#pragma once

// ATFW_UTIL_HAVE_BUILTIN&ATFW_UTIL_HAVE_FEATURE
//
// Checks whether the compiler supports a Clang Feature Checking Macro, and if
// so, checks whether it supports the provided builtin function "x" where x
// is one of the functions noted in
// https://clang.llvm.org/docs/LanguageExtensions.html
//
// Note: Use this macro to avoid an extra level of #ifdef __has_builtin check.
// http://releases.llvm.org/3.3/tools/clang/docs/LanguageExtensions.html
#if !defined(ATFW_UTIL_HAVE_BUILTIN)
#  ifdef __has_builtin
#    define ATFW_UTIL_HAVE_BUILTIN(x) __has_builtin(x)
#    define UTIL_HAVE_BUILTIN(x) __has_builtin(x)
#  else
#    define ATFW_UTIL_HAVE_BUILTIN(x) 0
#    define UTIL_HAVE_BUILTIN(x) 0
#  endif
#endif

#if !defined(ATFW_UTIL_HAVE_FEATURE)
#  ifdef __has_feature
#    define ATFW_UTIL_HAVE_FEATURE(f) __has_feature(f)
#    define UTIL_HAVE_FEATURE(f) __has_feature(f)
#  else
#    define ATFW_UTIL_HAVE_FEATURE(f) 0
#    define UTIL_HAVE_FEATURE(f) 0
#  endif
#endif

// ================ has feature ================
// ATFW_UTIL_HAVE_ATTRIBUTE
//
// A function-like feature checking macro that is a wrapper around
// `__has_attribute`, which is defined by GCC 5+ and Clang and evaluates to a
// nonzero constant integer if the attribute is supported or 0 if not.
//
// It evaluates to zero if `__has_attribute` is not defined by the compiler.
//
// GCC: https://gcc.gnu.org/gcc-5/changes.html
// Clang: https://clang.llvm.org/docs/LanguageExtensions.html
#if !defined(ATFW_UTIL_HAVE_ATTRIBUTE)
#  ifdef __has_attribute
#    define ATFW_UTIL_HAVE_ATTRIBUTE(x) __has_attribute(x)
#    define UTIL_HAVE_ATTRIBUTE(x) __has_attribute(x)
#  else
#    define ATFW_UTIL_HAVE_ATTRIBUTE(x) 0
#    define UTIL_HAVE_ATTRIBUTE(x) 0
#  endif
#endif

// ATFW_UTIL_HAVE_CPP_ATTRIBUTE
//
// A function-like feature checking macro that accepts C++11 style attributes.
// It's a wrapper around `__has_cpp_attribute`, defined by ISO C++ SD-6
// (https://en.cppreference.com/w/cpp/experimental/feature_test). If we don't
// find `__has_cpp_attribute`, will evaluate to 0.
#if !defined(ATFW_UTIL_HAVE_CPP_ATTRIBUTE)
#  if defined(__cplusplus) && defined(__has_cpp_attribute)
// NOTE: requiring __cplusplus above should not be necessary, but
// works around https://bugs.llvm.org/show_bug.cgi?id=23435.
#    define ATFW_UTIL_HAVE_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#    define UTIL_HAVE_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#  else
#    define ATFW_UTIL_HAVE_CPP_ATTRIBUTE(x) 0
#    define UTIL_HAVE_CPP_ATTRIBUTE(x) 0
#  endif
#endif

// ================ branch prediction information ================
#if !defined(ATFW_UTIL_LIKELY_IF) && defined(__cplusplus)
// GCC 9 has likely attribute but do not support declare it at the beginning of statement
#  if defined(__has_cpp_attribute) && (defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 9)
#    if __has_cpp_attribute(likely)
#      define ATFW_UTIL_LIKELY_IF(...) if (__VA_ARGS__) [[likely]]
#    endif
#  endif
#endif
#if !defined(ATFW_UTIL_LIKELY_IF) && (defined(__clang__) || defined(__GNUC__))
#  define ATFW_UTIL_LIKELY_IF(...) if (__builtin_expect(!!(__VA_ARGS__), true))
#endif
#ifndef ATFW_UTIL_LIKELY_IF
#  define ATFW_UTIL_LIKELY_IF(...) if (__VA_ARGS__)
#endif
#if !defined(ATFW_UTIL_LIKELY_CONDITION) && defined(__cplusplus)
// GCC 9 has likely attribute but do not support declare it at the beginning of statement
#  if defined(__has_cpp_attribute) && (defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 9)
#    if __has_cpp_attribute(likely)
#      define ATFW_UTIL_LIKELY_CONDITION(__C) (__C) [[likely]]
#    endif
#  endif
#endif
#if !defined(ATFW_UTIL_LIKELY_CONDITION) && (defined(__clang__) || defined(__GNUC__))
#  define ATFW_UTIL_LIKELY_CONDITION(__C) (__builtin_expect(!!(__C), true))
#endif
#ifndef ATFW_UTIL_LIKELY_CONDITION
#  define ATFW_UTIL_LIKELY_CONDITION(__C) (__C)
#endif

#if !defined(ATFW_UTIL_UNLIKELY_IF) && defined(__cplusplus)
// GCC 9 has likely attribute but do not support declare it at the beginning of statement
#  if defined(__has_cpp_attribute) && (defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 9)
#    if __has_cpp_attribute(likely)
#      define ATFW_UTIL_UNLIKELY_IF(...) if (__VA_ARGS__) [[unlikely]]
#    endif
#  endif
#endif
#if !defined(ATFW_UTIL_UNLIKELY_IF) && (defined(__clang__) || defined(__GNUC__))
#  define ATFW_UTIL_UNLIKELY_IF(...) if (__builtin_expect(!!(__VA_ARGS__), false))
#endif
#ifndef ATFW_UTIL_UNLIKELY_IF
#  define ATFW_UTIL_UNLIKELY_IF(...) if (__VA_ARGS__)
#endif
#if !defined(ATFW_UTIL_UNLIKELY_CONDITION) && defined(__cplusplus)
// GCC 9 has likely attribute but do not support declare it at the beginning of statement
#  if defined(__has_cpp_attribute) && (defined(__clang__) || !defined(__GNUC__) || __GNUC__ > 9)
#    if __has_cpp_attribute(likely)
#      define ATFW_UTIL_UNLIKELY_CONDITION(__C) (__C) [[unlikely]]
#    endif
#  endif
#endif
#if !defined(ATFW_UTIL_UNLIKELY_CONDITION) && (defined(__clang__) || defined(__GNUC__))
#  define ATFW_UTIL_UNLIKELY_CONDITION(__C) (__builtin_expect(!!(__C), false))
#endif
#ifndef ATFW_UTIL_UNLIKELY_CONDITION
#  define ATFW_UTIL_UNLIKELY_CONDITION(__C) (__C)
#endif

// Legacy macros
#ifndef UTIL_LIKELY_IF
#  define UTIL_LIKELY_IF(...) ATFW_UTIL_LIKELY_IF (__VA_ARGS__)
#endif
#ifndef UTIL_LIKELY_CONDITION
#  define UTIL_LIKELY_CONDITION(...) ATFW_UTIL_LIKELY_CONDITION(__VA_ARGS__)
#endif
#ifndef UTIL_UNLIKELY_IF
#  define UTIL_UNLIKELY_IF(...) ATFW_UTIL_UNLIKELY_IF (__VA_ARGS__)
#endif
#ifndef UTIL_UNLIKELY_CONDITION
#  define UTIL_UNLIKELY_CONDITION(...) ATFW_UTIL_UNLIKELY_CONDITION(__VA_ARGS__)
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
#      ifndef ATFW_UTIL_SYMBOL_EXPORT
#        define ATFW_UTIL_SYMBOL_EXPORT __attribute__((__dllexport__))
#      endif
#      ifndef ATFW_UTIL_SYMBOL_IMPORT
#        define ATFW_UTIL_SYMBOL_IMPORT __attribute__((__dllimport__))
#      endif

#    else

#      ifndef ATFW_UTIL_SYMBOL_EXPORT
#        define ATFW_UTIL_SYMBOL_EXPORT __attribute__((visibility("default")))
#      endif
#      ifndef ATFW_UTIL_SYMBOL_IMPORT
#        define ATFW_UTIL_SYMBOL_IMPORT __attribute__((visibility("default")))
#      endif
#      ifndef ATFW_UTIL_SYMBOL_VISIBLE
#        define ATFW_UTIL_SYMBOL_VISIBLE __attribute__((visibility("default")))
#      endif
#      ifndef ATFW_UTIL_SYMBOL_LOCAL
#        define ATFW_UTIL_SYMBOL_LOCAL __attribute__((visibility("hidden")))
#      endif

#    endif

#  else
// config/platform/win32.hpp will define ATFW_UTIL_SYMBOL_EXPORT, etc., unless already defined
#    ifndef ATFW_UTIL_SYMBOL_EXPORT
#      define ATFW_UTIL_SYMBOL_EXPORT
#    endif

#    ifndef ATFW_UTIL_SYMBOL_IMPORT
#      define ATFW_UTIL_SYMBOL_IMPORT
#    endif
#    ifndef ATFW_UTIL_SYMBOL_VISIBLE
#      define ATFW_UTIL_SYMBOL_VISIBLE
#    endif
#    ifndef ATFW_UTIL_SYMBOL_LOCAL
#      define ATFW_UTIL_SYMBOL_LOCAL
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
//  Default defines for ATFW_UTIL_SYMBOL_EXPORT and ATFW_UTIL_SYMBOL_IMPORT
//  If a compiler doesn't support __declspec(dllexport)/__declspec(dllimport),
//  its boost/config/compiler/ file must define UTIL_SYMBOL_EXPORT and
//  ATFW_UTIL_SYMBOL_IMPORT
#if !defined(ATFW_UTIL_SYMBOL_EXPORT) && \
    (defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__))

#  ifndef ATFW_UTIL_SYMBOL_EXPORT
#    define ATFW_UTIL_SYMBOL_EXPORT __declspec(dllexport)
#  endif
#  ifndef ATFW_UTIL_SYMBOL_IMPORT
#    define ATFW_UTIL_SYMBOL_IMPORT __declspec(dllimport)
#  endif
#endif
// ---------------- import/export: for platform ----------------

#ifndef ATFW_UTIL_SYMBOL_EXPORT
#  define ATFW_UTIL_SYMBOL_EXPORT
#endif
#ifndef ATFW_UTIL_SYMBOL_IMPORT
#  define ATFW_UTIL_SYMBOL_IMPORT
#endif
#ifndef ATFW_UTIL_SYMBOL_VISIBLE
#  define ATFW_UTIL_SYMBOL_VISIBLE
#endif
#ifndef ATFW_UTIL_SYMBOL_LOCAL
#  define ATFW_UTIL_SYMBOL_LOCAL
#endif
#ifndef ATFW_UTIL_SYMBOL_NONE
#  define ATFW_UTIL_SYMBOL_NONE
#endif

// Legacy macros
#ifndef UTIL_SYMBOL_EXPORT
#  define UTIL_SYMBOL_EXPORT ATFW_UTIL_SYMBOL_EXPORT
#endif
#ifndef UTIL_SYMBOL_IMPORT
#  define UTIL_SYMBOL_IMPORT ATFW_UTIL_SYMBOL_IMPORT
#endif
#ifndef UTIL_SYMBOL_VISIBLE
#  define UTIL_SYMBOL_VISIBLE ATFW_UTIL_SYMBOL_VISIBLE
#endif
#ifndef UTIL_SYMBOL_LOCAL
#  define UTIL_SYMBOL_LOCAL ATFW_UTIL_SYMBOL_LOCAL
#endif
#ifndef UTIL_SYMBOL_NONE
#  define UTIL_SYMBOL_NONE ATFW_UTIL_SYMBOL_NONE
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

#ifndef ATFW_UTIL_FORCEINLINE
#  if defined(__clang__)
#    if __cplusplus >= 201103L
#      define ATFW_UTIL_FORCEINLINE [[gnu::always_inline]] inline
#    else
#      define ATFW_UTIL_FORCEINLINE __attribute__((always_inline)) inline
#    endif
#  elif defined(__GNUC__) && __GNUC__ > 3
#    if __cplusplus >= 201103L && (__GNUC__ * 100 + __GNUC_MINOR__) >= 408
#      define ATFW_UTIL_FORCEINLINE [[gnu::always_inline]] inline
#    else
#      define ATFW_UTIL_FORCEINLINE __attribute__((always_inline)) inline
#    endif
#  elif defined(_MSC_VER)
#    define ATFW_UTIL_FORCEINLINE __forceinline
#  else
#    define ATFW_UTIL_FORCEINLINE inline
#  endif
#endif

// Legacy macros
#ifndef UTIL_FORCEINLINE
#  define UTIL_FORCEINLINE ATFW_UTIL_FORCEINLINE
#endif

#ifndef ATFW_UTIL_NOINLINE_NOCLONE
#  if defined(__clang__)
#    if __cplusplus >= 201103L
#      define ATFW_UTIL_NOINLINE_NOCLONE [[gnu::noinline]]
#    else
#      define ATFW_UTIL_NOINLINE_NOCLONE __attribute__((noinline))
#    endif
#  elif defined(__GNUC__) && __GNUC__ > 3
#    if __cplusplus >= 201103L && (__GNUC__ * 100 + __GNUC_MINOR__) >= 408
#      define ATFW_UTIL_NOINLINE_NOCLONE [[gnu::noinline, gnu::noclone]]
#    else
#      define ATFW_UTIL_NOINLINE_NOCLONE __attribute__((noinline, noclone))
#    endif
#  elif defined(_MSC_VER)
#    define ATFW_UTIL_NOINLINE_NOCLONE __declspec(noinline)
#  else
#    define ATFW_UTIL_NOINLINE_NOCLONE
#  endif
#endif

// Legacy macros
#ifndef UTIL_NOINLINE_NOCLONE
#  define UTIL_NOINLINE_NOCLONE ATFW_UTIL_NOINLINE_NOCLONE
#endif

#ifndef ATFW_UTIL_CONST_INIT
#  if defined(__cpp_constinit) && __cpp_constinit >= 201907L
#    if defined(_MSC_VER)
#      define ATFW_UTIL_CONST_INIT
#    else
#      define ATFW_UTIL_CONST_INIT constinit
#    endif
#  elif ATFW_UTIL_HAVE_CPP_ATTRIBUTE(clang::require_constant_initialization)
#    define ATFW_UTIL_CONST_INIT [[clang::require_constant_initialization]]
#  else
#    define ATFW_UTIL_CONST_INIT
#  endif
#endif

// Legacy macros
#ifndef UTIL_CONST_INIT
#  define UTIL_CONST_INIT ATFW_UTIL_CONST_INIT
#endif

// ATFW_UTIL_ATTRIBUTE_LIFETIME_BOUND indicates that a resource owned by a function
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
#ifndef ATFW_UTIL_ATTRIBUTE_LIFETIME_BOUND
#  if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(clang::lifetimebound)
#    define ATFW_UTIL_ATTRIBUTE_LIFETIME_BOUND [[clang::lifetimebound]]
#  elif defined(_MSC_VER) && ATFW_UTIL_HAVE_CPP_ATTRIBUTE(msvc::lifetimebound)
#    define ATFW_UTIL_ATTRIBUTE_LIFETIME_BOUND [[msvc::lifetimebound]]
#  elif ATFW_UTIL_HAVE_ATTRIBUTE(lifetimebound)
#    define ATFW_UTIL_ATTRIBUTE_LIFETIME_BOUND __attribute__((lifetimebound))
#  else
#    define ATFW_UTIL_ATTRIBUTE_LIFETIME_BOUND
#  endif
#endif

// Legacy macros
#ifndef UTIL_ATTRIBUTE_LIFETIME_BOUND
#  define UTIL_ATTRIBUTE_LIFETIME_BOUND ATFW_UTIL_ATTRIBUTE_LIFETIME_BOUND
#endif

// Internal attribute; name and documentation TBD.
//
// See the upstream documentation:
// https://clang.llvm.org/docs/AttributeReference.html#lifetime_capture_by
#ifndef ATFW_UTIL_INTERNAL_ATTRIBUTE_CAPTURED_BY
#  if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(clang::lifetime_capture_by)
#    define ATFW_UTIL_INTERNAL_ATTRIBUTE_CAPTURED_BY(Owner) [[clang::lifetime_capture_by(Owner)]]
#  else
#    define ATFW_UTIL_INTERNAL_ATTRIBUTE_CAPTURED_BY(Owner)
#  endif
#endif

// ATFW_UTIL_ATTRIBUTE_VIEW indicates that a type is solely a "view" of data that it
// points to, similarly to a span, string_view, or other non-owning reference
// type.
// This enables diagnosing certain lifetime issues similar to those enabled by
// ATFW_UTIL_ATTRIBUTE_LIFETIME_BOUND, such as:
//
//   struct ATFW_UTIL_ATTRIBUTE_VIEW StringView {
//     template<class R>
//     StringView(const R&);
//   };
//
//   StringView f(std::string s) {
//     return s;  // warning: address of stack memory returned
//   }
//
//
// See the following links for details:
// https://reviews.llvm.org/D64448
// https://lists.llvm.org/pipermail/cfe-dev/2018-November/060355.html
#ifndef ATFW_UTIL_ATTRIBUTE_VIEW
#  if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(gsl::Pointer) && (!defined(__clang_major__) || __clang_major__ >= 13)
#    define ATFW_UTIL_ATTRIBUTE_VIEW [[gsl::Pointer]]
#  else
#    define ATFW_UTIL_ATTRIBUTE_VIEW
#  endif
#endif

// ATFW_UTIL_ATTRIBUTE_OWNER indicates that a type is a container, smart pointer, or
// similar class that owns all the data that it points to.
// This enables diagnosing certain lifetime issues similar to those enabled by
// UTIL_ATTRIBUTE_LIFETIME_BOUND, such as:
//
//   struct ATFW_UTIL_ATTRIBUTE_VIEW StringView {
//     template<class R>
//     StringView(const R&);
//   };
//
//   struct ATFW_UTIL_ATTRIBUTE_OWNER String {};
//
//   StringView f(String s) {
//     return s;  // warning: address of stack memory returned
//   }
//
//
// See the following links for details:
// https://reviews.llvm.org/D64448
// https://lists.llvm.org/pipermail/cfe-dev/2018-November/060355.html
#ifndef ATFW_UTIL_ATTRIBUTE_OWNER
#  if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(gsl::Owner) && (!defined(__clang_major__) || __clang_major__ >= 13)
#    define ATFW_UTIL_ATTRIBUTE_OWNER [[gsl::Owner]]
#  else
#    define ATFW_UTIL_ATTRIBUTE_OWNER
#  endif
#endif

// ATFW_UTIL_ATTRIBUTE_NO_UNIQUE_ADDRESS
//
// Indicates a data member can be optimized to occupy no space (if it is empty)
// and/or its tail padding can be used for other members.
//
// For code that is assured to only build with C++20 or later, prefer using
// the standard attribute `[[no_unique_address]]` directly instead of this
// macro.
//
// https://devblogs.microsoft.com/cppblog/msvc-cpp20-and-the-std-cpp20-switch/#c20-no_unique_address
// Current versions of MSVC have disabled `[[no_unique_address]]` since it
// breaks ABI compatibility, but offers `[[msvc::no_unique_address]]` for
// situations when it can be assured that it is desired. Since Abseil does not
// claim ABI compatibility in mixed builds, we can offer it unconditionally.
#ifndef ATFW_UTIL_ATTRIBUTE_NO_UNIQUE_ADDRESS
#  if defined(_MSC_VER) && _MSC_VER >= 1929
#    define ATFW_UTIL_ATTRIBUTE_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#  elif ATFW_UTIL_HAVE_CPP_ATTRIBUTE(no_unique_address)
#    define ATFW_UTIL_ATTRIBUTE_NO_UNIQUE_ADDRESS [[no_unique_address]]
#  else
#    define ATFW_UTIL_ATTRIBUTE_NO_UNIQUE_ADDRESS
#  endif
#endif

// ATFW_UTIL_ATTRIBUTE_UNINITIALIZED
//
// GCC and Clang support a flag `-ftrivial-auto-var-init=<option>` (<option>
// can be "zero" or "pattern") that can be used to initialize automatic stack
// variables. Variables with this attribute will be left uninitialized,
// overriding the compiler flag.
//
// See https://clang.llvm.org/docs/AttributeReference.html#uninitialized
// and https://gcc.gnu.org/onlinedocs/gcc/Common-Variable-Attributes.html#index-uninitialized-variable-attribute
#ifndef ATFW_UTIL_ATTRIBUTE_UNINITIALIZED
#  if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(clang::uninitialized)
#    define ATFW_UTIL_ATTRIBUTE_UNINITIALIZED [[clang::uninitialized]]
#  elif ATFW_UTIL_HAVE_CPP_ATTRIBUTE(gnu::uninitialized)
#    define ATFW_UTIL_ATTRIBUTE_UNINITIALIZED [[gnu::uninitialized]]
#  elif ATFW_UTIL_HAVE_ATTRIBUTE(uninitialized)
#    define ATFW_UTIL_ATTRIBUTE_UNINITIALIZED __attribute__((uninitialized))
#  else
#    define ATFW_UTIL_ATTRIBUTE_UNINITIALIZED
#  endif
#endif

// ATFW_UTIL_ATTRIBUTE_WARN_UNUSED
//
// Compilers routinely warn about trivial variables that are unused.  For
// non-trivial types, this warning is suppressed since the
// constructor/destructor may be intentional and load-bearing, for example, with
// a RAII scoped lock.
//
// For example:
//
// class ATFW_UTIL_ATTRIBUTE_WARN_UNUSED MyType {
//  public:
//   MyType();
//   ~MyType();
// };
//
// void foo() {
//   // Warns with ATFW_UTIL_ATTRIBUTE_WARN_UNUSED attribute present.
//   MyType unused;
// }
//
// See https://clang.llvm.org/docs/AttributeReference.html#warn-unused and
// https://gcc.gnu.org/onlinedocs/gcc/C_002b_002b-Attributes.html#index-warn_005funused-type-attribute
#ifndef ATFW_UTIL_HAVE_CPP_ATTRIBUTE
#  if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(gnu::warn_unused)
#    define ATFW_UTIL_ATTRIBUTE_WARN_UNUSED [[gnu::warn_unused]]
#  else
#    define ATFW_UTIL_ATTRIBUTE_WARN_UNUSED
#  endif
#endif

// ATFW_UTIL_HAVE_MEMORY_SANITIZER
//
// MemorySanitizer (MSan) is a detector of uninitialized reads. It consists of
// a compiler instrumentation module and a run-time library.
#ifndef ATFW_UTIL_HAVE_MEMORY_SANITIZER
#  if !defined(__native_client__) && ATFW_UTIL_HAVE_FEATURE(memory_sanitizer)
#    define ATFW_UTIL_HAVE_MEMORY_SANITIZER 1
#  else
#    define ATFW_UTIL_HAVE_MEMORY_SANITIZER 0
#  endif
#endif

#if ATFW_UTIL_HAVE_MEMORY_SANITIZER && ATFW_UTIL_HAVE_ATTRIBUTE(no_sanitize_memory)
#  define ATFW_UTIL_SANITIZER_NO_MEMORY __attribute__((no_sanitize_memory))  // __attribute__((no_sanitize("memory")))
#else
#  define ATFW_UTIL_SANITIZER_NO_MEMORY
#endif

// Legacy macros
#ifndef UTIL_HAVE_MEMORY_SANITIZER
#  define UTIL_HAVE_MEMORY_SANITIZER ATFW_UTIL_HAVE_MEMORY_SANITIZER
#endif

#ifndef UTIL_SANITIZER_NO_MEMORY
#  define UTIL_SANITIZER_NO_MEMORY ATFW_UTIL_SANITIZER_NO_MEMORY
#endif

// ATFW_UTIL_HAVE_THREAD_SANITIZER
//
// ThreadSanitizer (TSan) is a fast data race detector.
#ifndef ATFW_UTIL_HAVE_THREAD_SANITIZER
#  if defined(__SANITIZE_THREAD__)
#    define ATFW_UTIL_HAVE_THREAD_SANITIZER 1
#  elif ATFW_UTIL_HAVE_FEATURE(thread_sanitizer)
#    define ATFW_UTIL_HAVE_THREAD_SANITIZER 1
#  else
#    define ATFW_UTIL_HAVE_THREAD_SANITIZER 0
#  endif
#endif

#if ATFW_UTIL_HAVE_THREAD_SANITIZER && ATFW_UTIL_HAVE_ATTRIBUTE(no_sanitize_thread)
#  define ATFW_UTIL_SANITIZER_NO_THREAD __attribute__((no_sanitize_thread))  // __attribute__((no_sanitize("thread")))
#else
#  define ATFW_UTIL_SANITIZER_NO_THREAD
#endif

// Legacy macros
#ifndef UTIL_HAVE_THREAD_SANITIZER
#  define UTIL_HAVE_THREAD_SANITIZER ATFW_UTIL_HAVE_THREAD_SANITIZER
#endif

#ifndef UTIL_SANITIZER_NO_THREAD
#  define UTIL_SANITIZER_NO_THREAD ATFW_UTIL_SANITIZER_NO_THREAD
#endif

// ATFW_UTIL_HAVE_ADDRESS_SANITIZER
//
// AddressSanitizer (ASan) is a fast memory error detector.
#ifndef ATFW_UTIL_HAVE_ADDRESS_SANITIZER
#  if defined(__SANITIZE_ADDRESS__)
#    define ATFW_UTIL_HAVE_ADDRESS_SANITIZER 1
#  elif ATFW_UTIL_HAVE_FEATURE(address_sanitizer)
#    define ATFW_UTIL_HAVE_ADDRESS_SANITIZER 1
#  else
#    define ATFW_UTIL_HAVE_ADDRESS_SANITIZER 0
#  endif
#endif

// ATFW_UTIL_HAVE_HWADDRESS_SANITIZER
//
// Hardware-Assisted AddressSanitizer (or HWASAN) is even faster than asan
// memory error detector which can use CPU features like ARM TBI, Intel LAM or
// AMD UAI.
#ifndef ATFW_UTIL_HAVE_HWADDRESS_SANITIZER
#  if defined(__SANITIZE_HWADDRESS__)
#    define ATFW_UTIL_HAVE_HWADDRESS_SANITIZER 1
#  elif ATFW_UTIL_HAVE_FEATURE(hwaddress_sanitizer)
#    define ATFW_UTIL_HAVE_HWADDRESS_SANITIZER 1
#  else
#    define ATFW_UTIL_HAVE_HWADDRESS_SANITIZER 0
#  endif
#endif

#if ATFW_UTIL_HAVE_ADDRESS_SANITIZER && ATFW_UTIL_HAVE_ATTRIBUTE(no_sanitize_address)
#  define ATFW_UTIL_SANITIZER_NO_ADDRESS \
    __attribute__((no_sanitize_address))  // __attribute__((no_sanitize("address")))
#elif ATFW_UTIL_HAVE_ADDRESS_SANITIZER && defined(_MSC_VER) && _MSC_VER >= 1928
#  define ATFW_UTIL_SANITIZER_NO_ADDRESS __declspec(no_sanitize_address)
#elif ATFW_UTIL_HAVE_HWADDRESS_SANITIZER && ATFW_UTIL_HAVE_ATTRIBUTE(no_sanitize)
#  define ATFW_UTIL_SANITIZER_NO_ADDRESS __attribute__((no_sanitize("hwaddress")))
#else
#  define ATFW_UTIL_SANITIZER_NO_ADDRESS
#endif

// Legacy macros
#ifndef UTIL_HAVE_ADDRESS_SANITIZER
#  define UTIL_HAVE_ADDRESS_SANITIZER ATFW_UTIL_HAVE_ADDRESS_SANITIZER
#endif

#ifndef UTIL_HAVE_HWADDRESS_SANITIZER
#  define UTIL_HAVE_HWADDRESS_SANITIZER ATFW_UTIL_HAVE_HWADDRESS_SANITIZER
#endif

#ifndef UTIL_SANITIZER_NO_ADDRESS
#  define UTIL_SANITIZER_NO_ADDRESS ATFW_UTIL_SANITIZER_NO_ADDRESS
#endif

// ATFW_UTIL_HAVE_DATAFLOW_SANITIZER
//
// Dataflow Sanitizer (or DFSAN) is a generalised dynamic data flow analysis.
#ifndef ATFW_UTIL_HAVE_DATAFLOW_SANITIZER
#  if defined(DATAFLOW_SANITIZER)
// GCC provides no method for detecting the presence of the standalone
// DataFlowSanitizer (-fsanitize=dataflow), so GCC users of -fsanitize=dataflow
// should also use -DDATAFLOW_SANITIZER.
#    define ATFW_UTIL_HAVE_DATAFLOW_SANITIZER 1
#  elif ATFW_UTIL_HAVE_FEATURE(dataflow_sanitizer)
#    define ATFW_UTIL_HAVE_DATAFLOW_SANITIZER 1
#  else
#    define ATFW_UTIL_HAVE_DATAFLOW_SANITIZER 0
#  endif
#endif

// Legacy macros
#ifndef UTIL_HAVE_DATAFLOW_SANITIZER
#  define UTIL_HAVE_DATAFLOW_SANITIZER ATFW_UTIL_HAVE_DATAFLOW_SANITIZER
#endif

// ATFW_UTIL_HAVE_LEAK_SANITIZER
//
// LeakSanitizer (or lsan) is a detector of memory leaks.
// https://clang.llvm.org/docs/LeakSanitizer.html
// https://github.com/google/sanitizers/wiki/AddressSanitizerLeakSanitizer
//
// The macro ATFW_UTIL_HAVE_LEAK_SANITIZER can be used to detect at compile-time
// whether the LeakSanitizer is potentially available. However, just because the
// LeakSanitizer is available does not mean it is active.
#ifndef ATFW_UTIL_HAVE_LEAK_SANITIZER
#  if defined(LEAK_SANITIZER)
// GCC provides no method for detecting the presence of the standalone
// LeakSanitizer (-fsanitize=leak), so GCC users of -fsanitize=leak should also
// use -DLEAK_SANITIZER.
#    define ATFW_UTIL_HAVE_LEAK_SANITIZER 1
// Clang standalone LeakSanitizer (-fsanitize=leak)
#  elif ATFW_UTIL_HAVE_FEATURE(leak_sanitizer)
#    define ATFW_UTIL_HAVE_LEAK_SANITIZER 1
#  elif defined(ATFW_UTIL_HAVE_ADDRESS_SANITIZER)
// GCC or Clang using the LeakSanitizer integrated into AddressSanitizer.
#    define ATFW_UTIL_HAVE_LEAK_SANITIZER 1
#  else
#    define ATFW_UTIL_HAVE_LEAK_SANITIZER 0
#  endif
#endif

// Legacy macros
#ifndef UTIL_HAVE_LEAK_SANITIZER
#  define UTIL_HAVE_LEAK_SANITIZER ATFW_UTIL_HAVE_LEAK_SANITIZER
#endif

#ifndef ATFW_UTIL_SANITIZER_NO_UNDEFINED
#  if ATFW_UTIL_HAVE_ATTRIBUTE(no_sanitize_undefined)
#    define ATFW_UTIL_SANITIZER_NO_UNDEFINED __attribute__((no_sanitize_undefined))
#  elif ATFW_UTIL_HAVE_ATTRIBUTE(no_sanitize)
#    define ATFW_UTIL_SANITIZER_NO_UNDEFINED __attribute__((no_sanitize("undefined")))
#  else
#    define ATFW_UTIL_SANITIZER_NO_UNDEFINED
#  endif
#endif

// Legacy macros
#ifndef UTIL_SANITIZER_NO_UNDEFINED
#  define UTIL_SANITIZER_NO_UNDEFINED ATFW_UTIL_SANITIZER_NO_UNDEFINED
#endif

#ifndef ATFW_UTIL_MACRO_INLINE_VARIABLE
#  if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    define ATFW_UTIL_MACRO_INLINE_VARIABLE inline
#  else
#    define ATFW_UTIL_MACRO_INLINE_VARIABLE
#  endif
#endif

// Legacy macros
#ifndef UTIL_MACRO_INLINE_VARIABLE
#  define UTIL_MACRO_INLINE_VARIABLE ATFW_UTIL_MACRO_INLINE_VARIABLE
#endif

// ATFW_UTIL_ATTRIBUTE_REINITIALIZES
//
// Indicates that a member function reinitializes the entire object to a known
// state, independent of the previous state of the object.
//
// The clang-tidy check bugprone-use-after-move allows member functions marked
// with this attribute to be called on objects that have been moved from;
// without the attribute, this would result in a use-after-move warning.
#ifndef ATFW_UTIL_ATTRIBUTE_REINITIALIZES
#  if ATFW_UTIL_HAVE_CPP_ATTRIBUTE(clang::reinitializes)
#    define ATFW_UTIL_ATTRIBUTE_REINITIALIZES [[clang::reinitializes]]
#  else
#    define ATFW_UTIL_ATTRIBUTE_REINITIALIZES
#  endif
#endif

// Legacy macros
#ifndef UTIL_ATTRIBUTE_REINITIALIZES
#  define UTIL_ATTRIBUTE_REINITIALIZES ATFW_UTIL_ATTRIBUTE_REINITIALIZES
#endif

// ATFW_UTIL_ATTRIBUTE_RETURNS_NONNULL
//
// Tells the compiler that a particular function never returns a null pointer.
#if ATFW_UTIL_HAVE_ATTRIBUTE(returns_nonnull) || \
    (defined(__GNUC__) && (__GNUC__ > 5 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9)) && !defined(__clang__))
#  define ATFW_UTIL_ATTRIBUTE_RETURNS_NONNULL __attribute__((returns_nonnull))
#else
#  define ATFW_UTIL_ATTRIBUTE_RETURNS_NONNULL
#endif

// Legacy macros
#ifndef UTIL_ATTRIBUTE_RETURNS_NONNULL
#  define UTIL_ATTRIBUTE_RETURNS_NONNULL ATFW_UTIL_ATTRIBUTE_RETURNS_NONNULL
#endif

// Macro for constexpr, to support in mixed 03/0x mode.
#ifndef ATFW_UTIL_ATTRIBUTE_CXX14_CONSTEXPR
#  if (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    define ATFW_UTIL_ATTRIBUTE_CXX14_CONSTEXPR constexpr
#  else
#    define ATFW_UTIL_ATTRIBUTE_CXX14_CONSTEXPR
#  endif
#endif

#ifndef ATFW_UTIL_ATTRIBUTE_CXX17_CONSTEXPR
#  if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    define ATFW_UTIL_ATTRIBUTE_CXX17_CONSTEXPR constexpr
#  else
#    define ATFW_UTIL_ATTRIBUTE_CXX17_CONSTEXPR
#  endif
#endif

#ifndef ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR
#  if (defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#    define ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR constexpr
#  else
#    define ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR
#  endif
#endif

#ifndef ATFW_UTIL_ATTRIBUTE_CXX23_CONSTEXPR
#  if (defined(__cplusplus) && __cplusplus >= 202100L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202100L)
#    define ATFW_UTIL_ATTRIBUTE_CXX23_CONSTEXPR constexpr
#  else
#    define ATFW_UTIL_ATTRIBUTE_CXX23_CONSTEXPR
#  endif
#endif

#ifndef ATFW_UTIL_ATTRIBUTE_CXX26_CONSTEXPR
#  if (defined(__cplusplus) && __cplusplus >= 202400L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202400L)
#    define ATFW_UTIL_ATTRIBUTE_CXX26_CONSTEXPR constexpr
#  else
#    define ATFW_UTIL_ATTRIBUTE_CXX26_CONSTEXPR
#  endif
#endif

#ifndef ATFW_UTIL_ATTRIBUTE_CXX17_INLINE
#  if (defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#    define ATFW_UTIL_ATTRIBUTE_CXX17_INLINE inline
#  else
#    define ATFW_UTIL_ATTRIBUTE_CXX17_INLINE
#  endif
#endif
