/**
 *
 * @file intrusive_ptr.h
 * @brief 侵入式智能指针
 * @note 这不是std标准中的一部分，但是这是对smart_ptr.h的补充
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT, owt5008137@live.com
 * @date 2017.05.18
 * @history
 *
 */

#ifndef STD_INTRUSIVE_PTR_H
#define STD_INTRUSIVE_PTR_H

#pragma once

#include <assert.h>
#include <cstddef>
#include <ostream>

#include <type_traits>

#ifdef __cpp_impl_three_way_comparison
#  include <compare>
#endif

#include <config/compiler_features.h>

#include <config/atframe_utils_build_feature.h>

namespace std {
//
//  intrusive_ptr
//
//  A smart pointer that uses intrusive reference counting.
//
//  Relies on unqualified calls to
//
//      void intrusive_ptr_add_ref(T * p);
//      void intrusive_ptr_release(T * p);
//
//          (p != nullptr)
//
//  The object is responsible for destroying itself.
//

template <typename T>
class intrusive_ptr {
 public:
  using self_type = intrusive_ptr<T>;
  using element_type = T;

  constexpr intrusive_ptr() noexcept : px(nullptr) {}

  intrusive_ptr(T *p, bool add_ref = true) : px(p) {
    if (px != nullptr && add_ref) {
      intrusive_ptr_add_ref(px);
    }
  }

  template <typename U>
  intrusive_ptr(intrusive_ptr<U> const &rhs,
                typename std::enable_if<std::is_convertible<U *, T *>::value>::type * = nullptr)
      : px(rhs.get()) {
    if (px != nullptr) {
      intrusive_ptr_add_ref(px);
    }
  }

  intrusive_ptr(self_type const &rhs) : px(rhs.px) {
    if (px != nullptr) {
      intrusive_ptr_add_ref(px);
    }
  }

  ~intrusive_ptr() {
    if (px != nullptr) {
      intrusive_ptr_release(px);
    }
  }

  template <typename U>
  friend class intrusive_ptr;

  template <typename U>
  intrusive_ptr &operator=(intrusive_ptr<U> const &rhs) {
    self_type(rhs).swap(*this);
    return *this;
  }

  // Move support
  intrusive_ptr(self_type &&rhs) noexcept : px(rhs.px) { rhs.px = nullptr; }

  self_type &operator=(self_type &&rhs) noexcept {
    self_type(static_cast<self_type &&>(rhs)).swap(*this);
    return *this;
  }

  template <typename U>
  intrusive_ptr(intrusive_ptr<U> &&rhs,
                typename std::enable_if<std::is_convertible<U *, T *>::value>::type * = nullptr) noexcept
      : px(rhs.px) {
    rhs.px = nullptr;
  }

  template <typename U, typename Deleter>
  self_type &operator=(std::unique_ptr<U, Deleter> &&rhs) {
    self_type(rhs.release()).swap(*this);
    return *this;
  }

  self_type &operator=(self_type const &rhs) {
    self_type(rhs).swap(*this);
    return *this;
  }

  inline void reset() noexcept { self_type().swap(*this); }

  inline void reset(element_type *rhs) { self_type(rhs).swap(*this); }

  inline void reset(element_type *rhs, bool add_ref) { self_type(rhs, add_ref).swap(*this); }

  inline element_type *get() const noexcept { return px; }

  inline element_type *detach() noexcept {
    element_type *ret = px;
    px = nullptr;
    return ret;
  }

  inline element_type &operator*() const {
    assert(px != 0);
    return *px;
  }

  inline element_type *operator->() const {
    assert(px != 0);
    return px;
  }

  // implicit conversion to "bool"
  inline operator bool() const noexcept { return px != nullptr; }
  // operator! is redundant, but some compilers need it
  inline bool operator!() const noexcept { return px == nullptr; }

  inline void swap(intrusive_ptr &rhs) noexcept {
    element_type *tmp = px;
    px = rhs.px;
    rhs.px = tmp;
  }

