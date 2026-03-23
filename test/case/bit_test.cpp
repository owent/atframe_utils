// Copyright 2026 atframework

#include <algorithm/bit.h>

#include <cstring>

#include "frame/test_macros.h"

CASE_TEST(bit_endian, is_little_endian) {
  // On x86/x64 and ARM (little-endian mode), this should be true
  bool le = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::is_little_endian();
  // Verify with runtime check
  const uint16_t x = 1;
  bool runtime_le = (*reinterpret_cast<const uint8_t *>(&x) == 1);
  CASE_EXPECT_EQ(le, runtime_le);
}

CASE_TEST(bit_endian, endian_enum_values) {
  // little and big must be distinct
  CASE_EXPECT_NE(static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::endian::little),
                 static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::endian::big));

  // native must equal one of them on standard platforms
  auto native_val = static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::endian::native);
  auto little_val = static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::endian::little);
  auto big_val = static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::endian::big);
  CASE_EXPECT_TRUE(native_val == little_val || native_val == big_val);
}

CASE_TEST(bit_endian, endian_enum_consistent_with_is_little_endian) {
  bool le = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::is_little_endian();
  if (le) {
    CASE_EXPECT_EQ(static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::endian::native),
                   static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::endian::little));
  } else {
    CASE_EXPECT_EQ(static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::endian::native),
                   static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::endian::big));
  }
}

// ============================================================================
// Big-endian read tests
// ============================================================================

CASE_TEST(bit_endian, read_be_uint16) {
  // 0x0102 in big-endian byte order
  const unsigned char buf[] = {0x01, 0x02};
  uint16_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint16(buf);
  CASE_EXPECT_EQ(val, static_cast<uint16_t>(0x0102));
}

CASE_TEST(bit_endian, read_be_uint16_max) {
  const unsigned char buf[] = {0xFF, 0xFF};
  uint16_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint16(buf);
  CASE_EXPECT_EQ(val, static_cast<uint16_t>(0xFFFF));
}

CASE_TEST(bit_endian, read_be_uint16_zero) {
  const unsigned char buf[] = {0x00, 0x00};
  uint16_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint16(buf);
  CASE_EXPECT_EQ(val, static_cast<uint16_t>(0));
}

CASE_TEST(bit_endian, read_be_uint32) {
  // 0x01020304 in big-endian byte order
  const unsigned char buf[] = {0x01, 0x02, 0x03, 0x04};
  uint32_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint32(buf);
  CASE_EXPECT_EQ(val, static_cast<uint32_t>(0x01020304));
}

CASE_TEST(bit_endian, read_be_uint32_high_bytes) {
  const unsigned char buf[] = {0xDE, 0xAD, 0xBE, 0xEF};
  uint32_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint32(buf);
  CASE_EXPECT_EQ(val, static_cast<uint32_t>(0xDEADBEEF));
}

