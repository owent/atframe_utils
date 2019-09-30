/**
 * @file lru_object_pool.h
 * @brief lru 算法的对象池<br />
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2015-12-22
 *
 * @history
 *     2015-12-28: 增加缓存超时功能
 *                 增加LRU管理器自适应限制保护（防止频繁调用主动gc时，有效检查列表和元素列表长度降为0）
 *                 增加LRU管理器自适应限制动态增长功能
 *                 增加UTIL_MEMPOOL_LRUOBJECTPOOL_CHECK_REPUSH宏用于检测上层逻辑可能重复push某一个资源（复杂度会导致复杂度由O(1)=>O(log(n))）
 *
 *     2016-02-24: 增加一些基本接口
 *                 empty定义改为const
 *                 尽早析构空list
 *
 *     2019-09-30: 优化内部实现
 *                 尽快清理无效的检查列表
 *
 */

#ifndef UTIL_MEMPOOL_LRUOBJECTPOOL_H
#define UTIL_MEMPOOL_LRUOBJECTPOOL_H

#pragma once

#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <ctime>
#include <limits>
#include <list>
#include <stdint.h>

#include <config/atframe_utils_build_feature.h>

#include "std/smart_ptr.h"

#include "lock/seq_alloc.h"

#if defined(__cplusplus) &&                                                                                         \
    (__cplusplus >= 201103L || (defined(_MSC_VER) && (_MSC_VER == 1500 && defined(_HAS_TR1)) || _MSC_VER > 1500) || \
     (defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__)))
#include <unordered_map>
#define UTIL_MEMPOOL_LRUOBJECTPOOL_MAP(...) std::unordered_map<__VA_ARGS__>
#else
#include <map>
#define UTIL_MEMPOOL_LRUOBJECTPOOL_MAP(...) std::map<__VA_ARGS__>
#endif

// 开启这个宏在包含此文件会开启对象重复push进同一个池的检测，同时也会导致push、pull和gc的复杂度由O(1)变为O(log(n))
#ifdef UTIL_MEMPOOL_LRUOBJECTPOOL_CHECK_REPUSH
#include <set>
#endif

namespace util {
    namespace mempool {

        class lru_pool_base {
        public:
            class list_type_base {
            public:
                virtual size_t size() const  = 0;
                virtual bool   gc()          = 0;
                virtual bool   empty() const = 0;

            protected:
                list_type_base() {}
                virtual ~list_type_base() {}
            };

        protected:
            lru_pool_base() {}
            virtual ~lru_pool_base() {}
        };

        /**
         * 需要注意保证lru_pool_manager所引用的所有lru_pool仍然有效
         */
        class lru_pool_manager {
        public:
            typedef std::shared_ptr<lru_pool_manager> ptr_t;

            struct check_item_t {
                time_t                                       push_tick;
                std::weak_ptr<lru_pool_base::list_type_base> list_;
            };

            typedef std::list<check_item_t> check_list_t;

        public:
            static ptr_t create() { return ptr_t(new lru_pool_manager()); }

#define _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(x) \
    void   set_##x(size_t v) { x##_ = v; }           \
    size_t get_##x() const { return x##_; }

            _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(item_min_bound);  // 主动GC的保留对象数量
            _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(item_max_bound);  // 超出对象数量触发GC
            _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(proc_item_count); // 每帧最大处理的对象数量
            _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(gc_item);         // 下一次GC保留的对象数量

            void set_list_tick_timeout(time_t v) { list_tick_timeout_ = v; }

            time_t get_list_tick_timeout() const { return list_tick_timeout_; }

            void set_item_adjust_min(size_t v) {
                item_adjust_min_ = v;
                item_adjust_max_ = item_adjust_min_ >= item_adjust_max_ ? (item_adjust_min_ + 1) : item_adjust_max_;
            }

            size_t get_item_adjust_min() const { return item_adjust_min_; }

            void set_item_adjust_max(size_t v) {
                item_adjust_max_ = v;
                item_adjust_min_ = item_adjust_min_ < item_adjust_max_ ? item_adjust_min_ : (item_adjust_max_ - 1);
            }

            size_t get_item_adjust_max() const { return item_adjust_max_; }

#undef _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER

            /**
             * @brief 获取实例缓存数量
             * @note 如果不是非常了解这个数值的作用，请不要修改它
             */
            inline const util::lock::seq_alloc_u64 &item_count() const { return item_count_; }

