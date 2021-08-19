// Copyright 2021 atframework
// Created by owent
// Stanards operations for Write Ahead Log

#pragma once

#include <design_pattern/nomovable.h>
#include <design_pattern/noncopyable.h>

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

#include "distributed_system/wal_common_defs.h"

#ifdef max
#  undef max
#endif

namespace util {
namespace distributed_system {

template <class StorageT, class LogOperatorT, class CallbackParamT, class PrivateDataT>
class LIBATFRAME_UTILS_API_HEAD_ONLY wal_object {
 public:
  using storage_type = StorageT;
  using private_data_type = PrivateDataT;
  using log_operator_type = LogOperatorT;

  using log_type = typename log_operator_type::log_type;
  using log_pointer = typename log_operator_type::log_pointer;
  using log_key_type = typename log_operator_type::log_key_type;
  using log_key_compare_type = typename log_operator_type::log_key_compare_type;
  using action_getter_type = typename log_operator_type::action_getter_type;
  using action_case_type = typename log_operator_type::action_case_type;
  using log_key_result_type = typename log_operator_type::log_key_result_type;

  using log_container_type = std::deque<log_pointer>;
  using log_iterator = typename log_container_type::iterator;
  using log_const_iterator = typename log_container_type::const_iterator;
  using callback_param_type = CallbackParamT;
  using time_point = wal_time_point;
  using duration = wal_duration;
  using meta_type = wal_meta_type<log_key_type, action_case_type>;
  using meta_result_type = util::design_pattern::result_type<meta_type, wal_result_code>;

  // Load data from storage into wal_object
  using callback_load_fn_t = std::function<wal_result_code(wal_object&, const storage_type&, callback_param_type)>;

  // Dump data from wal_object into storage
  using callback_dump_fn_t = std::function<wal_result_code(const wal_object&, storage_type&, callback_param_type)>;

  // Run log action
  using callback_log_action_fn_t = std::function<wal_result_code(wal_object&, const log_type&, callback_param_type)>;

  // Log event callback
  using callback_log_event_fn_t = std::function<void(wal_object&, const log_pointer&)>;

  // Merge action
  using callback_log_merge_fn_t =
      std::function<void(const wal_object&, callback_param_type, log_type&, const log_type&)>;

  // Get the meta data of a log
  using callback_log_get_meta_fn_t = std::function<meta_result_type(const wal_object&, const log_type&)>;

  // Set the meta data of a log
  using callback_log_set_meta_fn_t = std::function<void(const wal_object&, log_type&, const meta_type&)>;

  // Get the key of a log
  using callback_get_log_key_fn_t = std::function<log_key_type(const wal_object&, const log_type&)>;

  // Allocate a log id
  using callback_alloc_log_key_fn_t = std::function<log_key_result_type(wal_object&, callback_param_type)>;

  using action_map_type =
      std::unordered_map<action_case_type, callback_log_action_fn_t, typename log_operator_type::action_case_hash,
                         typename log_operator_type::action_case_equal>;

  struct vtable_type {
    callback_load_fn_t load;
    callback_dump_fn_t dump;
    callback_log_get_meta_fn_t get_meta;
    callback_log_set_meta_fn_t set_meta;
    callback_log_merge_fn_t merge_log;
    callback_get_log_key_fn_t get_log_key;
    callback_alloc_log_key_fn_t alloc_log_key;
    callback_log_event_fn_t on_log_added;
    callback_log_event_fn_t on_log_removed;
    action_map_type delegate_action;
    callback_log_action_fn_t default_action;
  };
  using vtable_pointer = std::shared_ptr<vtable_type>;

  struct congfigure_type {
    duration gc_expire_duration;
    size_t max_log_size;
    size_t gc_log_size;
  };
  using congfigure_pointer = std::shared_ptr<congfigure_type>;

 private:
  UTIL_DESIGN_PATTERN_NOMOVABLE(wal_object);
  UTIL_DESIGN_PATTERN_NOCOPYABLE(wal_object);
  struct construct_helper {
    vtable_pointer vt;
    congfigure_pointer conf;
  };

  struct finally_helper {
    std::function<void(wal_object&)> fn;
    wal_object* owner;
    finally_helper(wal_object& o, std::function<void(wal_object&)> f) : fn(f), owner(&o) {}
    ~finally_helper() {
      if (nullptr != owner && fn) {
        fn(*owner);
      }
    }
  };

 public:
  template <class... ArgsT>
  explicit wal_object(construct_helper& helper, ArgsT&&... args)
      : in_log_action_callback_(false),
        vtable_{helper.vt},
        configure_{helper.conf},
        private_data_(std::forward<ArgsT>(args)...) {}

