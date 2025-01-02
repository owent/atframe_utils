// Copyright 2021 atframework
// Created by owent
// Stanards operations for Write Ahead Log

#pragma once

#include <design_pattern/nomovable.h>
#include <design_pattern/noncopyable.h>
#include <nostd/type_traits.h>

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <iterator>
#include <limits>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

#include "gsl/select-gsl.h"

#include "distributed_system/wal_common_defs.h"

#ifdef max
#  undef max
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace distributed_system {

template <class StorageT, class LogOperatorT, class CallbackParamT, class PrivateDataT>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY wal_object {
 public:
  // Delare the types from wal_log_operator, we use it to check types and keep ABI compatibility
  using storage_type = StorageT;
  using private_data_type = PrivateDataT;
  using log_operator_type = LogOperatorT;

  using log_type = typename log_operator_type::log_type;
  using log_pointer = typename log_operator_type::log_pointer;
  using log_const_pointer = typename log_operator_type::log_const_pointer;
  using log_key_type = typename log_operator_type::log_key_type;
  using log_key_compare_type = typename log_operator_type::log_key_compare_type;
  using action_getter_type = typename log_operator_type::action_getter_type;
  using action_case_type = typename log_operator_type::action_case_type;
  using log_key_result_type = typename log_operator_type::log_key_result_type;

  using hash_code_traits =
      wal_log_hash_code_traits<nostd::remove_cvref_t<log_key_type>, nostd::remove_cvref_t<log_type>>;
  using hash_code_type = typename hash_code_traits::hash_code_type;

  using log_allocator = typename log_operator_type::log_allocator;
  using log_pointer_allocator = typename log_operator_type::log_pointer_allocator;
  using log_container_type = std::deque<log_pointer, log_pointer_allocator>;
  using log_iterator = typename log_container_type::iterator;
  using log_const_iterator = typename log_container_type::const_iterator;
  using callback_param_type = CallbackParamT;
  using callback_param_storage_type = typename std::decay<callback_param_type>::type;
  using callback_param_lvalue_reference_type = typename std::add_lvalue_reference<callback_param_type>::type;
  using callback_param_rvalue_reference_type = typename std::add_rvalue_reference<callback_param_type>::type;
  using time_point = wal_time_point;
  using duration = wal_duration;
  using meta_type = wal_meta_type<log_key_type, action_case_type>;
  using meta_result_type = ATFRAMEWORK_UTILS_NAMESPACE_ID::design_pattern::result_type<meta_type, wal_result_code>;

  // Load data from storage into wal_object
  using callback_load_fn_t = std::function<wal_result_code(wal_object&, const storage_type&, callback_param_type)>;

  // Dump data from wal_object into storage
  using callback_dump_fn_t = std::function<wal_result_code(const wal_object&, storage_type&, callback_param_type)>;

  // Run log action
  using callback_log_action_fn_t = std::function<wal_result_code(wal_object&, const log_type&, callback_param_type)>;

  // Patch log
  using callback_log_patch_fn_t = std::function<wal_result_code(wal_object&, log_type&, callback_param_type)>;

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
  using callback_alloc_log_key_fn_t =
      std::function<log_key_result_type(wal_object&, const log_type&, callback_param_type)>;

  // Get hash code from a WAL log
  using callback_get_hash_code_fn_t = std::function<hash_code_type(const wal_object&, const log_type&)>;

  // Set hash code into a WAL log
  using callback_set_hash_code_fn_t = std::function<void(const wal_object&, log_type&, hash_code_type)>;

  // Calulate hash code from a WAL log
  using callback_calulate_hash_code_fn_t =
      std::function<hash_code_type(const wal_object&, hash_code_type previous, const log_type&)>;

  struct callback_log_fn_group_t {
    // The log action to patch a log, we can modify the log in this callback
    callback_log_patch_fn_t patch;

    // The log action callback
    callback_log_action_fn_t action;
  };
  using callback_log_group_map_t =
      std::unordered_map<action_case_type, callback_log_fn_group_t, typename log_operator_type::action_case_hash,
                         typename log_operator_type::action_case_equal>;

  struct vtable_type {
    callback_load_fn_t load;
    callback_dump_fn_t dump;
    callback_log_get_meta_fn_t get_meta;
    callback_log_set_meta_fn_t set_meta;
    callback_log_merge_fn_t merge_log;
    callback_get_log_key_fn_t get_log_key;
    callback_alloc_log_key_fn_t allocate_log_key;
    callback_log_event_fn_t on_log_added;
    callback_log_event_fn_t on_log_removed;

    // We can custom the hash function to let users to dicide how to keep the consistent of logs
    callback_get_hash_code_fn_t get_hash_code;
    callback_set_hash_code_fn_t set_hash_code;
    callback_calulate_hash_code_fn_t calculate_hash_code;

    // Log action callback dispatcher by action case
    callback_log_group_map_t log_action_delegate;
    callback_log_fn_group_t default_delegate;
  };
  using vtable_pointer = typename wal_mt_mode_data_trait<vtable_type, log_operator_type::mt_mode>::strong_ptr;

  struct configure_type {
    // GC expire duration
    duration gc_expire_duration;

    // Max log size
    size_t max_log_size;

    // The log size ti trigger gc
    size_t gc_log_size;

    // Force accept the log even if action callback return a error but the hash code is matched
    // We can use this in main-replicator mode to force accept a log for replication
    bool accept_log_when_hash_matched;
  };
  using configure_pointer = typename wal_mt_mode_data_trait<configure_type, log_operator_type::mt_mode>::strong_ptr;

 private:
  UTIL_DESIGN_PATTERN_NOMOVABLE(wal_object);
  UTIL_DESIGN_PATTERN_NOCOPYABLE(wal_object);

  /**
   * @brief Internal class to protect the access of wal_object's constructor
   */
  struct construct_helper {
    vtable_pointer vt;
    configure_pointer conf;
  };

  /**
   * @brief A alternative to gsl::finally
   */
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
  static typename wal_mt_mode_data_trait<wal_object, log_operator_type::mt_mode>::strong_ptr create(
      vtable_pointer vt, configure_pointer conf, ArgsT&&... args) {
    if (!vt || !conf) {
      return nullptr;
    }

    if (!vt->get_meta || !vt->get_log_key) {
      return nullptr;
    }

    construct_helper helper;
    helper.vt = vt;
    helper.conf = conf;
    return log_operator_type::template make_strong<wal_object>(helper, std::forward<ArgsT>(args)...);
  }

  static void default_configure(configure_type& out) {
    out.gc_expire_duration = std::chrono::duration_cast<duration>(std::chrono::hours(7 * 24));
    out.max_log_size = 512;
    out.gc_log_size = 128;
    out.accept_log_when_hash_matched = false;
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

    if (vtable_ && vtable_->get_hash_code && vtable_->set_hash_code && vtable_->calculate_hash_code) {
      hash_code_type hash_code = hash_code_traits::initial_hash_code();
      for (auto& log : logs_) {
        hash_code = vtable_->calculate_hash_code(*this, hash_code, *log);
        vtable_->set_hash_code(*this, *log, hash_code);
      }
    }

    if (internal_event_on_assign_) {
      internal_event_on_assign_(*this);
    }
  }

  /**
   * @brief Clear and assign all logs
   * @note This function is useful when loading data and will not trigger event and action callback
   *
   * @param source Data source
   */
  template <class ContainerT>
  void assign_logs(const ContainerT& source) {
    assign_logs(source.begin(), source.end());
  }

  /**
   * @brief Clear and assign all logs
   * @note this function is useful when loading data and will not trigger event and action callback
   *
   * @param source Data source
   */
  void assign_logs(log_container_type&& source) {
    logs_.swap(source);
    source.clear();

    if (vtable_ && vtable_->get_hash_code && vtable_->set_hash_code && vtable_->calculate_hash_code) {
      hash_code_type hash_code = hash_code_traits::initial_hash_code();
      for (auto& log : logs_) {
        hash_code = vtable_->calculate_hash_code(*this, hash_code, *log);
        vtable_->set_hash_code(*this, *log, hash_code);
      }
    }

    if (internal_event_on_assign_) {
      internal_event_on_assign_(*this);
    }
  }

  /**
   * @brief Allocate a new log instance, set meta data and key from callbacks
   *
   * @param now Current time point
   * @param action_case The default action case
   * @param param Callback parameter
   * @param args Arguments to construct log instance
   */
  template <class... ArgsT>
  log_pointer allocate_log(time_point now, action_case_type action_case, callback_param_type param, ArgsT&&... args) {
    if (!configure_ || !vtable_ || !vtable_->allocate_log_key || !vtable_->set_meta) {
      return nullptr;
    }

    log_pointer ret = log_operator_type::template make_strong<log_type>(std::forward<ArgsT>(args)...);
    if (!ret) {
      return ret;
    }

    log_key_result_type new_key = vtable_->allocate_log_key(*this, *ret, param);
    if (!new_key.is_success()) {
      return nullptr;
    }

    // Call the action to set meta data
    meta_type meta;
    meta.timepoint = now;
    meta.log_key = *new_key.get_success();
    meta.action_case = action_case;
    vtable_->set_meta(*this, *ret, meta);

    return ret;
  }

  /**
   * @brief Allocate a new log instance with custom allocator, set meta data and key from callbacks
   *
   * @param alloc Custom allocator
   * @param now Current time point
   * @param action_case The default action case
   * @param param Callback parameter
   * @param args Arguments to construct log instance
   */
  template <class Alloc, class... ArgsT>
  log_pointer allocate_log(const Alloc& alloc, time_point now, action_case_type action_case, callback_param_type param,
                           ArgsT&&... args) {
    if (!configure_ || !vtable_ || !vtable_->allocate_log_key || !vtable_->set_meta) {
      return nullptr;
    }

    log_pointer ret = log_operator_type::template allocate_strong<log_type>(alloc, std::forward<ArgsT>(args)...);
    if (!ret) {
      return ret;
    }

    log_key_result_type new_key = vtable_->allocate_log_key(*this, *ret, param);
    if (!new_key.is_success()) {
      return nullptr;
    }

    // Call the action to set meta data
    meta_type meta;
    meta.timepoint = now;
    meta.log_key = *new_key.get_success();
    meta.action_case = action_case;
    vtable_->set_meta(*this, *ret, meta);

    return ret;
  }

  /**
   * @brief emplace back a new log instance
   * @note this function will trigger event and action callback
   * @param log The log to emplace back
   * @param param The callback parameter
   * @return The result code
   */
  wal_result_code emplace_back(log_pointer&& log, callback_param_type param) {
    if (!log) {
      return wal_result_code::kInvalidParam;
    }

    // We append the logs to pending queue in recursive calls
    if (!pending_logs_.empty() || in_log_action_callback_) {
      pending_logs_.emplace_back(std::pair<log_pointer, callback_param_storage_type>{std::move(log), param});
      return wal_result_code::kPending;
    }

    in_log_action_callback_ = true;
    finally_helper guard(*this, [](wal_object& self) { self.in_log_action_callback_ = false; });

    // Directly push back log
    wal_result_code ret;
    if (!vtable_ || !vtable_->get_log_key) {
      ret = pusk_back_internal(std::move(log), param);
    } else {
      // Check the key to keep idempotence
      log_key_type this_key = vtable_->get_log_key(*this, *log);
      if (global_ingore_ && !log_key_compare_(*global_ingore_, this_key)) {
        ret = wal_result_code::kIgnore;
      } else {
        ret = pusk_back_internal(std::move(log), param);
      }
    }

    // Process pending logs in recursive calls
    while (!pending_logs_.empty()) {
      auto pending_log = pending_logs_.begin();

      if (!(*pending_log).first) {
        pending_logs_.erase(pending_log);
        continue;
      }

      if (vtable_ && vtable_->get_log_key) {
        // Check the key to keep idempotence
        log_key_type this_key = vtable_->get_log_key(*this, *(*pending_log).first);
        if (global_ingore_ && log_key_compare_(this_key, *global_ingore_)) {
          continue;
        }
      }

      pusk_back_internal(std::move((*pending_log).first), (*pending_log).second);
      pending_logs_.erase(pending_log);
    }

    // Auto gc for max size
    if (configure_ && configure_->max_log_size > 0) {
      size_t max_log_size = configure_->max_log_size;
      while (logs_.size() > max_log_size) {
        pop_front_internal();
      }
    }

    return ret;
  }

  /**
   * @brief push back a new log instance
   * @note this function will trigger event and action callback
   * @param log The log to emplace back
   * @param param The callback parameter
   * @return The result code
   */
  wal_result_code push_back(log_pointer log, callback_param_type param) { return emplace_back(std::move(log), param); }

  /**
   * @brief Custom remove logs before a time point
   * @param now The time point to remove before
   * @param max_count Max log count to remove in one call
   * @return The result code
   */
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
        pop_front_internal();
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

      // Only trigger GC when logs size is greater than gc_log_size
      if (logs_.size() <= gc_log_size) {
        break;
      }

      // Keep the logs size less than max_log_size
      if (logs_.size() > max_log_size) {
        pop_front_internal();
        continue;
      }

      if (!vtable_ || !vtable_->get_meta) {
        pop_front_internal();
        continue;
      }

      // Actually remove log
      meta_result_type meta = vtable_->get_meta(*this, **logs_.begin());
      if (meta.is_error() || meta.is_none()) {
        pop_front_internal();
      } else if (meta.is_success() && meta.get_success()->timepoint + gc_expire_duration <= now) {
        if (nullptr != hold && log_key_compare_(*hold, meta.get_success()->log_key)) {
          break;
        }
        pop_front_internal();
      } else {
        break;
      }
    }

    return ret;
  }

  /**
   * @brief Get the log key to ignore logs with key less than or equal to the key
   * @return The log key or nullptr if not set
   */
  const log_key_type* get_global_ingore_key() const noexcept { return global_ingore_.get(); }

  /**
   * @brief Set ignore logs with key less than or equal to the given key
   * @param key The key since what to ignore logs
   */
  template <class ToKey>
  void set_global_ingore_key(ToKey&& key) {
    if (global_ingore_) {
      *global_ingore_ = std::forward<ToKey>(key);
    } else {
      global_ingore_.reset(new log_key_type{std::forward<ToKey>(key)});
    }
  }

  /**
   * @brief Get the last removed log key
   * @return The last removed log key or nullptr if not set
   */
  const log_key_type* get_last_removed_key() const noexcept { return global_last_removed_.get(); }

  /**
   * @brief Set the last removed log key
   * @param key The key to set
   */
  template <class ToKey>
  void set_last_removed_key(ToKey&& key) {
    if (global_last_removed_) {
      *global_last_removed_ = std::forward<ToKey>(key);
    } else {
      global_last_removed_.reset(new log_key_type{std::forward<ToKey>(key)});
    }
  }

  /**
   * @brief Get the last finished log key
   * @return The last finished log key or nullptr if not set
   */
  inline const log_container_type& get_all_logs() const noexcept { return logs_; }

  /**
   * @brief Get the private data
   * @return The private data
   */
  inline const private_data_type& get_private_data() const noexcept { return private_data_; }

  /**
   * @brief Get the private data
   * @return The private data
   */
  inline private_data_type& get_private_data() noexcept { return private_data_; }

  /**
   * @brief Get the log key compare function
   * @return The log key compare function
   */
  inline const log_key_compare_type& get_log_key_compare() const noexcept { return log_key_compare_; }

  /**
   * @brief Get the log key compare function
   * @return The log key compare function
   */
  inline log_key_compare_type& get_log_key_compare() noexcept { return log_key_compare_; }

  /**
   * @brief Get the configure
   * @return The configure
   */
  inline const configure_type& get_configure() const noexcept {
    // We can not create wal_object without configure, so it's safe here
    return *configure_;
  }

  /**
   * @brief Get the configure
   * @return The configure
   */
  inline configure_type& get_configure() noexcept {
    // We can not create wal_object without configure, so it's safe here
    return *configure_;
  }

  /**
   * @brief Get the vtable
   * @return The vtable
   */
  inline const vtable_type& get_vtable() const noexcept {
    // We can not create wal_object without vtable, so it's safe here
    return *vtable_;
  }

  /**
   * @brief Get the log by key
   * @return Log or nullptr if not found
   */
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

  /**
   * @brief Get the log by key
   * @return Log or nullptr if not found
   */
  log_const_pointer find_log(const log_key_type& key) const noexcept {
    log_const_iterator iter = log_lower_bound(key);
    if (iter == logs_.end()) {
      return nullptr;
    }

    log_key_type found_key = vtable_->get_log_key(*this, **iter);
    if (!log_key_compare_(key, found_key) && !log_key_compare_(found_key, key)) {
      return wal_mt_mode_func_trait<log_operator_type::mt_mode>::template const_pointer_cast<const log_type>(*iter);
    }

    return nullptr;
  }

  /**
   * @brief Begin iterator of all logs
   * @return Begin iterator
   */
  inline log_iterator log_begin() noexcept { return logs_.begin(); }

  /**
   * @brief End iterator of all logs
   * @return End iterator
   */
  inline log_iterator log_end() noexcept { return logs_.end(); }

  /**
   * @brief Const begin iterator of all logs
   * @return Const begin iterator
   */
  inline log_const_iterator log_cbegin() const noexcept { return logs_.begin(); }

  /**
   * @brief Const end iterator of all logs
   * @return Const end iterator
   */
  inline log_const_iterator log_cend() const noexcept { return logs_.end(); }

  /**
   * @brief Lower bound iterator by key
   * @param key The key to find
   * @return Lower bound iterator or end iterator if not found
   */
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

  /**
   * @brief Lower bound iterator by key
   * @param key The key to find
   * @return Lower bound iterator or end iterator if not found
   */
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

  /**
   * @brief Upper bound iterator by key
   * @param key The key to find
   * @return Upper bound iterator or end iterator if not found
   */
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

  /**
   * @brief Upper bound iterator by key
   * @param key The key to find
   * @return Upper bound iterator or end iterator if not found
   */
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

  /**
   * @brief Get begin and end iterator of all logs
   * @return (begin, end)
   */
  std::pair<log_iterator, log_iterator> log_all_range() noexcept {
    return std::pair<log_iterator, log_iterator>(logs_.begin(), logs_.end());
  }

  /**
   * @brief Get begin and end iterator of all logs
   * @return (begin, end)
   */
  std::pair<log_const_iterator, log_const_iterator> log_all_range() const noexcept {
    return std::pair<log_const_iterator, log_const_iterator>(logs_.begin(), logs_.end());
  }

  /**
   * @brief Get previous log key before the given key, it's usually used to calculate the next hash code
   * @return hash code
   */
  hash_code_type get_hash_code_before(const log_key_type& key) const noexcept {
    if (logs_.empty()) {
      return hash_code_traits::initial_hash_code();
    }

    if (!vtable_ || !vtable_->get_hash_code || !vtable_->get_log_key) {
      return hash_code_traits::initial_hash_code();
    }

    // Return the last hash code if the key is greater than the last log key
    log_key_type last_key = vtable_->get_log_key(*this, **logs_.rbegin());
    if (log_key_compare_(last_key, key)) {
      return vtable_->get_hash_code(*this, **logs_.rbegin());
    }

    auto iter = log_lower_bound(key);
    if (iter == logs_.begin()) {
      return hash_code_traits::initial_hash_code();
    }
    --iter;
    return vtable_->get_hash_code(*this, **iter);
  }

 private:
  /**
   * @brief Acutally do or redo the log
   * @param log The log to redo
   * @param param The callback parameter
   * @return The result code
   */
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

    wal_result_code ret = wal_result_code::kActionNotSet;
    do {
      // Dispatch the log action by action case
      auto iter = vtable_->log_action_delegate.find(meta.get_success()->action_case);
      if (iter != vtable_->log_action_delegate.end() && (iter->second.patch || iter->second.action)) {
        if (iter->second.patch) {
          ret = iter->second.patch(*this, *log, param);
          if (ret != wal_result_code::kOk) {
            break;
          }
        }
        if (iter->second.action) {
          ret = iter->second.action(*this, *log, param);
        }
        break;
      }

      // Fallback log action
      if (vtable_->default_delegate.patch) {
        ret = vtable_->default_delegate.patch(*this, *log, param);
        if (ret != wal_result_code::kOk) {
          break;
        }
      }

      if (vtable_->default_delegate.action) {
        ret = vtable_->default_delegate.action(*this, *log, param);
      }
    } while (false);
    return ret;
  }

  wal_result_code pusk_back_internal_uncheck(log_pointer&& log, callback_param_lvalue_reference_type param) {
    // Reset hash  code
    if (vtable_ && vtable_->set_hash_code && vtable_->get_hash_code && vtable_->calculate_hash_code &&
        vtable_->get_log_key) {
      hash_code_type hash_code = hash_code_traits::initial_hash_code();
      for (auto iter = logs_.rbegin(); iter != logs_.rend(); ++iter) {
        if (*iter) {
          hash_code = vtable_->get_hash_code(*this, **iter);
          break;
        }
      }
      hash_code = vtable_->calculate_hash_code(*this, hash_code, *log);
      vtable_->set_hash_code(*this, *log, hash_code);
    }

    // Do actions
    wal_result_code ret = redo_log(log, param);
    if (wal_result_code::kOk != ret && !(get_configure().accept_log_when_hash_matched && vtable_->set_hash_code &&
                                         vtable_->get_hash_code && vtable_->calculate_hash_code)) {
      return ret;
    }

    // Callback and push back log
    if (internal_event_on_add_log_) {
      internal_event_on_add_log_(*this, log);
    }

    logs_.push_back(log);
    if (vtable_ && vtable_->on_log_added) {
      vtable_->on_log_added(*this, log);
    }

    return ret;
  }

  wal_result_code pusk_back_internal(log_pointer&& log, callback_param_lvalue_reference_type param) {
    if (!log) {
      return wal_result_code::kInvalidParam;
    }

    if (!vtable_) {
      return wal_result_code::kInvalidParam;
    }

    // Empty
    if (logs_.empty()) {
      return pusk_back_internal_uncheck(std::move(log), param);
    }

    // Can not get log key
    if (!vtable_ || !vtable_->get_log_key) {
      return pusk_back_internal_uncheck(std::move(log), param);
    }

    log_key_type last_key = vtable_->get_log_key(*this, *logs_.back());
    log_key_type this_key = vtable_->get_log_key(*this, *log);
    // Has log key
    //   -- push_back
    if (log_key_compare_(last_key, this_key)) {
      return pusk_back_internal_uncheck(std::move(log), param);
    }
    //   -- insert
    auto iter =
        std::lower_bound(logs_.begin(), logs_.end(), this_key, [this](const log_pointer& l, const log_key_type& r) {
          log_key_type log_key = this->vtable_->get_log_key(*this, *l);
          return this->log_key_compare_(log_key, r);
        });

    if (iter != logs_.end()) {
      // Merge log if it's already exists
      last_key = vtable_->get_log_key(*this, **iter);
      if (!log_key_compare_(last_key, this_key) && !log_key_compare_(this_key, last_key)) {
        if (vtable_->merge_log) {
          if (vtable_->set_hash_code && vtable_->get_hash_code) {
            hash_code_type hash_code = vtable_->get_hash_code(*this, **iter);
            vtable_->merge_log(*this, param, **iter, *log);
            vtable_->set_hash_code(*this, **iter, hash_code);
          } else {
            vtable_->merge_log(*this, param, **iter, *log);
          }
        }
        return wal_result_code::kMerge;
      }
    }

    // Reset hash code
    if (vtable_->set_hash_code && vtable_->get_hash_code && vtable_->calculate_hash_code) {
      hash_code_type hash_code = hash_code_traits::initial_hash_code();
      if (iter == logs_.end()) {
        for (auto last_iter = logs_.rbegin(); last_iter != logs_.rend(); ++last_iter) {
          if (*last_iter) {
            hash_code = vtable_->get_hash_code(*this, **last_iter);
            break;
          }
        }
      } else if (iter != logs_.begin()) {
        auto previous_iter = iter;
        --previous_iter;
        hash_code = vtable_->get_hash_code(*this, **previous_iter);
      }
      hash_code = vtable_->calculate_hash_code(*this, hash_code, *log);
      vtable_->set_hash_code(*this, *log, hash_code);
    }

    wal_result_code ret = redo_log(log, param);
    if (wal_result_code::kOk != ret && !(get_configure().accept_log_when_hash_matched && vtable_->set_hash_code &&
                                         vtable_->get_hash_code && vtable_->calculate_hash_code)) {
      return ret;
    }

    if (vtable_->set_hash_code && vtable_->get_hash_code && vtable_->calculate_hash_code) {
      // Update next hash codes when got a hole log
      hash_code_type hash_code = vtable_->get_hash_code(*this, *log);
      for (auto fix_iter = iter; fix_iter != logs_.end(); ++fix_iter) {
        hash_code = vtable_->calculate_hash_code(*this, hash_code, **fix_iter);
        vtable_->set_hash_code(*this, **fix_iter, hash_code);
      }
    }

    // Callback and push back log
    if (internal_event_on_add_log_) {
      internal_event_on_add_log_(*this, log);
    }

    logs_.insert(iter, log);
    if (vtable_ && vtable_->on_log_added) {
      vtable_->on_log_added(*this, log);
    }
    return ret;
  }

  void pop_front_internal() {
    if (logs_.empty()) {
      return;
    }

    log_pointer log = logs_.front();
    logs_.pop_front();

    if (vtable_ && vtable_->get_log_key) {
      // Update last removed key, so we will send back a snapshot if the subscriber is out of date
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
  template <class, class, class, class, class>
  friend class wal_client;

  using callback_log_event_on_assign_fn_t = std::function<void(wal_object&)>;
  using callback_log_event_on_add_log_fn_t = std::function<void(wal_object&, const log_pointer&)>;

  void set_internal_event_on_assign_logs(callback_log_event_on_assign_fn_t fn) { internal_event_on_assign_ = fn; }

  void set_internal_event_on_assign_logs(callback_log_event_on_add_log_fn_t fn) { internal_event_on_add_log_ = fn; }

  void set_internal_event_on_loaded(callback_load_fn_t fn) { internal_event_on_loaded_ = fn; }

  void set_internal_event_on_dumped(callback_dump_fn_t fn) { internal_event_on_dumped_ = fn; }

 private:
  bool in_log_action_callback_;
  vtable_pointer vtable_;
  configure_pointer configure_;
  private_data_type private_data_;
  log_key_compare_type log_key_compare_;

  // global
  std::unique_ptr<log_key_type> global_last_removed_;  // ignore all log lower than this key
  std::unique_ptr<log_key_type> global_ingore_;        // ignore all log lower than this key

  // logs(libstdc++ is 512Byte for each block and maintain block index just like std::vector)
  log_container_type logs_;
  using pending_log_allocator = typename std::allocator_traits<log_allocator>::template rebind_alloc<
      std::pair<log_pointer, callback_param_storage_type>>;
  std::list<std::pair<log_pointer, callback_param_storage_type>, pending_log_allocator> pending_logs_;

  // internal events
  callback_log_event_on_assign_fn_t internal_event_on_assign_;
  callback_log_event_on_add_log_fn_t internal_event_on_add_log_;
  callback_load_fn_t internal_event_on_loaded_;
  callback_dump_fn_t internal_event_on_dumped_;
};

}  // namespace distributed_system
ATFRAMEWORK_UTILS_NAMESPACE_END
