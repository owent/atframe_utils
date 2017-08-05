#include <stdint.h>

#include "frame/test_macros.h"

#include "config/compiler_features.h"

#include "lock/atomic_int_type.h"

CASE_TEST(atomic_int_test, int8) {
    typedef int8_t tested_type;
    ::util::lock::atomic_int_type<tested_type> tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0x70));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0x7F));
    CASE_EXPECT_EQ(48, tested.load());
}

CASE_TEST(atomic_int_test, uint8) {
    typedef uint8_t tested_type;
    ::util::lock::atomic_int_type<tested_type> tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0xF0));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0xFF));
    CASE_EXPECT_EQ(0xB0, tested.load());
}


CASE_TEST(atomic_int_test, int16) {
    typedef int16_t tested_type;
    ::util::lock::atomic_int_type<tested_type> tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0xF0));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0xFF));
    CASE_EXPECT_EQ(0xB0, tested.load());
}


CASE_TEST(atomic_int_test, uint16) {
    typedef uint16_t tested_type;
    ::util::lock::atomic_int_type<tested_type> tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0xF0));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0xFF));
    CASE_EXPECT_EQ(0xB0, tested.load());
}


CASE_TEST(atomic_int_test, int32) {
    typedef int32_t tested_type;
    ::util::lock::atomic_int_type<tested_type> tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0xF0));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0xFF));
    CASE_EXPECT_EQ(0xB0, tested.load());
}


CASE_TEST(atomic_int_test, uint32) {
    typedef uint32_t tested_type;
    ::util::lock::atomic_int_type<tested_type> tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0xF0));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0xFF));
    CASE_EXPECT_EQ(0xB0, tested.load());
}


CASE_TEST(atomic_int_test, int64) {
    typedef int64_t tested_type;
    ::util::lock::atomic_int_type<tested_type> tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0xF0));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0xFF));
    CASE_EXPECT_EQ(0xB0, tested.load());
}


CASE_TEST(atomic_int_test, uint64) {
    typedef uint64_t tested_type;
    ::util::lock::atomic_int_type<tested_type> tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0xF0));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0xFF));
    CASE_EXPECT_EQ(0xB0, tested.load());
}


#if defined(UTIL_CONFIG_COMPILER_CXX_THREAD_LOCAL) && defined(UTIL_CONFIG_COMPILER_CXX_LAMBDAS) && UTIL_CONFIG_COMPILER_CXX_LAMBDAS

#include <std/smart_ptr.h>
#include <thread>
#include <vector>

CASE_TEST(atomic_int_test, multi_thread_add) {
    typedef uint64_t tested_type;
    ::util::lock::atomic_int_type<tested_type> tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    typedef std::shared_ptr<std::thread> thread_ptr;
    std::vector<thread_ptr> thds;
    uint64_t thd_num = 5000;
    thds.resize(static_cast<size_t>(thd_num));

    for (size_t i = 0; i < thds.size(); ++i) {
        thds[i] = std::make_shared<std::thread>([&tested, i]() {
            for (uint64_t j = 1; j <= 10; ++j) {
                tested.fetch_add(i * 10 + j);
            }
        });
    }

    for (size_t i = 0; i < thds.size(); ++i) {
        thds[i]->join();
    }

    CASE_EXPECT_EQ(5 * thd_num * (1 + 10 * thd_num) + 43, tested.load());
}
#endif

CASE_TEST(atomic_int_test, unsafe_int8) {
    typedef int8_t tested_type;
    ::util::lock::atomic_int_type< ::util::lock::unsafe_int_type<tested_type> > tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0x70));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0x7F));
    CASE_EXPECT_EQ(48, tested.load());
}

CASE_TEST(atomic_int_test, unsafe_uint32) {
    typedef uint32_t tested_type;
    ::util::lock::atomic_int_type< ::util::lock::unsafe_int_type<tested_type> > tested(43);

    // store and load
    CASE_EXPECT_EQ(43, tested.load());

    tested.store(57);
    CASE_EXPECT_EQ(57, (tested_type)tested);


    // operator ++ and --
    CASE_EXPECT_EQ(58, ++tested);
    CASE_EXPECT_EQ(58, tested++);

    CASE_EXPECT_EQ(59, tested.load());

    CASE_EXPECT_EQ(58, --tested);
    CASE_EXPECT_EQ(58, tested--);

    CASE_EXPECT_EQ(57, tested.load());

    // exchange and CAS
    CASE_EXPECT_EQ(57, tested.exchange(64));
    CASE_EXPECT_EQ(64, tested.load());

    tested_type cas_var = 64;
    CASE_EXPECT_TRUE(tested.compare_exchange_weak(cas_var, 73));
    CASE_EXPECT_EQ(73, tested.load());
    CASE_EXPECT_EQ(64, cas_var);

    CASE_EXPECT_FALSE(tested.compare_exchange_strong(cas_var, 81));
    CASE_EXPECT_EQ(73, cas_var);

    // fetch add and sub
    CASE_EXPECT_EQ(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_EQ(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_EQ(73, tested.fetch_and(0xF0));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_EQ(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_EQ(79, tested.fetch_xor(0xFF));
    CASE_EXPECT_EQ(0xB0, tested.load());
}