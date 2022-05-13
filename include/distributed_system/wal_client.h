// Copyright 2021 atframework
// Created by owent
// Stanards operations for Write Ahead Log client

#pragma once

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include "distributed_system/wal_common_defs.h"
#include "distributed_system/wal_object.h"

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace distributed_system {

template <class StorageT, class LogOperatorT, class CallbackParamT, class PrivateDataT, class SnapshotT>
class LIBATFRAME_UTILS_API_HEAD_ONLY wal_client {
 public:
  using log_operator_type = LogOperatorT;
  using object_type = wal_object<StorageT, LogOperatorT, CallbackParamT, PrivateDataT>;
  using snapshot_type = SnapshotT;

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

  // Receive snapshot from publisher
  using callback_on_receive_snapshot_fn_t =
      std::function<wal_result_code(wal_client&, const snapshot_type&, callback_param_type)>;

  // Receive subscribe heartbeat response
  using callback_on_receive_subscribe_response_fn_t = std::function<wal_result_code(wal_client&, callback_param_type)>;

  // Send subscribe request
  using callback_send_subscribe_request_fn_t = std::function<wal_result_code(wal_client&, callback_param_type)>;

  struct vtable_type : public object_type::vtable_type {
    callback_on_receive_snapshot_fn_t on_receive_snapshot;
    callback_on_receive_subscribe_response_fn_t on_receive_subscribe_response;
    callback_send_subscribe_request_fn_t subscribe_request;
  };
  using vtable_pointer = std::shared_ptr<vtable_type>;

  struct configure_type : public object_type::configure_type {
    duration subscriber_heartbeat_interval;
    duration subscriber_heartbeat_retry_interval;
  };
  using configure_pointer = std::shared_ptr<configure_type>;

 private:
  UTIL_DESIGN_PATTERN_NOMOVABLE(wal_client);
  UTIL_DESIGN_PATTERN_NOCOPYABLE(wal_client);
  struct construct_helper {
    time_point next_heartbeat;
    vtable_pointer vt;
    configure_pointer conf;
    std::shared_ptr<object_type> wal_object;
  };

 public:
  explicit wal_client(construct_helper& helper)
      : vtable_(helper.vt),
        configure_(helper.conf),
        wal_object_(helper.wal_object),
        next_heartbeat_timepoint_(helper.next_heartbeat) {
    if (wal_object_) {
      wal_object_->set_internal_event_on_assign_logs([this](object_type& wal) {
        // reset finished key
        if (!wal.get_all_logs().empty() && this->vtable_ && this->vtable_->get_log_key) {
          auto log_key = this->vtable_->get_log_key(wal, **wal.get_all_logs().rbegin());
          if (!this->get_last_finished_log_key()) {
            this->set_last_finished_log_key(std::move(log_key));
          } else if (get_log_key_compare()(*this->get_last_finished_log_key(), log_key)) {
            this->set_last_finished_log_key(std::move(log_key));
          }
        }
      });
    }
  }

  template <class... ArgsT>
  static std::shared_ptr<wal_client> create(time_point now, vtable_pointer vt, configure_pointer conf,
                                            ArgsT&&... args) {
    if (!vt || !conf) {
      return nullptr;
    }

    if (!vt->get_meta || !vt->get_log_key || !vt->allocate_log_key) {
      return nullptr;
    }

    if (!vt->on_receive_snapshot) {
      return nullptr;
    }

    construct_helper helper;
    helper.next_heartbeat = now;
    helper.vt = vt;
    helper.conf = conf;
    helper.wal_object = object_type::create(std::static_pointer_cast<typename object_type::vtable_type>(helper.vt),
                                            std::static_pointer_cast<typename object_type::configure_type>(helper.conf),
                                            std::forward<ArgsT>(args)...);
    if (!helper.wal_object) {
      return nullptr;
    }

    return std::make_shared<wal_client>(helper);
  }

  static configure_pointer make_configure() {
    configure_pointer ret = std::make_shared<configure_type>();
    if (!ret) {
      return ret;
    }

    object_type::default_configure(*ret);
    ret->subscriber_heartbeat_interval = std::chrono::duration_cast<duration>(std::chrono::minutes{3});
    ret->subscriber_heartbeat_retry_interval = std::chrono::duration_cast<duration>(std::chrono::minutes{1});
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

      size_t res = wal_object_->gc(now, nullptr, round);
      if (res > 0) {
        has_event = true;
        ret += res;
      }

      // Subscriber expires
      if (now >= next_heartbeat_timepoint_) {
        if (vtable_ && vtable_->subscribe_request) {
          wal_result_code subscribe_result = vtable_->subscribe_request(*this, param);
          if (wal_result_code::kOk == subscribe_result) {
            next_heartbeat_timepoint_ = now + get_configure().subscriber_heartbeat_interval;
          } else {
            next_heartbeat_timepoint_ = now + get_configure().subscriber_heartbeat_retry_interval;
          }
        } else {
          next_heartbeat_timepoint_ = now + get_configure().subscriber_heartbeat_interval;
        }
        UTIL_UNLIKELY_IF(next_heartbeat_timepoint_ <= now) {
          next_heartbeat_timepoint_ = now + std::chrono::duration_cast<duration>(std::chrono::minutes{3});
        }

        has_event = true;
        ++ret;
      }
    }

    return ret;
  }

  wal_result_code receive_log(callback_param_type param, log_pointer&& log) {
    if (!wal_object_) {
      return wal_result_code::kInitlization;
    }

    if (!log) {
      return wal_result_code::kInvalidParam;
    }

    if (vtable_ && vtable_->get_log_key) {
      auto log_key = vtable_->get_log_key(*wal_object_, *log);
      if (get_last_finished_log_key() && !get_log_key_compare()(*this->get_last_finished_log_key(), log_key)) {
        return wal_result_code::kIgnore;
      }

      set_last_finished_log_key(std::move(log_key));
    }

    return wal_object_->emplace_back(std::move(log), param);
  }

  wal_result_code receive_log(callback_param_type param, const log_pointer& log) {
    return receive_log(log_pointer{log}, param);
  }

  template <class... LogCtorArgsT>
  wal_result_code receive_log(callback_param_type param, LogCtorArgsT&&... args) {
    return receive_log(param, std::make_shared<log_type>(std::forward<LogCtorArgsT>(args)...));
  }

  template <class IteratorT>
  size_t receive_logs(callback_param_type param, IteratorT begin, IteratorT end) {
    size_t ret = 0;
    while (begin != end) {
      if (wal_result_code::kOk == receive_log(param, *begin)) {
        ++ret;
      }
      ++begin;
    }

    return ret;
  }

  wal_result_code receive_snapshot(const snapshot_type& snapshot, callback_param_type param) {
    if (vtable_ && vtable_->on_receive_snapshot) {
      return vtable_->on_receive_snapshot(*this, snapshot, param);
    }

    return wal_result_code::kInitlization;
  }

  wal_result_code receive_subscribe_response(callback_param_type param) {
    if (vtable_ && vtable_->on_receive_subscribe_response) {
      return vtable_->on_receive_subscribe_response(*this, param);
    }

    return wal_result_code::kInitlization;
  }

  template <class... ArgsT>
  void set_last_finished_log_key(ArgsT&&... args) {
    if (last_finished_log_key_) {
      *last_finished_log_key_ = log_key_type{std::forward<ArgsT>(args)...};
    } else {
      last_finished_log_key_.reset(new log_key_type{std::forward<ArgsT>(args)...});
    }
  }

  const log_key_type* get_last_finished_log_key() const { return last_finished_log_key_.get(); }

 private:
  vtable_pointer vtable_;
  configure_pointer configure_;

  // logs
  std::shared_ptr<object_type> wal_object_;

  // publish-subscribe
  time_point next_heartbeat_timepoint_;
  std::unique_ptr<log_key_type> last_finished_log_key_;
};

}  // namespace distributed_system
LIBATFRAME_UTILS_NAMESPACE_END
