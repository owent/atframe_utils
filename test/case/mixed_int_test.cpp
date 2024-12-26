// Copyright 2021 atframework

#include <typeinfo>
#include "frame/test_macros.h"

#define ATFRAMEWORK_UTILS_ENABLE_MIXEDINT_MAGIC_MASK 3

#include "algorithm/mixed_int.h"

CASE_TEST(mixed_int_test, basic) {
#if defined(ATFRAMEWORK_UTILS_ENABLE_RTTI) && ATFRAMEWORK_UTILS_ENABLE_RTTI
  CASE_EXPECT_TRUE(typeid(uint32_t) != typeid(mixed_uint32_t));
#endif

  {
    mixed_uint8_t a1 = 123;
    CASE_EXPECT_EQ((uint8_t)123, (uint8_t)a1);

    mixed_uint16_t a2(1234);
    CASE_EXPECT_EQ((uint16_t)1234, (uint16_t)a2);

    mixed_uint32_t a3 = 65540;
    CASE_EXPECT_EQ((uint32_t)65540, (uint32_t)a3);

    mixed_uint64_t a4 = 20000000000LL;
    CASE_EXPECT_EQ((uint64_t)20000000000LL, (uint64_t)a4);

    CASE_EXPECT_EQ(4294901756, -a3);
    CASE_EXPECT_EQ(65540, +a3);
  }

  {
    mixed_int8_t a1 = -123;
    CASE_EXPECT_EQ((int8_t)-123, (int8_t)a1);

    mixed_int16_t a2 = -1234;
    CASE_EXPECT_EQ((int16_t)-1234, (int16_t)a2);

    mixed_int32_t a3 = -65540;
    CASE_EXPECT_EQ((int32_t)-65540, (int32_t)a3);

    mixed_int64_t a4 = -20000000000LL;
    CASE_EXPECT_EQ((int64_t)-20000000000LL, (int64_t)a4);

    CASE_EXPECT_EQ(65540, -a3);
    CASE_EXPECT_EQ(-65540, +a3);
  }
}

CASE_TEST(mixed_int_test, compare) {
  mixed_int32_t a2 = -65540;
  mixed_uint32_t a3 = 65540;

  CASE_EXPECT_TRUE((uint32_t)65540 == a3);
  CASE_EXPECT_TRUE(a2 == (int32_t)-65540);

  CASE_EXPECT_TRUE(a3 > (uint32_t)65536);
  CASE_EXPECT_TRUE((int32_t)-70000 < a2);

  mixed_int32_t a1 = 0;
  CASE_EXPECT_TRUE(!a1);
  CASE_EXPECT_FALSE((bool)a1);

  CASE_EXPECT_TRUE((uint32_t)65540 >= a3);
  CASE_EXPECT_TRUE(a2 <= (int32_t)-65540);
}

CASE_TEST(mixed_int_test, calculate) {
  mixed_int32_t a2 = 100;

  CASE_EXPECT_EQ(-10, a2 - 110);
  CASE_EXPECT_EQ(150, a2 + 50);
  CASE_EXPECT_EQ(10, a2 / 10);
  CASE_EXPECT_EQ(1000, a2 * 10);

  a2 += 23;
  CASE_EXPECT_EQ(23, a2 % 100);

  a2 -= 24;
  CASE_EXPECT_EQ(99, (int32_t)a2);

  int32_t x = a2 *= 10;
  CASE_EXPECT_EQ(990, x);

  a2 /= 9;
  a2 %= 100;
  CASE_EXPECT_EQ(10, (int32_t)a2);
}

CASE_TEST(mixed_int_test, bit_operator) {
  mixed_uint64_t a2 = 65535;
  a2 >>= 8;
  CASE_EXPECT_EQ((uint64_t)255, (uint64_t)a2);

  a2 <<= 8;
  a2 |= 0xFF;

  CASE_EXPECT_EQ((uint64_t)65535, (uint64_t)a2);

  a2 ^= 0xFF00;
  CASE_EXPECT_EQ((uint64_t)255, (uint64_t)a2);

  CASE_EXPECT_EQ((uint64_t)15, a2 >> 4);
  CASE_EXPECT_EQ((uint64_t)15, 0xF & a2);
  CASE_EXPECT_EQ((uint64_t)65535, 0xFF00 | a2);
  CASE_EXPECT_EQ((uint64_t)15, 0xF0 ^ a2);
}
