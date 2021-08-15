// Copyright 2021 Tencent
// Created by owentou
// Stanards operations for Write Ahead Log publisher

#pragma once

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <unordered_map>
#include <utility>

#include "distributed_system/wal_common_defs.h"
#include "distributed_system/wal_object.h"
#include "distributed_system/wal_subscriber.h"

namespace util {
namespace distributed_system {

template <class StorageT, class LogOperatorT, class CallbackParamT, class PrivateDataT, class WalSubscriber>
class LIBATFRAME_UTILS_API_HEAD_ONLY wal_publisher {
 public:
  using log_operator_type = LogOperatorT;
  using object_type = wal_object<StorageT, LogOperatorT, CallbackParamT, PrivateDataT>;
  using subscriber_type = WalSubscriber;

  using storage_type = typename object_type::storage_type;
  using log_type = typename log_operator_type::log_type;
  using log_pointer = typename log_operator_type::log_pointer;
  using log_key_type = typename log_operator_type::log_key_type;
  using log_key_compare_type = typename log_operator_type::log_key_compare_type;
  using action_getter_type = typename log_operator_type::action_getter_type;
  using action_case_type = typename log_operator_type::action_case_type;
  using log_key_result_type = typename log_operator_type::log_key_result_type;

  using log_iterator = typename object_type::log_iterator;
  using log_const_iterator = typename object_type::log_const_iterator;
  using callback_param_type = typename object_type::callback_param_type;
  using time_point = typename object_type::time_point;
  using duration = typename object_type::duration;
  using meta_type = typename object_type::meta_type;
  using meta_result_type = typename object_type::meta_result_type;

  using private_data_type = typename object_type::private_data_type;

  using subscriber_key_type = typename subscriber_type::key_type;
  using subscriber_iterator = typename subscriber_type::subscriber_iterator;
  using subscriber_const_iterator = typename subscriber_type::subscriber_const_iterator;
  using subscriber_pointer = typename subscriber_type::pointer;
  using subscriber_private_data_type = typename subscriber_type::private_data_type;
  using subscriber_manager_type = typename subscriber_type::manager;

  using callback_load_fn_t = typename object_type::callback_load_fn_t;
  using callback_dump_fn_t = typename object_type::callback_dump_fn_t;
  using callback_log_action_fn_t = typename object_type::callback_log_action_fn_t;
  using callback_log_event_fn_t = typename object_type::callback_log_event_fn_t;
  using callback_log_merge_fn_t = typename object_type::callback_log_merge_fn_t;
  using callback_log_get_meta_fn_t = typename object_type::callback_log_get_meta_fn_t;
  using callback_log_set_meta_fn_t = typename object_type::callback_log_set_meta_fn_t;
  using callback_get_log_key_fn_t = typename object_type::callback_get_log_key_fn_t;
  using callback_alloc_log_key_fn_t = typename object_type::callback_alloc_log_key_fn_t;

  // Send snapshot to a subscriber clients
  using callback_send_snapshot_fn_t =
      std::function<wal_result_code(wal_publisher&, subscriber_iterator, subscriber_iterator, callback_param_type)>;

  // Send logs to a subscriber clients
  using callback_send_logs_fn_t =
      std::function<wal_result_code(wal_publisher&, log_const_iterator, log_const_iterator, subscriber_iterator,
                                    subscriber_iterator, callback_param_type)>;

  // Send subscribe response
  using callback_send_subscribe_response_fn_t =
      std::function<wal_result_code(wal_publisher&, const subscriber_pointer&, wal_result_code, callback_param_type)>;

  // Send subscribe response
  using callback_check_subscriber_fn_t =
      std::function<bool(wal_publisher&, const subscriber_pointer&, callback_param_type)>;

  // On subscriber added
  using callback_on_subscriber_added_fn_t =
      std::function<void(wal_publisher&, const subscriber_pointer&, callback_param_type)>;

  // On subscriber removed
  using callback_on_subscriber_removed_fn_t =
      std::function<void(wal_publisher&, const subscriber_pointer&, wal_unsubscribe_reason, callback_param_type)>;

  struct vtable_type : public object_type::vtable_type {
    callback_send_snapshot_fn_t send_snapshot;
    callback_send_logs_fn_t send_logs;
    callback_send_subscribe_response_fn_t subscribe_response;