            /**
             * @brief 主动GC，会触发阈值自适应
             * @return 此次调用回收的元素的个数
             */
            size_t gc() {
                if (gc_item_ <= 0) {
                    item_min_bound_ = static_cast<size_t>((item_count_.get() + item_min_bound_) / 2);
                    item_max_bound_ = static_cast<size_t>((item_count_.get() + item_max_bound_ + 1) / 2);

                    if (item_min_bound_ > item_adjust_max_ - 1) {
                        item_min_bound_ = item_adjust_max_ - 1;
                    }

                    if (item_max_bound_ < item_min_bound_ + 1) {
                        item_max_bound_ = item_min_bound_ + 1;
                    }

                    if (item_max_bound_ < item_adjust_min_ + 1) {
                        item_max_bound_ = item_adjust_min_ + 1;
                    }

                    gc_item_ = item_min_bound_;
                }

                return proc(last_proc_tick_);
            }

            /**
             * @brief 定时回调
             * @param tick 用于判定超时的tick时间，时间单位由业务逻辑决定
             * @return 此次调用回收的元素的个数
             */
            size_t proc(time_t tick) {
                last_proc_tick_ = tick;

                if (gc_item_ <= 0) {
                    // 如果没有失效的check list缓存则不用继续走资源回收流程
                    if (checked_list_.empty() || check_tick(checked_list_.front().push_tick)) {
                        return 0;
                    }
                }

                size_t ret           = 0;
                size_t left_item_num = proc_item_count_;

                while (left_item_num > 0) {
                    if (0 != gc_item_ && item_count_.get() <= gc_item_) {
                        gc_item_ = 0;
                    }

                    if (0 == gc_item_) {
                        // 如果没有失效的check list缓存则后续流程也可以取消
                        if (checked_list_.empty() || check_tick(checked_list_.front().push_tick)) {
                            break;
                        }
                    }

                    if (checked_list_.empty()) {
                        gc_item_ = 0;
                        item_count_.set(0);
                        break;
                    }

                    check_item_t checked_item = checked_list_.front();

                    if (checked_item.list_.expired()) {
                        item_count_.dec();
                        checked_list_.pop_front();
                        continue;
                    }

                    std::shared_ptr<lru_pool_base::list_type_base> tar_ls = checked_item.list_.lock();
                    if (!tar_ls) {
                        item_count_.dec();
                        checked_list_.pop_front();
                        continue;
                    }

                    if (tar_ls->gc()) {
                        ++ret;
                        --left_item_num;
                    } else {
                        // protect codes. it should not run
                        assert(false);
                        if (left_item_num > 10) {
                            left_item_num = 10;
                        } else {
                            --left_item_num;
                        }
                    }
                }

                return ret;
            }

            /**
             * @brief 添加检查列表
             */
            check_list_t::iterator push_check_list(std::weak_ptr<lru_pool_base::list_type_base> list_) {
                check_list_t::iterator ret = checked_list_.insert(checked_list_.end(), check_item_t());
                if (ret == checked_list_.end()) {
                    return ret;
                }
                (*ret).list_     = list_;
                (*ret).push_tick = last_proc_tick_;

                item_count_.inc();

                if (item_count_.get() > item_max_bound_) {
                    inner_gc();

                    // 自适应，慢速增大上限值
                    if (item_max_bound_ < item_adjust_max_) {
                        ++item_max_bound_;
                    }
                }

                return ret;
            }

            bool erase_check_list(check_list_t::iterator iter) {
                if (iter == checked_list_.end()) {
                    return false;
                }
                checked_list_.erase(iter);
                item_count_.dec();
                return true;
            }

            bool erase_check_list(check_list_t::const_iterator iter) {
                if (iter == checked_list_.end()) {
                    return false;
                }
                checked_list_.erase(iter);
                item_count_.dec();
                return true;
            }

            check_list_t::iterator end_check_list() { return checked_list_.end(); }

        private:
            lru_pool_manager()
                : item_min_bound_(0), item_max_bound_(1024), proc_item_count_(std::numeric_limits<size_t>::max()), gc_item_(0),
                  item_adjust_min_(256), item_adjust_max_(std::numeric_limits<size_t>::max()), last_proc_tick_(0), list_tick_timeout_(0) {
                item_count_.set(0);
            }

            lru_pool_manager(const lru_pool_manager &);
            lru_pool_manager &operator=(const lru_pool_manager &);

            size_t inner_gc() {
                if (gc_item_ <= 0) {
                    gc_item_ = item_max_bound_;
                }

                return proc(last_proc_tick_);
            }

            inline bool check_tick(time_t tp) {
                using std::abs;
                return 0 == list_tick_timeout_ || abs(last_proc_tick_ - tp) <= list_tick_timeout_;
            }

        private:
            size_t                    item_min_bound_;
            size_t                    item_max_bound_;
            util::lock::seq_alloc_u64 item_count_;
            size_t                    proc_item_count_;
            size_t                    gc_item_;
            check_list_t              checked_list_;

