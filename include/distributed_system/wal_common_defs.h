// Copyright 2021 atframework
// Created by owent on 2021-08-12
// Common definitions for Write Ahead Log

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compiler_features.h>

#include <design_pattern/result_type.h>
#include <memory/rc_ptr.h>

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace distributed_system {

using wal_time_point = std::chrono::system_clock::time_point;
using wal_duration = std::chrono::system_clock::duration;

template <class LogKeyT, class ActionCaseT>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_meta_type {
  wal_time_point timepoint;
  LogKeyT log_key;
  ActionCaseT action_case;

  inline wal_meta_type() = default;
  template <class ToTimepointT, class ToLogKeyT, class ToActionCaseT>
  inline wal_meta_type(ToTimepointT&& t, ToLogKeyT&& k, ToActionCaseT&& act)
      : timepoint(std::forward<ToTimepointT>(t)),
        log_key(std::forward<ToLogKeyT>(k)),
        action_case(std::forward<ToActionCaseT>(act)) {}
};

enum class wal_result_code : int32_t {
  kSubscriberNotFound = -201,

  kHashCodeMismatch = -106,
  kInitlization = -105,
  kCallbackError = -104,
  kInvalidParam = -103,
  kBadLogKey = -102,
  kActionNotSet = -101,

  kOk = 0,

  kIgnore = 101,
  kPending = 102,
  kMerge = 103,
};

enum class wal_unsubscribe_reason : int32_t {
  kNone = 0,
  kTimeout = 1,
  kClientRequest = 2,
  kInvalid = 3,
};

enum class wal_mt_mode : int8_t {
  kSingleThread = 0,
  kMultiThread = 1,
};

template <class LogT, class ActionGetter>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_log_action_getter_trait {
#if defined(__cplusplus) && __cplusplus >= 201703L
  using type = std::invoke_result_t<ActionGetter, LogT>;
#else
  using type = typename std::result_of<ActionGetter(LogT)>::type;
#endif
};

template <class T, wal_mt_mode MTMode>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_mt_mode_data_trait;

template <wal_mt_mode MTMode>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_mt_mode_func_trait;

template <class T>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_mt_mode_data_trait<T, wal_mt_mode::kSingleThread> {
  using strong_ptr = memory::strong_rc_ptr<T>;
  using weak_ptr = memory::weak_rc_ptr<T>;
};

template <class T>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_mt_mode_data_trait<T, wal_mt_mode::kMultiThread> {
  using strong_ptr = std::shared_ptr<T>;
  using weak_ptr = std::weak_ptr<T>;
};

template <>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_mt_mode_func_trait<wal_mt_mode::kSingleThread> {
  template <class Y>
  using enable_shared_from_this = memory::enable_shared_rc_from_this<Y>;

  template <class Y, class Alloc, class... ArgsT>
  static inline memory::strong_rc_ptr<Y> allocate_strong(const Alloc& alloc, ArgsT&&... args) {
    return memory::allocate_strong_rc<Y>(alloc, std::forward<ArgsT>(args)...);
  }

  template <class Y, class F>
  static inline memory::strong_rc_ptr<Y> static_pointer_cast(F&& f) {
    return memory::static_pointer_cast<Y>(std::forward<F>(f));
  }

  template <class Y, class F>
  static inline memory::strong_rc_ptr<Y> const_pointer_cast(F&& f) {
    return memory::const_pointer_cast<Y>(std::forward<F>(f));
  }

#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
  template <class Y, class F>
  static inline memory::strong_rc_ptr<Y> dynamic_pointer_cast(F&& f) {
    return memory::dynamic_pointer_cast<Y>(std::forward<F>(f));
  }
#endif
};

template <>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_mt_mode_func_trait<wal_mt_mode::kMultiThread> {
  template <class Y>
  using enable_shared_from_this = std::enable_shared_from_this<Y>;

  template <class Y, class Alloc, class... ArgsT>
  static inline std::shared_ptr<Y> allocate_strong(const Alloc& alloc, ArgsT&&... args) {
#include "config/compiler/internal/stl_compact_prefix.h.inc"  // NOLINT: build/include
    return std::allocate_shared<Y>(alloc, std::forward<ArgsT>(args)...);
#include "config/compiler/internal/stl_compact_suffix.h.inc"  // NOLINT: build/include
  }

  template <class Y, class F>
  static inline std::shared_ptr<Y> static_pointer_cast(F&& f) {
    return std::static_pointer_cast<Y>(std::forward<F>(f));
  }

  template <class Y, class F>
  static inline std::shared_ptr<Y> const_pointer_cast(F&& f) {
    return std::const_pointer_cast<Y>(std::forward<F>(f));
  }

#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
  template <class Y, class F>
  static inline std::shared_ptr<Y> dynamic_pointer_cast(F&& f) {
    return std::dynamic_pointer_cast<Y>(std::forward<F>(f));
  }
#endif
};

template <class /*LogKeyT*/, class /*LogT*/>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_log_hash_code_traits {
  using hash_code_type = size_t;

  static inline hash_code_type initial_hash_code() noexcept { return 0; }
  static inline hash_code_type validate(const hash_code_type& hash_code) noexcept { return 0 != hash_code; }
  static inline hash_code_type equal(const hash_code_type& l, const hash_code_type& r) noexcept { return l == r; }
};

template <class LogKeyT, class LogT, class ActionGetter, class CompareLogKeyT = std::less<LogKeyT>,
          class HashActionCaseT = std::hash<typename wal_log_action_getter_trait<LogT, ActionGetter>::type>,
          class EqualActionCaseT = std::equal_to<typename wal_log_action_getter_trait<LogT, ActionGetter>::type>,
          class Allocator = std::allocator<LogT>, wal_mt_mode MTMode = wal_mt_mode::kMultiThread>
class LIBATFRAME_UTILS_API_HEAD_ONLY wal_log_operator {
 public:
  using log_key_type = LogKeyT;
  using log_type = LogT;
  using log_allocator = Allocator;
  using action_getter_trait = wal_log_action_getter_trait<LogT, ActionGetter>;
  using action_getter_type = ActionGetter;
  using action_case_type = typename action_getter_trait::type;
  using log_key_compare_type = CompareLogKeyT;
  using action_case_hash = HashActionCaseT;
  using action_case_equal = EqualActionCaseT;

  static UTIL_CONFIG_CONSTEXPR const wal_mt_mode mt_mode = MTMode;

  using log_pointer = typename wal_mt_mode_data_trait<log_type, mt_mode>::strong_ptr;
  using log_const_pointer = typename wal_mt_mode_data_trait<const log_type, mt_mode>::strong_ptr;
  using log_pointer_allocator = typename std::allocator_traits<log_allocator>::template rebind_alloc<log_pointer>;
  using log_key_result_type = LIBATFRAME_UTILS_NAMESPACE_ID::design_pattern::result_type<log_key_type, wal_result_code>;

  template <class Y, class... ArgsT>
  static inline typename wal_mt_mode_data_trait<Y, mt_mode>::strong_ptr make_strong(ArgsT&&... args) {
#include "config/compiler/internal/stl_compact_prefix.h.inc"  // NOLINT: build/include
    using alloc_type = typename std::allocator_traits<log_allocator>::template rebind_alloc<Y>;
    return wal_mt_mode_func_trait<mt_mode>::template allocate_strong<Y>(alloc_type(), std::forward<ArgsT>(args)...);
#include "config/compiler/internal/stl_compact_suffix.h.inc"  // NOLINT: build/include
  }

  template <class Y, class Alloc, class... ArgsT>
  static inline typename wal_mt_mode_data_trait<Y, mt_mode>::strong_ptr allocate_strong(const Alloc& alloc,
                                                                                        ArgsT&&... args) {
#include "config/compiler/internal/stl_compact_prefix.h.inc"  // NOLINT: build/include
    return wal_mt_mode_func_trait<mt_mode>::template allocate_strong<Y>(alloc, std::forward<ArgsT>(args)...);
#include "config/compiler/internal/stl_compact_suffix.h.inc"  // NOLINT: build/include
  }

  template <class Y, class F>
  static inline typename wal_mt_mode_data_trait<Y, mt_mode>::strong_ptr static_pointer_cast(F&& f) {
    return wal_mt_mode_func_trait<mt_mode>::template static_pointer_cast<Y>(std::forward<F>(f));
  }

  template <class Y, class F>
  static inline typename wal_mt_mode_data_trait<Y, mt_mode>::strong_ptr const_pointer_cast(F&& f) {
    return wal_mt_mode_func_trait<mt_mode>::template const_pointer_cast<Y>(std::forward<F>(f));
  }

#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
  template <class Y, class F>
  static inline typename wal_mt_mode_data_trait<Y, mt_mode>::strong_ptr dynamic_pointer_cast(F&& f) {
    return wal_mt_mode_func_trait<mt_mode>::template dynamic_pointer_cast<Y>(std::forward<F>(f));
  }
#endif

  template <class Y>
  using strong_ptr = typename wal_mt_mode_data_trait<Y, mt_mode>::strong_ptr;
  template <class Y>
  using weak_ptr = typename wal_mt_mode_data_trait<Y, mt_mode>::weak_ptr;
  template <class Y>
  using enable_shared_from_this = typename wal_mt_mode_func_trait<mt_mode>::template enable_shared_from_this<Y>;
};

template <class LogKeyT, class LogT, class ActionGetter, wal_mt_mode MTMode, class CompareLogKeyT = std::less<LogKeyT>,
          class HashActionCaseT = std::hash<typename wal_log_action_getter_trait<LogT, ActionGetter>::type>,
          class EqualActionCaseT = std::equal_to<typename wal_log_action_getter_trait<LogT, ActionGetter>::type>,
          class Allocator = std::allocator<LogT>>
using wal_log_operator_with_mt_mode =
    wal_log_operator<LogKeyT, LogT, ActionGetter, CompareLogKeyT, HashActionCaseT, EqualActionCaseT, Allocator, MTMode>;

}  // namespace distributed_system
LIBATFRAME_UTILS_NAMESPACE_END
