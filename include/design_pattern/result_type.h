// Copyright 2021 atframework
// Created by owent on 2021-08-10
// This is a simple alternative to std::variant to hold success type or failure type.
// This is also just like std::Result<OK, ERROR> in rust and an alternative to std::Expected<OK, ERROR> in C++23.
// We use dynamic memory allocation to store the success type or failure type for no-trivially copyable types and large
// types by default, and we can use template specialization or ATFW_UTIL_MACRO_DECLARE_INPLACE_RESULT_STORAGE to change
// the behaviour. Example:
// ```cpp
// struct custom_type {
//   /* ... */
// };
// ATFW_UTIL_MACRO_DECLARE_INPLACE_RESULT_STORAGE(custom_type);
// ```

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <nostd/nullability.h>
#include <nostd/type_traits.h>
#include <std/explicit_declare.h>

#include <assert.h>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace design_pattern {
template <class TOK, class TERR>
class result_type;

template <class T>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __inplace_result_storage_delegate {
  inline void operator()(T *) const noexcept {
    // Do nothing
  }

  template <class U>
  inline void operator()(U *) const noexcept {
    // Do nothing
  }
};

template <class LT, class RT, bool>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __max_storage_size_helper;

template <class LT, class RT>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __max_storage_size_helper<LT, RT, true> {
  static constexpr const std::size_t value = sizeof(typename nostd::aligned_storage<sizeof(RT)>::type);
};

template <class LT, class RT>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __max_storage_size_helper<LT, RT, false> {
  static constexpr const std::size_t value = sizeof(typename nostd::aligned_storage<sizeof(LT)>::type);
};

template <class T>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __compact_storage_select_type;

struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __void_result_storage {};

struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __void_result_constructor_tag {};

template <>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __compact_storage_select_type<void> {
  using type = __inplace_result_storage_delegate<void>;
};

template <class T>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __compact_storage_select_type {
  using type = typename std::conditional<
#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)) || \
    (defined(__cplusplus) && __cplusplus >= 201402L &&  \
     !(!defined(__clang__) && defined(__GNUC__) && defined(__GNUC_MINOR__) && __GNUC__ * 100 + __GNUC_MINOR__ <= 409))
      std::is_trivially_copyable<T>::value
#elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
      std::is_trivial<T>::value
#else
      std::is_pod<T>::value
#endif
          // malloc&free may cost 20x+ time than copy memory, so we use copy memory for small trivially copyable types
          // Coherency line size in most modern CPU is 64, we need space to store mode_, so we use 56 bytes as the
          // threshold.
          && sizeof(T) <= 56,
      __inplace_result_storage_delegate<T>, ::std::unique_ptr<T>>::type;
};

#if ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)) || \
    (defined(__cplusplus) && __cplusplus >= 201402L &&  \
     !(!defined(__clang__) && defined(__GNUC__) && defined(__GNUC_MINOR__) && __GNUC__ * 100 + __GNUC_MINOR__ <= 409))
static_assert(std::is_trivially_destructible<__void_result_storage>::value &&
                  std::is_trivially_destructible<__void_result_storage>::value,
              "__void_result_storage must be trivially");
#elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
static_assert(std::is_trivial<__void_result_storage>::value, "__void_result_storage must be trivially");
#else
static_assert(std::is_pod<__void_result_storage>::value, "__void_result_storage must be trivially");
#endif

template <class T>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY compact_storage_type;

template </**void**/>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY compact_storage_type<__inplace_result_storage_delegate<void>>
    : public std::true_type {
  using value_type = void *;
  using pointer = void *;
  using const_pointer = const void *;
  using storage_type = __void_result_storage;

  UTIL_FORCEINLINE static void destroy_storage(storage_type &) noexcept {}

  UTIL_FORCEINLINE static void construct_storage(void *) noexcept {}

  UTIL_FORCEINLINE static void construct_storage(void *, const __void_result_constructor_tag &) noexcept {}

  UTIL_FORCEINLINE static void construct_move_storage(void *, storage_type &&) noexcept {}

  UTIL_FORCEINLINE static void swap(storage_type &, storage_type &) noexcept {}

  UTIL_FORCEINLINE static pointer unref(storage_type &) noexcept { return default_true_instance(); }
  UTIL_FORCEINLINE static const_pointer unref(const storage_type &) noexcept { return default_true_instance(); }

  UTIL_FORCEINLINE static constexpr pointer default_instance() noexcept { return nullptr; }

  UTIL_FORCEINLINE static pointer default_true_instance() noexcept { return reinterpret_cast<pointer>(1); }
};

