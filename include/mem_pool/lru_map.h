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

#ifndef UTIL_MEMPOOL_LRU_MAP_H
#define UTIL_MEMPOOL_LRU_MAP_H

#pragma once

#include <cstddef>
#include <list>
#include <stdint.h>

#include <config/atframe_utils_build_feature.h>
#include <config/compiler_features.h>


#include <std/smart_ptr.h>

#if defined(__cplusplus) &&                                                                                         \
    (__cplusplus >= 201103L || (defined(_MSC_VER) && (_MSC_VER == 1500 && defined(_HAS_TR1)) || _MSC_VER > 1500) || \
     (defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__)))
#include <unordered_map>
#define UTIL_MEMPOOL_LRU_MAP_IS_HASHMAP 1

#if UTIL_CONFIG_COMPILER_IS_GNU && (__GNUC__ * 100 + __GNUC_MINOR__) <= 407
#define UTIL_MEMPOOL_LRU_MAP_DISABLE_RESERVE 1
#else
#define UTIL_MEMPOOL_LRU_MAP_DISABLE_RESERVE 0
#endif

#else
#include <map>
#define UTIL_MEMPOOL_LRU_MAP_IS_HASHMAP 0
#define UTIL_MEMPOOL_LRU_MAP_DISABLE_RESERVE 0
#endif

namespace util {
    namespace mempool {
        template <class TKEY, class TVALUE>
        struct LIBATFRAME_UTILS_API_HEAD_ONLY lru_map_type_traits {
            typedef TKEY                              key_type;
            typedef TVALUE                            mapped_type;
            typedef std::shared_ptr<TVALUE>           store_type;
            typedef std::pair<const TKEY, store_type> value_type;
            typedef size_t                            size_type;

            typedef std::list<value_type>              list_type;
            typedef typename list_type::iterator       iterator;
            typedef typename list_type::const_iterator const_iterator;
        };

#if UTIL_MEMPOOL_LRU_MAP_IS_HASHMAP
        template <class TKEY, class TVALUE, class THasher = std::hash<TKEY>, class TKeyEQ = std::equal_to<TKEY>,
                  class TAlloc = std::allocator<std::pair<const TKEY, typename lru_map_type_traits<TKEY, TVALUE>::iterator> > >
#else
        template <typename TKEY, typename TVALUE, class TLESSCMP = std::less<TKEY>,
                  class TAlloc = std::allocator<std::pair<const TKEY, typename lru_map_type_traits<TKEY, TVALUE>::iterator> > >
#endif
        class LIBATFRAME_UTILS_API_HEAD_ONLY lru_map {
        public:
            typedef typename lru_map_type_traits<TKEY, TVALUE>::key_type    key_type;
            typedef typename lru_map_type_traits<TKEY, TVALUE>::mapped_type mapped_type;
            typedef typename lru_map_type_traits<TKEY, TVALUE>::value_type  value_type;
            typedef typename lru_map_type_traits<TKEY, TVALUE>::size_type   size_type;
            typedef typename lru_map_type_traits<TKEY, TVALUE>::store_type  store_type;
            typedef TAlloc                                                  allocator_type;
            typedef value_type &                                            reference;
            typedef const value_type &                                      const_reference;
            typedef value_type *                                            pointer;
            typedef const value_type *                                      const_pointer;

            typedef typename lru_map_type_traits<TKEY, TVALUE>::list_type      lru_history_list_type;
            typedef typename lru_map_type_traits<TKEY, TVALUE>::iterator       iterator;
            typedef typename lru_map_type_traits<TKEY, TVALUE>::const_iterator const_iterator;

#if UTIL_MEMPOOL_LRU_MAP_IS_HASHMAP
            typedef std::unordered_map<TKEY, iterator, THasher, TKeyEQ, TAlloc> lru_key_value_map_type;
            typedef lru_map<TKEY, TVALUE, THasher, TKeyEQ, TAlloc>              self_type;
#else
            typedef std::map<TKEY, iterator, TLESSCMP, TAlloc>       lru_key_value_map_type;
            typedef lru_map<TKEY, TVALUE, THasher, TLESSCMP, TAlloc> self_type;
#endif

