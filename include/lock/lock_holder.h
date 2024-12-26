/**
 * @file lock_holder.h
 * @brief 锁管理器
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2015-06-29
 *
 * @note 实现锁的自管理操作,类似std::lock_guard，增加了针对特定场合使用的try_unlock函数
 *
 * @history
 */

#pragma once

#include <cstring>

#include "config/atframe_utils_build_feature.h"
#include "config/compiler_features.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace lock {
namespace detail {
template <typename TLock>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY default_lock_action{bool operator()(TLock &lock) const {lock.lock();
return true;
}  // namespace detail
};  // namespace lock

template <typename TLock>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY default_try_lock_action{bool operator()(TLock &lock)
                                                                   const {return lock.try_lock();
}
}
;

template <typename TLock>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY default_unlock_action{void operator()(TLock &lock) const {lock.unlock();
}
}
;

template <typename TLock>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY default_try_unlock_action{bool operator()(TLock &lock)
                                                                     const {return lock.try_unlock();
}
}
;

template <typename TLock>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY default_read_lock_action{bool operator()(TLock &lock) const {lock.read_lock();
return true;
}
}
;

template <typename TLock>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY default_read_unlock_action{void operator()(TLock &lock)
                                                                      const {lock.read_unlock();
}
}
;

template <typename TLock>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY default_write_lock_action{bool operator()(TLock &lock) const {lock.write_lock();
return true;
}
}
;

template <typename TLock>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY default_write_unlock_action{void operator()(TLock &lock)
                                                                       const {lock.write_unlock();
}
}
;
}  // namespace detail

template <typename TLock, typename TLockAct = detail::default_lock_action<TLock>,
          typename TUnlockAct = detail::default_unlock_action<TLock> >
class ATFRAMEWORK_UTILS_API_HEAD_ONLY lock_holder {
 public:
  using value_type = TLock;

  inline lock_holder() noexcept : lock_flag_(nullptr) {}

  lock_holder(TLock &lock)  // NOLINT: runtime/explicit
      : lock_flag_(&lock) {
    if (false == TLockAct()(lock)) {
      lock_flag_ = nullptr;
    }
  }

  lock_holder(lock_holder &&other) noexcept : lock_flag_(other.lock_flag_) { other.lock_flag_ = nullptr; }

  ~lock_holder() { reset(); }

  lock_holder &operator=(lock_holder &&other) {
    reset();

    lock_flag_ = other.lock_flag_;
    other.lock_flag_ = nullptr;

    return *this;
  }

  inline void reset() {
    if (nullptr != lock_flag_) {
      value_type *lock = lock_flag_;
      lock_flag_ = nullptr;
      TUnlockAct()(*lock);
    }
  }

  inline bool is_available() const { return nullptr != lock_flag_; }

 private:
  lock_holder(const lock_holder &) = delete;
  lock_holder &operator=(const lock_holder &) = delete;

 private:
  value_type *lock_flag_;
};

template <typename TLock>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY read_lock_holder
    : public lock_holder<TLock, detail::default_read_lock_action<TLock>, detail::default_read_unlock_action<TLock> > {
 public:
  read_lock_holder(TLock &lock)  // NOLINT: runtime/explicit
      : lock_holder<TLock, detail::default_read_lock_action<TLock>, detail::default_read_unlock_action<TLock> >(lock) {}

  inline read_lock_holder() noexcept = default;
  read_lock_holder(read_lock_holder &&) = default;
  read_lock_holder &operator=(read_lock_holder &&) = default;
};

template <typename TLock>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY write_lock_holder
    : public lock_holder<TLock, detail::default_write_lock_action<TLock>, detail::default_write_unlock_action<TLock> > {
 public:
  write_lock_holder(TLock &lock)  // NOLINT: runtime/explicit
      : lock_holder<TLock, detail::default_write_lock_action<TLock>, detail::default_write_unlock_action<TLock> >(
            lock) {}

  inline write_lock_holder() noexcept = default;
  write_lock_holder(write_lock_holder &&) = default;
  write_lock_holder &operator=(write_lock_holder &&) = default;
};
}  // namespace lock
ATFRAMEWORK_UTILS_NAMESPACE_END
