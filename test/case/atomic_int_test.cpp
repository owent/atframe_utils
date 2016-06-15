#include <stdint.h>
#include "frame/test_macros.h"

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
    CASE_EXPECT_FALSE(73, tested.fetch_add(10));
    CASE_EXPECT_EQ(83, tested.load());

    CASE_EXPECT_FALSE(83, tested.fetch_sub(10));
    CASE_EXPECT_EQ(73, tested.load());

    // fetch and, or, xor
    CASE_EXPECT_FALSE(73, tested.fetch_and(0xF0));
    CASE_EXPECT_EQ(64, tested.load());

    CASE_EXPECT_FALSE(64, tested.fetch_or(0x0F));
    CASE_EXPECT_EQ(79, tested.load());

    CASE_EXPECT_FALSE(79, tested.fetch_xor(0xFF));
    CASE_EXPECT_EQ(0xB0, tested.load());
}