            // 自适应下限
            size_t item_adjust_min_;
            size_t item_adjust_max_;

            // 检查列表，tick有效期
            time_t last_proc_tick_;
            time_t list_tick_timeout_;
        };

        template <typename TObj>
        struct lru_default_action {
            void push(TObj *) {}
            void pull(TObj *) {}
            void reset(TObj *) {}
            void gc(TObj *obj) { delete obj; }
        };

        template <typename TKey, typename TObj, typename TAction = lru_default_action<TObj> >
        class lru_pool : public lru_pool_base {
        public:
            typedef TKey    key_t;
            typedef TObj    value_type;
            typedef TAction action_type;

            class list_type;
            typedef std::shared_ptr<list_type> list_ptr_type;
            typedef UTIL_MEMPOOL_LRUOBJECTPOOL_MAP(key_t, list_ptr_type) cat_map_type;

            class list_type : public lru_pool_base::list_type_base {
            public:
                struct wrapper {
                    value_type *                                      object;
                    typename lru_pool_manager::check_list_t::iterator refer_iterator;
                };

                list_type(lru_pool<TKey, TObj, TAction> &owner, key_t id) : owner_(&owner), id_(id) {}
                virtual ~list_type() { clear_manager(); }

                virtual size_t size() const { return cache_.size(); };

                virtual bool gc() {
                    if (cache_.empty()) {
                        return false;
                    }

                    wrapper obj = cache_.back();
                    cache_.pop_back();

                    if (owner_ && owner_->mgr_) {
                        owner_->mgr_->erase_check_list(obj.refer_iterator);
                    }

#ifdef UTIL_MEMPOOL_LRUOBJECTPOOL_CHECK_REPUSH
                    owner_->check_pushed_.erase(obj.object);
#endif

                    TAction act;
                    act.gc(obj.object);

                    // NOTICE, it's iterator may be used in for - loop now, can not erase it
                    // owner_->data_.erase(id_);
                    return true;
                }

                virtual bool empty() const { return cache_.empty(); }

                void clear_manager() {
                    if (!owner_->mgr_) {
                        return;
                    }

                    typename lru_pool_manager::check_list_t::iterator end_iter = owner_->mgr_->end_check_list();
                    for (typename std::list<wrapper>::iterator iter = cache_.begin(); iter != cache_.end(); ++iter) {
                        if (owner_->mgr_->erase_check_list((*iter).refer_iterator)) {
                            (*iter).refer_iterator = end_iter;
                        }
                    }
                }

                void setup_manager(list_ptr_type &self) {
                    if (!owner_->mgr_) {
                        return;
                    }

                    for (typename std::list<wrapper>::iterator iter = cache_.begin(); iter != cache_.end(); ++iter) {
#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
                        (*iter).refer_iterator =
                            owner_->mgr_->push_check_list(std::dynamic_pointer_cast<lru_pool_base::list_type_base>(self));
#else
                        (*iter).refer_iterator =
                            owner_->mgr_->push_check_list(std::static_pointer_cast<lru_pool_base::list_type_base>(self));
#endif
                    }
                }

                bool push(value_type *obj, list_ptr_type &self) {
                    // push, FILO
                    typename std::list<wrapper>::iterator iter = cache_.insert(cache_.begin(), wrapper());
                    if (iter == cache_.end()) {
                        return false;
                    }

                    (*iter).object = obj;
                    if (owner_->mgr_) {
#if defined(LIBATFRAME_UTILS_ENABLE_RTTI) && LIBATFRAME_UTILS_ENABLE_RTTI
                        (*iter).refer_iterator =
                            owner_->mgr_->push_check_list(std::dynamic_pointer_cast<lru_pool_base::list_type_base>(self));
#else
                        (*iter).refer_iterator =
                            owner_->mgr_->push_check_list(std::static_pointer_cast<lru_pool_base::list_type_base>(self));
#endif
                    }

                    return true;
                }

                value_type *pull() {
                    // pull, FILO
                    if (cache_.empty()) {
                        return NULL;
                    }

                    wrapper res = cache_.front();
                    cache_.pop_front();

                    if (owner_->mgr_) {
                        owner_->mgr_->erase_check_list(res.refer_iterator);
                    }

                    return res.object;
                }

            private:
                lru_pool<TKey, TObj, TAction> *owner_;
                key_t                          id_;
                std::list<wrapper>             cache_;
            };

            struct flag_t {
                enum type { INITED = 0, CLEARING };
            };

        private:
            lru_pool(const lru_pool &);
            lru_pool &operator=(const lru_pool &);

