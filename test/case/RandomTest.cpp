#include <time.h>

#include "frame/test_macros.h"
#include "Random/RandomGenerator.h"

CASE_TEST(RandomTest, RandomGenMT19937)
{
    util::random::MT19937 stGen1;
    stGen1.InitSeed(123);

    uint32_t a1 = stGen1.Random();
    uint32_t a2 = stGen1.Random();
    uint32_t a3 = stGen1();

	CASE_EXPECT_EQ(static_cast<uint32_t>(2991312382), a1);
	CASE_EXPECT_EQ(static_cast<uint32_t>(3062119789), a2);
	CASE_EXPECT_EQ(static_cast<uint32_t>(1228959102), a3);
}

CASE_TEST(RandomTest, RandomGenMT19937_64)
{
    util::random::MT19937_64 stGen1;
    stGen1.InitSeed(321);

    uint64_t a1 = stGen1.Random();
    uint64_t a2 = stGen1.Random();
    uint64_t a3 = stGen1();

    CASE_EXPECT_EQ(10254371118423419891ULL, a1);
    CASE_EXPECT_EQ(8078970281289081ULL, a2);
    CASE_EXPECT_EQ(13774509987902109069ULL, a3);
}

CASE_TEST(RandomTest, RandomGen64MT11213B)
{
    util::random::MT11213B stGen1;
    stGen1.InitSeed(789);

    uint64_t a1 = stGen1.Random();
    uint64_t a2 = stGen1.Random();
    uint64_t a3 = stGen1();

    CASE_EXPECT_EQ(static_cast<uint32_t>(3740219552), a1);
    CASE_EXPECT_EQ(static_cast<uint32_t>(740436508), a2);
    CASE_EXPECT_EQ(static_cast<uint32_t>(649207690), a3);
}

CASE_TEST(RandomTest, RandomGenTAUS88)
{
    util::random::TAUS88 stGen1;
    stGen1.InitSeed(321);

    uint32_t a1 = stGen1.Random();
    uint32_t a2 = stGen1.Random();
    uint32_t a3 = stGen1();

    CASE_EXPECT_EQ(static_cast<uint32_t>(43258884), a1);
    CASE_EXPECT_EQ(static_cast<uint32_t>(1073987586), a2);
    CASE_EXPECT_EQ(static_cast<uint32_t>(2769562570), a3);
}
