// Copyright 2021 atframework
// Created by owent
// Stanards operations for Write Ahead Log subscriber

#pragma once

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <list>
#include <memory>
#include <unordered_map>
#include <utility>

#include "distributed_system/wal_common_defs.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace distributed_system {

template <class PrivateDataT, class KeyT, class HashSubscriberKeyT = std::hash<KeyT>,
          class EqualSubscriberKeyT = std::equal_to<KeyT>, class Allocator = std::allocator<KeyT>,
          wal_mt_mode MTMode = wal_mt_mode::kMultiThread>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY wal_subscriber {
 public:
  using pointer = typename wal_mt_mode_data_trait<wal_subscriber, MTMode>::strong_ptr;

  using private_data_type = PrivateDataT;

  using key_type = KeyT;
  using key_hash = HashSubscriberKeyT;
  using key_equal = EqualSubscriberKeyT;
  using time_point = wal_time_point;
  using duration = wal_duration;

  using collector_allocator =
      typename std::allocator_traits<Allocator>::template rebind_alloc<std::pair<const key_type, pointer>>;
  using subscriber_collector_type = std::unordered_map<key_type, pointer, key_hash, key_equal, collector_allocator>;
  using subscriber_iterator = typename subscriber_collector_type::iterator;
  using subscriber_const_iterator = typename subscriber_collector_type::const_iterator;

  struct timer_type {
    time_point timeout;
    typename wal_mt_mode_data_trait<wal_subscriber, MTMode>::weak_ptr subscriber;
  };

 private:
  UTIL_DESIGN_PATTERN_NOMOVABLE(wal_subscriber);
  UTIL_DESIGN_PATTERN_NOCOPYABLE(wal_subscriber);
  struct construct_helper {};

  friend class manager;

 public:
  class manager {
   private:
    UTIL_DESIGN_PATTERN_NOMOVABLE(manager);
    UTIL_DESIGN_PATTERN_NOCOPYABLE(manager);

    void remove_subscriber_timer(typename std::list<timer_type>::iterator& out) {
      if (out == subscribers_timer_.end()) {
        return;
      }

      subscribers_timer_.erase(out);
      out = subscribers_timer_.end();
    }

    void insert_subscriber_timer(const time_point& now, const pointer& subscriber) {
      if (!subscriber) {
        return;
      }

      if (this != subscriber->owner_) {
        return;
      }

      if (subscriber->timer_handle_ != subscribers_timer_.end()) {
        remove_subscriber_timer(subscriber->timer_handle_);
      }

      timer_type timer;
      timer.timeout = now + subscriber->get_heartbeat_timeout();
      timer.subscriber = subscriber;

      subscriber->timer_handle_ = subscribers_timer_.insert(subscribers_timer_.end(), timer);
    }

    void unbind_owner(wal_subscriber& subscriber) {
      if (nullptr != subscriber.owner_) {
        subscriber.owner_->remove_subscriber_timer(subscriber.timer_handle_);
        subscriber.owner_ = nullptr;
      }
    }

   public:
    manager() {}

    void reset_timer(const pointer& subscriber, const time_point& now) {
      if (!subscriber) {
        return;
      }

      if (subscriber->owner_ != this) {
        return;
      }

      remove_subscriber_timer(subscriber->timer_handle_);
      insert_subscriber_timer(now, subscriber);
    }

    void subscribe(const pointer& subscriber, const time_point& now) {
      if (!subscriber) {
        return;
      }

      if (subscriber->owner_ != this) {
        return;
      }

      subscriber->update_heartbeat_time_point(now);
      reset_timer(subscriber, now);
    }

    pointer unsubscribe(const key_type& key, wal_unsubscribe_reason) {
      auto iter = subscribers_.find(key);
      if (iter == subscribers_.end()) {
        return nullptr;
      }

      pointer subscriber = iter->second;
      // Remove from collector
      subscribers_.erase(iter);

      // Unbind
      if (subscriber) {
        unbind_owner(*subscriber);
      }

      return subscriber;
    }

    pointer unsubscribe(const pointer& subscriber, wal_unsubscribe_reason) {
      if (!subscriber) {
        return nullptr;
      }

      auto iter = subscribers_.find(subscriber->get_key());
      if (iter == subscribers_.end()) {
        return nullptr;
      }

      if (iter->second != subscriber) {
        return nullptr;
      }

      // Remove from collector
      subscribers_.erase(iter);

      // Unbind
      if (subscriber) {
        unbind_owner(*subscriber);
      }

      return subscriber;
    }

    /**
     * @brief Allocate a new subscriber with custom allocator
     * @param alloc Custom allocator
     * @param key The key to allocate
     * @param now Current time point
     * @param timeout The timeout duration
     * @param args Arguments to construct subscriber instance
     * @return The allocated subscriber
     */
    template <class Alloc, class... ArgsT>
    pointer allocate(const Alloc& alloc, const key_type& key, const time_point& now, const duration& timeout,
                     ArgsT&&... args) {
      auto old = subscribers_.find(key);
      // if found, just update the timeout
      if (old != subscribers_.end()) {
        if (!old->second) {
          subscribers_.erase(old);
        } else {
          old->second->update_heartbeat_time_point(now);
          old->second->set_heartbeat_timeout(timeout);
          return old->second;
        }
      }

      // Create a new subscriber and insert timer
      construct_helper guard;
      auto ret = wal_mt_mode_func_trait<MTMode>::template allocate_strong<wal_subscriber>(
          alloc, guard, *this, key, now, timeout, subscribers_timer_.end(), std::forward<ArgsT>(args)...);
      if (!ret) {
        return ret;
      }

      subscribers_[key] = ret;
      insert_subscriber_timer(now, ret);

      return ret;
    }

    /**
     * @brief Allocate a new subscriber with default allocator
     * @param alloc Custom allocator
     * @param key The key to allocate
     * @param now Current time point
     * @param timeout The timeout duration
     * @param args Arguments to construct subscriber instance
     * @return The allocated subscriber
     */
    template <class... ArgsT>
    pointer create(const key_type& key, const time_point& now, const duration& timeout, ArgsT&&... args) {
      using subscriber_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<wal_subscriber>;
      return allocate(subscriber_allocator(), key, now, timeout, std::forward<ArgsT>(args)...);
    }

    /**
     * @brief Find a subscriber by key
     * @param key The key to find
     * @return The subscriber if found, nullptr if not found
     */
    pointer find(const key_type& key) noexcept {
      auto iter = subscribers_.find(key);
      if (iter == subscribers_.end()) {
        return nullptr;
      }

      return iter->second;
    }

    /**
     * @brief Find a subscriber by key
     * @param key The key to find
     * @return The subscriber iterator range if found, end iterator if not found
     */
    std::pair<subscriber_iterator, subscriber_iterator> find_iterator(const key_type& key) noexcept {
      auto iter = subscribers_.find(key);
      if (iter == subscribers_.end()) {
        return std::pair<subscriber_iterator, subscriber_iterator>(subscribers_.end(), subscribers_.end());
      }

      subscriber_iterator end = iter;
      ++end;
      return std::pair<subscriber_iterator, subscriber_iterator>(iter, end);
    }

    /**
     * @brief Find a subscriber by key
     * @param key The key to find
     * @return The subscriber iterator range if found, end iterator if not found
     */
    std::pair<subscriber_const_iterator, subscriber_const_iterator> find_iterator(const key_type& key) const noexcept {
      auto iter = subscribers_.find(key);
      if (iter == subscribers_.end()) {
        return std::pair<subscriber_const_iterator, subscriber_const_iterator>(subscribers_.end(), subscribers_.end());
      }

      subscriber_const_iterator end = iter;
      ++end;
      return std::pair<subscriber_const_iterator, subscriber_const_iterator>(iter, end);
    }

    /**
     * @brief Get all subscribers
     * @return The subscriber iterator range
     */
    std::pair<subscriber_iterator, subscriber_iterator> all_range() noexcept {
      return std::pair<subscriber_iterator, subscriber_iterator>(subscribers_.begin(), subscribers_.end());
    }

    /**
     * @brief Get all subscribers
     * @return The subscriber iterator range
     */
    std::pair<subscriber_const_iterator, subscriber_const_iterator> all_range() const noexcept {
      return std::pair<subscriber_const_iterator, subscriber_const_iterator>(subscribers_.begin(), subscribers_.end());
    }

    /**
     * @brief Get the first expired subscriber
     * @param now Current time point
     * @return The first expired subscriber if found, nullptr if not found
     */
    pointer get_first_expired(const time_point& now) {
      while (!subscribers_timer_.empty()) {
        auto begin = subscribers_timer_.begin();
        if ((*begin).timeout >= now) {
          break;
        }

        if ((*begin).subscriber.expired()) {
          subscribers_timer_.erase(begin);
          continue;
        }

        pointer subscriber = (*begin).subscriber.lock();
        if (!subscriber) {
          subscribers_timer_.erase(begin);
          continue;
        }

        // duplicated iterator
        if (subscriber->timer_handle_ != begin) {
          subscribers_timer_.erase(begin);
          continue;
        }

        return subscriber;
      }

      return nullptr;
    }

   private:
    using timer_allocator = typename std::allocator_traits<Allocator>::template rebind_alloc<timer_type>;
    std::list<timer_type, timer_allocator> subscribers_timer_;
    subscriber_collector_type subscribers_;
  };

 public:
  template <class... ArgsT>
  wal_subscriber(construct_helper&, manager& owner, const key_type& key, const time_point& now, const duration& timeout,
                 typename std::list<timer_type>::iterator timer_iter, ArgsT&&... args)
      : owner_(&owner),
        key_(key),
        last_heartbeat_timepoint_(now),
        heartbeat_timeout_(timeout),
        private_data_{std::forward<ArgsT>(args)...},
        timer_handle_(timer_iter) {}

  inline const key_type& get_key() const noexcept { return key_; }

  inline const private_data_type& get_private_data() const noexcept { return private_data_; }
  inline private_data_type& get_private_data() noexcept { return private_data_; }

  inline duration get_heartbeat_timeout() const noexcept { return heartbeat_timeout_; }
  inline void set_heartbeat_timeout(duration timeout) noexcept { heartbeat_timeout_ = timeout; }

  inline time_point get_last_heartbeat_time_point() const noexcept { return last_heartbeat_timepoint_; }
  inline void update_heartbeat_time_point(time_point tp) noexcept { last_heartbeat_timepoint_ = tp; }

  inline bool is_offline(const time_point& now) const noexcept {
    return last_heartbeat_timepoint_ + heartbeat_timeout_ <= now;
  }

 private:
  manager* owner_;
  key_type key_;
  time_point last_heartbeat_timepoint_;
  duration heartbeat_timeout_;
  private_data_type private_data_;

  typename std::list<timer_type>::iterator timer_handle_;
};

template <class PrivateDataT, class KeyT, wal_mt_mode MTMode, class HashSubscriberKeyT = std::hash<KeyT>,
          class EqualSubscriberKeyT = std::equal_to<KeyT>, class Allocator = std::allocator<KeyT>>
using wal_subscriber_with_mt_mode =
    wal_subscriber<PrivateDataT, KeyT, HashSubscriberKeyT, EqualSubscriberKeyT, Allocator, MTMode>;

}  // namespace distributed_system
ATFRAMEWORK_UTILS_NAMESPACE_END
