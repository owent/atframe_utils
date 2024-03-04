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
  template <class Y, class... ArgsT>
  static inline memory::strong_rc_ptr<Y> make_strong(ArgsT&&... args) {
    return memory::make_strong_rc<Y>(std::forward<ArgsT>(args)...);
  }

  template <class Y, class F>
  static inline memory::strong_rc_ptr<Y> static_pointer_cast(F&& f) {
    return memory::static_pointer_cast<Y>(std::forward<F>(f));
  }

  template <class Y, class F>
  static inline memory::strong_rc_ptr<Y> const_pointer_cast(F&& f) {
    return memory::const_pointer_cast<Y>(std::forward<F>(f));
  }
};

template <>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_mt_mode_func_trait<wal_mt_mode::kMultiThread> {
  template <class Y, class... ArgsT>
  static inline std::shared_ptr<Y> make_strong(ArgsT&&... args) {
    return std::make_shared<Y>(std::forward<ArgsT>(args)...);
  }

  template <class Y, class F>
  static inline std::shared_ptr<Y> static_pointer_cast(F&& f) {
    return std::static_pointer_cast<Y>(std::forward<F>(f));
  }

  template <class Y, class F>
  static inline std::shared_ptr<Y> const_pointer_cast(F&& f) {
    return std::const_pointer_cast<Y>(std::forward<F>(f));
  }
};

template <class LogKeyT, class LogT, class ActionGetter, class CompareLogKeyT = std::less<LogKeyT>,
          class HashActionCaseT = std::hash<typename wal_log_action_getter_trait<LogT, ActionGetter>::type>,
          class EqualActionCaseT = std::equal_to<typename wal_log_action_getter_trait<LogT, ActionGetter>::type>,
          wal_mt_mode MTMode = wal_mt_mode::kMultiThread>
class LIBATFRAME_UTILS_API_HEAD_ONLY wal_log_operator {
 public:
  using log_key_type = LogKeyT;
  using log_type = LogT;
  using action_getter_trait = wal_log_action_getter_trait<LogT, ActionGetter>;
  using action_getter_type = ActionGetter;
  using action_case_type = typename action_getter_trait::type;
  using log_key_compare_type = CompareLogKeyT;
  using action_case_hash = HashActionCaseT;
  using action_case_equal = EqualActionCaseT;

  static UTIL_CONFIG_CONSTEXPR const wal_mt_mode mt_mode = MTMode;

  using log_pointer = typename wal_mt_mode_data_trait<log_type, mt_mode>::strong_ptr;
  using log_const_pointer = typename wal_mt_mode_data_trait<const log_type, mt_mode>::strong_ptr;
  using log_key_result_type = LIBATFRAME_UTILS_NAMESPACE_ID::design_pattern::result_type<log_key_type, wal_result_code>;

  template <class Y, class... ArgsT>
  static inline typename wal_mt_mode_data_trait<Y, mt_mode>::strong_ptr make_strong(ArgsT&&... args) {
    return wal_mt_mode_func_trait<mt_mode>::template make_strong<Y>(std::forward<ArgsT>(args)...);
  }

  template <class Y, class F>
  static inline typename wal_mt_mode_data_trait<Y, mt_mode>::strong_ptr static_pointer_cast(F&& f) {
    return wal_mt_mode_func_trait<mt_mode>::template static_pointer_cast<Y>(std::forward<F>(f));
  }

  template <class Y, class F>
  static inline typename wal_mt_mode_data_trait<Y, mt_mode>::strong_ptr const_pointer_cast(F&& f) {
    return wal_mt_mode_func_trait<mt_mode>::template const_pointer_cast<Y>(std::forward<F>(f));
  }
};

template <class LogKeyT, class LogT, class ActionGetter, wal_mt_mode MTMode, class CompareLogKeyT = std::less<LogKeyT>,
          class HashActionCaseT = std::hash<typename wal_log_action_getter_trait<LogT, ActionGetter>::type>,
          class EqualActionCaseT = std::equal_to<typename wal_log_action_getter_trait<LogT, ActionGetter>::type>>
using wal_log_operator_with_mt_mode =
    wal_log_operator<LogKeyT, LogT, ActionGetter, CompareLogKeyT, HashActionCaseT, EqualActionCaseT, MTMode>;

}  // namespace distributed_system
LIBATFRAME_UTILS_NAMESPACE_END