template <class T>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY compact_storage_type<__inplace_result_storage_delegate<T>>
    : public std::true_type {
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using storage_type = value_type;

  UTIL_FORCEINLINE static void destroy_storage(storage_type &out) noexcept { out.~storage_type(); }

  template <class... TARGS>
  UTIL_FORCEINLINE static void construct_storage(void *out, TARGS &&...in) noexcept(
      std::is_nothrow_constructible<value_type, TARGS...>::value) {
    // Placement new
    new (out) storage_type{std::forward<TARGS>(in)...};
  }
  UTIL_FORCEINLINE static void construct_storage(void *out, const __void_result_constructor_tag &) noexcept {
    // Placement new
    new (out) storage_type();
  }
  // Using template function so allow non-movable types
  template <class StorageType>
  UTIL_FORCEINLINE static void construct_move_storage(void *out, StorageType &&in) noexcept(
      std::is_nothrow_move_assignable<value_type>::value) {
    if (out == reinterpret_cast<void *>(&in)) {
      return;
    }
#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#    pragma GCC diagnostic push
#  endif
#  pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
#endif
    // Placement new
    new (out) storage_type(std::move(in));
#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#  if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#    pragma GCC diagnostic pop
#  endif
#endif
  }

  UTIL_FORCEINLINE static void swap(storage_type &l, storage_type &r) noexcept {
    using ::std::swap;
    swap(l, r);
  }

  UTIL_FORCEINLINE static pointer unref(storage_type &storage) noexcept { return &storage; }
  UTIL_FORCEINLINE static const_pointer unref(const storage_type &storage) noexcept { return &storage; }

  UTIL_FORCEINLINE static constexpr pointer default_instance() noexcept { return nullptr; }
};

template <class T, class DeleterT>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY compact_storage_type<::std::unique_ptr<T, DeleterT>> : public std::false_type {
  using value_type = T;
  using pointer = T *;
  using const_pointer = const T *;
  using storage_type = ::std::unique_ptr<T, DeleterT>;

  UTIL_FORCEINLINE static void destroy_storage(storage_type &out) noexcept { out.~storage_type(); }

  UTIL_FORCEINLINE static void construct_storage(void *out, storage_type &&in) noexcept {
    // Placement new
    new (out) storage_type{std::move(in)};
  }

  template <class... TARGS>
  UTIL_FORCEINLINE static void construct_storage(void *out, TARGS &&...in) noexcept(
      std::is_nothrow_constructible<value_type, TARGS...>::value) {
    // Placement new
    new (out) storage_type{new value_type(std::forward<TARGS>(in)...)};
  }

  UTIL_FORCEINLINE static void construct_move_storage(void *out, storage_type &&in) noexcept {
    if (out == reinterpret_cast<void *>(&in)) {
      return;
    }

    // Placement new
    new (out) storage_type{std::move(in)};
  }

  UTIL_FORCEINLINE static void swap(storage_type &l, storage_type &r) noexcept { l.swap(r); }

  UTIL_FORCEINLINE static pointer unref(storage_type &storage) noexcept { return storage.get(); }
  UTIL_FORCEINLINE static const_pointer unref(const storage_type &storage) noexcept { return storage.get(); }

  UTIL_FORCEINLINE static constexpr pointer default_instance() noexcept { return nullptr; }
};

template <class T>
struct default_compact_storage_type {
  using type = compact_storage_type<typename __compact_storage_select_type<T>::type>;
};

template <class OriginValueType, class FuncType, bool InputVoid, bool OutputVoid>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_map_invoke_utility_base;

template <class OriginValueType, class FuncType>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_map_invoke_utility_base<OriginValueType, FuncType, true, true> {
  using value_type = void;

  template <class ParamPointerType, class InputFuncType>
  static __void_result_constructor_tag invoke(ParamPointerType &&, InputFuncType &&f) noexcept(
      noexcept(nostd::invoke(::std::declval<InputFuncType>()))) {
    nostd::invoke(std::forward<InputFuncType>(f));
    return __void_result_constructor_tag{};
  }
};