  template <class... ArgsT>
  static std::shared_ptr<wal_object> create(vtable_pointer vt, congfigure_pointer conf, ArgsT&&... args) {
    if (!vt || !conf) {
      return nullptr;
    }

    if (!vt->get_meta || !vt->get_log_key || !vt->alloc_log_key) {
      return nullptr;
    }

    construct_helper helper;
    helper.vt = vt;
    helper.conf = conf;
    return std::make_shared<wal_object>(helper, std::forward<ArgsT>(args)...);
  }

  static void default_configure(congfigure_type& out) {
    out.gc_expire_duration = std::chrono::duration_cast<duration>(std::chrono::hours(7 * 24));
    out.max_log_size = 512;
    out.gc_log_size = 128;
  }

  wal_result_code load(const storage_type& storage, callback_param_type param) {
    if (!configure_ || !vtable_) {
      return wal_result_code::kInitlization;
    }

    if (!vtable_->load) {
      return wal_result_code::kActionNotSet;
    }

    wal_result_code ret = vtable_->load(*this, storage, param);
    if (wal_result_code::kOk == ret && internal_event_on_loaded_) {
      internal_event_on_loaded_(*this, storage, param);
    }
    return ret;
  }

  wal_result_code dump(storage_type& storage, callback_param_type param) {
    if (!configure_ || !vtable_) {
      return wal_result_code::kInitlization;
    }

    if (!vtable_->dump) {
      return wal_result_code::kActionNotSet;
    }

    wal_result_code ret = vtable_->dump(*this, storage, param);
    if (wal_result_code::kOk == ret && internal_event_on_dumped_) {
      internal_event_on_dumped_(*this, storage, param);
    }
    return ret;
  }

  /**
   * @brief clear and assign all logs
   * @note this function is useful when loading data and will not trigger event and action callback
   *
   * @tparam IteratorT iterator type
   * @param begin
   * @param end
   */
  template <class IteratorT>
  void assign_logs(IteratorT&& begin, IteratorT&& end) {
    logs_.clear();
    logs_.assign(std::forward<IteratorT>(begin), std::forward<IteratorT>(end));

    if (internal_event_on_assign_) {
      internal_event_on_assign_(*this);
    }
  }

  /**
   * @brief clear and assign all logs
   * @note this function is useful when loading data and will not trigger event and action callback
   *
   * @tparam ContainerT container type
   * @param source
   */
  template <class ContainerT>
  void assign_logs(const ContainerT& source) {
    assign_logs(source.begin(), source.end());
  }

  void assign_logs(log_container_type&& source) {
    logs_.swap(source);
    source.clear();

    if (internal_event_on_assign_) {
      internal_event_on_assign_(*this);
    }
  }

  template <class... ArgsT>
  log_pointer allocate_log(time_point now, action_case_type action_case, callback_param_type param, ArgsT&&... args) {
    if (!configure_ || !vtable_ || !vtable_->alloc_log_key || !vtable_->set_meta) {
      return nullptr;
    }

    log_key_result_type new_key = vtable_->alloc_log_key(*this, param);
    if (!new_key.is_success()) {
      return nullptr;
    }

    log_pointer ret = std::make_shared<log_type>(std::forward<ArgsT>(args)...);
    if (!ret) {
      return ret;
    }

    meta_type meta;
    meta.timepoint = now;
    meta.log_key = *new_key.get_success();
    meta.action_case = action_case;
    vtable_->set_meta(*this, *ret, meta);

    return ret;
  }

  wal_result_code emplace_back(log_pointer&& log, callback_param_type param) {
    if (!log) {
      return wal_result_code::kInvalidParam;
    }

    if (!pending_logs_.empty() || in_log_action_callback_) {
      pending_logs_.emplace_back(std::pair<log_pointer, callback_param_type>{std::move(log), param});
      return wal_result_code::kPending;
    }

    in_log_action_callback_ = true;
    finally_helper guard(*this, [](wal_object& self) { self.in_log_action_callback_ = false; });

    wal_result_code ret;
    if (!vtable_ || !vtable_->get_log_key) {
      ret = pusk_back_inner(std::move(log), param);
    } else {
      log_key_type this_key = vtable_->get_log_key(*this, *log);
      if (global_ingore_ && !log_key_compare_(*global_ingore_, this_key)) {
        ret = wal_result_code::kIgnore;
      } else {
        ret = pusk_back_inner(std::move(log), param);
      }
    }

    while (!pending_logs_.empty()) {
      auto pending_log = pending_logs_.front();
      pending_logs_.pop_front();

      if (!pending_log.first) {
        continue;
      }

      if (vtable_ && vtable_->get_log_key) {
        log_key_type this_key = vtable_->get_log_key(*this, *pending_log.first);
        if (global_ingore_ && log_key_compare_(this_key, *global_ingore_)) {
          continue;
        }
      }

      pusk_back_inner(std::move(pending_log.first), pending_log.second);
    }

    return ret;
  }

