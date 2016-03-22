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

#if defined(__cplusplus) && defined(__clang__) && \
    (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 1)) && __cplusplus >= 201103L
#include <atomic>
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_STD
#elif defined(_MSC_VER) && (_MSC_VER > 1700 || (defined(_HAS_CPP0X) && _HAS_CPP0X))
#include <atomic>
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_STD
#elif defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 5) || __GNUC__ > 4) && \
    (__cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__))
#include <atomic>
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_STD
#endif


#ifdef _MSC_VER
#include <Windows.h>
#endif

namespace util {
    namespace lock {
// C++ 0x/11版实现
#if defined(__UTIL_LOCK_SEQ_ALLOC_ATOMIC_STD)
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_BRANCH_NAME "std::atomic<T>"

        template <typename Ty>
        class seq_alloc {
        public:
            typedef Ty value_type;

        private:
            std::atomic<value_type> data_;

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

            value_type inc() { return add(static_cast<value_type>(1)) + static_cast<value_type>(1); }

            value_type dec() { return sub(static_cast<value_type>(1)) - static_cast<value_type>(1); }
        };

        typedef seq_alloc<uint8_t> seq_alloc_u8;
        typedef seq_alloc<uint16_t> seq_alloc_u16;
        typedef seq_alloc<uint32_t> seq_alloc_u32;
        typedef seq_alloc<uint64_t> seq_alloc_u64;
        typedef seq_alloc<int8_t> seq_alloc_i8;
        typedef seq_alloc<int16_t> seq_alloc_i16;
        typedef seq_alloc<int32_t> seq_alloc_i32;
        typedef seq_alloc<int64_t> seq_alloc_i64;

#else

#if defined(__clang__)
#if !defined(__GCC_ATOMIC_INT_LOCK_FREE) && \
    (!defined(__GNUC__) || __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1))
#error Clang version is too old
#endif
#if defined(__GCC_ATOMIC_INT_LOCK_FREE)
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC 1
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_BRANCH_NAME "gcc __atomic_*"

#else
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC 1
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_BRANCH_NAME "gcc __sync_lock_*"

#endif
#elif defined(_MSC_VER)
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC 1
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_BRANCH_NAME "vc Interlocked*"

#elif defined(__GNUC__) || defined(__clang__) || defined(__clang__) || defined(__INTEL_COMPILER)
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1))
#error GCC version must be greater bor equal than 4.1
#endif

#if defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1100
#error Intel Compiler version must be greater bor equal than 11.0
#endif

#if defined(__GCC_ATOMIC_INT_LOCK_FREE)
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC 1
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_BRANCH_NAME "gcc __atomic_*"

#else
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC 1
#define __UTIL_LOCK_SEQ_ALLOC_ATOMIC_BRANCH_NAME "gcc __sync_lock_*"

#endif
#else
#error Currently only gcc, msvc, intel compiler & llvm-clang are supported
#endif

#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC

        template <bool is_64bit>
        class seq_alloc_vc_base;

        template <>
        class seq_alloc_vc_base<false> {
        public:
            typedef LONG value_type;

        private:
            volatile value_type data_;

        public:
            seq_alloc_vc_base() : data_(0) {}

            value_type get() const { return InterlockedExchangeAddAcquire(const_cast<value_type *>(&data_), 0L); }

            value_type set(value_type val) { return InterlockedExchange(&data_, val); }

            value_type add(value_type val) { return InterlockedExchangeAddRelease(&data_, val); }

            value_type sub(value_type val) { return add(-val); }

            value_type band(value_type val) { return InterlockedAndRelease(&data_, val); }

            value_type bor(value_type val) { return InterlockedOrRelease(&data_, val); }

            value_type bxor(value_type val) { return InterlockedXorRelease(&data_, val); }

            bool compare_exchange(value_type expected, value_type val) {
                return !!InterlockedCompareExchange(&data_, val, expected);
            }

            value_type inc() { return InterlockedIncrementRelease(&data_); }

            value_type dec() { return InterlockedDecrementRelease(&data_); }
        };

        template <>
        class seq_alloc_vc_base<true> {
        public:
            typedef LONGLONG value_type;

        private:
            volatile value_type data_;

        public:
            seq_alloc_vc_base() : data_(0) {}

            value_type get() const { return InterlockedAddAcquire64(const_cast<value_type *>(&data_), 0LL); }

            value_type set(value_type val) { return InterlockedExchange64(&data_, val); }

            value_type add(value_type val) { return InterlockedAddRelease64(&data_, val); }

            value_type sub(value_type val) { return add(-val); }

            value_type band(value_type val) { return InterlockedAnd64Release(&data_, val); }

            value_type bor(value_type val) { return InterlockedOr64(&data_, val); }

            value_type bxor(value_type val) { return InterlockedXor64(&data_, val); }

            bool compare_exchange(value_type expected, value_type val) {
                return !!InterlockedCompareExchange64(&data_, val, expected);
            }

            value_type inc() { return InterlockedIncrement64(&data_); }

            value_type dec() { return InterlockedDecrement64(&data_); }
        };