template <class OriginValueType, class FuncType>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_map_invoke_utility_base<OriginValueType, FuncType, true, false> {
  using value_type = nostd::invoke_result_t<FuncType>;

  template <class ParamPointerType, class InputFuncType>
  static value_type invoke(ParamPointerType &&,
                           InputFuncType &&f) noexcept(noexcept(nostd::invoke(::std::declval<InputFuncType>()))) {
    return nostd::invoke(std::forward<InputFuncType>(f));
  }
};

template <class OriginValueType, class FuncType>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_map_invoke_utility_base<OriginValueType, FuncType, false, true> {
  using value_type = void;

  template <class ParamPointerType, class InputFuncType>
  static __void_result_constructor_tag invoke(ParamPointerType &&p, InputFuncType &&f) noexcept(
      noexcept(nostd::invoke(::std::declval<InputFuncType>(),
                             ::std::declval<nostd::remove_cv_t<nostd::remove_pointer_t<ParamPointerType>>>()))) {
    nostd::invoke(std::forward<InputFuncType>(f), std::move(*p));
    return __void_result_constructor_tag{};
  }
};

template <class OriginValueType, class FuncType>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_map_invoke_utility_base<OriginValueType, FuncType, false, false> {
  using value_type = nostd::invoke_result_t<FuncType, nostd::add_rvalue_reference_t<OriginValueType>>;

  template <class ParamPointerType, class InputFuncType>
  static value_type invoke(ParamPointerType &&p, InputFuncType &&f) noexcept(
      noexcept(nostd::invoke(::std::declval<InputFuncType>(),
                             ::std::declval<nostd::remove_cv_t<nostd::remove_pointer_t<ParamPointerType>>>()))) {
    return nostd::invoke(std::forward<InputFuncType>(f), std::move(*p));
  }
};

template <class OriginValueType, class FuncType, bool InputVoid>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_map_invoke_utility_output;

template <class OriginValueType, class FuncType>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_map_invoke_utility_output<OriginValueType, FuncType, true>
    : public __result_map_invoke_utility_base<OriginValueType, FuncType, true,
                                              ::std::is_void<nostd::invoke_result_t<FuncType>>::value> {};

template <class OriginValueType, class FuncType>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_map_invoke_utility_output<OriginValueType, FuncType, false>
    : public __result_map_invoke_utility_base<
          OriginValueType, FuncType, false,
          ::std::is_void<nostd::invoke_result_t<FuncType, nostd::add_rvalue_reference_t<OriginValueType>>>::value> {};

template <class OriginValueType, class FuncType>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_map_invoke_utility
    : public __result_map_invoke_utility_output<OriginValueType, FuncType,
                                                ::std::is_void<nostd::remove_cv_t<OriginValueType>>::value> {};

template <class OriginValueType, class FuncType>
using __result_map_invoke_utility_t = typename __result_map_invoke_utility<OriginValueType, FuncType>::value_type;

template <class StorageValueType>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_storage_value_operations;

template <>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_storage_value_operations<void> {
  template <class StorageType>
  static inline void construct_copy(void *target_address, const typename StorageType::storage_type &) noexcept {
    StorageType::construct_storage(target_address);
  }
};

template <class StorageValueType>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY __result_storage_value_operations {
  template <class StorageType>
  static inline void construct_copy(void *target_address, const typename StorageType::storage_type &source) noexcept(
      noexcept(StorageType::construct_storage(nullptr, ::std::declval<typename StorageType::value_type>()))) {
    if (StorageType::unref(source)) {
      StorageType::construct_storage(target_address, *StorageType::unref(source));
    } else {
      StorageType::construct_storage(target_address);
    }
  }
};

