// Copyright 2021 atframework
// Created by owent on 2021-08-10
// This is a simple alternative to std::variant to hold success type or failure type.
// This is also just like std::Result<OK, ERROR> in rust

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <nostd/type_traits.h>
#include <std/explicit_declare.h>

#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

namespace util {
namespace design_pattern {
template <class T>
struct LIBATFRAME_UTILS_API_HEAD_ONLY small_object_optimize_storage_deleter {
  inline void operator()(T *) const noexcept {
    // Do nothing
  }
  template <class U>
  inline void operator()(U *) const noexcept {
    // Do nothing
  }
};

template <class LT, class RT, bool>
struct LIBATFRAME_UTILS_API_HEAD_ONLY max_storage_size_helper;

template <class LT, class RT>
struct LIBATFRAME_UTILS_API_HEAD_ONLY max_storage_size_helper<LT, RT, true> {
  static constexpr const std::size_t value = sizeof(typename std::aligned_storage<sizeof(RT)>::type);
};

template <class LT, class RT>
struct LIBATFRAME_UTILS_API_HEAD_ONLY max_storage_size_helper<LT, RT, false> {
  static constexpr const std::size_t value = sizeof(typename std::aligned_storage<sizeof(LT)>::type);
};

template <class T>
struct LIBATFRAME_UTILS_API_HEAD_ONLY compact_storage_select_type;

template <>
struct LIBATFRAME_UTILS_API_HEAD_ONLY compact_storage_select_type<void> {
  typedef std::unique_ptr<void, small_object_optimize_storage_deleter<void>> type;
};

template <class T>
struct LIBATFRAME_UTILS_API_HEAD_ONLY compact_storage_select_type {
  typedef typename std::conditional<
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)) || \
    (defined(__cplusplus) && __cplusplus >= 201402L &&  \
     !(defined(__GNUC_MAJOR__) && defined(__GNUC_MINOR__) && __GNUC_MAJOR__ * 100 + __GNUC_MINOR__ <= 409))
      std::is_trivially_copyable<T>::value
#elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
      std::is_trivial<T>::value
#else
      std::is_pod<T>::value
#endif
          && sizeof(T) < (sizeof(size_t) << 2),
      std::unique_ptr<T, small_object_optimize_storage_deleter<T>>, typename std::unique_ptr<T>::deleter_type>::type
      type;
};

template <class T, class DeleterT>
struct LIBATFRAME_UTILS_API_HEAD_ONLY compact_storage_type;

template </**void**/>
struct LIBATFRAME_UTILS_API_HEAD_ONLY
    compact_storage_type<void, std::unique_ptr<void, small_object_optimize_storage_deleter<void>>>
    : public std::true_type {
  using value_type = void *;
  using pointer = std::unique_ptr<void, small_object_optimize_storage_deleter<void>>;
  using storage_type = pointer;

  static UTIL_FORCEINLINE void destroy_storage(storage_type &out) {
    out.release();
    out.~storage_type();
  }

  static UTIL_FORCEINLINE void construct_storage(void *out) {
    // Placement new
    new (out) storage_type(out);
  }

  static UTIL_FORCEINLINE void move_storage(storage_type &out, storage_type &&in) noexcept {
    if (&out == &in) {
      return;
    }

    out = std::move(in);
  }

  static UTIL_FORCEINLINE void swap(storage_type &l, storage_type &r) noexcept { l.swap(r); }

  static UTIL_FORCEINLINE pointer &unref(storage_type &storage) noexcept { return storage; }
  static UTIL_FORCEINLINE const pointer &unref(const storage_type &storage) noexcept { return storage; }

  static UTIL_FORCEINLINE pointer &default_instance() noexcept {
    static pointer empty;
    return empty;
  }
};