    callback_check_subscriber_fn_t check_subscriber;
    callback_on_subscriber_added_fn_t on_subscriber_added;
    callback_on_subscriber_removed_fn_t on_subscriber_removed;
  };
  using vtable_pointer = std::shared_ptr<vtable_type>;

  struct congfigure_type : public object_type::congfigure_type {
    duration subscriber_timeout;
  };
  using congfigure_pointer = std::shared_ptr<congfigure_type>;

 private:
  UTIL_DESIGN_PATTERN_NOMOVABLE(wal_publisher);
  UTIL_DESIGN_PATTERN_NOCOPYABLE(wal_publisher);
  struct construct_helper {
    vtable_pointer vt;
    congfigure_pointer conf;
    std::shared_ptr<object_type> wal_object;
    std::shared_ptr<subscriber_manager_type> subscriber_manager;
  };

 public:
  template <class... ArgsT>
  explicit wal_publisher(construct_helper& helper, ArgsT&&... args)
      : vtable_(helper.vt),
        configure_(helper.conf),
        wal_object_(helper.wal_object),
        subscriber_manager_(helper.subscriber_manager) {}

  template <class... ArgsT>
  static std::shared_ptr<wal_publisher> create(vtable_pointer vt, congfigure_pointer conf, ArgsT&&... args) {
    if (!vt || !conf) {
      return nullptr;
    }

    if (!vt->get_meta || !vt->get_log_key || !vt->alloc_log_key) {
      return nullptr;
    }

    if (!vt->send_snapshot || !vt->send_logs) {
      return nullptr;
    }

    construct_helper helper;
    helper.vt = vt;
    helper.conf = conf;
    helper.wal_object = object_type::create(
        std::static_pointer_cast<typename object_type::vtable_type>(helper.vt),
        std::static_pointer_cast<typename object_type::congfigure_type>(helper.conf), std::forward<ArgsT>(args)...);
    helper.subscriber_manager = std::make_shared<subscriber_manager_type>();
    if (!helper.wal_object || !helper.subscriber_manager) {
      return nullptr;
    }

    return std::make_shared<wal_publisher>(helper, std::forward<ArgsT>(args)...);
  }

  static congfigure_pointer make_configure() {
    congfigure_pointer ret = std::make_shared<congfigure_type>();
    if (!ret) {
      return ret;
    }

    object_type::default_configure(*ret);
    ret->subscriber_timeout = std::chrono::duration_cast<duration>(std::chrono::minutes{10});
    return ret;
  }

  wal_result_code load(const storage_type& storage, callback_param_type param) {
    if (!wal_object_) {
      return wal_result_code::kInitlization;
    }

    return wal_object_->load(storage, param);
  }

  wal_result_code dump(storage_type& storage, callback_param_type param) {
    if (!wal_object_) {
      return wal_result_code::kInitlization;
    }

    return wal_object_->dump(storage, param);
  }

  template <class... ArgsT>
  log_pointer allocate_log(time_point now, action_case_type action_case, callback_param_type param, ArgsT&&... args) {
    if (!wal_object_) {
      return nullptr;
    }

    return wal_object_->allocate_log(now, action_case, param, std::forward<ArgsT>(args));
  }

  wal_result_code emplace_back_log(log_pointer&& log, callback_param_type param) {
    if (!wal_object_) {
      return wal_result_code::kInitlization;
    }

    return wal_object_->emplace_back(std::move(log), param);
  }

  wal_result_code push_back_log(log_pointer log, callback_param_type param) {
    if (!wal_object_) {
      return wal_result_code::kInitlization;
    }

    return wal_object_->emplace_back(std::move(log), param);
  }

  const std::unique_ptr<log_key_type>& get_global_log_ingore_key() const noexcept {
    if (!wal_object_) {
      return nullptr;
    }
    return wal_object_->get_global_ingore_key();
  }

  template <class ToKey>
  void set_global_log_ingore_key(ToKey&& key) {
    if (!wal_object_) {
      return;
    }

    wal_object_->set_global_ingore_key(std::forward<ToKey>(key));
  }

  log_pointer find_log(const log_key_type& key) noexcept {
    if (!wal_object_) {
      return nullptr;
    }

    return wal_object_->find_log(key);
  }

  log_const_iterator find_log(const log_key_type& key) const noexcept {
    if (!wal_object_) {
      return nullptr;
    }

    return wal_object_->find_log(key);
  }

