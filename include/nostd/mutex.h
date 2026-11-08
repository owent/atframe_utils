// Copyright 2026 atframework
//
// Created by owent on 2024-05-20

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>
#include <std/explicit_declare.h>

#include <mutex>

#if !(defined(__cpp_lib_scoped_lock) && __cpp_lib_scoped_lock >= 201703L)
#  include <nostd/utility_sequence.h>

#  include <tuple>
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace nostd {
#if defined(__cpp_lib_scoped_lock) && __cpp_lib_scoped_lock >= 201703L
using ::std::scoped_lock;
#else
template <class... MutexType>
class ATFW_UTIL_SYMBOL_VISIBLE ATFW_EXPLICIT_NODISCARD_TYPE_ATTR scoped_lock {
  // class with destructor that unlocks mutexes
 public:
  explicit scoped_lock(MutexType&... _Mtxes) : mutex_refs_(_Mtxes...) {
    // construct and lock
    ::std::lock(_Mtxes...);
  }

  explicit scoped_lock(::std::adopt_lock_t, MutexType&... _Mtxes) noexcept : mutex_refs_(_Mtxes...) {
    // construct but don't lock
  }

  ~scoped_lock() noexcept { __unlock_all(index_sequence_for<MutexType...>{}); }

  scoped_lock(const scoped_lock&) = delete;
  scoped_lock& operator=(const scoped_lock&) = delete;

 private:
  template <std::size_t... _Indices>
  void __unlock_all(index_sequence<_Indices...>) noexcept {
    using expand = int[];

    (void)expand{0, ((void)std::get<_Indices>(mutex_refs_).unlock(), 0)...};
  }

 private:
  ::std::tuple<MutexType&...> mutex_refs_;
};

template <class MutexType>
class ATFW_UTIL_SYMBOL_VISIBLE ATFW_EXPLICIT_NODISCARD_TYPE_ATTR scoped_lock<MutexType> {
 public:
  using mutex_type = MutexType;

  explicit scoped_lock(MutexType& _Mtx) : mutex_ref_(_Mtx) {  // construct and lock
    mutex_ref_.lock();
  }

  explicit scoped_lock(::std::adopt_lock_t, MutexType& _Mtx) noexcept  // strengthened
      : mutex_ref_(_Mtx) {}                                            // construct but don't lock

  ~scoped_lock() noexcept { mutex_ref_.unlock(); }

  scoped_lock(const scoped_lock&) = delete;
  scoped_lock& operator=(const scoped_lock&) = delete;

 private:
  MutexType& mutex_ref_;
};

template <>
class scoped_lock<> {
 public:
  explicit scoped_lock() = default;
  explicit scoped_lock(::std::adopt_lock_t) noexcept /* strengthened */ {}

  scoped_lock(const scoped_lock&) = delete;
  scoped_lock& operator=(const scoped_lock&) = delete;
};
#endif
}  // namespace nostd
ATFRAMEWORK_UTILS_NAMESPACE_END
