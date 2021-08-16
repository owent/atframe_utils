// Copyright 2021 Tencent
// Created by owentou on 2021-08-12
// Common definitions for Write Ahead Log

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <design_pattern/result_type.h>

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

namespace util {
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

template <class LogT, class ActionGetter>
struct LIBATFRAME_UTILS_API_HEAD_ONLY wal_log_action_getter_trait {
#if defined(__cplusplus) && __cplusplus >= 201703L
  using type = std::invoke_result_t<ActionGetter, LogT>;
#else
  using type = typename std::result_of<ActionGetter(LogT)>::type;
#endif
};

template <class LogKeyT, class LogT, class ActionGetter, class CompareLogKeyT = std::less<LogKeyT>,
          class HashActionCaseT = std::hash<typename wal_log_action_getter_trait<LogT, ActionGetter>::type>,
          class EqualActionCaseT = std::equal_to<typename wal_log_action_getter_trait<LogT, ActionGetter>::type> >
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

  using log_pointer = std::shared_ptr<log_type>;
  using log_key_result_type = util::design_pattern::result_type<log_key_type, wal_result_code>;
};

}  // namespace distributed_system
}  // namespace util