  inline const private_data_type& get_private_data() const noexcept { return wal_object_->get_private_data(); }
  inline private_data_type& get_private_data() noexcept { return wal_object_->get_private_data(); }

  inline const log_key_compare_type& get_log_key_compare() const noexcept { return wal_object_->get_log_key_compare(); }
  inline log_key_compare_type& get_log_key_compare() noexcept { return wal_object_->get_log_key_compare(); }

  inline const congfigure_type& get_configure() const noexcept {
    // We can not create wal_object without congfigure, so it's safe here
    return *configure_;
  }

  inline congfigure_type& get_configure() noexcept {
    // We can not create wal_object without congfigure, so it's safe here
    return *configure_;
  }

  const object_type& get_log_manager() const noexcept { return *wal_object_; }
  object_type& get_log_manager() noexcept { return *wal_object_; }

  const subscriber_manager_type& get_subscribe_manager() const noexcept { return *subscriber_manager_; }
  subscriber_manager_type& get_subscribe_manager() noexcept { return *subscriber_manager_; }

  subscriber_pointer find_subscriber(const subscriber_key_type& key, callback_param_type param) {
    subscriber_pointer ret = subscriber_manager_->find(key);

    if (ret && !check_subscriber(ret, param)) {
      ret.reset();
    }

    return ret;
  }

  bool check_subscriber(const subscriber_pointer& subscriber, callback_param_type param) {
    if (!subscriber) {
      return false;
    }

    if (vtable_ && vtable_->check_subscriber) {
      if (!vtable_->check_subscriber(*this, subscriber, param)) {
        remove_subscriber(subscriber, wal_unsubscribe_reason::kInvalid, std::move(param));
        return false;
      }
    }

    return true;
  }

  template <class... ArgsT>
  subscriber_pointer create_subscriber(const subscriber_key_type& key, const time_point& now, callback_param_type param,
                                       ArgsT&&... args) {
    subscriber_pointer subscriber = find_subscriber(key);
    if (subscriber) {
      subscriber->set_heartbeat_timeout(configure_->subscriber_timeout);
      subscriber_manager_->subscribe(*subscriber, now);
      return subscriber;
    }

    subscriber = subscriber_manager_->create(key, now, configure_->subscriber_timeout, std::forward<ArgsT>(args)...);
    if (subscriber && vtable_) {
      if (vtable_->on_subscriber_added) {
        vtable_->on_subscriber_added(*this, subscriber, param);
      }

      auto range = subscriber_manager_->find_iterator(key);
      if (range.first != range.second) {
        send_snapshot(range.first, range.second, std::move(param));
      }
    }
  }

  void remove_subscriber(const subscriber_key_type& key, wal_unsubscribe_reason reason, callback_param_type param) {
    subscriber_pointer subscriber = subscriber_manager_->unsubscribe(key, reason);
    if (subscriber && vtable_ && vtable_->on_subscriber_removed) {
      vtable_->on_subscriber_removed(this, subscriber, reason, param);
    }
  }

  void remove_subscriber(const subscriber_pointer& checked, wal_unsubscribe_reason reason, callback_param_type param) {
    subscriber_pointer subscriber = subscriber_manager_->unsubscribe(checked, reason);
    if (subscriber && vtable_ && vtable_->on_subscriber_removed) {
      vtable_->on_subscriber_removed(this, subscriber, reason, param);
    }
  }

  size_t tick(const time_point& now, callback_param_type param, size_t max_event = std::numeric_limits<size_t>::max()) {
    size_t ret = 0;
    if (0 == max_event) {
      max_event = std::numeric_limits<size_t>::max();
    }

    bool has_event = true;
    while (ret < max_event && has_event) {
      size_t round = max_event;
      has_event = false;
      if (max_event > 16) {
        round = max_event / 16;
      }

      // GC logs
      size_t res = wal_object_->gc(now, &broadcast_key_bound_, round);
      if (res > 0) {
        has_event = true;
        ret += res;
      }

      // Subscriber expires
      for (size_t j = 0; j < round; ++j) {
        subscriber_pointer subscriber = subscriber_manager_->get_first_expired(now);
        if (subscriber) {
          remove_subscriber(subscriber, wal_unsubscribe_reason::kTimeout, param);
          ++ret;
          has_event = true;
        } else {
          break;
        }
      }

      // broadcast logs
      res = broadcast(param);
      if (res > 0) {
        has_event = true;
        ret += res;
      }
    }

    return ret;
  }