 private:
  element_type *px;
};

template <typename T, typename U>
inline bool operator==(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
  return a.get() == b.get();
}

template <typename T, typename U>
inline bool operator==(intrusive_ptr<T> const &a, U *b) noexcept {
  return a.get() == b;
}

template <typename T, typename U>
inline bool operator==(T *a, intrusive_ptr<U> const &b) noexcept {
  return a == b.get();
}

#ifdef __cpp_impl_three_way_comparison
template <typename T, typename U>
inline std::strong_ordering operator<=>(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
  return a.get() <=> b.get();
}

template <typename T, typename U>
inline std::strong_ordering operator<=>(intrusive_ptr<T> const &a, U *b) noexcept {
  return a.get() <=> b;
}

template <typename T, typename U>
inline std::strong_ordering operator<=>(T *a, intrusive_ptr<U> const &b) noexcept {
  return a <=> b.get();
}
#else
template <typename T, typename U>
inline bool operator!=(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
  return a.get() != b.get();
}

template <typename T, typename U>
inline bool operator!=(intrusive_ptr<T> const &a, U *b) noexcept {
  return a.get() != b;
}

template <typename T, typename U>
inline bool operator!=(T *a, intrusive_ptr<U> const &b) noexcept {
  return a != b.get();
}

template <typename T, typename U>
inline bool operator<(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
  return a.get() < b.get();
}

template <typename T, typename U>
inline bool operator<(intrusive_ptr<T> const &a, U *b) noexcept {
  return a.get() < b;
}

template <typename T, typename U>
inline bool operator<(T *a, intrusive_ptr<U> const &b) noexcept {
  return a < b.get();
}

template <typename T, typename U>
inline bool operator<=(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
  return a.get() <= b.get();
}

template <typename T, typename U>
inline bool operator<=(intrusive_ptr<T> const &a, U *b) noexcept {
  return a.get() <= b;
}

template <typename T, typename U>
inline bool operator<=(T *a, intrusive_ptr<U> const &b) noexcept {
  return a <= b.get();
}

template <typename T, typename U>
inline bool operator>(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
  return a.get() > b.get();
}

template <typename T, typename U>
inline bool operator>(intrusive_ptr<T> const &a, U *b) noexcept {
  return a.get() > b;
}

template <typename T, typename U>
inline bool operator>(T *a, intrusive_ptr<U> const &b) noexcept {
  return a > b.get();
}

template <typename T, typename U>
inline bool operator>=(intrusive_ptr<T> const &a, intrusive_ptr<U> const &b) noexcept {
  return a.get() >= b.get();
}

template <typename T, typename U>
inline bool operator>=(intrusive_ptr<T> const &a, U *b) noexcept {
  return a.get() >= b;
}

template <typename T, typename U>
inline bool operator>=(T *a, intrusive_ptr<U> const &b) noexcept {
  return a >= b.get();
}

#endif

template <typename T>
inline bool operator==(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() == nullptr;
}

template <typename T>
inline bool operator==(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
  return p.get() == nullptr;
}

#ifdef __cpp_impl_three_way_comparison
template <typename T>
inline std::strong_ordering operator<=>(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() <=> nullptr;
}

template <typename T>
inline std::strong_ordering operator<=>(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
  return p.get() <=> nullptr;
}
#else
template <typename T>
inline bool operator!=(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() != nullptr;
}

template <typename T>
inline bool operator!=(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
  return p.get() != nullptr;
}

template <typename T>
inline bool operator<(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() < nullptr;
}

template <typename T>
inline bool operator<(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
  return p.get() < nullptr;
}

template <typename T>
inline bool operator<=(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() <= nullptr;
}

template <typename T>
inline bool operator<=(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
  return p.get() <= nullptr;
}

template <typename T>
inline bool operator>(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() > nullptr;
}

template <typename T>
inline bool operator>(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
  return p.get() > nullptr;
}

template <typename T>
inline bool operator>=(intrusive_ptr<T> const &p, std::nullptr_t) noexcept {
  return p.get() >= nullptr;
}

template <typename T>
inline bool operator>=(std::nullptr_t, intrusive_ptr<T> const &p) noexcept {
  return p.get() >= nullptr;
}
#endif

template <typename T>
void swap(intrusive_ptr<T> &lhs, intrusive_ptr<T> &rhs) {
  lhs.swap(rhs);
}

// mem_fn support

template <typename T>
T *get_pointer(intrusive_ptr<T> const &p) {
  return p.get();
}

template <typename T, typename U>
intrusive_ptr<T> static_pointer_cast(intrusive_ptr<U> const &p) {
  return static_cast<T *>(p.get());
}

template <typename T, typename U>
intrusive_ptr<T> const_pointer_cast(intrusive_ptr<U> const &p) {
  return const_cast<T *>(p.get());
}

#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
template <typename T, typename U>
intrusive_ptr<T> dynamic_pointer_cast(intrusive_ptr<U> const &p) {
  return dynamic_cast<T *>(p.get());
}
#endif

// operator<<
template <typename E, typename T, typename Y>
std::basic_ostream<E, T> &operator<<(std::basic_ostream<E, T> &os, intrusive_ptr<Y> const &p) {
  os << p.get();
  return os;
}
}  // namespace std

#if defined(LIBATFRAME_UTILS_LOCK_DISABLE_MT) && LIBATFRAME_UTILS_LOCK_DISABLE_MT
#  define UTIL_INTRUSIVE_PTR_ATOMIC_TYPE \
    LIBATFRAME_UTILS_NAMESPACE_ID::lock::atomic_int_type<LIBATFRAME_UTILS_NAMESPACE_ID::lock::unsafe_int_type<size_t> >
#else
#  define UTIL_INTRUSIVE_PTR_ATOMIC_TYPE LIBATFRAME_UTILS_NAMESPACE_ID::lock::atomic_int_type<size_t>
#endif

#define UTIL_INTRUSIVE_PTR_REF_MEMBER_DECL(T)            \
                                                         \
 private:                                                \
  UTIL_INTRUSIVE_PTR_ATOMIC_TYPE intrusive_ref_counter_; \
  friend void intrusive_ptr_add_ref(T *p);               \
  friend void intrusive_ptr_release(T *p);               \
                                                         \
 public:                                                 \
  size_t use_count() const { return intrusive_ref_counter_.load(); }

#define UTIL_INTRUSIVE_PTR_REF_FN_DECL(T) \
  void intrusive_ptr_add_ref(T *p);       \
  void intrusive_ptr_release(T *p);

#define UTIL_INTRUSIVE_PTR_REF_MEMBER_INIT() this->intrusive_ref_counter_.store(0)

#define UTIL_INTRUSIVE_PTR_REF_FN_DEFI(T)         \
  void intrusive_ptr_add_ref(T *p) {              \
    if (nullptr != p) {                           \
      ++p->intrusive_ref_counter_;                \
    }                                             \
  }                                               \
  void intrusive_ptr_release(T *p) {              \
    if (nullptr == p) {                           \
      return;                                     \
    }                                             \
    assert(p->intrusive_ref_counter_.load() > 0); \
    size_t ref = --p->intrusive_ref_counter_;     \
    if (0 == ref) {                               \
      delete p;                                   \
    }                                             \
  }

#endif
