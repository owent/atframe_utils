// Copyright 2021 atframework
#ifndef UTIL_CONFIG_COMPILER_PROTOBUF_PREFIX_H
#define UTIL_CONFIG_COMPILER_PROTOBUF_PREFIX_H

// https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warnings-by-compiler-version

#if defined(_MSC_VER)
#  pragma warning(push)

#  if ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
#    pragma warning(disable : 4996)
#    pragma warning(disable : 4309)
#  endif

#  pragma warning(disable : 4244)
#  pragma warning(disable : 4251)
#  pragma warning(disable : 4267)
#  pragma warning(disable : 4668)
#  pragma warning(disable : 4800)
#  pragma warning(disable : 4946)

#endif

#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#    pragma GCC diagnostic push
#  endif
#  pragma GCC diagnostic ignored "-Wunused-parameter"
#  pragma GCC diagnostic ignored "-Wtype-limits"
#  pragma GCC diagnostic ignored "-Wsign-compare"
#  pragma GCC diagnostic ignored "-Wsign-conversion"
#elif defined(__clang__) || defined(__apple_build_version__)
#  pragma clang diagnostic push
#  pragma clang diagnostic ignored "-Wunused-parameter"
#  pragma clang diagnostic ignored "-Wtype-limits"
#  pragma clang diagnostic ignored "-Wsign-compare"
#  pragma clang diagnostic ignored "-Wsign-conversion"
#endif

#include "template_prefix.h"

#ifdef max
#  undef max
#endif

#ifdef min
#  undef min
#endif

#endif