template <class T>
struct LIBATFRAME_UTILS_API_HEAD_ONLY
    compact_storage_type<T, std::unique_ptr<T, small_object_optimize_storage_deleter<T>>> : public std::true_type {
  using value_type = T;
  using pointer = std::unique_ptr<value_type, small_object_optimize_storage_deleter<value_type>>;
  using storage_type = std::pair<value_type, pointer>;

  static UTIL_FORCEINLINE void destroy_storage(storage_type &out) {
    out.second.release();
    out.~storage_type();
  }

  template <class... TARGS>
  static UTIL_FORCEINLINE void construct_storage(void *out, TARGS &&...in) {
    // Placement new
    storage_type *ptr = new (out) storage_type(value_type{std::forward<TARGS>(in)...}, nullptr);
    ptr->second.reset(&ptr->first);
  }

  template <class U, class UDELETOR,
            typename std::enable_if<std::is_base_of<T, typename std::decay<U>::type>::value ||
                                        std::is_convertible<typename std::decay<U>::type, T>::value,
                                    bool>::type = false>
  static UTIL_FORCEINLINE void construct_storage(void *out, std::unique_ptr<U, UDELETOR> &&in) noexcept {
    storage_type *ptr;
    if (in) {
      // Placement new
      ptr = new (out) storage_type(value_type{std::move(*in)}, nullptr);
    } else {
      // Placement new
      ptr = new (out) storage_type();
    }
    ptr->second.reset(ptr->first);
  }

  static UTIL_FORCEINLINE void move_storage(storage_type &out, storage_type &&in) noexcept {
    if (&out == &in) {
      return;
    }

    out.first = in.first;
  }

  static UTIL_FORCEINLINE void swap(storage_type &l, storage_type &r) noexcept {
    value_type lv = l.first;
    l.first = r.first;
    r.first = lv;
  }

  static UTIL_FORCEINLINE pointer &unref(storage_type &storage) noexcept { return storage.second; }
  static UTIL_FORCEINLINE const pointer &unref(const storage_type &storage) noexcept { return storage.second; }

  static UTIL_FORCEINLINE pointer &default_instance() noexcept {
    static pointer empty;
    return empty;
  }
};

template <class T>
struct LIBATFRAME_UTILS_API_HEAD_ONLY compact_storage_type<T, typename std::unique_ptr<T>::deleter_type>
    : public std::false_type {
  using value_type = T;
  using pointer = std::unique_ptr<T, typename std::unique_ptr<T>::deleter_type>;
  using storage_type = pointer;

  static UTIL_FORCEINLINE void destroy_storage(storage_type &out) { out.~storage_type(); }

  template <class U, typename std::enable_if<std::is_convertible<typename std::decay<U>::type, pointer>::value,
                                             bool>::type = false>
  static UTIL_FORCEINLINE void construct_storage(void *out, U &&in) {
    // Placement new
    new (out) pointer{std::move(in)};
  }

  template <class... TARGS>
  static UTIL_FORCEINLINE void construct_storage(void *out, TARGS &&...in) {
    // Placement new
    new (out) pointer{new T(std::forward<TARGS>(in)...)};
  }

  static UTIL_FORCEINLINE void move_storage(storage_type &out, storage_type &&in) noexcept {
    if (&out == &in) {
      return;
    }

    out.swap(in);
    in.reset();
  }

  static UTIL_FORCEINLINE void swap(storage_type &l, storage_type &r) noexcept { l.swap(r); }

  static UTIL_FORCEINLINE pointer &unref(storage_type &storage) noexcept { return storage; }
  static UTIL_FORCEINLINE const pointer &unref(const storage_type &storage) noexcept { return storage; }

  static UTIL_FORCEINLINE pointer &default_instance() noexcept {
    static pointer empty;
    return empty;
  }
};

template <class T, class DeleterT>
struct LIBATFRAME_UTILS_API_HEAD_ONLY compact_storage_type : public std::false_type {
  using value_type = T;
  using pointer = std::unique_ptr<T, DeleterT>;
  using storage_type = pointer;

  static UTIL_FORCEINLINE void destroy_storage(storage_type &out) { out.~storage_type(); }

  template <class... TARGS>
  static UTIL_FORCEINLINE void construct_storage(void *out, TARGS &&...in) {
    // Placement new
    new (out) pointer{std::forward<TARGS>(in)...};
  }