            lru_map() {}

            template <class TCONTAINER>
            LIBATFRAME_UTILS_API_HEAD_ONLY lru_map(const TCONTAINER &other) {
                reserve(static_cast<size_type>(other.size()));
                insert(other.begin(), other.end());
            }

#if UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES
            lru_map(lru_map &&other) { swap(other); }
#endif

            inline iterator       begin() { return visit_history_.begin(); }
            inline const_iterator cbegin() const { return visit_history_.cbegin(); }
            inline iterator       end() { return visit_history_.end(); }
            inline const_iterator cend() const { return visit_history_.cend(); }

            inline value_type &      front() { return visit_history_.front(); }
            inline const value_type &front() const { return visit_history_.front(); }
            inline value_type &      back() { return visit_history_.back(); }
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

            inline bool      empty() const { return kv_data_.empty(); }
            inline size_type size() const { return kv_data_.size(); }

#if UTIL_MEMPOOL_LRU_MAP_DISABLE_RESERVE
            inline void reserve(size_type) { /* do nothing, some old compiler don't support this. */
            }
#else

            inline void reserve(size_type s) { kv_data_.reserve(s); }
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
            LIBATFRAME_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(const TPARAMKEY &key, const TPARAMVALUE &copy_value) {
                return insert_key_value(key, std::make_shared<mapped_type>(copy_value));
            }

            template <class TPARAMKEY, class TPARAMVALUE>
            LIBATFRAME_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(const TPARAMKEY &                   key,
                                                                                      const std::shared_ptr<TPARAMVALUE> &value) {
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
            LIBATFRAME_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert(const std::pair<TCKEY, TCVALUE> &value) {
                return insert_key_value(value.first, value.second);
            }

#if UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES
            std::pair<iterator, bool> insert(value_type &&value) {
                key_type                                  key = value.first;
                typename lru_key_value_map_type::iterator it  = kv_data_.find(key);
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
            LIBATFRAME_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(TPARAMKEY &&                   key,
                                                                                      std::shared_ptr<TPARAMVALUE> &&value) {
                return insert(value_type(std::forward<TPARAMKEY>(key), value));
            }

            template <class TPARAMKEY, class TPARAMVALUE>
            LIBATFRAME_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(TPARAMKEY &&                  key,
                                                                                      std::shared_ptr<TPARAMVALUE> &value) {
                return insert(value_type(std::forward<TPARAMKEY>(key), value));
            }

            template <class TPARAMKEY, class TPARAMVALUE>
            LIBATFRAME_UTILS_API_HEAD_ONLY std::pair<iterator, bool> insert_key_value(TPARAMKEY &&key, TPARAMVALUE &&copy_value) {
                return insert(
                    value_type(std::forward<TPARAMKEY>(key), std::make_shared<mapped_type>(std::forward<TPARAMVALUE>(copy_value))));
            }
#endif

            template <class InputIt>
            LIBATFRAME_UTILS_API_HEAD_ONLY void insert(InputIt first, InputIt last) {
                while (first != last) {
                    insert(*(first++));
                }
            }

            iterator erase(iterator pos) {
                if (pos == visit_history_.end()) {
                    return visit_history_.end();
                }

                key_type                                  key = (*pos).first;
                typename lru_key_value_map_type::iterator it  = kv_data_.find(key);
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
                iterator it = find(key);
                if (it == end()) {
                    std::pair<iterator, bool> res = insert(value_type(key, std::make_shared<mapped_type>()));
                    return *(*res.first).second;
                }

                return *(*it).second;
            }

        private:
            lru_history_list_type  visit_history_;
            lru_key_value_map_type kv_data_;
        };
    } // namespace mempool
} // namespace util

#endif /* _UTIL_MEMPOOL_LRUOBJECTPOOL_H_ */
