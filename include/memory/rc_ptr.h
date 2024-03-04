// Copyright 2024 atframework
// Licenses under the MIT License

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#ifdef __cpp_impl_three_way_comparison
#  include <compare>
#endif

#include <config/atframe_utils_build_feature.h>

#include <config/compile_optimize.h>
#include <config/compiler_features.h>
#include <nostd/type_traits.h>

#if defined(_MSC_VER)
#  define LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION 2
#elif defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 700
#    define LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION 2
#  else
#    define LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION 1
#  endif
#elif defined(__clang__) || defined(__apple_build_version__)
#  define LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION 2
#endif

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace memory {

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY weak_rc_ptr;

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_ptr;

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY enable_shared_rc_from_this;

class UTIL_SYMBOL_VISIBLE rc_ptr_counted_data_base {
 public:
  UTIL_CONFIG_CONSTEXPR rc_ptr_counted_data_base() noexcept : use_count_(1), weak_count_(1) {}

  LIBATFRAME_UTILS_API virtual ~rc_ptr_counted_data_base() noexcept;

  // Called when use_count_ drops to zero, to release the resources
  // managed by *this.
  virtual void dispose() noexcept = 0;

  // Called when weak_count_ drops to zero.
  virtual void destroy() noexcept = 0;

  LIBATFRAME_UTILS_API static void throw_bad_weak_ptr();

  // Increment the use count if it is non-zero, throw otherwise.
  UTIL_FORCEINLINE void add_ref() {
    if (!add_ref_nothrow()) {
      throw_bad_weak_ptr();
    }
  }

  // Increment the use count if it is non-zero.
  UTIL_FORCEINLINE bool add_ref_nothrow() noexcept {
    if (use_count_ == 0) {
      return false;
    }

    ++use_count_;
    return true;
  }

  // Decrement the use count.
  UTIL_FORCEINLINE void release() noexcept {
    if (--use_count_ == 0) {
      dispose();
      if (--weak_count_ == 0) {
        destroy();
      }
    }
  }

  // Increment the weak count.
  UTIL_FORCEINLINE void weak_add_ref() noexcept { ++weak_count_; }

  // Decrement the weak count.
  UTIL_FORCEINLINE void weak_release() noexcept {
    if (--weak_count_ == 0) {
      destroy();
    }
  }

  UTIL_FORCEINLINE std::size_t use_count() const noexcept { return use_count_; }

 private:
  rc_ptr_counted_data_base(const rc_ptr_counted_data_base&) = delete;
  rc_ptr_counted_data_base& operator=(const rc_ptr_counted_data_base&) = delete;

 private:
  std::size_t use_count_;
  std::size_t weak_count_;
};

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY rc_ptr_counted_data_default : public rc_ptr_counted_data_base {
 public:
  explicit rc_ptr_counted_data_default(T* p) noexcept : ptr_(p) {}

  void dispose() noexcept override { delete ptr_; }

  void destroy() noexcept override { delete this; }

 private:
  T* ptr_;
};

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY rc_ptr_counted_data_inplace : public rc_ptr_counted_data_base {
 public:
  template <class... Args>
  explicit rc_ptr_counted_data_inplace(Args&&... args) {
    // allocator_traits<_Alloc>::construct(__a, addr(), std::forward<Args>(args)...);
    ::new (addr()) T(std::forward<Args>(args)...);
  }

  void dispose() noexcept override { reinterpret_cast<T*>(addr())->~T(); }

  void destroy() noexcept override {
    this->~rc_ptr_counted_data_inplace();
    unsigned char* p = reinterpret_cast<unsigned char*>(this);
    delete[] p;
  }

  inline T* ptr() noexcept { return reinterpret_cast<T*>(addr()); }

 private:
  inline void* addr() { return reinterpret_cast<void*>(&storage_); }

  typename nostd::aligned_storage<sizeof(T), alignof(T)>::type storage_;
};

template <class T, class Deleter>
class LIBATFRAME_UTILS_API_HEAD_ONLY rc_ptr_counted_data_with_deleter : public rc_ptr_counted_data_base {
 public:
  rc_ptr_counted_data_with_deleter(T* p, const Deleter& d) noexcept : ptr_(p), deleter_(d) {}
  rc_ptr_counted_data_with_deleter(T* p, Deleter&& d) noexcept : ptr_(p), deleter_(std::move(d)) {}

  void dispose() noexcept override { deleter_(ptr_); }

  void destroy() noexcept override { delete this; }

 private:
  T* ptr_;
  Deleter deleter_;
};

template <class T>
struct LIBATFRAME_UTILS_API_HEAD_ONLY _strong_rc_alloc_shared_tag {};

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY weak_rc_counter;

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_counter {
  // Prevent _strong_rc_alloc_shared_tag from matching the shared_ptr(P, D) ctor.
  template <class>
  struct __not_alloc_shared_tag {
    using type = void;
  };

  template <class Y>
  struct __not_alloc_shared_tag<_strong_rc_alloc_shared_tag<Y>> {};

 public:
  UTIL_CONFIG_CONSTEXPR strong_rc_counter() noexcept : pi_(nullptr) {}

  template <class Y>
  explicit strong_rc_counter(Y* p) : pi_(nullptr) {
#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
    try {
#endif
      pi_ = new rc_ptr_counted_data_default<Y>(p);
#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
    } catch (...) {
      delete p;
      throw;
    }
#endif
  }

  template <class Y, class Deleter, class = typename __not_alloc_shared_tag<Deleter>::type>
  strong_rc_counter(Y* p, Deleter&& d) : pi_(nullptr) {
#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
    try {
#endif
      pi_ = new rc_ptr_counted_data_with_deleter<
          Y, typename std::remove_cv<typename std::remove_reference<Deleter>::type>::type>(p, std::move(d));
#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
    } catch (...) {
      d(p);
      throw;
    }
#endif
  }

  template <class... Args>
  strong_rc_counter(T*& __p, _strong_rc_alloc_shared_tag<T>, Args&&... args) : pi_(nullptr) {
    rc_ptr_counted_data_inplace<T>* tpi_ = ::new rc_ptr_counted_data_inplace<T>(std::forward<Args>(args)...);
    pi_ = tpi_;
    __p = tpi_->ptr();
  }

  template <class UT, class UDeleter>
  explicit strong_rc_counter(std::unique_ptr<UT, UDeleter>&& r) : pi_(nullptr) {
    if (r.get() == nullptr) {
      return;
    }

#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
    try {
#endif
      pi_ = new rc_ptr_counted_data_with_deleter<
          UT, typename std::remove_cv<typename std::remove_reference<UDeleter>::type>::type>(
          r.get(), std::forward<UDeleter>(r.get_deleter()));

      r.release();
#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
    } catch (...) {
      throw;
    }
#endif
  }

  explicit strong_rc_counter(const weak_rc_counter<T>& w);

  strong_rc_counter(const weak_rc_counter<T>& w, std::nothrow_t) noexcept;

  ~strong_rc_counter() {
    if (nullptr != pi_) {
      pi_->release();
    }
  }

  strong_rc_counter(const strong_rc_counter& r) noexcept : pi_(r.pi_) {
    if (nullptr != pi_) {
      pi_->add_ref();
    }
  }

  strong_rc_counter(strong_rc_counter&& r) noexcept : pi_(r.pi_) { r.pi_ = nullptr; }

  template <class Y>
  strong_rc_counter(const strong_rc_counter<Y>& r) noexcept : pi_(r.pi_) {
    if (nullptr != pi_) {
      pi_->add_ref();
    }
  }

  template <class Y>
  strong_rc_counter(strong_rc_counter<Y>&& r) noexcept : pi_(r.pi_) {
    r.pi_ = nullptr;
  }

  strong_rc_counter& operator=(const strong_rc_counter& r) noexcept {
    if (pi_ != r.pi_) {
      rc_ptr_counted_data_base* origin_pi = pi_;

      pi_ = r.pi_;
      if (nullptr != pi_) {
        pi_->add_ref();
      }
      if (nullptr != origin_pi) {
        origin_pi->release();
      }
    }
    return *this;
  }

  strong_rc_counter& operator=(strong_rc_counter&& r) noexcept {
    if (pi_ != r.pi_) {
      rc_ptr_counted_data_base* origin_pi = pi_;
      pi_ = r.pi_;
      r.pi_ = nullptr;

      if (nullptr != origin_pi) {
        origin_pi->release();
      }
    }
    return *this;
  }

  inline void swap(strong_rc_counter& r) noexcept {
    rc_ptr_counted_data_base* tmp = pi_;
    pi_ = r.pi_;
    r.pi_ = tmp;
  }

  template <class Y>
  inline void swap(strong_rc_counter<Y>& r) noexcept {
    rc_ptr_counted_data_base* tmp = pi_;
    pi_ = r.pi_;
    r.pi_ = tmp;
  }

  inline std::size_t use_count() const noexcept { return (nullptr != pi_) ? pi_->use_count() : 0; }

  inline rc_ptr_counted_data_base* ref_counter() const noexcept { return pi_; }

 private:
  template <class>
  friend class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_counter;

  rc_ptr_counted_data_base* pi_;
};

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY weak_rc_counter {
 public:
  UTIL_CONFIG_CONSTEXPR weak_rc_counter() noexcept : pi_(nullptr) {}

  template <class Y>
  explicit weak_rc_counter(const strong_rc_counter<Y>& other) noexcept : pi_(other.ref_counter()) {
    if (nullptr != pi_) {
      pi_->weak_add_ref();
    }
  }

  ~weak_rc_counter() {
    if (nullptr != pi_) {
      pi_->weak_release();
    }
  }

  weak_rc_counter(const weak_rc_counter& r) noexcept : pi_(r.pi_) {
    if (nullptr != pi_) {
      pi_->weak_add_ref();
    }
  }

  weak_rc_counter(weak_rc_counter&& r) noexcept : pi_(r.pi_) { r.pi_ = nullptr; }

  template <class Y>
  weak_rc_counter(const weak_rc_counter<Y>& r) noexcept : pi_(r.pi_) {
    if (nullptr != pi_) {
      pi_->weak_add_ref();
    }
  }

  template <class Y>
  weak_rc_counter(weak_rc_counter<Y>&& r) noexcept : pi_(r.pi_) {
    r.pi_ = nullptr;
  }

  weak_rc_counter& operator=(const weak_rc_counter& r) noexcept {
    if (pi_ != r.pi_) {
      rc_ptr_counted_data_base* origin_pi = pi_;
      pi_ = r.pi_;
      if (nullptr != pi_) {
        pi_->weak_add_ref();
      }

      if (nullptr != origin_pi) {
        origin_pi->weak_release();
      }
    }
    return *this;
  }

  weak_rc_counter& operator=(weak_rc_counter&& r) noexcept {
    if (pi_ != r.pi_) {
      rc_ptr_counted_data_base* origin_pi = pi_;
      pi_ = r.pi_;
      r.pi_ = nullptr;

      if (nullptr != origin_pi) {
        origin_pi->weak_release();
      }
    }
    return *this;
  }

  inline void swap(weak_rc_counter& r) noexcept {
    rc_ptr_counted_data_base* tmp = pi_;
    pi_ = r.pi_;
    r.pi_ = tmp;
  }

  template <class Y>
  inline void swap(weak_rc_counter<Y>& r) noexcept {
    rc_ptr_counted_data_base* tmp = pi_;
    pi_ = r.pi_;
    r.pi_ = tmp;
  }

  inline std::size_t use_count() const noexcept { return (nullptr != pi_) ? pi_->use_count() : 0; }

  inline rc_ptr_counted_data_base* ref_counter() const noexcept { return pi_; }

 private:
  template <class>
  friend class LIBATFRAME_UTILS_API_HEAD_ONLY weak_rc_counter;

  rc_ptr_counted_data_base* pi_;
};

template <class T>
strong_rc_counter<T>::strong_rc_counter(const weak_rc_counter<T>& w) : pi_(w.ref_counter()) {
  if (nullptr == pi_ || !pi_->add_ref_nothrow()) {
    pi_ = nullptr;
    rc_ptr_counted_data_base::throw_bad_weak_ptr();
  }
}

template <class T>
strong_rc_counter<T>::strong_rc_counter(const weak_rc_counter<T>& w, std::nothrow_t) noexcept : pi_(w.ref_counter()) {
  if (nullptr != pi_ && !pi_->add_ref_nothrow()) {
    pi_ = nullptr;
  }
}

template <class T, bool = std::is_array<T>::value, bool = std::is_void<T>::value>
class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_ptr_access {
 public:
  using element_type = T;

  inline element_type& operator*() const noexcept { return *get(); }

  inline element_type* operator->() const noexcept { return get(); }

 private:
  inline element_type* get() const noexcept { return static_cast<const strong_rc_ptr<T>*>(this)->get(); }
};

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_ptr_access<T, false, true> {
 public:
  using element_type = T;

  inline element_type* operator->() const noexcept { return get(); }

 private:
  inline element_type* get() const noexcept { return static_cast<const strong_rc_ptr<T>*>(this)->get(); }
};

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_ptr_access<T, true, false> {
 public:
  using element_type = typename std::remove_extent<T>::type;

  element_type& operator[](std::ptrdiff_t __i) const noexcept { return get()[__i]; }

 private:
  inline element_type* get() const noexcept { return static_cast<const strong_rc_ptr<T>*>(this)->get(); }
};

#if LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION <= 1
template <class T1, class T2>
LIBATFRAME_UTILS_API_HEAD_ONLY void __enable_shared_from_this_with(const strong_rc_counter<T1>&,
                                                                   enable_shared_rc_from_this<T2>* p);

template <class T>
LIBATFRAME_UTILS_API_HEAD_ONLY inline void __enable_shared_from_this_with(const strong_rc_counter<T>&, ...) {}
#endif

template <class T>
class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_ptr : public strong_rc_ptr_access<T> {
 public:
  using element_type = typename std::remove_extent<T>::type;
  using weak_type = weak_rc_ptr<T>;

 public:
  UTIL_CONFIG_CONSTEXPR strong_rc_ptr() noexcept : ptr_(nullptr), ref_counter_() {}

  UTIL_CONFIG_CONSTEXPR strong_rc_ptr(std::nullptr_t) noexcept : ptr_(nullptr), ref_counter_() {}

  template <class Y>
  explicit strong_rc_ptr(Y* ptr) noexcept : ptr_(ptr), ref_counter_(ptr) {
    static_assert(!std::is_void<Y>::value, "incomplete type");
    static_assert(sizeof(Y) > 0, "incomplete type");
#if LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION > 1
    __enable_shared_from_this_with(ptr_);
#else
    __enable_shared_from_this_with(ref_counter_, ptr_);
#endif
  }

  template <class Y, class Deleter>
  strong_rc_ptr(Y* ptr, Deleter d) noexcept : ptr_(ptr), ref_counter_(ptr, std::move(d)) {
#if LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION > 1
    __enable_shared_from_this_with(ptr_);
#else
    __enable_shared_from_this_with(ref_counter_, ptr_);
#endif
  }

  template <class Deleter>
  strong_rc_ptr(std::nullptr_t ptr, Deleter d) : ptr_(ptr), ref_counter_(ptr, std::move(d)) {}

  template <class Y>
  strong_rc_ptr(const strong_rc_ptr<Y>& other) noexcept : ptr_(other.ptr_), ref_counter_(other.ref_counter_) {}

  template <class Y>
  strong_rc_ptr(strong_rc_ptr<Y>&& other) noexcept : ptr_(other.ptr_), ref_counter_() {
    ref_counter_.swap(other.ref_counter_);
    other.ptr_ = nullptr;
  }

  template <class Y>
  strong_rc_ptr(const strong_rc_ptr<Y>& other, element_type* ptr) noexcept
      : ptr_(ptr), ref_counter_(other.ref_counter_) {}

  template <class Y>
  strong_rc_ptr(strong_rc_ptr<Y>&& other, element_type* ptr) noexcept : ptr_(ptr), ref_counter_() {
    ref_counter_.swap(other.ref_counter_);
    other.ptr_ = nullptr;
  }

  explicit strong_rc_ptr(const weak_rc_ptr<T>& other) : ptr_(nullptr), ref_counter_(other.ref_counter_) {
    ptr_ = other.ptr_;
  }

  explicit strong_rc_ptr(const weak_rc_ptr<T>& other, std::nothrow_t) noexcept
      : ptr_(nullptr), ref_counter_(other.ref_counter_, std::nothrow) {
    ptr_ = other.use_count() > 0 ? other.ptr_ : nullptr;
  }

  template <class Y>
  explicit strong_rc_ptr(const weak_rc_ptr<Y>& other) : ptr_(nullptr), ref_counter_(other.ref_counter_) {
    ptr_ = other.ptr_;
  }

  template <class Y, class Deleter>
  explicit strong_rc_ptr(std::unique_ptr<Y, Deleter>&& other) noexcept : ptr_(other.get()), ref_counter_() {
    ref_counter_ = ref_counter_(std::move(other));
#if LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION > 1
    __enable_shared_from_this_with(ptr_);
#else
    __enable_shared_from_this_with(ref_counter_, ptr_);
#endif
  }

  template <class... Args>
  explicit strong_rc_ptr(_strong_rc_alloc_shared_tag<T> __tag, Args&&... args) noexcept
      : ptr_(nullptr), ref_counter_(ptr_, __tag, std::forward<Args>(args)...) {
#if LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION > 1
    __enable_shared_from_this_with(ptr_);
#else
    __enable_shared_from_this_with(ref_counter_, ptr_);
#endif
  }

  ~strong_rc_ptr() noexcept = default;
  explicit strong_rc_ptr(const strong_rc_ptr& other) noexcept = default;
  strong_rc_ptr& operator=(const strong_rc_ptr& other) noexcept = default;

  explicit strong_rc_ptr(strong_rc_ptr&& other) noexcept : ptr_(other.ptr_), ref_counter_() {
    ref_counter_.swap(other.ref_counter_);
    other.ptr_ = nullptr;
  }

  inline strong_rc_ptr& operator=(strong_rc_ptr&& other) noexcept {
    strong_rc_ptr{std::move(other)}.swap(*this);
    return *this;
  }

  template <class Y>
  inline strong_rc_ptr& operator=(const strong_rc_ptr<Y>& other) noexcept {
    ptr_ = other.ptr_;
    ref_counter_ = other.ref_counter_;
    return *this;
  }

  template <class Y>
  inline strong_rc_ptr& operator=(strong_rc_ptr<Y>&& other) noexcept {
    strong_rc_ptr{std::move(other)}.swap(*this);
    return *this;
  }

  inline void reset() noexcept { strong_rc_ptr().swap(*this); }

  template <class Y>
  inline void reset(Y* ptr) noexcept {
    if (ptr_ == ptr && ptr != nullptr) {
      return;
    }

    strong_rc_ptr{ptr}.swap(*this);
  }

  template <class Y, class Deleter>
  inline void reset(Y* ptr, Deleter d) noexcept {
    strong_rc_ptr{ptr, std::move(d)}.swap(*this);
  }

  inline void swap(strong_rc_ptr& other) noexcept {
    std::swap(ptr_, other.ptr_);
    ref_counter_.swap(other.ref_counter_);
  }

  inline element_type* get() const noexcept { return ptr_; }

  inline std::size_t use_count() const noexcept { return ref_counter_.use_count(); }

  inline bool unique() const noexcept { return ref_counter_.use_count() == 1; }

  inline explicit operator bool() const noexcept { return nullptr != get(); }

  template <class Y>
  inline bool owner_before(strong_rc_ptr<Y> const& r) const noexcept {
    return std::less<rc_ptr_counted_data_base*>()(ref_counter_.ref_counter(), r.ref_counter_.ref_counter());
  }

  template <class Y>
  inline bool owner_before(weak_rc_ptr<Y> const& r) const noexcept {
    return std::less<rc_ptr_counted_data_base*>()(ref_counter_.ref_counter(), r.ref_counter_.ref_counter());
  }

  template <class Y>
  inline bool owner_equal(strong_rc_ptr<Y> const& r) const noexcept {
    return ref_counter_.ref_counter() == r.ref_counter_.ref_counter();
  }

  template <class Y>
  inline bool owner_equal(weak_rc_ptr<Y> const& r) const noexcept {
    return ref_counter_.ref_counter() == r.ref_counter_.ref_counter();
  }

  std::size_t owner_hash() const noexcept { return std::hash<rc_ptr_counted_data_base*>()(ref_counter_.ref_counter()); }

 private:
#if LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION > 1
  template <class Yp>
  using __esft_base_t = decltype(__strong_rc_enable_shared_rc_from_this_base(std::declval<Yp*>()));

  template <class Yp, class = void>
  struct __has_esft_base : std::false_type {};

  template <class>
  using __void_t = void;

  using __has_esft_check_type =
      typename std::conditional<std::is_array<T>::value, std::false_type, std::true_type>::type;

  template <typename Yp>
  struct __has_esft_base<Yp, __void_t<__esft_base_t<Yp>>> : public __has_esft_check_type {
  };  // No enable shared_from_this for arrays

  template <class Y, class Y2 = typename std::remove_cv<Y>::type,
            typename std::enable_if<__has_esft_base<Y2>::value, uint32_t>::type = 0>
  void __enable_shared_from_this_with(Y* p) noexcept {
    auto base = __strong_rc_enable_shared_rc_from_this_base(p);
    if (nullptr != base) {
      base->weak_assign();
    }
  }

  template <class Y, class Y2 = typename std::remove_cv<Y>::type,
            typename std::enable_if<!__has_esft_base<Y2>::value, int32_t>::type = 0>
  void __enable_shared_from_this_with(Y*) noexcept {}
#endif

  template <class>
  friend class LIBATFRAME_UTILS_API_HEAD_ONLY weak_rc_ptr;

  template <class>
  friend class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_ptr;

 private:
  element_type* ptr_;
  strong_rc_counter<element_type> ref_counter_;
};

template <class T1, class T2>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator==(const strong_rc_ptr<T1>& l, const strong_rc_ptr<T2>& r) noexcept {
  return l.get() == r.get();
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator==(const strong_rc_ptr<T1>& l, ::std::nullptr_t) noexcept {
  return !l;
}

template <class T1, class T2, bool = std::is_convertible<T1*, T2*>::value || std::is_convertible<T2*, T1*>::value>
struct __strong_rc_ptr_compare_common_type;

template <class T1, class T2>
struct __strong_rc_ptr_compare_common_type<T1, T2, true> {
  using left_type = T1*;
  using right_type = T2*;

  using common_type = typename std::common_type<left_type, right_type>::type;
};

template <class T1, class T2>
struct __strong_rc_ptr_compare_common_type<T1, T2, false> {
  using left_type = const void*;
  using right_type = const void*;

  using common_type = const void*;
};

#ifdef __cpp_impl_three_way_comparison
template <class T1, class T2>
LIBATFRAME_UTILS_API_HEAD_ONLY inline std::strong_ordering operator<=>(const strong_rc_ptr<T1>& l,
                                                                       const strong_rc_ptr<T2>& r) noexcept {
  return reinterpret_cast<typename __strong_rc_ptr_compare_common_type<T1, T2>::left_type>(l.get()) <=>
         reinterpret_cast<typename __strong_rc_ptr_compare_common_type<T1, T2>::right_type>(r.get());
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline std::strong_ordering operator<=>(const strong_rc_ptr<T1>& l,
                                                                       ::std::nullptr_t) noexcept {
  return l.get() <=> static_cast<T1*>(nullptr);
}
#else

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator==(::std::nullptr_t, const strong_rc_ptr<T1>& r) noexcept {
  return !r;
}

template <class T1, class T2>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator!=(const strong_rc_ptr<T1>& l, const strong_rc_ptr<T2>& r) noexcept {
  return l.get() != r.get();
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator!=(const strong_rc_ptr<T1>& l, ::std::nullptr_t) noexcept {
  return l.get() != nullptr;
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator!=(::std::nullptr_t, const strong_rc_ptr<T1>& r) noexcept {
  return r.get() != nullptr;
}

template <class T1, class T2>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator<(const strong_rc_ptr<T1>& l, const strong_rc_ptr<T2>& r) noexcept {
  return std::less<typename __strong_rc_ptr_compare_common_type<
      typename strong_rc_ptr<T1>::element_type, typename strong_rc_ptr<T2>::element_type>::common_type>()(l.get(),
                                                                                                          r.get());
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator<(const strong_rc_ptr<T1>& l, ::std::nullptr_t) noexcept {
  return std::less<T1>()(l.get(), nullptr);
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator<(::std::nullptr_t, const strong_rc_ptr<T1>& r) noexcept {
  return std::less<T1>()(nullptr, r.get());
}

template <class T1, class T2>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator>(const strong_rc_ptr<T1>& l, const strong_rc_ptr<T2>& r) noexcept {
  return std::greater<typename __strong_rc_ptr_compare_common_type<
      typename strong_rc_ptr<T1>::element_type, typename strong_rc_ptr<T2>::element_type>::common_type>()(l.get(),
                                                                                                          r.get());
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator>(const strong_rc_ptr<T1>& l, ::std::nullptr_t) noexcept {
  return std::greater<T1>()(l.get(), nullptr);
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator>(::std::nullptr_t, const strong_rc_ptr<T1>& r) noexcept {
  return std::greater<T1>()(nullptr, r.get());
}

template <class T1, class T2>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator<=(const strong_rc_ptr<T1>& l, const strong_rc_ptr<T2>& r) noexcept {
  return !(r < l);
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator<=(const strong_rc_ptr<T1>& l, ::std::nullptr_t) noexcept {
  return !(nullptr < l);
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator<=(::std::nullptr_t, const strong_rc_ptr<T1>& r) noexcept {
  return !(r < nullptr);
}

template <class T1, class T2>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator>=(const strong_rc_ptr<T1>& l, const strong_rc_ptr<T2>& r) noexcept {
  return !(r > l);
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator>=(const strong_rc_ptr<T1>& l, ::std::nullptr_t) noexcept {
  return !(nullptr > l);
}

template <class T1>
LIBATFRAME_UTILS_API_HEAD_ONLY inline bool operator>=(::std::nullptr_t, const strong_rc_ptr<T1>& r) noexcept {
  return !(r > nullptr);
}
#endif

template <typename T>
class LIBATFRAME_UTILS_API_HEAD_ONLY weak_rc_ptr {
 public:
  using element_type = typename std::remove_extent<T>::type;

 public:
  UTIL_CONFIG_CONSTEXPR weak_rc_ptr() noexcept : ptr_(nullptr), ref_counter_() {}

  explicit weak_rc_ptr(const weak_rc_ptr& other) noexcept = default;
  weak_rc_ptr& operator=(const weak_rc_ptr& other) noexcept = default;
  ~weak_rc_ptr() = default;

  explicit weak_rc_ptr(weak_rc_ptr&& other) noexcept : ptr_(other.ptr_), ref_counter_(std::move(other.ref_counter_)) {
    other.ptr_ = nullptr;
  }

  weak_rc_ptr& operator=(weak_rc_ptr&& other) noexcept {
    weak_rc_ptr{std::move(other)}.swap(*this);
    return *this;
  }

  template <class Y>
  explicit weak_rc_ptr(const strong_rc_ptr<Y>& other) noexcept : ptr_(other.ptr_), ref_counter_(other.ref_counter_) {}

  template <class Y>
  weak_rc_ptr(const weak_rc_ptr<Y>& other) noexcept : ref_counter_(other.ref_counter_) {
    ptr_ = other.lock().get();
  }

  template <class Y>
  weak_rc_ptr(weak_rc_ptr<Y>&& other) noexcept : ptr_(other.lock().get()), ref_counter_(std::move(other.ref_counter_)) {
    other.ptr_ = nullptr;
  }

  template <class Y>
  weak_rc_ptr& operator=(const weak_rc_ptr<Y>& other) noexcept {
    ptr_ = other.lock().get();
    ref_counter_ = other.ref_counter_;
    return *this;
  }

  template <class Y>
  weak_rc_ptr& operator=(weak_rc_ptr<Y>&& other) noexcept {
    weak_rc_ptr{std::move(other)}.swap(*this);
    return *this;
  }

  template <class Y>
  weak_rc_ptr& operator=(const strong_rc_ptr<Y>& other) noexcept {
    ptr_ = other.ptr_;
    weak_rc_counter<T>{other.ref_counter_}.swap(ref_counter_);
    return *this;
  }

  inline void reset() noexcept { weak_rc_ptr{}.swap(*this); }

  inline void swap(weak_rc_ptr& r) noexcept {
    std::swap(ptr_, r.ptr_);
    ref_counter_.swap(r.ref_counter_);
  }

  inline std::size_t use_count() const noexcept { return ref_counter_.use_count(); }

  inline bool expired() const noexcept { return ref_counter_.use_count() == 0; }

  inline strong_rc_ptr<T> lock() const noexcept { return strong_rc_ptr<T>(*this, std::nothrow); }

  template <class Y>
  inline bool owner_before(strong_rc_ptr<Y> const& r) const noexcept {
    return std::less<rc_ptr_counted_data_base*>()(ref_counter_.ref_counter(), r.ref_counter_.ref_counter());
  }

  template <class Y>
  inline bool owner_before(weak_rc_ptr<Y> const& r) const noexcept {
    return std::less<rc_ptr_counted_data_base*>()(ref_counter_.ref_counter(), r.ref_counter_.ref_counter());
  }

  template <class Y>
  inline bool owner_equal(strong_rc_ptr<Y> const& r) const noexcept {
    return ref_counter_.ref_counter() == r.ref_counter_.ref_counter();
  }

  template <class Y>
  inline bool owner_equal(weak_rc_ptr<Y> const& r) const noexcept {
    return ref_counter_.ref_counter() == r.ref_counter_.ref_counter();
  }

  std::size_t owner_hash() const noexcept { return std::hash<rc_ptr_counted_data_base*>()(ref_counter_.ref_counter()); }

 private:
  template <class>
  friend class LIBATFRAME_UTILS_API_HEAD_ONLY enable_shared_rc_from_this;

  // Used by enable_shared_rc_from_this.
  void assign(element_type* __ptr, const strong_rc_counter<T>& __refcount) noexcept {
    if (use_count() == 0) {
      ptr_ = __ptr;
      ref_counter_ = __refcount;
    }
  }

  template <class>
  friend class LIBATFRAME_UTILS_API_HEAD_ONLY weak_rc_ptr;

  template <class>
  friend class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_ptr;

 private:
  element_type* ptr_;
  weak_rc_counter<T> ref_counter_;
};

template <class T>
class enable_shared_rc_from_this {
 public:
  strong_rc_ptr<T> shared_from_this() { return weak_this_.lock(); }

  strong_rc_ptr<const T> shared_from_this() const { return weak_this_.lock(); }

  weak_rc_ptr<T> weak_from_this() { return weak_this_; }

  weak_rc_ptr<const T> weak_from_this() const { return weak_this_; }

 protected:
  enable_shared_rc_from_this() = default;
  enable_shared_rc_from_this(const enable_shared_rc_from_this&) = default;
  enable_shared_rc_from_this& operator=(const enable_shared_rc_from_this&) = default;
  ~enable_shared_rc_from_this() = default;

 private:
  template <class>
  friend class LIBATFRAME_UTILS_API_HEAD_ONLY strong_rc_ptr;

  template <class Y>
  void weak_assign(T* __p, const strong_rc_counter<Y>& __n) const noexcept {
    weak_this_.assign(__p, __n);
  }

#if LIBATFRAME_UTILS_STRONG_RC_PTR_ENABLE_SHARED_FROM_THIS_VERSION == 1
  template <class Y>
  friend void __enable_shared_from_this_with(const strong_rc_counter<Y>&, enable_shared_rc_from_this* p) {
    if (nullptr != p) {
      p->weak_assign();
    }
  }
#else
  friend inline const enable_shared_rc_from_this* __strong_rc_enable_shared_rc_from_this_base(
      const enable_shared_rc_from_this* __p) {
    return __p;
  }
#endif

 private:
  weak_rc_ptr<T> weak_this_;
};

template <class T, class... TArgs>
strong_rc_ptr<T> make_strong_rc(TArgs&&... args) {
  static_assert(!std::is_array<T>::value, "make_shared<T[]> not supported");

  using strong_rc_ptr_element = typename std::remove_const<T>::type;

  return strong_rc_ptr<T>(_strong_rc_alloc_shared_tag<T>{}, std::forward<TArgs>(args)...);
}

template <class T, class Y>
strong_rc_ptr<T> static_pointer_cast(const strong_rc_ptr<Y>& r) noexcept {
  return strong_rc_ptr<T>(r, static_cast<typename strong_rc_ptr<T>::element_type*>(r.get()));
}

template <class T, class Y>
strong_rc_ptr<T> const_pointer_cast(const strong_rc_ptr<Y>& r) noexcept {
  return strong_rc_ptr<T>(r, const_cast<typename strong_rc_ptr<T>::element_type*>(r.get()));
}

#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
template <class T, class Y>
strong_rc_ptr<T> dynamic_pointer_cast(const strong_rc_ptr<Y>& r) noexcept {
  return strong_rc_ptr<T>(r, dynamic_cast<typename strong_rc_ptr<T>::element_type*>(r.get()));
}
#endif

}  // namespace memory
LIBATFRAME_UTILS_NAMESPACE_END

namespace std {
template <class T>
LIBATFRAME_UTILS_API_HEAD_ONLY void swap(LIBATFRAME_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<T>& a,
                                         LIBATFRAME_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<T>& b) noexcept {
  a.swap(b);
}

template <class T>
struct LIBATFRAME_UTILS_API_HEAD_ONLY hash<LIBATFRAME_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<T>> {
  std::size_t operator()(const LIBATFRAME_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<T>& s) const noexcept {
    return std::hash<T*>()(s.get());
  }
};
}  // namespace std
