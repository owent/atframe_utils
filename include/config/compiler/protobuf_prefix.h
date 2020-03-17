#ifndef UTIL_CONFIG_COMPILER_PROTOBUF_PREFIX_H
#define UTIL_CONFIG_COMPILER_PROTOBUF_PREFIX_H

// https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warnings-by-compiler-version

#if defined(_MSC_VER) && ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4309)
#if _MSC_VER >= 1922 && ((defined(__cplusplus) && __cplusplus >= 201704L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201704L))
#pragma warning(disable : 5054)
#endif
#endif

#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)  // && (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wunused-parameter"
#elif defined(__clang__) || defined(__apple_build_version__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#endif

#include "template_prefix.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#endif