CASE_TEST(bit_endian, read_be_uint64) {
  const unsigned char buf[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  uint64_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint64(buf);
  CASE_EXPECT_EQ(val, static_cast<uint64_t>(0x0102030405060708ULL));
}

CASE_TEST(bit_endian, read_be_uint64_high_bytes) {
  const unsigned char buf[] = {0xFE, 0xDC, 0xBA, 0x98, 0x76, 0x54, 0x32, 0x10};
  uint64_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint64(buf);
  CASE_EXPECT_EQ(val, static_cast<uint64_t>(0xFEDCBA9876543210ULL));
}

// ============================================================================
// Big-endian write tests
// ============================================================================

CASE_TEST(bit_endian, write_be_uint16) {
  unsigned char buf[2] = {0};
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_be_uint16(buf, 0x0102);
  CASE_EXPECT_EQ(buf[0], 0x01);
  CASE_EXPECT_EQ(buf[1], 0x02);
}

CASE_TEST(bit_endian, write_be_uint32) {
  unsigned char buf[4] = {0};
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_be_uint32(buf, 0xDEADBEEF);
  CASE_EXPECT_EQ(buf[0], 0xDE);
  CASE_EXPECT_EQ(buf[1], 0xAD);
  CASE_EXPECT_EQ(buf[2], 0xBE);
  CASE_EXPECT_EQ(buf[3], 0xEF);
}

CASE_TEST(bit_endian, write_be_uint64) {
  unsigned char buf[8] = {0};
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_be_uint64(buf, 0xFEDCBA9876543210ULL);
  CASE_EXPECT_EQ(buf[0], 0xFE);
  CASE_EXPECT_EQ(buf[1], 0xDC);
  CASE_EXPECT_EQ(buf[2], 0xBA);
  CASE_EXPECT_EQ(buf[3], 0x98);
  CASE_EXPECT_EQ(buf[4], 0x76);
  CASE_EXPECT_EQ(buf[5], 0x54);
  CASE_EXPECT_EQ(buf[6], 0x32);
  CASE_EXPECT_EQ(buf[7], 0x10);
}

// ============================================================================
// Little-endian read tests
// ============================================================================

CASE_TEST(bit_endian, read_le_uint16) {
  // 0x0201 in little-endian byte order: low byte first
  const unsigned char buf[] = {0x01, 0x02};
  uint16_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint16(buf);
  CASE_EXPECT_EQ(val, static_cast<uint16_t>(0x0201));
}

CASE_TEST(bit_endian, read_le_uint32) {
  // 0x04030201 in little-endian byte order
  const unsigned char buf[] = {0x01, 0x02, 0x03, 0x04};
  uint32_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint32(buf);
  CASE_EXPECT_EQ(val, static_cast<uint32_t>(0x04030201));
}

CASE_TEST(bit_endian, read_le_uint64) {
  const unsigned char buf[] = {0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08};
  uint64_t val = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint64(buf);
  CASE_EXPECT_EQ(val, static_cast<uint64_t>(0x0807060504030201ULL));
}

// ============================================================================
// Little-endian write tests
// ============================================================================

CASE_TEST(bit_endian, write_le_uint16) {
  unsigned char buf[2] = {0};
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_le_uint16(buf, 0x0201);
  CASE_EXPECT_EQ(buf[0], 0x01);
  CASE_EXPECT_EQ(buf[1], 0x02);
}

CASE_TEST(bit_endian, write_le_uint32) {
  unsigned char buf[4] = {0};
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_le_uint32(buf, 0x04030201);
  CASE_EXPECT_EQ(buf[0], 0x01);
  CASE_EXPECT_EQ(buf[1], 0x02);
  CASE_EXPECT_EQ(buf[2], 0x03);
  CASE_EXPECT_EQ(buf[3], 0x04);
}

CASE_TEST(bit_endian, write_le_uint64) {
  unsigned char buf[8] = {0};
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_le_uint64(buf, 0x0807060504030201ULL);
  CASE_EXPECT_EQ(buf[0], 0x01);
  CASE_EXPECT_EQ(buf[1], 0x02);
  CASE_EXPECT_EQ(buf[2], 0x03);
  CASE_EXPECT_EQ(buf[3], 0x04);
  CASE_EXPECT_EQ(buf[4], 0x05);
  CASE_EXPECT_EQ(buf[5], 0x06);
  CASE_EXPECT_EQ(buf[6], 0x07);
  CASE_EXPECT_EQ(buf[7], 0x08);
}

// ============================================================================
// Roundtrip tests
// ============================================================================

CASE_TEST(bit_endian, roundtrip_be_uint16) {
  uint16_t original = 0xABCD;
  unsigned char buf[2];
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_be_uint16(buf, original);
  uint16_t result = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint16(buf);
  CASE_EXPECT_EQ(original, result);
}

CASE_TEST(bit_endian, roundtrip_be_uint32) {
  uint32_t original = 0x12345678;
  unsigned char buf[4];
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_be_uint32(buf, original);
  uint32_t result = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint32(buf);
  CASE_EXPECT_EQ(original, result);
}

CASE_TEST(bit_endian, roundtrip_be_uint64) {
  uint64_t original = 0x123456789ABCDEF0ULL;
  unsigned char buf[8];
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_be_uint64(buf, original);
  uint64_t result = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint64(buf);
  CASE_EXPECT_EQ(original, result);
}

CASE_TEST(bit_endian, roundtrip_le_uint16) {
  uint16_t original = 0xABCD;
  unsigned char buf[2];
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_le_uint16(buf, original);
  uint16_t result = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint16(buf);
  CASE_EXPECT_EQ(original, result);
}

CASE_TEST(bit_endian, roundtrip_le_uint32) {
  uint32_t original = 0x12345678;
  unsigned char buf[4];
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_le_uint32(buf, original);
  uint32_t result = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint32(buf);
  CASE_EXPECT_EQ(original, result);
}

CASE_TEST(bit_endian, roundtrip_le_uint64) {
  uint64_t original = 0x123456789ABCDEF0ULL;
  unsigned char buf[8];
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_le_uint64(buf, original);
  uint64_t result = ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint64(buf);
  CASE_EXPECT_EQ(original, result);
}

// ============================================================================
// Cross-endian consistency: BE and LE produce reversed byte patterns
// ============================================================================

CASE_TEST(bit_endian, cross_endian_uint32) {
  unsigned char be_buf[4], le_buf[4];
  uint32_t value = 0x01020304;
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_be_uint32(be_buf, value);
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_le_uint32(le_buf, value);
  // Big-endian and little-endian should produce reversed byte patterns
  CASE_EXPECT_EQ(be_buf[0], le_buf[3]);
  CASE_EXPECT_EQ(be_buf[1], le_buf[2]);
  CASE_EXPECT_EQ(be_buf[2], le_buf[1]);
  CASE_EXPECT_EQ(be_buf[3], le_buf[0]);
}

CASE_TEST(bit_endian, cross_endian_uint64) {
  unsigned char be_buf[8], le_buf[8];
  uint64_t value = 0x0102030405060708ULL;
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_be_uint64(be_buf, value);
  ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::write_le_uint64(le_buf, value);
  for (int i = 0; i < 8; ++i) {
    CASE_EXPECT_EQ(be_buf[i], le_buf[7 - i]);
  }
}

// ============================================================================
// Boundary value tests
// ============================================================================

CASE_TEST(bit_endian, boundary_values) {
  // Zero
  {
    unsigned char buf[8] = {0};
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint16(buf), static_cast<uint16_t>(0));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint32(buf), static_cast<uint32_t>(0));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint64(buf), static_cast<uint64_t>(0));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint16(buf), static_cast<uint16_t>(0));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint32(buf), static_cast<uint32_t>(0));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint64(buf), static_cast<uint64_t>(0));
  }
  // Max values
  {
    unsigned char buf[8];
    std::memset(buf, 0xFF, sizeof(buf));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint16(buf), static_cast<uint16_t>(0xFFFF));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint32(buf), static_cast<uint32_t>(0xFFFFFFFF));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_be_uint64(buf),
                   static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint16(buf), static_cast<uint16_t>(0xFFFF));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint32(buf), static_cast<uint32_t>(0xFFFFFFFF));
    CASE_EXPECT_EQ(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::read_le_uint64(buf),
                   static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL));
  }
}