  wal_result_code push_back(log_pointer log, callback_param_type param) { return emplace_back(std::move(log), param); }

  wal_result_code remove_before(time_point now, size_t max_count = std::numeric_limits<size_t>::max()) {
    if (!vtable_ || !vtable_->get_meta) {
      return wal_result_code::kInitlization;
    }

    for (size_t count = 0; count < max_count; ++count) {
      if (logs_.empty()) {
        break;
      }

      meta_result_type meta = vtable_->get_meta(*this, *logs_.back());
      if (meta.is_success() && meta.get_success()->timepoint < now) {
        pop_front_inner();
      } else if (meta.is_error()) {
        return *meta.get_error();
      } else if (meta.is_none()) {
        return wal_result_code::kCallbackError;
      } else {
        break;
      }
    }

    return wal_result_code::kOk;
  }

  /**
   * @brief Clean and remove expired logs
   *
   * @param now current time point
   * @param hold Do not remove logs with key greater than `hold`
   * @param max_count Max log count to remove
   * @return size_t Log count removed
   */
  size_t gc(time_point now, const log_key_type* hold = nullptr, size_t max_count = std::numeric_limits<size_t>::max()) {
    duration gc_expire_duration = std::chrono::duration_cast<duration>(std::chrono::hours(7 * 24));
    size_t max_log_size = 512;
    size_t gc_log_size = 128;
    if (configure_) {
      if (configure_->gc_log_size > 0) {
        gc_log_size = configure_->gc_log_size;
      }
      if (configure_->max_log_size > 0) {
        max_log_size = configure_->max_log_size;
      }
      if (configure_->gc_expire_duration > duration::zero()) {
        gc_expire_duration = configure_->gc_expire_duration;
      }
    }
    size_t ret = 0;
    for (; ret < max_count; ++ret) {
      if (logs_.empty()) {
        break;
      }

      if (logs_.size() <= gc_log_size) {
        break;
      }

      if (logs_.size() > max_log_size) {
        pop_front_inner();
        continue;
      }

      if (!vtable_ || !vtable_->get_meta) {
        pop_front_inner();
        continue;
      }

      meta_result_type meta = vtable_->get_meta(*this, **logs_.begin());
      if (meta.is_error() || meta.is_none()) {
        pop_front_inner();
      } else if (meta.is_success() && meta.get_success()->timepoint + gc_expire_duration <= now) {
        if (nullptr != hold && log_key_compare_(*hold, meta.get_success()->log_key)) {
          break;
        }
        pop_front_inner();
      } else {
        break;
      }
    }

    return ret;
  }

  const std::unique_ptr<log_key_type>& get_global_ingore_key() const noexcept { return global_ingore_; }

  template <class ToKey>
  void set_global_ingore_key(ToKey&& key) {
    if (global_ingore_) {
      *global_ingore_ = std::forward<ToKey>(key);
    } else {
      global_ingore_.reset(new log_key_type{std::forward<ToKey>(key)});
    }
  }

  const std::unique_ptr<log_key_type>& get_last_removed_key() const noexcept { return global_last_removed_; }

  template <class ToKey>
  void set_last_removed_key(ToKey&& key) {
    if (global_last_removed_) {
      *global_last_removed_ = std::forward<ToKey>(key);
    } else {
      global_last_removed_.reset(new log_key_type{std::forward<ToKey>(key)});
    }
  }

  inline const log_container_type& get_all_logs() const noexcept { return logs_; }

  inline const private_data_type& get_private_data() const noexcept { return private_data_; }
  inline private_data_type& get_private_data() noexcept { return private_data_; }

  inline const log_key_compare_type& get_log_key_compare() const noexcept { return log_key_compare_; }
  inline log_key_compare_type& get_log_key_compare() noexcept { return log_key_compare_; }

  inline const congfigure_type& get_configure() const noexcept {
    // We can not create wal_object without congfigure, so it's safe here
    return *configure_;
  }

  inline congfigure_type& get_configure() noexcept {
    // We can not create wal_object without congfigure, so it's safe here
    return *configure_;
  }

  log_pointer find_log(const log_key_type& key) noexcept {
    log_iterator iter = log_lower_bound(key);
    if (iter == logs_.end()) {
      return nullptr;
    }

    log_key_type found_key = vtable_->get_log_key(*this, **iter);
    if (!log_key_compare_(key, found_key) && !log_key_compare_(found_key, key)) {
      return *iter;
    }

    return nullptr;
  }

