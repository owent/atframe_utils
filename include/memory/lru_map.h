/**
 * @file lru_map.h
 * @brief lru 算法的map<br />
 *        对于单一简单类型的数据结构，不需要使用多对象管理的 lru_object_pool.h ， 直接用这个即可
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2019-09-30
 *
 * @history
 *
 */

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compiler_features.h>
#include <memory/rc_ptr.h>

#include <stdint.h>
#include <cstddef>
#include <functional>
#include <list>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace memory {

template <compat_strong_ptr_mode PtrMode>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY lru_map_option {
  static UTIL_CONFIG_CONSTEXPR const compat_strong_ptr_mode ptr_mode = PtrMode;
};

template <class TKEY, class TVALUE, class TOption = lru_map_option<compat_strong_ptr_mode::kStl>>
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY lru_map_type_traits {
  using key_type = TKEY;
  using mapped_type = TVALUE;
  using option_type = TOption;
  using store_type = typename compat_strong_ptr_function_trait<option_type::ptr_mode>::template shared_ptr<TVALUE>;
  using value_type = std::pair<const TKEY, store_type>;
  using size_type = size_t;

  using list_type = std::list<value_type>;
  using iterator = typename list_type::iterator;
  using const_iterator = typename list_type::const_iterator;
  using reverse_iterator = typename list_type::reverse_iterator;
  using const_reverse_iterator = typename list_type::const_reverse_iterator;
};

