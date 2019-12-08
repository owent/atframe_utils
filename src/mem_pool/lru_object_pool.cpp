
#include <mem_pool/lru_object_pool.h>

namespace util {
    namespace mempool {
        LIBATFRAME_UTILS_API lru_pool_base::list_type_base::list_type_base() {}
        LIBATFRAME_UTILS_API lru_pool_base::list_type_base::~list_type_base() {}

        LIBATFRAME_UTILS_API lru_pool_base::lru_pool_base() {}
        LIBATFRAME_UTILS_API lru_pool_base::~lru_pool_base() {}

        LIBATFRAME_UTILS_API lru_pool_manager::ptr_t lru_pool_manager::create() { return ptr_t(new lru_pool_manager()); }

#define _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(x)                            \
    LIBATFRAME_UTILS_API void lru_pool_manager::set_##x(size_t v) { x##_ = v; } \
    LIBATFRAME_UTILS_API size_t lru_pool_manager::get_##x() const { return x##_; }

        _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(item_min_bound);  // 主动GC的保留对象数量
        _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(item_max_bound);  // 超出对象数量触发GC
        _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(proc_item_count); // 每帧最大处理的对象数量
        _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER(gc_item);         // 下一次GC保留的对象数量
#undef _UTIL_MEMPOOL_LRUOBJECTPOOL_SETTER_GETTER


        LIBATFRAME_UTILS_API void lru_pool_manager::set_list_tick_timeout(time_t v) { list_tick_timeout_ = v; }

        LIBATFRAME_UTILS_API time_t lru_pool_manager::get_list_tick_timeout() const { return list_tick_timeout_; }

        LIBATFRAME_UTILS_API void lru_pool_manager::set_item_adjust_min(size_t v) {
            item_adjust_min_ = v;
            item_adjust_max_ = item_adjust_min_ >= item_adjust_max_ ? (item_adjust_min_ + 1) : item_adjust_max_;
        }

        LIBATFRAME_UTILS_API size_t lru_pool_manager::get_item_adjust_min() const { return item_adjust_min_; }

        LIBATFRAME_UTILS_API void lru_pool_manager::set_item_adjust_max(size_t v) {
            item_adjust_max_ = v;
            item_adjust_min_ = item_adjust_min_ < item_adjust_max_ ? item_adjust_min_ : (item_adjust_max_ - 1);
        }

        LIBATFRAME_UTILS_API size_t lru_pool_manager::get_item_adjust_max() const { return item_adjust_max_; }

        /**
         * @brief 获取实例缓存数量
         * @note 如果不是非常了解这个数值的作用，请不要修改它
         */
        LIBATFRAME_UTILS_API const util::lock::seq_alloc_u64 &lru_pool_manager::item_count() const { return item_count_; }

        /**
         * @brief 主动GC，会触发阈值自适应
         * @return 此次调用回收的元素的个数
         */
        LIBATFRAME_UTILS_API size_t lru_pool_manager::gc() {
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
        LIBATFRAME_UTILS_API size_t lru_pool_manager::proc(time_t tick) {
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
        LIBATFRAME_UTILS_API lru_pool_manager::check_list_t::iterator
                             lru_pool_manager::push_check_list(std::weak_ptr<lru_pool_base::list_type_base> list_) {
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

        LIBATFRAME_UTILS_API bool lru_pool_manager::erase_check_list(check_list_t::iterator iter) {
            if (iter == checked_list_.end()) {
                return false;
            }
            checked_list_.erase(iter);
            item_count_.dec();
            return true;
        }

        // GCC 4.4 don't support erase(const_iterator)
        // LIBATFRAME_UTILS_API bool lru_pool_manager::erase_check_list(check_list_t::const_iterator iter) {
        //     if (iter == checked_list_.end()) {
        //         return false;
        //     }
        //     checked_list_.erase(iter);
        //     item_count_.dec();
        //     return true;
        // }

        LIBATFRAME_UTILS_API lru_pool_manager::check_list_t::iterator lru_pool_manager::end_check_list() { return checked_list_.end(); }

        LIBATFRAME_UTILS_API lru_pool_manager::lru_pool_manager()
            : item_min_bound_(0), item_max_bound_(1024),
#if defined(_MSC_VER) && _MSC_VER < 1900
              proc_item_count_(ULONG_MAX), gc_item_(0), item_adjust_min_(256), item_adjust_max_(ULONG_MAX),
#else
              proc_item_count_(std::numeric_limits<size_t>::max()), gc_item_(0), item_adjust_min_(256),
              item_adjust_max_(std::numeric_limits<size_t>::max()),
#endif
              last_proc_tick_(0), list_tick_timeout_(0) {
            item_count_.set(0);
        }

        LIBATFRAME_UTILS_API size_t lru_pool_manager::inner_gc() {
            if (gc_item_ <= 0) {
                gc_item_ = item_max_bound_;
            }

            return proc(last_proc_tick_);
        }

        LIBATFRAME_UTILS_API bool lru_pool_manager::check_tick(time_t tp) {
            using std::abs;
            return 0 == list_tick_timeout_ || abs(last_proc_tick_ - tp) <= list_tick_timeout_;
        }

    } // namespace mempool
} // namespace util