  log_const_iterator find_log(const log_key_type& key) const noexcept {
    log_const_iterator iter = log_lower_bound(key);
    if (iter == logs_.end()) {
      return nullptr;
    }

    log_key_type found_key = vtable_->get_log_key(*this, **iter);
    if (!log_key_compare_(key, found_key) && !log_key_compare_(found_key, key)) {
      return *iter;
    }

    return nullptr;
  }

  inline log_iterator log_begin() noexcept { return logs_.begin(); }
  inline log_iterator log_end() noexcept { return logs_.end(); }
  inline log_const_iterator log_cbegin() const noexcept { return logs_.begin(); }
  inline log_const_iterator log_cend() const noexcept { return logs_.end(); }

  log_iterator log_lower_bound(const log_key_type& key) noexcept {
    if (!vtable_ || !vtable_->get_log_key) {
      return logs_.end();
    }

    if (logs_.empty()) {
      return logs_.end();
    } else {
      // Optimization for nothing
      // The most frequently usage of this function is used to renew subscriber, which already has the latest log
      log_key_type last_key = vtable_->get_log_key(*this, **logs_.rbegin());
      if (log_key_compare_(last_key, key)) {
        return logs_.end();
      }
    }

    return std::lower_bound(logs_.begin(), logs_.end(), key, [this](const log_pointer& l, const log_key_type& r) {
      log_key_type log_key = this->vtable_->get_log_key(*this, *l);
      return this->log_key_compare_(log_key, r);
    });
  }

  log_const_iterator log_lower_bound(const log_key_type& key) const noexcept {
    if (!vtable_ || !vtable_->get_log_key) {
      return logs_.end();
    }

    if (logs_.empty()) {
      return logs_.end();
    } else {
      // Optimization for nothing
      // The most frequently usage of this function is used to renew subscriber, which already has the latest log
      log_key_type last_key = vtable_->get_log_key(*this, **logs_.rbegin());
      if (log_key_compare_(last_key, key)) {
        return logs_.end();
      }
    }

    return std::lower_bound(logs_.begin(), logs_.end(), key, [this](const log_pointer& l, const log_key_type& r) {
      log_key_type log_key = this->vtable_->get_log_key(*this, *l);
      return this->log_key_compare_(log_key, r);
    });
  }

  log_iterator log_upper_bound(const log_key_type& key) noexcept {
    if (!vtable_ || !vtable_->get_log_key) {
      return logs_.end();
    }

    if (logs_.empty()) {
      return logs_.end();
    } else {
      // Optimization for nothing
      // The most frequently usage of this function is used to renew subscriber, which already has the latest log
      log_key_type last_key = vtable_->get_log_key(*this, **logs_.rbegin());
      if (!log_key_compare_(key, last_key)) {
        return logs_.end();
      }
    }

    return std::upper_bound(logs_.begin(), logs_.end(), key, [this](const log_key_type& l, const log_pointer& r) {
      log_key_type log_key = this->vtable_->get_log_key(*this, *r);
      return this->log_key_compare_(l, log_key);
    });
  }

  log_const_iterator log_upper_bound(const log_key_type& key) const noexcept {
    if (!vtable_ || !vtable_->get_log_key) {
      return logs_.end();
    }

    if (logs_.empty()) {
      return logs_.end();
    } else {
      // Optimization for nothing
      // The most frequently usage of this function is used to renew subscriber, which already has the latest log
      log_key_type last_key = vtable_->get_log_key(*this, **logs_.rbegin());
      if (!log_key_compare_(key, last_key)) {
        return logs_.end();
      }
    }

    return std::upper_bound(logs_.begin(), logs_.end(), key, [this](const log_key_type& l, const log_pointer& r) {
      log_key_type log_key = this->vtable_->get_log_key(*this, *r);
      return this->log_key_compare_(l, log_key);
    });
  }

  std::pair<log_iterator, log_iterator> log_all_range() noexcept {
    return std::pair<log_iterator, log_iterator>(logs_.begin(), logs_.end());
  }

  std::pair<log_const_iterator, log_const_iterator> log_all_range() const noexcept {
    return std::pair<log_const_iterator, log_const_iterator>(logs_.begin(), logs_.end());
  }