  wal_result_code receive_subscribe(const subscriber_key_type& key, log_key_type last_checkpoint, const time_point& now,
                                    callback_param_type param) {
    subscriber_pointer subscriber = find_subscriber(key, param);
    if (!subscriber) {
      return wal_result_code::kSubscriberNotFound;
    }

    // Reset timer
    subscriber_manager_->reset_timer(subscriber, now);

    if (wal_object_->get_last_removed_key()) {
      // Some log can not be resend, send snapshot
      if (wal_object_->get_log_key_compare()(last_checkpoint, *wal_object_->get_last_removed_key())) {
        auto iters = subscriber_manager_->find_iterator(key);
        auto notify_result = send_snapshot(iters.first, iters.second, std::move(param));
        return send_subscribe_response(subscriber, notify_result, param);
      }
    }

    log_const_iterator log_iter = wal_object_->log_upper_bound(key);
    if (log_iter != wal_object_->log_cend()) {
      auto iters = subscriber_manager_->find_iterator(key);
      auto notify_result = send_logs(log_iter, wal_object_->log_cend(), iters.first, iters.second, std::move(param));
      return send_subscribe_response(subscriber, notify_result, param);
    }

    return send_subscribe_response(subscriber, wal_result_code::kOk, param);
  }

  template <class ParamT>
  size_t broadcast(ParamT&& param) {
    if (!vtable_ || !vtable_->get_log_key) {
      return 0;
    }

    // No more to broadcast
    std::pair<log_const_iterator, log_const_iterator> logs;
    if (broadcast_key_bound_) {
      logs = std::pair<log_const_iterator, log_const_iterator>(wal_object_->log_upper_bound(*broadcast_key_bound_),
                                                               wal_object_->log_cend());
    } else {
      logs = wal_object_->log_all_range();
    }
    if (logs.first == logs.second) {
      return 0;
    }

    std::pair<subscriber_iterator, subscriber_iterator> subscribers = subscriber_manager_->all_range();
    if (subscribers.first != subscribers.second) {
      send_logs(logs.first, logs.second, subscribers.first, subscribers.second, std::forward<ParamT>(param));
    }

    int ret = 0;
    log_const_iterator last;
    while (logs.first != logs.second) {
      last = logs.first;
      ++logs.first;
      ++ret;
    }

    if (broadcast_key_bound_) {
      *broadcast_key_bound_ = vtable_->get_log_key(*wal_object_, **last);
    } else {
      broadcast_key_bound_.reset(new log_key_type{vtable_->get_log_key(*wal_object_, **last)});
    }

    return ret;
  }

  template <class ParamT>
  wal_result_code send_snapshot(subscriber_iterator begin, subscriber_iterator end, ParamT&& param) {
    if (!vtable_ || !vtable_->send_snapshot) {
      return wal_result_code::kActionNotSet;
    }

    if (begin == end) {
      return wal_result_code::kOk;
    }

    return vtable_->send_snapshot(*this, begin, end, std::forward<ParamT>(param));
  }

  template <class ParamT>
  wal_result_code send_logs(log_const_iterator log_begin, log_const_iterator log_end, subscriber_iterator sub_begin,
                            subscriber_iterator sub_end, ParamT&& param) {
    if (!vtable_ || !vtable_->send_logs) {
      return wal_result_code::kActionNotSet;
    }

    if (log_begin == log_end || sub_end == sub_begin) {
      return wal_result_code::kOk;
    }

    return vtable_->send_logs(*this, log_begin, log_end, sub_begin, sub_end, std::forward<ParamT>(param));
  }

  template <class ParamT>
  wal_result_code send_subscribe_response(const subscriber_pointer& subscriber, wal_result_code code, ParamT&& param) {
    if (!subscriber) {
      return wal_result_code::kSubscriberNotFound;
    }

    if (!vtable_ || !vtable_->subscribe_response) {
      return code;
    }

    return vtable_->subscribe_response(*this, subscriber, code, std::forward<ParamT>(param));
  }

 private:
  vtable_pointer vtable_;
  congfigure_pointer configure_;

  // logs
  std::shared_ptr<object_type> wal_object_;
  std::shared_ptr<subscriber_manager_type> subscriber_manager_;

  // publish-subscribe
  std::unique_ptr<log_key_type> broadcast_key_bound_;
};

}  // namespace distributed_system
}  // namespace util
