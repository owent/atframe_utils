/**
 * @file seq_alloc.h
 * @brief 进程内序号分配器
 * @note 使用原子操作保证进程内多线程环境下不冲突
 * @note 拥有比spin_lock更精确的内存屏障策略
 * @note 相比std::atomic<T>而言，仅提供适合用于序号分配的简单的原子操作。但比其更能支持一些老版本编译器
 * @note 如果可以使用boost::atomic<T>，请直接使用boost::atomic<T>
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2015-12-14
 *
 * @note VC 2012+, GCC 4.4 + 使用C++0x/11实现实现原子操作
 * @note 低版本 VC使用InterlockedExchange等实现原子操作
 * @note 低版本 GCC采用__sync_lock_test_and_set等实现原子操作
 *
 * @history
 *     2015-12-14   created
 */

#ifndef _UTIL_LOCK_SEQ_ALLOC_H_
#define _UTIL_LOCK_SEQ_ALLOC_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif
#include <stdint.h>

#include "atomic_int_type.h"

namespace util {
    namespace lock {
        template <typename Ty>
        class seq_alloc {
        public:
            typedef Ty value_type;

        private:
            ::util::lock::atomic_int_type<value_type> data_;

        public:
            seq_alloc() { data_.store(static_cast<value_type>(0)); }

            value_type get() const { return data_.load(std::memory_order_acquire); }

            value_type set(value_type val) { return data_.exchange(val, std::memory_order_release); }

            value_type add(value_type val) { return data_.fetch_add(val, std::memory_order_release); }

            value_type sub(value_type val) { return data_.fetch_sub(val, std::memory_order_release); }

            value_type band(value_type val) { return data_.fetch_and(val, std::memory_order_release); }

            value_type bor(value_type val) { return data_.fetch_or(val, std::memory_order_release); }

            value_type bxor(value_type val) { return data_.fetch_xor(val, std::memory_order_release); }

            bool compare_exchange(value_type expected, value_type val) {
                return data_.compare_exchange_strong(expected, val, std::memory_order_acq_rel);
            }

            value_type inc() { return ++data_; }

            value_type dec() { return --data; }
        };

        typedef seq_alloc<uint8_t> seq_alloc_u8;
        typedef seq_alloc<uint16_t> seq_alloc_u16;
        typedef seq_alloc<uint32_t> seq_alloc_u32;
        typedef seq_alloc<uint64_t> seq_alloc_u64;
        typedef seq_alloc<int8_t> seq_alloc_i8;
        typedef seq_alloc<int16_t> seq_alloc_i16;
        typedef seq_alloc<int32_t> seq_alloc_i32;
        typedef seq_alloc<int64_t> seq_alloc_i64;
    }
}

#endif /* _UTIL_LOCK_SEQ_ALLOC_H_ */