 private:
  wal_result_code redo_log(const log_pointer& log, callback_param_type param) {
    if (!log) {
      return wal_result_code::kInvalidParam;
    }

    if (!vtable_ || !vtable_->get_meta) {
      return wal_result_code::kInitlization;
    }

    meta_result_type meta = vtable_->get_meta(*this, *log);
    if (meta.is_error()) {
      return *meta.get_error();
    } else if (!meta.is_success()) {
      return wal_result_code::kCallbackError;
    }

    auto iter = vtable_->delegate_action.find(meta.get_success()->action_case);
    if (iter != vtable_->delegate_action.end()) {
      return iter->second(*this, *log, param);
    } else if (vtable_->default_action) {
      return vtable_->default_action(*this, *log, param);
    } else {
      return wal_result_code::kActionNotSet;
    }
  }

  wal_result_code pusk_back_inner_uncheck(log_pointer&& log, callback_param_type param) {
    wal_result_code ret = redo_log(log, param);
    if (wal_result_code::kOk != ret) {
      return ret;
    }
    logs_.push_back(log);
    if (vtable_ && vtable_->on_log_added) {
      vtable_->on_log_added(*this, log);
    }

    return ret;
  }

  wal_result_code pusk_back_inner(log_pointer&& log, callback_param_type param) {
    if (!log) {
      return wal_result_code::kInvalidParam;
    }

    if (!vtable_) {
      return wal_result_code::kInvalidParam;
    }

    // Empty
    if (logs_.empty()) {
      return pusk_back_inner_uncheck(std::move(log), param);
    }

    // Can not get log key
    if (!vtable_ || !vtable_->get_log_key) {
      return pusk_back_inner_uncheck(std::move(log), param);
    }

    log_key_type last_key = vtable_->get_log_key(*this, *logs_.back());
    log_key_type this_key = vtable_->get_log_key(*this, *log);
    // Has log key
    //   -- push_back
    if (log_key_compare_(last_key, this_key)) {
      return pusk_back_inner_uncheck(std::move(log), param);
    }
    //   -- insert
    auto iter =
        std::lower_bound(logs_.begin(), logs_.end(), this_key, [this](const log_pointer& l, const log_key_type& r) {
          log_key_type log_key = this->vtable_->get_log_key(*this, *l);
          return this->log_key_compare_(log_key, r);
        });

    if (iter != logs_.end()) {
      last_key = vtable_->get_log_key(*this, *logs_.back());
      if (!log_key_compare_(last_key, this_key) && !log_key_compare_(this_key, last_key)) {
        if (vtable_->merge_log) {
          vtable_->merge_log(*this, param, **iter, *log);
        }
        return wal_result_code::kMerge;
      }
    }
    wal_result_code ret = redo_log(log, param);
    if (wal_result_code::kOk != ret) {
      return ret;
    }
    logs_.insert(iter, log);
    if (vtable_ && vtable_->on_log_added) {
      vtable_->on_log_added(*this, log);
    }
    return ret;
  }

  void pop_front_inner() {
    if (logs_.empty()) {
      return;
    }

    log_pointer log = logs_.front();
    logs_.pop_front();

    if (vtable_ && vtable_->get_log_key) {
      log_key_type key = vtable_->get_log_key(*this, *log);
      if (!(global_last_removed_ && log_key_compare_(key, *global_last_removed_))) {
        set_last_removed_key(std::move(key));
      }
    }

    if (vtable_ && vtable_->on_log_removed) {
      vtable_->on_log_removed(*this, log);
    }
  }

 private:
  template <class, class, class, class, class>
  friend class wal_publisher;
  using callback_log_event_on_assign_fn_t = std::function<void(wal_object&)>;

  void set_internal_event_on_assign_logs(callback_log_event_on_assign_fn_t fn) { internal_event_on_assign_ = fn; }

  void set_internal_event_on_loaded(callback_load_fn_t fn) { internal_event_on_loaded_ = fn; }

  void set_internal_event_on_dumped(callback_dump_fn_t fn) { internal_event_on_dumped_ = fn; }

 private:
  bool in_log_action_callback_;
  vtable_pointer vtable_;
  congfigure_pointer configure_;
  private_data_type private_data_;
  log_key_compare_type log_key_compare_;

  // global
  std::unique_ptr<log_key_type> global_last_removed_;  // ignore all log lower than this key
  std::unique_ptr<log_key_type> global_ingore_;        // ignore all log lower than this key

  // logs(libstdc++ is 512Byte for each block and maintain block index just like std::vector)
  log_container_type logs_;
  std::list<std::pair<log_pointer, callback_param_type> > pending_logs_;

  // internal events
  callback_log_event_on_assign_fn_t internal_event_on_assign_;
  callback_load_fn_t internal_event_on_loaded_;
  callback_dump_fn_t internal_event_on_dumped_;
};

}  // namespace distributed_system
}  // namespace util