template <class TOK, class TERR>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY result_base_type {
 public:
  using success_type = TOK;
  using error_type = TERR;
  using success_storage_type = typename default_compact_storage_type<success_type>::type;
  using error_storage_type = typename default_compact_storage_type<error_type>::type;
  using success_pointer = typename success_storage_type::pointer;
  using success_const_pointer = typename success_storage_type::const_pointer;
  using error_pointer = typename error_storage_type::pointer;
  using error_const_pointer = typename error_storage_type::const_pointer;

  enum class mode : int8_t {
    kSuccess = 0,
    kError = 1,
    kNone = 2,
  };

  UTIL_FORCEINLINE bool is_success() const noexcept { return mode_ == mode::kSuccess; }
  UTIL_FORCEINLINE bool is_error() const noexcept { return mode_ == mode::kError; }
  UTIL_FORCEINLINE bool is_none() const noexcept { return mode_ == mode::kNone; }

  /**
   * @brief Just like std::excepted<T, E>::has_value()
   */
  UTIL_FORCEINLINE bool has_value() const noexcept { return is_success(); }

  /**
   * @brief Just like std::excepted<T, E>::operator bool()
   */
  UTIL_FORCEINLINE explicit operator bool() const noexcept { return is_success(); }

  UTIL_FORCEINLINE nostd::nullable<success_const_pointer> get_success() const noexcept {
    return is_success() ? success_storage_type::unref(success_data_storage())
                        : success_storage_type::default_instance();
  }

  UTIL_FORCEINLINE nostd::nullable<success_pointer> get_success() noexcept {
    return is_success() ? success_storage_type::unref(success_data_storage())
                        : success_storage_type::default_instance();
  }

  UTIL_FORCEINLINE nostd::nullable<error_const_pointer> get_error() const noexcept {
    return is_error() ? error_storage_type::unref(error_data_storage()) : error_storage_type::default_instance();
  }

  UTIL_FORCEINLINE nostd::nullable<error_pointer> get_error() noexcept {
    return is_error() ? error_storage_type::unref(error_data_storage()) : error_storage_type::default_instance();
  }

  result_base_type() : mode_(mode::kNone) {}
  ~result_base_type() { reset(); }

  explicit result_base_type(result_base_type &&other) noexcept(std::is_nothrow_constructible<success_type>::value &&
                                                               std::is_nothrow_constructible<error_type>::value)
      : mode_(mode::kNone) {
    swap(other);
  }

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
        success_storage_type::swap(success_data_storage(), other.success_data_storage());
      } else if (mode::kError == mode_) {
        error_storage_type::swap(error_data_storage(), other.error_data_storage());
      }
    } else if (mode::kNone == mode_) {
      if (mode::kSuccess == other.mode_) {
        construct_move_success(std::move(other.success_data_storage()));
      } else if (mode::kError == other.mode_) {
        construct_move_error(std::move(other.error_data_storage()));
      }

      other.reset();
    } else if (mode::kNone == other.mode_) {
      if (mode::kSuccess == mode_) {
        other.construct_move_success(std::move(success_data_storage()));
      } else if (mode::kError == mode_) {
        other.construct_move_error(std::move(error_data_storage()));
      }

      reset();
    } else {
      if (mode::kSuccess == mode_) {
        // temporary data
        data_buffer_type t;
        success_storage_type::construct_move_storage(reinterpret_cast<void *>(&t), std::move(success_data_storage()));
        // self
        construct_move_error(std::move(other.error_data_storage()));
        // other
        other.construct_move_success(std::move(success_data_storage(t)));
        // destroy temporary data
        success_storage_type::destroy_storage(success_data_storage(t));
      } else {
        // temporary data
        data_buffer_type t;
        error_storage_type::construct_move_storage(reinterpret_cast<void *>(&t), std::move(error_data_storage()));
        // self
        construct_move_success(std::move(other.success_data_storage()));
        // other
        other.construct_move_error(std::move(error_data_storage(t)));
        // destroy temporary data
        error_storage_type::destroy_storage(error_data_storage(t));
      }
    }
  }

 private:
  template <class UOK, class UERR>
  friend class result_type;

  template <class... TARGS>
  inline void construct_success(TARGS &&...args) noexcept(
      noexcept(success_storage_type::construct_storage(nullptr, std::declval<TARGS>()...))) {
    reset();
    success_storage_type::construct_storage(success_data_arena(), std::forward<TARGS>(args)...);
    mode_ = mode::kSuccess;
  }

  template <class... TARGS>
  inline void construct_error(TARGS &&...args) noexcept(
      noexcept(error_storage_type::construct_storage(nullptr, std::declval<TARGS>()...))) {
    reset();
    error_storage_type::construct_storage(error_data_arena(), std::forward<TARGS>(args)...);
    mode_ = mode::kError;
  }

  inline void construct_copy_success(const typename success_storage_type::storage_type &other) noexcept(
      noexcept(__result_storage_value_operations<success_type>::template construct_copy<success_storage_type>(
          nullptr, ::std::declval<typename success_storage_type::storage_type>()))) {
    reset();
    __result_storage_value_operations<success_type>::template construct_copy<success_storage_type>(success_data_arena(),
                                                                                                   other);
    mode_ = mode::kSuccess;
  }

  inline void construct_move_success(typename success_storage_type::storage_type &&other) noexcept(noexcept(
      success_storage_type::construct_move_storage(nullptr,
                                                   ::std::declval<typename success_storage_type::storage_type>()))) {
    reset();
    success_storage_type::construct_move_storage(success_data_arena(), std::move(other));
    mode_ = mode::kSuccess;
  }

  inline void construct_move_error(typename error_storage_type::storage_type &&other) noexcept(noexcept(
      error_storage_type::construct_move_storage(nullptr,
                                                 ::std::declval<typename error_storage_type::storage_type>()))) {
    reset();
    error_storage_type::construct_move_storage(error_data_arena(), std::move(other));
    mode_ = mode::kError;
  }

  inline void construct_copy_error(const typename error_storage_type::storage_type &other) noexcept(
      noexcept(__result_storage_value_operations<error_type>::template construct_copy<error_storage_type>(
          nullptr, ::std::declval<typename error_storage_type::storage_type>()))) {
    reset();
    __result_storage_value_operations<error_type>::template construct_copy<error_storage_type>(error_data_arena(),
                                                                                               other);
    mode_ = mode::kError;
  }

  void reset() noexcept {
    if (mode::kSuccess == mode_) {
      success_storage_type::destroy_storage(success_data_storage());
    } else if (mode::kError == mode_) {
      error_storage_type::destroy_storage(error_data_storage());
    }

    mode_ = mode::kNone;
  }

 private:
  using data_buffer_type = typename nostd::aligned_union<0, typename success_storage_type::storage_type,
                                                         typename error_storage_type::storage_type>::type;

  static inline const typename success_storage_type::storage_type &success_data_storage(
      const data_buffer_type &data) noexcept {
    return *reinterpret_cast<const typename success_storage_type::storage_type *>(
        reinterpret_cast<const void *>(&data));
  }

  static inline typename success_storage_type::storage_type &success_data_storage(data_buffer_type &data) noexcept {
    return *reinterpret_cast<typename success_storage_type::storage_type *>(reinterpret_cast<void *>(&data));
  }

  inline const typename success_storage_type::storage_type &success_data_storage() const noexcept {
    return success_data_storage(data_);
  }

  inline typename success_storage_type::storage_type &success_data_storage() noexcept {
    return success_data_storage(data_);
  }

  static inline const typename error_storage_type::storage_type &error_data_storage(
      const data_buffer_type &data) noexcept {
    return *reinterpret_cast<const typename error_storage_type::storage_type *>(reinterpret_cast<const void *>(&data));
  }

  static inline typename error_storage_type::storage_type &error_data_storage(data_buffer_type &data) noexcept {
    return *reinterpret_cast<typename error_storage_type::storage_type *>(reinterpret_cast<void *>(&data));
  }

  inline const typename error_storage_type::storage_type &error_data_storage() const noexcept {
    return error_data_storage(data_);
  }

  inline typename error_storage_type::storage_type &error_data_storage() noexcept { return error_data_storage(data_); }

  inline void *success_data_arena() noexcept { return reinterpret_cast<void *>(&data_); }

  inline const void *success_data_arena() const noexcept { return reinterpret_cast<const void *>(&data_); }

  inline void *error_data_arena() noexcept { return reinterpret_cast<void *>(&data_); }

  inline const void *error_data_arena() const noexcept { return reinterpret_cast<const void *>(&data_); }
  data_buffer_type data_;
  mode mode_;
};

