// Copyright 2024 atframework
// Created by owent

#ifndef UTIL_INTERNAL_MACRO_COMPILER_INTERNAL_STL_COMPACT_SUFFIX_H_INC
#  define UTIL_INTERNAL_MACRO_COMPILER_INTERNAL_STL_COMPACT_SUFFIX_H_INC
// ============= Patch for some Compilers's mistake =============
#  if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#    include "config/compiler/internal/gcc_compact_prefix.h.inc"
#  endif
#endif