  static UTIL_FORCEINLINE void move_storage(storage_type &out, storage_type &&in) noexcept {
    if (&out == &in) {
      return;
    }

    out.swap(in);
    in.reset();
  }

  static UTIL_FORCEINLINE void swap(storage_type &l, storage_type &r) noexcept { l.swap(r); }

  static UTIL_FORCEINLINE pointer &unref(storage_type &storage) noexcept { return storage; }
  static UTIL_FORCEINLINE const pointer &unref(const storage_type &storage) noexcept { return storage; }

  static UTIL_FORCEINLINE pointer &default_instance() noexcept {
    static pointer empty;
    return empty;
  }
};

template <class T>
struct default_compact_storage_type {
  using type = compact_storage_type<T, typename compact_storage_select_type<T>::type>;
};

template <class TOK, class TERR>
class LIBATFRAME_UTILS_API_HEAD_ONLY result_base_type {
 public:
  using success_type = TOK;
  using error_type = TERR;
  using success_storage_type = typename default_compact_storage_type<success_type>::type;
  using error_storage_type = typename default_compact_storage_type<error_type>::type;
  using success_pointer = typename success_storage_type::pointer;
  using error_pointer = typename error_storage_type::pointer;

  enum class mode : int32_t {
    kSuccess = 0,
    kError = 1,
    kNone = 2,
  };

  UTIL_FORCEINLINE bool is_success() const noexcept { return mode_ == mode::kSuccess; }
  UTIL_FORCEINLINE bool is_error() const noexcept { return mode_ == mode::kError; }
  UTIL_FORCEINLINE bool is_none() const noexcept { return mode_ == mode::kNone; }

  UTIL_FORCEINLINE const success_pointer &get_success() const noexcept {
    return is_success()
               ? success_storage_type::unref(
                     *reinterpret_cast<const typename success_storage_type::storage_type *>(success_data_arena()))
               : success_storage_type::default_instance();
  }

  UTIL_FORCEINLINE const error_pointer &get_error() const noexcept {
    return is_error() ? error_storage_type::unref(
                            *reinterpret_cast<const typename error_storage_type::storage_type *>(error_data_arena()))
                      : error_storage_type::default_instance();
  }

  result_base_type() : mode_(mode::kNone) {}
  ~result_base_type() { reset(); }

  explicit result_base_type(result_base_type &&other) : mode_(mode::kNone) { swap(other); }

  result_base_type &operator=(result_base_type &&other) noexcept {
    if (this == &other) {
      return *this;
    }

    swap(other);
    other.reset();
    return *this;
  }