template <class TOK, class TERR>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY result_type : public result_base_type<TOK, TERR> {
 public:
  using base_type = result_base_type<TOK, TERR>;
  using self_type = result_type<TOK, TERR>;
  using success_type = typename base_type::success_type;
  using error_type = typename base_type::error_type;
  using success_pointer = typename base_type::success_pointer;
  using success_const_pointer = typename base_type::success_const_pointer;
  using error_const_pointer = typename base_type::error_const_pointer;
  using error_pointer = typename base_type::error_pointer;

  using success_storage_type = typename base_type::success_storage_type;
  using error_storage_type = typename base_type::error_storage_type;

 private:
  using base_type::construct_copy_error;
  using base_type::construct_copy_success;
  using base_type::construct_error;
  using base_type::construct_move_error;
  using base_type::construct_move_success;
  using base_type::construct_success;
  using base_type::error_data_storage;
  using base_type::success_data_storage;

  template <class UOK, class UERR>
  friend class result_type;

 public:
  using base_type::has_value;
  using base_type::is_error;
  using base_type::is_none;
  using base_type::is_success;
  using base_type::operator bool;
  using base_type::get_error;
  using base_type::get_success;
  using base_type::swap;

  template <class... TARGS>
  static inline self_type make_success(TARGS &&...args) noexcept(
      noexcept(success_storage_type::construct_storage(nullptr, std::declval<TARGS>()...))) {
    self_type ret;
    ret.construct_success(std::forward<TARGS>(args)...);
    return ret;
  }

  template <class... TARGS>
  static inline self_type make_error(TARGS &&...args) noexcept(
      noexcept(error_storage_type::construct_storage(nullptr, std::declval<TARGS>()...))) {
    self_type ret;
    ret.construct_error(std::forward<TARGS>(args)...);
    return ret;
  }

  /**
   * @brief Just like std::excepted<T, E>::transform(), but returns a new result_type
   * @note If the current result is an error, the function will not be called and the error result will be returned
   * directly.
   * @note If the current result is a success, the function will be called and the result will be returned.
   * @note Just like std::excepted<T, E>::transform
   * @see https://en.cppreference.com/w/cpp/utility/expected/transform
   * @param f The function to call when this is a successful result
   * @return The new result contains the result of the function as success result or the error
   */
  template <class FuncType>
  inline result_type<__result_map_invoke_utility_t<success_type, nostd::remove_cvref_t<FuncType>>, error_type>
  transform(FuncType &&f) const & noexcept(
      noexcept(__result_map_invoke_utility<success_type, nostd::remove_cvref_t<FuncType>>::invoke(
          std::declval<success_const_pointer>(), std::forward<FuncType>(f)))) {
    using utility_type = __result_map_invoke_utility<success_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_success_type = __result_map_invoke_utility_t<success_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_result_type = result_type<rebind_success_type, error_type>;

    if (is_success()) {
      assert(::std::is_void<nostd::remove_cvref_t<success_type>>::value || get_success());
      return rebind_result_type::make_success(utility_type::invoke(get_success(), std::forward<FuncType>(f)));
    } else {
      assert(::std::is_void<nostd::remove_cvref_t<error_type>>::value || get_error());
      rebind_result_type ret;
      ret.construct_copy_error(error_data_storage());
      return ret;
    }
  }

  /**
   * @brief Just like std::excepted<T, E>::transform(), but returns a new result_type
   * @note If the current result is an error, the function will not be called and the error result will be returned
   * directly.
   * @note If the current result is a success, the function will be called and the result will be returned.
   * @note Just like std::excepted<T, E>::transform
   * @see https://en.cppreference.com/w/cpp/utility/expected/transform
   * @param f The function to call when this is a successful result
   * @return The new result contains the result of the function as success result or the error
   */
  template <class FuncType>
  inline result_type<__result_map_invoke_utility_t<success_type, nostd::remove_cvref_t<FuncType>>, error_type>
  transform(FuncType &&f) && noexcept(
      noexcept(__result_map_invoke_utility<success_type, nostd::remove_cvref_t<FuncType>>::invoke(
          std::declval<success_pointer>(), std::forward<FuncType>(f)))) {
    using utility_type = __result_map_invoke_utility<success_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_success_type = __result_map_invoke_utility_t<success_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_result_type = result_type<rebind_success_type, error_type>;

    if (is_success()) {
      assert(::std::is_void<nostd::remove_cvref_t<success_type>>::value || get_success());
      return rebind_result_type::make_success(utility_type::invoke(get_success(), std::forward<FuncType>(f)));
    } else {
      assert(::std::is_void<nostd::remove_cvref_t<error_type>>::value || get_error());
      rebind_result_type ret;
      ret.construct_move_error(std::move(error_data_storage()));
      base_type::reset();
      return ret;
    }
  }

  /**
   * @brief Just like std::excepted<T, E>::transform(), but returns a new result_type
   * @note If the current result is an error, the function will not be called and the error result will be returned
   * directly.
   * @note If the current result is a success, the function will be called and the result will be returned.
   * @note Just like std::excepted<T, E>::and_then
   * @see https://en.cppreference.com/w/cpp/utility/expected/and_then
   * @param f The function to call when this is a successful result
   * @return The new result contains the result of the function as success result or the error
   */
  template <class FuncType>
  inline __result_map_invoke_utility_t<success_type, nostd::remove_cvref_t<FuncType>> and_then(FuncType &&f)
      const & noexcept(noexcept(__result_map_invoke_utility<success_type, nostd::remove_cvref_t<FuncType>>::invoke(
          std::declval<success_const_pointer>(), std::forward<FuncType>(f)))) {
    using utility_type = __result_map_invoke_utility<success_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_result_type = __result_map_invoke_utility_t<success_type, nostd::remove_cvref_t<FuncType>>;

    if (is_success()) {
      assert(::std::is_void<nostd::remove_cvref_t<success_type>>::value || get_success());
      return utility_type::invoke(get_success(), std::forward<FuncType>(f));
    } else {
      assert(::std::is_void<nostd::remove_cvref_t<error_type>>::value || get_error());
      rebind_result_type ret;
      ret.construct_copy_error(error_data_storage());
      return ret;
    }
  }

  /**
   * @brief Just like std::excepted<T, E>::transform(), but returns a new result_type
   * @note If the current result is an error, the function will not be called and the error result will be returned
   * directly.
   * @note If the current result is a success, the function will be called and the result will be returned.
   * @note Just like std::excepted<T, E>::and_then
   * @see https://en.cppreference.com/w/cpp/utility/expected/and_then
   * @param f The function to call when this is a successful result
   * @return The new result contains the result of the function as success result or the error
   */
  template <class FuncType>
  inline __result_map_invoke_utility_t<success_type, nostd::remove_cvref_t<FuncType>> and_then(FuncType &&f)
      && noexcept(noexcept(__result_map_invoke_utility<success_type, nostd::remove_cvref_t<FuncType>>::invoke(
          std::declval<success_pointer>(), std::forward<FuncType>(f)))) {
    using utility_type = __result_map_invoke_utility<success_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_result_type = __result_map_invoke_utility_t<success_type, nostd::remove_cvref_t<FuncType>>;

    if (is_success()) {
      assert(::std::is_void<nostd::remove_cvref_t<success_type>>::value || get_success());
      return utility_type::invoke(get_success(), std::forward<FuncType>(f));
    } else {
      assert(::std::is_void<nostd::remove_cvref_t<error_type>>::value || get_error());
      rebind_result_type ret;
      ret.construct_move_error(std::move(error_data_storage()));
      base_type::reset();
      return ret;
    }
  }

  /**
   * @brief Just like std::excepted<T, E>::transform_error(), but returns a new result_type
   * @note If the current result is an success, the function will not be called and the success result will be returned
   * directly.
   * @note If the current result is a error, the function will be called and the result will be returned.
   * @note Just like std::excepted<T, E>::transform_error
   * @see https://en.cppreference.com/w/cpp/utility/expected/transform_error
   * @param f The function to call when this is a successful result
   * @return The new result contains the result of the function as error result or the success
   */
  template <class FuncType>
  inline result_type<success_type, __result_map_invoke_utility_t<error_type, nostd::remove_cvref_t<FuncType>>>
  transform_error(FuncType &&f) const & noexcept(
      noexcept(__result_map_invoke_utility<error_type, nostd::remove_cvref_t<FuncType>>::invoke(
          std::declval<error_const_pointer>(), std::forward<FuncType>(f)))) {
    using utility_type = __result_map_invoke_utility<error_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_error_type = __result_map_invoke_utility_t<error_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_result_type = result_type<success_type, rebind_error_type>;

    if (is_success()) {
      assert(::std::is_void<nostd::remove_cvref_t<success_type>>::value || get_success());
      rebind_result_type ret;
      ret.construct_copy_success(success_data_storage());
      return ret;
    } else {
      assert(::std::is_void<nostd::remove_cvref_t<error_type>>::value || get_error());
      return rebind_result_type::make_error(utility_type::invoke(get_error(), std::forward<FuncType>(f)));
    }
  }

  /**
   * @brief Just like std::excepted<T, E>::transform_error(), but returns a new result_type
   * @note If the current result is an success, the function will not be called and the success result will be returned
   * directly.
   * @note If the current result is a error, the function will be called and the result will be returned.
   * @note Just like std::excepted<T, E>::transform_error
   * @see https://en.cppreference.com/w/cpp/utility/expected/transform_error
   * @param f The function to call when this is a successful result
   * @return The new result contains the result of the function as error result or the success
   */
  template <class FuncType>
  inline result_type<success_type, __result_map_invoke_utility_t<error_type, nostd::remove_cvref_t<FuncType>>>
  transform_error(FuncType &&f) && noexcept(noexcept(
      __result_map_invoke_utility<error_type, nostd::remove_cvref_t<FuncType>>::invoke(std::declval<error_pointer>(),
                                                                                       std::forward<FuncType>(f)))) {
    using utility_type = __result_map_invoke_utility<error_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_error_type = __result_map_invoke_utility_t<error_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_result_type = result_type<success_type, rebind_error_type>;

    if (is_success()) {
      assert(::std::is_void<nostd::remove_cvref_t<success_type>>::value || get_success());
      rebind_result_type ret;
      ret.construct_move_success(std::move(success_data_storage()));
      base_type::reset();
      return ret;
    } else {
      assert(::std::is_void<nostd::remove_cvref_t<error_type>>::value || get_error());
      return rebind_result_type::make_error(utility_type::invoke(get_error(), std::forward<FuncType>(f)));
    }
  }

  /**
   * @brief Just like std::excepted<T, E>::or_else(), but returns a new result_type
   * @note If the current result is an success, the function will not be called and the success result will be returned
   * directly.
   * @note If the current result is a error, the function will be called and the result will be returned.
   * @note Just like std::excepted<T, E>::or_else
   * @see https://en.cppreference.com/w/cpp/utility/expected/or_else
   * @param f The function to call when this is a successful result
   * @return The new result contains the result of the function as error result or the success
   */
  template <class FuncType>
  inline __result_map_invoke_utility_t<error_type, nostd::remove_cvref_t<FuncType>> or_else(FuncType &&f)
      const & noexcept(noexcept(__result_map_invoke_utility<error_type, nostd::remove_cvref_t<FuncType>>::invoke(
          std::declval<error_const_pointer>(), std::forward<FuncType>(f)))) {
    using utility_type = __result_map_invoke_utility<error_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_result_type = __result_map_invoke_utility_t<error_type, nostd::remove_cvref_t<FuncType>>;

    if (is_success()) {
      assert(::std::is_void<nostd::remove_cvref_t<success_type>>::value || get_success());
      rebind_result_type ret;
      ret.construct_copy_success(success_data_storage());
      return ret;
    } else {
      assert(::std::is_void<nostd::remove_cvref_t<error_type>>::value || get_error());
      return utility_type::invoke(get_error(), std::forward<FuncType>(f));
    }
  }

  /**
   * @brief Just like std::excepted<T, E>::or_else(), but returns a new result_type
   * @note If the current result is an success, the function will not be called and the success result will be returned
   * directly.
   * @note If the current result is a error, the function will be called and the result will be returned.
   * @note Just like std::excepted<T, E>::or_else
   * @see https://en.cppreference.com/w/cpp/utility/expected/or_else
   * @param f The function to call when this is a successful result
   * @return The new result contains the result of the function as error result or the success
   */
  template <class FuncType>
  inline __result_map_invoke_utility_t<error_type, nostd::remove_cvref_t<FuncType>> or_else(FuncType &&f) && noexcept(
      noexcept(__result_map_invoke_utility<error_type, nostd::remove_cvref_t<FuncType>>::invoke(
          std::declval<error_pointer>(), std::forward<FuncType>(f)))) {
    using utility_type = __result_map_invoke_utility<error_type, nostd::remove_cvref_t<FuncType>>;
    using rebind_result_type = __result_map_invoke_utility_t<error_type, nostd::remove_cvref_t<FuncType>>;

    if (is_success()) {
      assert(::std::is_void<nostd::remove_cvref_t<success_type>>::value || get_success());
      rebind_result_type ret;
      ret.construct_move_success(std::move(success_data_storage()));
      base_type::reset();
      return ret;
    } else {
      assert(::std::is_void<nostd::remove_cvref_t<error_type>>::value || get_error());
      return utility_type::invoke(get_error(), std::forward<FuncType>(f));
    }
  }
};
}  // namespace design_pattern
ATFRAMEWORK_UTILS_NAMESPACE_END

#define ATFW_UTIL_MACRO_DECLARE_INPLACE_RESULT_STORAGE(__CUSTOM_TYPE)                    \
  ATFRAMEWORK_UTILS_NAMESPACE_BEGIN                                                      \
  namespace design_pattern {                                                             \
  template <>                                                                            \
  struct default_compact_storage_type<__CUSTOM_TYPE> {                                   \
    using type = compact_storage_type<__inplace_result_storage_delegate<__CUSTOM_TYPE>>; \
  };                                                                                     \
  }                                                                                      \
  ATFRAMEWORK_UTILS_NAMESPACE_END