            struct flag_guard {
                static inline bool test(const uint32_t &fs, typename flag_t::type t) { return 0 != (fs & (static_cast<uint32_t>(1) << t)); }

                static inline void set(uint32_t &fs, typename flag_t::type t) { fs |= static_cast<uint32_t>(1) << t; }

                static inline void unset(uint32_t &fs, typename flag_t::type t) { fs &= ~(static_cast<uint32_t>(1) << t); }

                flag_guard(uint32_t &fs, typename flag_t::type t) : flag_set(fs), flag_opt(t) { set(flag_set, flag_opt); }

                ~flag_guard() { unset(flag_set, flag_opt); }

                uint32_t &            flag_set;
                typename flag_t::type flag_opt;
            };

        public:
            lru_pool() : flags_(0) {}

            virtual ~lru_pool() {
                set_manager(lru_pool_manager::ptr_t());
                clear();
            }

            /**
             * @brief 初始化
             * @param m 所属的全局管理器。相应的事件会通知全局管理器
             */
            int init(lru_pool_manager::ptr_t m) {
                set_manager(m);

                flag_guard::set(flags_, flag_t::INITED);
                return 0;
            }

            void set_manager(lru_pool_manager::ptr_t m) {
                if (mgr_ == m) {
                    return;
                }

                for (typename cat_map_type::iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                    if (iter->second) {
                        iter->second->clear_manager();
                    }
                }

                mgr_ = m;

                for (typename cat_map_type::iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                    if (iter->second) {
                        iter->second->setup_manager(iter->second);
                    }
                }
            }

            bool push(key_t id, TObj *obj) {
#ifdef UTIL_MEMPOOL_LRUOBJECTPOOL_CHECK_REPUSH
                if (check_pushed_.find(obj) != check_pushed_.end()) {
                    return false;
                }
#endif
                if (NULL == obj) {
                    return false;
                }

                // clear过程中再推送的对象一律走GC
                if (flag_guard::test(flags_, flag_t::CLEARING)) {
                    TAction act;
                    act.gc(obj);
                    return false;
                }

                list_ptr_type &list_ = data_[id];
                if (!list_) {
                    list_ = std::make_shared<list_type>(*this, id);
                    if (!list_) {
                        return false;
                    }
                }

                if (!list_->push(obj, list_)) {
                    return false;
                }

                TAction act;
                act.push(obj);

#ifdef UTIL_MEMPOOL_LRUOBJECTPOOL_CHECK_REPUSH
                check_pushed_.insert(obj);
#endif

                return true;
            }

            TObj *pull(key_t id) {
                typename cat_map_type::iterator iter = data_.find(id);
                if (iter == data_.end()) {
                    return NULL;
                }

                if (!iter->second || iter->second->empty()) {
                    data_.erase(iter);
                    return NULL;
                }

                TObj *ret = iter->second->pull();

                if (iter->second->empty()) {
                    data_.erase(iter);
                }

                if (NULL == ret) {
                    return ret;
                }

                TAction act;
                act.pull(ret);
                act.reset(ret);

#ifdef UTIL_MEMPOOL_LRUOBJECTPOOL_CHECK_REPUSH
                check_pushed_.erase(ret);
#endif
                return ret;
            }

            void clear() {
                // 递归调用clear，直接返回
                if (flag_guard::test(flags_, flag_t::CLEARING)) {
                    return;
                }

                flag_guard fg(flags_, flag_t::CLEARING);

                for (typename cat_map_type::iterator iter = data_.begin(); iter != data_.end();) {
                    typename cat_map_type::iterator checked_it = iter++;
                    if (checked_it->second) {
                        while (checked_it->second->gc())
                            ;
                    }
                }

                data_.clear();
            }

            bool empty() const {
                if (data_.empty()) {
                    return true;
                }

                for (typename cat_map_type::const_iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                    if (iter->second && !iter->second->empty()) {
                        return false;
                    }
                }

                return true;
            }

            // high cost, do not use it frequently
            size_t size() const {
                size_t ret = 0;
                for (typename cat_map_type::const_iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                    if (iter->second) {
                        ret += iter->second->size();
                    }
                }

                return ret;
            }

            const cat_map_type &data() const { return data_; }

        private:
            cat_map_type            data_;
            lru_pool_manager::ptr_t mgr_;
            uint32_t                flags_;
#ifdef UTIL_MEMPOOL_LRUOBJECTPOOL_CHECK_REPUSH
            std::set<value_type *> check_pushed_;
#endif
        };
    } // namespace mempool
} // namespace util

#endif /* _UTIL_MEMPOOL_LRUOBJECTPOOL_H_ */