  inline void swap(result_base_type &other) noexcept {
    using std::swap;
    if (this == &other) {
      return;
    }

    if (mode_ == other.mode_) {
      if (mode::kSuccess == mode_) {
        success_storage_type::swap(
            *reinterpret_cast<typename success_storage_type::storage_type *>(success_data_arena()),
            *reinterpret_cast<typename success_storage_type::storage_type *>(other.success_data_arena()));
      } else if (mode::kError == mode_) {
        error_storage_type::swap(
            *reinterpret_cast<typename error_storage_type::storage_type *>(error_data_arena()),
            *reinterpret_cast<typename error_storage_type::storage_type *>(other.error_data_arena()));
      }
    } else if (mode::kNone == mode_) {
      if (mode::kSuccess == other.mode_) {
        construct_success();
        success_storage_type::move_storage(
            *reinterpret_cast<typename success_storage_type::storage_type *>(success_data_arena()),
            std::move(*reinterpret_cast<typename success_storage_type::storage_type *>(other.success_data_arena())));
      } else if (mode::kError == other.mode_) {
        construct_error();
        error_storage_type::move_storage(
            *reinterpret_cast<typename error_storage_type::storage_type *>(error_data_arena()),
            std::move(*reinterpret_cast<typename error_storage_type::storage_type *>(other.error_data_arena())));
      }

      other.reset();
    } else if (mode::kNone == other.mode_) {
      if (mode::kSuccess == mode_) {
        other.construct_success();
        success_storage_type::move_storage(
            *reinterpret_cast<typename success_storage_type::storage_type *>(other.success_data_arena()),
            std::move(*reinterpret_cast<typename success_storage_type::storage_type *>(success_data_arena())));
      } else if (mode::kError == mode_) {
        other.construct_error();
        error_storage_type::move_storage(
            *reinterpret_cast<typename error_storage_type::storage_type *>(other.error_data_arena()),
            std::move(*reinterpret_cast<typename error_storage_type::storage_type *>(error_data_arena())));
      }

      reset();
    } else {
      if (mode::kSuccess == mode_) {
        // tempory data
        typename success_storage_type::storage_type t;
        success_storage_type::move_storage(
            t, std::move(*reinterpret_cast<typename success_storage_type::storage_type *>(success_data_arena())));
        // self
        construct_error();
        error_storage_type::move_storage(
            *reinterpret_cast<typename error_storage_type::storage_type *>(error_data_arena()),
            std::move(*reinterpret_cast<typename error_storage_type::storage_type *>(other.error_data_arena())));
        // other
        other.construct_success();
        success_storage_type::move_storage(
            *reinterpret_cast<typename success_storage_type::storage_type *>(other.success_data_arena()), std::move(t));
      } else {
        // tempory data
        typename error_storage_type::storage_type t;
        error_storage_type::move_storage(
            t, std::move(*reinterpret_cast<typename error_storage_type::storage_type *>(error_data_arena())));
        // self
        construct_success();
        success_storage_type::move_storage(
            *reinterpret_cast<typename success_storage_type::storage_type *>(success_data_arena()),
            std::move(*reinterpret_cast<typename success_storage_type::storage_type *>(other.success_data_arena())));
        // other
        other.construct_error();
        error_storage_type::move_storage(
            *reinterpret_cast<typename error_storage_type::storage_type *>(other.error_data_arena()), std::move(t));
      }
    }
  }

 private:
  template <class UOK, class UERR>
  friend class result_type;

  template <class... TARGS>
  inline void construct_success(TARGS &&...args) {
    reset();
    success_storage_type::construct_storage(success_data_arena(), std::forward<TARGS>(args)...);
    mode_ = mode::kSuccess;
  }

  template <class... TARGS>
  inline void construct_error(TARGS &&...args) {
    reset();
    error_storage_type::construct_storage(error_data_arena(), std::forward<TARGS>(args)...);
    mode_ = mode::kError;
  }

  void reset() {
    if (mode::kSuccess == mode_) {
      success_storage_type::destroy_storage(
          *reinterpret_cast<typename success_storage_type::storage_type *>(success_data_arena()));
    } else if (mode::kError == mode_) {
      error_storage_type::destroy_storage(
          *reinterpret_cast<typename error_storage_type::storage_type *>(error_data_arena()));
    }

    mode_ = mode::kNone;
  }

 private:
  inline void *success_data_arena() noexcept { return reinterpret_cast<void *>(&data_); }

  inline const void *success_data_arena() const noexcept { return reinterpret_cast<const void *>(&data_); }

  inline void *error_data_arena() noexcept { return reinterpret_cast<void *>(&data_); }

  inline const void *error_data_arena() const noexcept { return reinterpret_cast<const void *>(&data_); }

  using data_buffer_type = typename nostd::aligned_union<0, typename success_storage_type::storage_type,
                                                         typename error_storage_type::storage_type>::type;
  data_buffer_type data_;
  mode mode_;
};

template <class TOK, class TERR>
class LIBATFRAME_UTILS_API_HEAD_ONLY result_type : public result_base_type<TOK, TERR> {
 public:
  using base_type = result_base_type<TOK, TERR>;
  using self_type = result_type<TOK, TERR>;

 public:
  template <class... TARGS>
  static inline self_type make_success(TARGS &&...args) {
    self_type ret;
    ret.construct_success(std::forward<TARGS>(args)...);
    return ret;
  }

  template <class... TARGS>
  static inline self_type make_error(TARGS &&...args) {
    self_type ret;
    ret.construct_error(std::forward<TARGS>(args)...);
    return ret;
  }
};
}  // namespace design_pattern
}  // namespace util
