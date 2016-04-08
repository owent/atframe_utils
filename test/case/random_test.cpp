#include <time.h>

#include "frame/test_macros.h"
#include "random/random_generator.h"

CASE_TEST(random_test, random_gen_mt19937) {
    util::random::mt19937 gen1;
    gen1.init_seed(123);

    uint32_t a1 = gen1.random();
    uint32_t a2 = gen1.random();
    uint32_t a3 = gen1();

    CASE_EXPECT_EQ(static_cast<uint32_t>(2991312382), a1);
    CASE_EXPECT_EQ(static_cast<uint32_t>(3062119789), a2);
    CASE_EXPECT_EQ(static_cast<uint32_t>(1228959102), a3);
}

CASE_TEST(random_test, random_gen_mt19937_64) {
    util::random::mt19937_64 gen1;
    gen1.init_seed(321);

    uint64_t a1 = gen1.random();
    uint64_t a2 = gen1.random();
    uint64_t a3 = gen1();

    CASE_EXPECT_EQ(10254371118423419891ULL, a1);
    CASE_EXPECT_EQ(8078970281289081ULL, a2);
    CASE_EXPECT_EQ(13774509987902109069ULL, a3);
}

CASE_TEST(random_test, random_gen_64_mt11213B) {
    util::random::mt11213b gen1;
    gen1.init_seed(789);

    uint64_t a1 = gen1.random();
    uint64_t a2 = gen1.random();
    uint64_t a3 = gen1();

    CASE_EXPECT_EQ(static_cast<uint32_t>(3740219552), a1);
    CASE_EXPECT_EQ(static_cast<uint32_t>(740436508), a2);
    CASE_EXPECT_EQ(static_cast<uint32_t>(649207690), a3);
}

CASE_TEST(random_test, random_gen_taus88) {
    util::random::taus88 gen1;
    gen1.init_seed(321);

    uint32_t a1 = gen1.random();
    uint32_t a2 = gen1.random();
    uint32_t a3 = gen1();

    CASE_EXPECT_EQ(static_cast<uint32_t>(43258884), a1);
    CASE_EXPECT_EQ(static_cast<uint32_t>(1073987586), a2);
    CASE_EXPECT_EQ(static_cast<uint32_t>(2769562570), a3);
}