template <class TKEY, class TVALUE, class THasher = std::hash<TKEY>, class TKeyEQ = std::equal_to<TKEY>,
          class TOption = lru_map_option<compat_strong_ptr_mode::kStl>,
          class TAlloc =
              std::allocator<std::pair<const TKEY, typename lru_map_type_traits<TKEY, TVALUE, TOption>::iterator>>>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY lru_map {
 public:
  using key_type = typename lru_map_type_traits<TKEY, TVALUE, TOption>::key_type;
  using mapped_type = typename lru_map_type_traits<TKEY, TVALUE, TOption>::mapped_type;
  using value_type = typename lru_map_type_traits<TKEY, TVALUE, TOption>::value_type;
  using size_type = typename lru_map_type_traits<TKEY, TVALUE, TOption>::size_type;
  using store_type = typename lru_map_type_traits<TKEY, TVALUE, TOption>::store_type;
  using option_type = typename lru_map_type_traits<TKEY, TVALUE, TOption>::option_type;
  using allocator_type = TAlloc;
  using reference = value_type &;
  using const_reference = const value_type &;
  using pointer = value_type *;
  using const_pointer = const value_type *;
  using lru_history_list_type = typename lru_map_type_traits<TKEY, TVALUE, TOption>::list_type;
  using iterator = typename lru_map_type_traits<TKEY, TVALUE, TOption>::iterator;
  using const_iterator = typename lru_map_type_traits<TKEY, TVALUE, TOption>::const_iterator;
  using reverse_iterator = typename lru_map_type_traits<TKEY, TVALUE, TOption>::reverse_iterator;
  using const_reverse_iterator = typename lru_map_type_traits<TKEY, TVALUE, TOption>::const_reverse_iterator;

  using lru_key_value_map_type = std::unordered_map<TKEY, iterator, THasher, TKeyEQ, TAlloc>;
  using self_type = lru_map<TKEY, TVALUE, THasher, TKeyEQ, TOption, TAlloc>;

  lru_map() noexcept(std::is_nothrow_constructible<lru_history_list_type>::value &&
                     std::is_nothrow_constructible<lru_key_value_map_type>::value) {}

  template <class TCONTAINER>
  ATFRAMEWORK_UTILS_API_HEAD_ONLY lru_map(const TCONTAINER &other) {
    reserve(static_cast<size_type>(other.size()));
    insert(other.begin(), other.end());
  }

  lru_map(const lru_map &other) {
    reserve(static_cast<size_type>(other.size()));
    insert(other.cbegin(), other.cend());
  }

  lru_map(lru_map &&other) noexcept(
      std::is_nothrow_constructible<lru_history_list_type, lru_history_list_type &&>::value &&
      std::is_nothrow_constructible<lru_key_value_map_type, lru_key_value_map_type &&>::value) {
    swap(other);
  }

  lru_map &operator=(const lru_map &other) {
    clear();
    reserve(static_cast<size_type>(other.size()));
    insert(other.cbegin(), other.cend());
    return *this;
  }

  lru_map &operator=(lru_map &&other) noexcept(
      std::is_nothrow_constructible<lru_history_list_type, lru_history_list_type &&>::value &&
      std::is_nothrow_constructible<lru_key_value_map_type, lru_key_value_map_type &&>::value) {
    swap(other);
    other.clear();
    return *this;
  }

  inline iterator begin() { return visit_history_.begin(); }
  inline const_iterator cbegin() const { return visit_history_.cbegin(); }
  inline iterator end() { return visit_history_.end(); }
  inline const_iterator cend() const { return visit_history_.cend(); }

  inline reverse_iterator rbegin() { return visit_history_.rbegin(); }
  inline const_reverse_iterator crbegin() const { return visit_history_.crbegin(); }
  inline reverse_iterator rend() { return visit_history_.rend(); }
  inline const_reverse_iterator crend() const { return visit_history_.crend(); }

  inline value_type &front() { return visit_history_.front(); }
  inline const value_type &front() const { return visit_history_.front(); }
  inline value_type &back() { return visit_history_.back(); }
  inline const value_type &back() const { return visit_history_.back(); }

  void pop_front() {
    if (visit_history_.empty()) {
      return;
    }
    erase(begin());
  }

  void pop_back() {
    if (visit_history_.empty()) {
      return;
    }
    erase(back().first);
  }

  inline bool empty() const { return kv_data_.empty(); }
  inline size_type size() const { return kv_data_.size(); }

#if defined(ATFRAMEWORK_UTILS_UNORDERED_MAP_SET_HAS_RESERVE) && ATFRAMEWORK_UTILS_UNORDERED_MAP_SET_HAS_RESERVE
  inline void reserve(size_type s) { kv_data_.reserve(s); }

#else
  inline void reserve(size_type) { /* do nothing, some old compiler don't support this. */ }
#endif

  void swap(self_type &other) {
    other.visit_history_.swap(visit_history_);
    other.kv_data_.swap(kv_data_);
  }

  void clear() {
    kv_data_.clear();
    visit_history_.clear();
  }

  template <class TPARAMKEY, class TPARAMVALUE>
  ATFRAMEWORK_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(const TPARAMKEY &key,
                                                                             const TPARAMVALUE &copy_value) {
    using alloc_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<mapped_type>;
    return insert_key_value(
        key, compat_strong_ptr_function_trait<option_type::ptr_mode>::template allocate_shared<mapped_type>(
                 alloc_type(), copy_value));
  }

  template <class TPARAMKEY, class TPARAMVALUE>
  ATFRAMEWORK_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(
      const TPARAMKEY &key,
      const typename compat_strong_ptr_function_trait<option_type::ptr_mode>::template shared_ptr<TPARAMVALUE> &value) {
    typename lru_key_value_map_type::iterator it = kv_data_.find(key);
    if (it != kv_data_.end()) {
      return std::pair<iterator, bool>(visit_history_.end(), false);
    }

    typename lru_history_list_type::iterator res = visit_history_.insert(visit_history_.end(), value_type(key, value));
    if (res == visit_history_.end()) {
      return std::pair<iterator, bool>(res, false);
    }
    kv_data_[key] = res;
    return std::pair<iterator, bool>(res, true);
  }

  template <class TCKEY, class TCVALUE>
  ATFRAMEWORK_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert(const std::pair<TCKEY, TCVALUE> &value) {
    return insert_key_value(value.first, value.second);
  }

  std::pair<iterator, bool> insert(value_type &&value) {
    key_type key = value.first;
    typename lru_key_value_map_type::iterator it = kv_data_.find(key);
    if (it != kv_data_.end()) {
      return std::pair<iterator, bool>(visit_history_.end(), false);
    }

    typename lru_history_list_type::iterator res = visit_history_.insert(visit_history_.end(), value);
    if (res == visit_history_.end()) {
      return std::pair<iterator, bool>(res, false);
    }
    kv_data_[key] = res;
    return std::pair<iterator, bool>(res, true);
  }

  template <class TPARAMKEY, class TPARAMVALUE>
  ATFRAMEWORK_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(
      TPARAMKEY &&key,
      typename compat_strong_ptr_function_trait<option_type::ptr_mode>::template shared_ptr<TPARAMVALUE> &&value) {
    return insert(value_type(std::forward<TPARAMKEY>(key), value));
  }

  template <class TPARAMKEY, class TPARAMVALUE>
  ATFRAMEWORK_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(
      TPARAMKEY &&key,
      typename compat_strong_ptr_function_trait<option_type::ptr_mode>::template shared_ptr<TPARAMVALUE> &value) {
    return insert(value_type(std::forward<TPARAMKEY>(key), value));
  }

  template <class TPARAMKEY, class TPARAMVALUE>
  ATFRAMEWORK_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(TPARAMKEY &&key,
                                                                             TPARAMVALUE &&copy_value) {
    using alloc_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<mapped_type>;
    return insert(
        value_type(std::forward<TPARAMKEY>(key),
                   compat_strong_ptr_function_trait<option_type::ptr_mode>::template allocate_shared<mapped_type>(
                       alloc_type(), std::forward<TPARAMVALUE>(copy_value))));
  }

  template <class InputIt>
  ATFRAMEWORK_UTILS_API_HEAD_ONLY void insert(InputIt first, InputIt last) {
    while (first != last) {
      insert(*(first++));
    }
  }

  iterator erase(iterator pos) {
    if (pos == visit_history_.end()) {
      return visit_history_.end();
    }

    key_type key = (*pos).first;
    typename lru_key_value_map_type::iterator it = kv_data_.find(key);
    if (it == kv_data_.end() || it->second != pos) {
      return visit_history_.end();
    }

    kv_data_.erase(it);
    return visit_history_.erase(pos);
  }

  iterator erase(iterator first, iterator last) {
    iterator ret = visit_history_.end();
    while (first != last) {
      ret = erase(first++);
    }
    return ret;
  }

  size_type erase(const key_type &key) {
    typename lru_key_value_map_type::iterator it = kv_data_.find(key);
    if (it == kv_data_.end()) {
      return 0;
    }

    visit_history_.erase(it->second);
    kv_data_.erase(it);
    return 1;
  }

  iterator find(const key_type &key, bool update_visit = true) {
    typename lru_key_value_map_type::iterator it = kv_data_.find(key);
    if (it == kv_data_.end()) {
      return visit_history_.end();
    }

    if (update_visit) {
      value_type move_out = *it->second;
      visit_history_.erase(it->second);
      it->second = visit_history_.insert(visit_history_.end(), move_out);
    }

    return it->second;
  }

  mapped_type &operator[](const key_type &key) {
    using alloc_type = typename std::allocator_traits<allocator_type>::template rebind_alloc<mapped_type>;

    iterator it = find(key);
    if (it == end()) {
      std::pair<iterator, bool> res = insert(value_type(
          key, compat_strong_ptr_function_trait<option_type::ptr_mode>::template allocate_shared<mapped_type>(
                   alloc_type())));
      return *(*res.first).second;
    }

    return *(*it).second;
  }

 private:
  lru_history_list_type visit_history_;
  lru_key_value_map_type kv_data_;
};
}  // namespace memory
ATFRAMEWORK_UTILS_NAMESPACE_END
