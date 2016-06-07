/**
 * @file atomic.h
 * @brief 导入atomic,在不存在的时候可以使用boost作为备选
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT, owt5008137@live.com
 * @date 2016.06.07
 *
 * @history
 *
 */
 
#ifndef _STD_ATOMIC_H_
#define _STD_ATOMIC_H_
 
#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// ============================================================
// 公共包含部分
// 自动导入atomic库
// ============================================================
 
#include <cstddef>
#if defined(__cplusplus) && __cplusplus >= 201103L
#include <atomic>
#define __UTIL_STD_ATOMIC_STD
#elif defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 1)) && __cplusplus >= 201103L
#include <atomic>
#define __UTIL_STD_ATOMIC_STD
#elif defined(_MSC_VER) && (_MSC_VER > 1700 || (defined(_HAS_CPP0X) && _HAS_CPP0X))
#include <atomic>
#define __UTIL_STD_ATOMIC_STD
#elif defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 5) || __GNUC__ > 4) && defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <atomic>
#define __UTIL_STD_ATOMIC_STD
#endif
 
#if !defined(__UTIL_STD_ATOMIC_STD)

// 采用boost.atomic 
// @see http://www.boost.org/doc/html/atomic

#include <boost/atomic.hpp>
#include <boost/memory_order.hpp>

namespace std {
    using boost::atomic;
    using boost::atomic_char;
    using boost::atomic_uchar;
    using boost::atomic_schar;
    using boost::atomic_uint8_t;
    using boost::atomic_int8_t;
    using boost::atomic_ushort;
    using boost::atomic_short;
    using boost::atomic_uint16_t;
    using boost::atomic_int16_t;
    using boost::atomic_uint;
    using boost::atomic_int;
    using boost::atomic_uint32_t;
    using boost::atomic_int32_t;
    using boost::atomic_ulong;
    using boost::atomic_long;
    using boost::atomic_uint64_t;
    using boost::atomic_int64_t;

    using boost::atomic_int_least8_t;
    using boost::atomic_uint_least8_t;
    using boost::atomic_int_least16_t;
    using boost::atomic_uint_least16_t;
    using boost::atomic_int_least32_t;
    using boost::atomic_uint_least32_t;
    using boost::atomic_int_least64_t;
    using boost::atomic_uint_least64_t;
    using boost::atomic_int_fast8_t;
    using boost::atomic_uint_fast8_t;
    using boost::atomic_int_fast16_t;
    using boost::atomic_uint_fast16_t;
    using boost::atomic_int_fast32_t;
    using boost::atomic_uint_fast32_t;
    using boost::atomic_int_fast64_t;
    using boost::atomic_uint_fast64_t;
    using boost::atomic_intmax_t;
    using boost::atomic_uintmax_t;

    using boost::atomic_size_t;
    using boost::atomic_ptrdiff_t;
    using boost::atomic_intptr_t;
    using boost::atomic_uintptr_t;

    using boost::atomic_flag;

    // atomic_store
    // atomic_store_explicit
    // atomic_load
    // atomic_load_explicit
    // atomic_exchange
    // atomic_exchange_explicit
    // atomic_compare_exchange_weak
    // atomic_compare_exchange_weak_explicit
    // atomic_compare_exchange_strong
    // atomic_compare_exchange_strong_explicit
    // atomic_fetch_add
    // atomic_fetch_add_explicit
    // atomic_fetch_sub
    // atomic_fetch_sub_explicit
    // atomic_fetch_and
    // atomic_fetch_and_explicit
    // atomic_fetch_or
    // atomic_fetch_or_explicit
    // atomic_fetch_xor
    // atomic_fetch_xor_explicit
    // atomic_flag
    // atomic_flag_test_and_set
    // atomic_flag_test_and_set_explicit
    // atomic_flag_clear
    // atomic_flag_clear_explicit
    // atomic_init

    using boost::memory_order;

    using boost::atomic_thread_fence;
    using boost::atomic_signal_fence;
}

#endif

#endif