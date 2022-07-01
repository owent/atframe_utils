/**
 * @file spin_rw_lock.h
 * @brief 基于自旋锁技术的读写锁(乐观读写锁)
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2018-12-26
 *
 * @note VC 2012+, GCC 4.4 + 使用C++0x/11实现实现原子操作
 * @note 低版本 VC使用InterlockedExchange等实现原子操作
 * @note 低版本 GCC采用__sync_lock_test_and_set等实现原子操作
 *
 */

#ifndef UTIL_LOCK_SPIN_RW_LOCK_H
#define UTIL_LOCK_SPIN_RW_LOCK_H

#pragma once

#include "spin_lock.h"

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace lock {
class LIBATFRAME_UTILS_API_HEAD_ONLY spin_rw_lock {
 private:
  LIBATFRAME_UTILS_NAMESPACE_ID::lock::atomic_int_type<
#if defined(LIBATFRAME_UTILS_LOCK_DISABLE_MT) && LIBATFRAME_UTILS_LOCK_DISABLE_MT
      LIBATFRAME_UTILS_NAMESPACE_ID::lock::unsafe_int_type<int32_t>
#else
      int32_t
#endif
      >
      lock_status_;

  enum {
    WRITE_LOCK_FLAG = 0x01,
  };
  enum {
    MAX_READ_LOCK_HOLDER = INT32_MAX - 1,
  };

 public:
  spin_rw_lock() { lock_status_.store(0); }

  void read_lock() {
    unsigned char try_times = 0;
    while (!try_read_lock()) __UTIL_LOCK_SPIN_LOCK_WAIT(try_times++); /* busy-wait */
  }

  void read_unlock() { try_read_unlock(); }

  bool is_read_locked() { return lock_status_.load(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acquire) >= 2; }

  bool try_read_lock() {
    int32_t src_status = lock_status_.load(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acquire);
    while (true) {
      // failed if already lock writable
      if (src_status & WRITE_LOCK_FLAG) {
        return false;
      }

      // max locker
      if (src_status >= MAX_READ_LOCK_HOLDER) {
        return false;
      }

      int32_t dst_status = src_status + 2;
      if (lock_status_.compare_exchange_weak(src_status, dst_status,
                                             LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acq_rel)) {
        return true;
      }
    }
  }

  bool try_read_unlock() {
    int32_t src_status = lock_status_.load(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acquire);
    while (true) {
      if (src_status < 2) {
        return false;
      }

      int32_t dst_status = src_status - 2;
      if (lock_status_.compare_exchange_weak(src_status, dst_status,
                                             LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acq_rel)) {
        return true;
      }
    }
  }

  void write_lock() {
    bool is_already_lock_writable = false;
    unsigned char try_times = 0;

    while (true) {
      int32_t src_status = lock_status_.load(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acquire);
      // already lock writable
      if (is_already_lock_writable) {
        if (src_status < 2) {
          return;
        }

        __UTIL_LOCK_SPIN_LOCK_WAIT(try_times++); /* busy-wait */
        continue;
      }

      // failed if already locked
      if (src_status & WRITE_LOCK_FLAG) {
        __UTIL_LOCK_SPIN_LOCK_WAIT(try_times++); /* busy-wait */
        continue;
      }

      // lock writable and then wait for all read lock to free
      int32_t dst_status = src_status + WRITE_LOCK_FLAG;
      if (lock_status_.compare_exchange_weak(src_status, dst_status,
                                             LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acq_rel)) {
        is_already_lock_writable = true;
      }
    }
  }

  void write_unlock() { try_write_unlock(); }

  bool is_write_locked() {
    return 0 != (lock_status_.load(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acquire) & WRITE_LOCK_FLAG);
  }

  bool try_write_lock() {
    int32_t src_status = lock_status_.load(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acquire);
    while (true) {
      // failed if already locked
      if (src_status & WRITE_LOCK_FLAG) {
        return false;
      }

      // failed if there is any read lock
      if (src_status >= 2) {
        return false;
      }

      int32_t dst_status = src_status + WRITE_LOCK_FLAG;
      if (lock_status_.compare_exchange_weak(src_status, dst_status,
                                             LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acq_rel)) {
        return true;
      }
    }
  }

  bool try_write_unlock() {
    int32_t src_status = lock_status_.load(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acquire);
    while (true) {
      if (0 == (src_status & WRITE_LOCK_FLAG)) {
        return false;
      }

      int32_t dst_status = src_status - WRITE_LOCK_FLAG;
      if (lock_status_.compare_exchange_weak(src_status, dst_status,
                                             LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acq_rel)) {
        return true;
      }
    }
  }
};
}  // namespace lock
LIBATFRAME_UTILS_NAMESPACE_END

#endif /* SPINLOCK_H_ */
