// Copyright 2024 atframework
// Created by owent

// ============= Patch for some Compilers's mistake =============
#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#    pragma GCC diagnostic push
#  endif

#  if (__GNUC__ * 100 + __GNUC_MINOR__) == 1402
#    pragma GCC diagnostic ignored "-Wuninitialized"
#    pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#  endif
#endif
