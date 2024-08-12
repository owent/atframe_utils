// Copyright 2024 atframework
// Created by owent

// ============= Patch for some Compilers's mistake =============
#if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#  pragma GCC diagnostic push
#endif

#if (__GNUC__ * 100 + __GNUC_MINOR__) == 1402
#  pragma GCC diagnostic ignored "-Wuninitialized"
#  pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif

#if defined(__SANITIZE_THREAD__)
#  pragma GCC diagnostic ignored "-Wtsan"
#elif defined(__has_feature)
#  if __has_feature(thread_sanitizer)
#    pragma GCC diagnostic ignored "-Wtsan"
#  endif
#endif
