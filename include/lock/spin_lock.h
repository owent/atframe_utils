// Copyright 2026 atframework
//
// @file spin_lock.h
// @brief 自旋锁
// Licensed under the MIT licenses.

#ifndef UTIL_LOCK_SPINLOCK_H
#define UTIL_LOCK_SPINLOCK_H

#pragma once

#include <chrono>
#include <thread>

#if defined(_MSC_VER)
#  include <intrin.h>
#endif

#include <config/atframe_utils_build_feature.h>
#include "atomic_int_type.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace lock {
namespace detail {

ATFW_UTIL_FORCEINLINE void spin_pause() noexcept {
#if defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
  _mm_pause();
#elif (defined(__i386__) || defined(__x86_64__)) && (defined(__clang__) || defined(__GNUC__))
  __builtin_ia32_pause();
#elif (defined(__arm__) || defined(__aarch64__)) && (defined(__clang__) || defined(__GNUC__))
  __asm__ __volatile__("yield" ::: "memory");
#else
  // no-op
#endif
}

ATFW_UTIL_FORCEINLINE void cpu_yield() noexcept { spin_pause(); }

ATFW_UTIL_FORCEINLINE void thread_yield() noexcept { ::std::this_thread::yield(); }

ATFW_UTIL_FORCEINLINE void thread_sleep() noexcept { ::std::this_thread::sleep_for(::std::chrono::milliseconds(1)); }

ATFW_UTIL_FORCEINLINE void spin_wait(unsigned int try_times) noexcept {
  if (try_times < 4) {
    // busy-wait
  } else if (try_times < 16) {
    spin_pause();
  } else if (try_times < 32) {
    thread_yield();
  } else if (try_times < 64) {
    cpu_yield();
  } else {
    thread_sleep();
  }
}

}  // namespace detail

/**
 * @brief 自旋锁
 * @see
 * http://www.boost.org/doc/libs/1_61_0/doc/html/atomic/usage_examples.html#boost_atomic.usage_examples.example_spinlock
 */
class ATFRAMEWORK_UTILS_API_HEAD_ONLY spin_lock {
 private:
  enum lock_state_t {
    UNLOCKED = 0,
    LOCKED = 1,
  };

  ATFRAMEWORK_UTILS_NAMESPACE_ID::lock::atomic_int_type<
#if defined(ATFRAMEWORK_UTILS_LOCK_DISABLE_MT) && ATFRAMEWORK_UTILS_LOCK_DISABLE_MT
      ATFRAMEWORK_UTILS_NAMESPACE_ID::lock::unsafe_int_type<unsigned int>
#else
      unsigned int
#endif
      >
      lock_status_;

 public:
  spin_lock() noexcept { lock_status_.store(static_cast<unsigned int>(UNLOCKED)); }

  spin_lock(const spin_lock&) = delete;
  spin_lock& operator=(const spin_lock&) = delete;

  void lock() noexcept {
    unsigned int try_times = 0;
    while (lock_status_.exchange(static_cast<unsigned int>(LOCKED),
                                 ATFRAMEWORK_UTILS_NAMESPACE_ID::lock::memory_order_acq_rel) ==
           static_cast<unsigned int>(LOCKED)) {
      detail::spin_wait(try_times++);
    }
  }

  void unlock() noexcept {
    lock_status_.store(static_cast<unsigned int>(UNLOCKED), ATFRAMEWORK_UTILS_NAMESPACE_ID::lock::memory_order_release);
  }

  bool is_locked() noexcept {
    return lock_status_.load(ATFRAMEWORK_UTILS_NAMESPACE_ID::lock::memory_order_acquire) ==
           static_cast<unsigned int>(LOCKED);
  }

  bool try_lock() noexcept {
    return lock_status_.exchange(static_cast<unsigned int>(LOCKED),
                                 ATFRAMEWORK_UTILS_NAMESPACE_ID::lock::memory_order_acq_rel) ==
           static_cast<unsigned int>(UNLOCKED);
  }

  bool try_unlock() noexcept {
    return lock_status_.exchange(static_cast<unsigned int>(UNLOCKED),
                                 ATFRAMEWORK_UTILS_NAMESPACE_ID::lock::memory_order_acq_rel) ==
           static_cast<unsigned int>(LOCKED);
  }
};

}  // namespace lock
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif /* UTIL_LOCK_SPINLOCK_H */
