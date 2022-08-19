// Copyright 2021 atframework
// Created by owent
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

LIBATFRAME_UTILS_NAMESPACE_BEGIN
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
  using log_const_pointer = typename log_operator_type::log_const_pointer;
  using log_key_type = typename log_operator_type::log_key_type;
  using log_key_compare_type = typename log_operator_type::log_key_compare_type;
  using action_getter_type = typename log_operator_type::action_getter_type;
  using action_case_type = typename log_operator_type::action_case_type;
  using log_key_result_type = typename log_operator_type::log_key_result_type;

  using log_container_type = typename object_type::log_container_type;
  using log_iterator = typename object_type::log_iterator;
  using log_const_iterator = typename object_type::log_const_iterator;
  using callback_param_type = typename object_type::callback_param_type;
  using time_point = typename object_type::time_point;
  using duration = typename object_type::duration;
  using meta_type = typename object_type::meta_type;
  using meta_result_type = typename object_type::meta_result_type;

  using private_data_type = typename object_type::private_data_type;

  using subscriber_key_type = typename subscriber_type::key_type;
  using subscriber_collector_type = typename subscriber_type::subscriber_collector_type;
  using subscriber_iterator = typename subscriber_type::subscriber_iterator;
  using subscriber_const_iterator = typename subscriber_type::subscriber_const_iterator;
  using subscriber_pointer = typename subscriber_type::pointer;
  using subscriber_private_data_type = typename subscriber_type::private_data_type;
  using subscriber_manager_type = typename subscriber_type::manager;

  using callback_load_fn_t = typename object_type::callback_load_fn_t;
  using callback_dump_fn_t = typename object_type::callback_dump_fn_t;
  using callback_log_action_fn_t = typename object_type::callback_log_action_fn_t;
  using callback_log_patch_fn_t = typename object_type::callback_log_patch_fn_t;
  using callback_log_event_fn_t = typename object_type::callback_log_event_fn_t;
  using callback_log_merge_fn_t = typename object_type::callback_log_merge_fn_t;
  using callback_log_get_meta_fn_t = typename object_type::callback_log_get_meta_fn_t;
  using callback_log_set_meta_fn_t = typename object_type::callback_log_set_meta_fn_t;
  using callback_get_log_key_fn_t = typename object_type::callback_get_log_key_fn_t;
  using callback_alloc_log_key_fn_t = typename object_type::callback_alloc_log_key_fn_t;

  using callback_log_fn_group_t = typename object_type::callback_log_fn_group_t;
  using callback_log_group_map_t = typename object_type::callback_log_group_map_t;

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

  // Check if subscriber still available
  using callback_check_subscriber_fn_t =
      std::function<bool(wal_publisher&, const subscriber_pointer&, callback_param_type)>;

  // Check if need send backup snapshot
  using callback_subscriber_force_sync_snapshot_fn_t =
      std::function<bool(wal_publisher&, const subscriber_pointer&, callback_param_type)>;

  // On subscriber request
  using callback_on_subscriber_request_fn_t =
      std::function<void(wal_publisher&, const subscriber_pointer&, callback_param_type)>;

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
    callback_subscriber_force_sync_snapshot_fn_t subscriber_force_sync_snapshot;
    callback_on_subscriber_request_fn_t on_subscriber_request;
    callback_on_subscriber_added_fn_t on_subscriber_added;
    callback_on_subscriber_removed_fn_t on_subscriber_removed;
  };
  using vtable_pointer = std::shared_ptr<vtable_type>;

  struct configure_type : public object_type::configure_type {
    duration subscriber_timeout;
    bool enable_last_broadcast_for_removed_subscriber;
    bool enable_hole_log;
  };
  using configure_pointer = std::shared_ptr<configure_type>;

 private:
  UTIL_DESIGN_PATTERN_NOMOVABLE(wal_publisher);
  UTIL_DESIGN_PATTERN_NOCOPYABLE(wal_publisher);
  struct construct_helper {
    vtable_pointer vt;
    configure_pointer conf;
    std::shared_ptr<object_type> wal_object;
    std::shared_ptr<subscriber_manager_type> subscriber_manager;
  };

 public:
  explicit wal_publisher(construct_helper& helper)
      : vtable_(helper.vt),
        configure_(helper.conf),
        wal_object_(helper.wal_object),
        subscriber_manager_(helper.subscriber_manager) {
    if (wal_object_) {
      wal_object_->set_internal_event_on_assign_logs([this](object_type& wal) {
        // reset broadcast
        if (!wal.get_all_logs().empty() && this->vtable_ && this->vtable_->get_log_key) {
          this->set_broadcast_key_bound(this->vtable_->get_log_key(wal, **wal.get_all_logs().rbegin()));
        }
        broadcast_hole_logs_.clear();
      });

      wal_object_->set_internal_event_on_assign_logs([this](object_type& wal, const log_pointer& log) {
        if (!log) {
          return;
        }

        if (broadcast_key_bound_ && configure_ && configure_->enable_hole_log && this->vtable_ &&
            this->vtable_->get_log_key &&
            wal.get_log_key_compare()(this->vtable_->get_log_key(wal, *log), *broadcast_key_bound_)) {
          broadcast_hole_logs_.push_back(log);
        }
      });
    }
  }

  template <class... ArgsT>
  static std::shared_ptr<wal_publisher> create(vtable_pointer vt, configure_pointer conf, ArgsT&&... args) {
    if (!vt || !conf) {
      return nullptr;
    }

    if (!vt->get_meta || !vt->get_log_key || !vt->allocate_log_key) {
      return nullptr;
    }

    if (!vt->send_snapshot || !vt->send_logs) {
      return nullptr;
    }

    construct_helper helper;
    helper.vt = vt;
    helper.conf = conf;
    helper.wal_object = object_type::create(std::static_pointer_cast<typename object_type::vtable_type>(helper.vt),
                                            std::static_pointer_cast<typename object_type::configure_type>(helper.conf),
                                            std::forward<ArgsT>(args)...);
    helper.subscriber_manager = std::make_shared<subscriber_manager_type>();
    if (!helper.wal_object || !helper.subscriber_manager) {
      return nullptr;
    }

    return std::make_shared<wal_publisher>(helper);
  }

  static configure_pointer make_configure() {
    configure_pointer ret = std::make_shared<configure_type>();
    if (!ret) {
      return ret;
    }

    object_type::default_configure(*ret);
    ret->subscriber_timeout = std::chrono::duration_cast<duration>(std::chrono::minutes{10});
    ret->enable_last_broadcast_for_removed_subscriber = false;
    ret->enable_hole_log = false;
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
  void assign_logs(ArgsT&&... args) {
    if (!wal_object_) {
      return;
    }
    wal_object_->assign_logs(std::forward<ArgsT>(args)...);
  }

  template <class... ArgsT>
  log_pointer allocate_log(time_point now, action_case_type action_case, callback_param_type param, ArgsT&&... args) {
    if (!wal_object_) {
      return nullptr;
    }

    return wal_object_->allocate_log(now, action_case, param, std::forward<ArgsT>(args)...);
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

  const log_key_type* get_global_log_ingore_key() const noexcept {
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

  log_const_pointer find_log(const log_key_type& key) const noexcept {
    if (!wal_object_) {
      return nullptr;
    }

    return wal_object_->find_log(key);
  }

  inline const private_data_type& get_private_data() const noexcept { return wal_object_->get_private_data(); }
  inline private_data_type& get_private_data() noexcept { return wal_object_->get_private_data(); }

  inline const log_key_compare_type& get_log_key_compare() const noexcept { return wal_object_->get_log_key_compare(); }
  inline log_key_compare_type& get_log_key_compare() noexcept { return wal_object_->get_log_key_compare(); }

  inline const configure_type& get_configure() const noexcept {
    // We can not create wal_object without configure, so it's safe here
    return *configure_;
  }

  inline configure_type& get_configure() noexcept {
    // We can not create wal_object without configure, so it's safe here
    return *configure_;
  }

  const object_type& get_log_manager() const noexcept { return *wal_object_; }
  object_type& get_log_manager() noexcept { return *wal_object_; }

  const subscriber_manager_type& get_subscribe_manager() const noexcept { return *subscriber_manager_; }
  subscriber_manager_type& get_subscribe_manager() noexcept { return *subscriber_manager_; }

  const subscriber_collector_type& get_subscribe_gc_pool() const noexcept { return gc_subscribers_; }
  subscriber_collector_type& get_subscribe_gc_pool() noexcept { return gc_subscribers_; }

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
  subscriber_pointer create_subscriber(const subscriber_key_type& key, const time_point& now,
                                       log_key_type last_checkpoint, callback_param_type param, ArgsT&&... args) {
    subscriber_pointer subscriber = find_subscriber(key, param);
    if (subscriber) {
      subscriber->set_heartbeat_timeout(configure_->subscriber_timeout);
      subscriber_manager_->subscribe(subscriber, now);
      return subscriber;
    }

    subscriber = subscriber_manager_->create(key, now, configure_->subscriber_timeout, std::forward<ArgsT>(args)...);
    if (subscriber && vtable_) {
      if (vtable_->on_subscriber_added) {
        vtable_->on_subscriber_added(*this, subscriber, param);
      }

      _receive_subscribe_request(key, last_checkpoint, now, param, false);
    }

    return subscriber;
  }

  void remove_subscriber(const subscriber_key_type& key, wal_unsubscribe_reason reason, callback_param_type param) {
    subscriber_pointer subscriber = subscriber_manager_->unsubscribe(key, reason);
    if (!subscriber) {
      return;
    }

    if (configure_ && configure_->enable_last_broadcast_for_removed_subscriber) {
      gc_subscribers_[key] = subscriber;
    }

    if (vtable_ && vtable_->on_subscriber_removed) {
      vtable_->on_subscriber_removed(*this, subscriber, reason, param);
    }
  }

  void remove_subscriber(const subscriber_pointer& checked, wal_unsubscribe_reason reason, callback_param_type param) {
    subscriber_pointer subscriber = subscriber_manager_->unsubscribe(checked, reason);
    if (!subscriber) {
      return;
    }

    if (configure_ && configure_->enable_last_broadcast_for_removed_subscriber) {
      gc_subscribers_[subscriber->get_key()] = subscriber;
    }

    if (vtable_ && vtable_->on_subscriber_removed) {
      vtable_->on_subscriber_removed(*this, subscriber, reason, param);
    }
  }

  std::pair<subscriber_iterator, subscriber_iterator> subscriber_all_range() noexcept {
    return subscriber_manager_->all_range();
  }

  std::pair<subscriber_const_iterator, subscriber_const_iterator> subscriber_all_range() const noexcept {
    return subscriber_manager_->all_range();
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

      size_t res;

      // broadcast logs
      res = broadcast(param);
      if (res > 0) {
        has_event = true;
        ret += res;
      }

      // GC logs
      if (broadcast_key_bound_) {
        res = wal_object_->gc(now, broadcast_key_bound_.get(), round);
      } else {
        res = wal_object_->gc(now, nullptr, round);
      }

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
    }

    return ret;
  }

 private:
  wal_result_code _receive_subscribe_request(const subscriber_key_type& key, log_key_type last_checkpoint,
                                             const time_point& now, callback_param_type param, bool reset_timer) {
    subscriber_pointer subscriber = subscriber_manager_->find(key);
    if (!subscriber) {
      return wal_result_code::kSubscriberNotFound;
    }

    // Reset timer
    if (reset_timer) {
      subscriber_manager_->reset_timer(subscriber, now);
    }

    if (!check_subscriber(subscriber, param)) {
      return wal_result_code::kSubscriberNotFound;
    }

    subscriber->update_heartbeat_time_point(now);

    if (vtable_ && vtable_->on_subscriber_request) {
      vtable_->on_subscriber_request(*this, subscriber, param);
    }

    if (vtable_ && vtable_->subscriber_force_sync_snapshot) {
      if (vtable_->subscriber_force_sync_snapshot(*this, subscriber, param)) {
        auto iters = subscriber_manager_->find_iterator(key);
        auto notify_result = send_snapshot(iters.first, iters.second, param);
        return send_subscribe_response(subscriber, notify_result, std::move(param));
      }
    }

    if (nullptr != wal_object_->get_last_removed_key()) {
      // Some log can not be resend, send snapshot
      if (wal_object_->get_log_key_compare()(last_checkpoint, *wal_object_->get_last_removed_key())) {
        auto iters = subscriber_manager_->find_iterator(key);
        auto notify_result = send_snapshot(iters.first, iters.second, param);
        return send_subscribe_response(subscriber, notify_result, std::move(param));
      }
    }

    log_const_iterator log_iter = wal_object_->log_upper_bound(last_checkpoint);
    if (log_iter != wal_object_->log_cend()) {
      auto iters = subscriber_manager_->find_iterator(key);
      auto notify_result = send_logs(log_iter, wal_object_->log_cend(), iters.first, iters.second, param);
      return send_subscribe_response(subscriber, notify_result, std::move(param));
    }

    return send_subscribe_response(subscriber, wal_result_code::kOk, std::move(param));
  }

 public:
  wal_result_code receive_subscribe_request(const subscriber_key_type& key, log_key_type last_checkpoint,
                                            const time_point& now, callback_param_type param) {
    return _receive_subscribe_request(key, last_checkpoint, now, param, true);
  }

  template <class... ArgsT>
  void set_broadcast_key_bound(ArgsT&&... args) {
    if (broadcast_key_bound_) {
      *broadcast_key_bound_ = log_key_type{std::forward<ArgsT>(args)...};
    } else {
      broadcast_key_bound_.reset(new log_key_type{std::forward<ArgsT>(args)...});
    }
  }

  const log_key_type* get_broadcast_key_bound() const { return broadcast_key_bound_.get(); }

  size_t broadcast(callback_param_type param) {
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

    std::pair<subscriber_iterator, subscriber_iterator> subscribers = subscriber_manager_->all_range();
    if (subscribers.first != subscribers.second) {
      if (logs.first != logs.second) {
        send_logs(logs.first, logs.second, subscribers.first, subscribers.second, param);
      }
      if (!broadcast_hole_logs_.empty()) {
        send_logs(broadcast_hole_logs_.begin(), broadcast_hole_logs_.end(), subscribers.first, subscribers.second,
                  param);
      }
    }

    size_t retry_last_broadcast = 3;
    while (!gc_subscribers_.empty() && retry_last_broadcast > 0) {
      --retry_last_broadcast;
      subscriber_collector_type cache;
      cache.swap(gc_subscribers_);
      bool send_logs_result = true;
      bool send_hole_logs_result = true;
      if (logs.first != logs.second) {
        send_logs_result =
            (wal_result_code::kOk == send_logs(logs.first, logs.second, cache.begin(), cache.end(), param));
      }
      if (!broadcast_hole_logs_.empty()) {
        send_hole_logs_result =
            (wal_result_code::kOk ==
             send_logs(broadcast_hole_logs_.begin(), broadcast_hole_logs_.end(), cache.begin(), cache.end(), param));
      }
      if (send_logs_result && send_hole_logs_result) {
        retry_last_broadcast = 0;
      } else {
        for (auto& new_removed_subscriber : gc_subscribers_) {
          cache[new_removed_subscriber.first] = new_removed_subscriber.second;
        }
        cache.swap(gc_subscribers_);
      }
    }
    if (!gc_subscribers_.empty()) {
      gc_subscribers_.clear();
    }

    size_t ret = broadcast_hole_logs_.size();
    if (!broadcast_hole_logs_.empty()) {
      broadcast_hole_logs_.clear();
    }

    log_const_iterator last;
    bool reset_broadcast_key_bound = false;
    while (logs.first != logs.second) {
      last = logs.first;
      reset_broadcast_key_bound = true;
      ++logs.first;
      ++ret;
    }

    if (reset_broadcast_key_bound) {
      set_broadcast_key_bound(vtable_->get_log_key(*wal_object_, **last));
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
  configure_pointer configure_;

  // logs
  std::shared_ptr<object_type> wal_object_;
  std::shared_ptr<subscriber_manager_type> subscriber_manager_;
  subscriber_collector_type gc_subscribers_;

  // publish-subscribe
  std::unique_ptr<log_key_type> broadcast_key_bound_;
  log_container_type broadcast_hole_logs_;
};

}  // namespace distributed_system
LIBATFRAME_UTILS_NAMESPACE_END