#endif

        template <typename Ty>
        class seq_alloc

#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
            : public seq_alloc_vc_base<64 == sizeof(Ty)>
#endif

        {
        public:
            typedef Ty value_type;

#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
        public:
            typedef seq_alloc_vc_base<64 == sizeof(Ty)> _base_type;
            seq_alloc() {}
#else
        private:
            volatile value_type data_;

        public:
            seq_alloc() : data_(0) {}
#endif

            value_type get() {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(_base_type::get());
#elif defined(__UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC)
                return __atomic_load_n(&data_, __ATOMIC_ACQUIRE);
#else
                return __sync_fetch_and_add(&data_, static_cast<value_type>(0));
#endif
            }

            value_type set(value_type val) {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(_base_type::set(static_cast<typename _base_type::value_type>(val)));
#elif defined(__UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC)
                return __atomic_exchange_n(&data_, val, __ATOMIC_RELEASE);
#else
                return __sync_lock_test_and_set(&data_, val);
#endif
            }

            value_type add(value_type val) {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(_base_type::add(static_cast<typename _base_type::value_type>(val)));
#elif defined(__UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC)
                return __atomic_fetch_add(&data_, val, __ATOMIC_RELEASE);
#else
                return __sync_fetch_and_add(&data_, val);
#endif
            }

            value_type sub(value_type val) {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(_base_type::sub(static_cast<typename _base_type::value_type>(val)));
#elif defined(__UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC)
                return __atomic_fetch_sub(&data_, val, __ATOMIC_RELEASE);
#else
                return __sync_fetch_and_sub(&data_, val);
#endif
            }

            value_type band(value_type val) {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(_base_type::band(static_cast<typename _base_type::value_type>(val)));
#elif defined(__UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC)
                return __atomic_fetch_and(&data_, val, __ATOMIC_RELEASE);
#else
                return __sync_fetch_and_and(&data_, val);
#endif
            }

            value_type bor(value_type val) {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(_base_type::bor(static_cast<typename _base_type::value_type>(val)));
#elif defined(__UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC)
                return __atomic_fetch_or(&data_, val, __ATOMIC_RELEASE);
#else
                return __sync_fetch_and_or(&data_, val);
#endif
            }

            value_type bxor(value_type val) {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(_base_type::bxor(static_cast<typename _base_type::value_type>(val)));
#elif defined(__UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC)
                return __atomic_fetch_xor(&data_, val, __ATOMIC_RELEASE);
#else
                return __sync_fetch_and_xor(&data_, val);
#endif
            }

            bool compare_exchange(value_type expected, value_type val) {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(
                    _base_type::compare_exchange(static_cast<typename _base_type::value_type>(expected),
                                                 static_cast<typename _base_type::value_type>(val)));
#elif defined(__UTIL_LOCK_SEQ_ALLOC_ATOMIC_GCC_ATOMIC)
                return __atomic_compare_exchange_n(&data_, &expected, val, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE);
#else
                return __sync_val_compare_and_swap(&data_, expected, val);
#endif
            }

            value_type inc() {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(_base_type::inc());
#else
                return add(static_cast<value_type>(1)) + static_cast<value_type>(1);
#endif
            }

            value_type dec() {
#ifdef __UTIL_LOCK_SEQ_ALLOC_ATOMIC_MSVC
                return static_cast<value_type>(_base_type::dec());
#else
                return sub(static_cast<value_type>(1)) - static_cast<value_type>(1);
#endif
            }
        };


        typedef seq_alloc<uint8_t> seq_alloc_u8;
        typedef seq_alloc<uint16_t> seq_alloc_u16;
        typedef seq_alloc<uint32_t> seq_alloc_u32;
        typedef seq_alloc<uint64_t> seq_alloc_u64;
        typedef seq_alloc<int8_t> seq_alloc_i8;
        typedef seq_alloc<int16_t> seq_alloc_i16;
        typedef seq_alloc<int32_t> seq_alloc_i32;
        typedef seq_alloc<int64_t> seq_alloc_i64;
#endif
    }
}

#endif /* _UTIL_LOCK_SEQ_ALLOC_H_ */
