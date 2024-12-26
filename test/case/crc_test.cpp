// Copyright 2021 atframework

#include <cstdlib>
#include <cstring>
#include <string>

#include "frame/test_macros.h"

#include "algorithm/crc.h"

CASE_TEST(crc, crc16) {
  unsigned char data[24] = "123456789";

  CASE_EXPECT_EQ(0x29B1, atfw::util::crc16(data, 9, 0xFFFF));
  CASE_EXPECT_EQ(0xE5CC, atfw::util::crc16(data, 9, 0x1D0F));
  CASE_EXPECT_EQ(0x31C3, atfw::util::crc16(data, 9, 0x0000));
}

CASE_TEST(crc, crc32) {
  unsigned char data[24] = "123456789123456789";

  CASE_EXPECT_EQ(0x4B837AE4, atfw::util::crc32(data, 18, 0xFFFFFFFF) ^ 0xFFFFFFFF);
}

CASE_TEST(crc, crc64) {
  unsigned char data[24] = "123456789123456789";

  CASE_EXPECT_EQ(0x1D240DCFEDFF621BULL, atfw::util::crc64(data, 18, 0xFFFFFFFFFFFFFFFFULL) ^ 0xFFFFFFFFFFFFFFFFULL);
}
