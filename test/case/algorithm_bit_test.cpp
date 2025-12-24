// Copyright 2025 atframework

#include "frame/test_macros.h"

#include <cstdint>
#include <limits>

#include "algorithm/bit.h"

// ============================================================================
// countl_zero tests - Count leading zeros
// ============================================================================

CASE_TEST(algorithm_bit, countl_zero_u8) {
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint8_t>(0)));
  CASE_EXPECT_EQ(7, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint8_t>(1)));
  CASE_EXPECT_EQ(6, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint8_t>(2)));
  CASE_EXPECT_EQ(6, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint8_t>(3)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint8_t>(0x0F)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint8_t>(0x80)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint8_t>(0xFF)));
}

CASE_TEST(algorithm_bit, countl_zero_u16) {
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint16_t>(0)));
  CASE_EXPECT_EQ(15, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint16_t>(1)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint16_t>(0x00FF)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint16_t>(0x8000)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint16_t>(0xFFFF)));
}

CASE_TEST(algorithm_bit, countl_zero_u32) {
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint32_t>(0)));
  CASE_EXPECT_EQ(31, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint32_t>(1)));
  CASE_EXPECT_EQ(24, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint32_t>(0x000000FF)));
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint32_t>(0x0000FFFF)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint32_t>(0x80000000)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint32_t>(0xFFFFFFFF)));
}

CASE_TEST(algorithm_bit, countl_zero_u64) {
  CASE_EXPECT_EQ(64, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint64_t>(0)));
  CASE_EXPECT_EQ(63, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint64_t>(1)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint64_t>(0x00000000FFFFFFFFULL)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint64_t>(0x8000000000000000ULL)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL)));
}

// ============================================================================
// countl_one tests - Count leading ones
// ============================================================================

CASE_TEST(algorithm_bit, countl_one_u8) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint8_t>(0)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint8_t>(1)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint8_t>(0xFF)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint8_t>(0x80)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint8_t>(0xF0)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint8_t>(0xF7)));
}

CASE_TEST(algorithm_bit, countl_one_u32) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint32_t>(0)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint32_t>(0xFFFFFFFF)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint32_t>(0x80000000)));
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint32_t>(0xFFFF0000)));
}

CASE_TEST(algorithm_bit, countl_one_u64) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint64_t>(0)));
  CASE_EXPECT_EQ(64, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_one(static_cast<uint64_t>(0xFFFFFFFF00000000ULL)));
}

// ============================================================================
// countr_zero tests - Count trailing zeros
// ============================================================================

CASE_TEST(algorithm_bit, countr_zero_u8) {
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint8_t>(0)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint8_t>(1)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint8_t>(2)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint8_t>(0xF0)));
  CASE_EXPECT_EQ(7, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint8_t>(0x80)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint8_t>(0xFF)));
}

CASE_TEST(algorithm_bit, countr_zero_u16) {
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint16_t>(0)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint16_t>(1)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint16_t>(0xFF00)));
  CASE_EXPECT_EQ(15, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint16_t>(0x8000)));
}

CASE_TEST(algorithm_bit, countr_zero_u32) {
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint32_t>(0)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint32_t>(1)));
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint32_t>(0xFFFF0000)));
  CASE_EXPECT_EQ(31, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint32_t>(0x80000000)));
}

CASE_TEST(algorithm_bit, countr_zero_u64) {
  CASE_EXPECT_EQ(64, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint64_t>(0)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint64_t>(1)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint64_t>(0xFFFFFFFF00000000ULL)));
  CASE_EXPECT_EQ(63, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(static_cast<uint64_t>(0x8000000000000000ULL)));
}

// ============================================================================
// countr_one tests - Count trailing ones
// ============================================================================

CASE_TEST(algorithm_bit, countr_one_u8) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint8_t>(0)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint8_t>(1)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint8_t>(0xFF)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint8_t>(0x0F)));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint8_t>(0x80)));
}

CASE_TEST(algorithm_bit, countr_one_u32) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint32_t>(0)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint32_t>(0xFFFFFFFF)));
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint32_t>(0x0000FFFF)));
}

CASE_TEST(algorithm_bit, countr_one_u64) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint64_t>(0)));
  CASE_EXPECT_EQ(64, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(static_cast<uint64_t>(0x00000000FFFFFFFFULL)));
}

// ============================================================================
// popcount tests - Count set bits
// ============================================================================

CASE_TEST(algorithm_bit, popcount_u8) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint8_t>(0)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint8_t>(1)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint8_t>(0xFF)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint8_t>(0x0F)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint8_t>(0xF0)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint8_t>(0xAA)));  // 10101010
}

CASE_TEST(algorithm_bit, popcount_u16) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint16_t>(0)));
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint16_t>(0xFFFF)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint16_t>(0x00FF)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint16_t>(0xAAAA)));  // 1010...
}

CASE_TEST(algorithm_bit, popcount_u32) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint32_t>(0)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint32_t>(0xFFFFFFFF)));
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint32_t>(0x0000FFFF)));
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint32_t>(0xAAAAAAAA)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint32_t>(0x80000000)));
}

CASE_TEST(algorithm_bit, popcount_u64) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint64_t>(0)));
  CASE_EXPECT_EQ(64, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint64_t>(0x00000000FFFFFFFFULL)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(static_cast<uint64_t>(0xAAAAAAAAAAAAAAAAULL)));
}

// ============================================================================
// rotl tests - Rotate left
// ============================================================================

CASE_TEST(algorithm_bit, rotl_u8) {
  CASE_EXPECT_EQ(0x02, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint8_t>(0x01), 1));
  CASE_EXPECT_EQ(0x80, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint8_t>(0x01), 7));
  CASE_EXPECT_EQ(0x01, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint8_t>(0x01), 8));  // Full rotation
  CASE_EXPECT_EQ(0x01, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint8_t>(0x80), 1));  // Wrap around
  CASE_EXPECT_EQ(0xAB, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint8_t>(0xAB), 0));  // No rotation
}

CASE_TEST(algorithm_bit, rotl_u32) {
  CASE_EXPECT_EQ(0x00000002U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint32_t>(0x00000001), 1));
  CASE_EXPECT_EQ(0x80000000U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint32_t>(0x00000001), 31));
  CASE_EXPECT_EQ(0x00000001U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint32_t>(0x00000001), 32));
  CASE_EXPECT_EQ(0x00000001U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint32_t>(0x80000000), 1));
  CASE_EXPECT_EQ(0xDEADBEEFU, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint32_t>(0xDEADBEEF), 0));
}

CASE_TEST(algorithm_bit, rotl_u64) {
  CASE_EXPECT_EQ(0x0000000000000002ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint64_t>(0x0000000000000001ULL), 1));
  CASE_EXPECT_EQ(0x8000000000000000ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint64_t>(0x0000000000000001ULL), 63));
  CASE_EXPECT_EQ(0x0000000000000001ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(static_cast<uint64_t>(0x8000000000000000ULL), 1));
}

// ============================================================================
// rotr tests - Rotate right
// ============================================================================

CASE_TEST(algorithm_bit, rotr_u8) {
  CASE_EXPECT_EQ(0x80, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint8_t>(0x01), 1));  // Wrap around
  CASE_EXPECT_EQ(0x02, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint8_t>(0x01), 7));
  CASE_EXPECT_EQ(0x01, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint8_t>(0x01), 8));  // Full rotation
  CASE_EXPECT_EQ(0x40, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint8_t>(0x80), 1));
  CASE_EXPECT_EQ(0xAB, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint8_t>(0xAB), 0));  // No rotation
}

CASE_TEST(algorithm_bit, rotr_u32) {
  CASE_EXPECT_EQ(0x80000000U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint32_t>(0x00000001), 1));
  CASE_EXPECT_EQ(0x00000002U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint32_t>(0x00000001), 31));
  CASE_EXPECT_EQ(0x00000001U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint32_t>(0x00000001), 32));
  CASE_EXPECT_EQ(0x40000000U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint32_t>(0x80000000), 1));
}

CASE_TEST(algorithm_bit, rotr_u64) {
  CASE_EXPECT_EQ(0x8000000000000000ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint64_t>(0x0000000000000001ULL), 1));
  CASE_EXPECT_EQ(0x0000000000000002ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint64_t>(0x0000000000000001ULL), 63));
  CASE_EXPECT_EQ(0x4000000000000000ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(static_cast<uint64_t>(0x8000000000000000ULL), 1));
}

// ============================================================================
// bit_width tests - Number of bits needed to represent x
// ============================================================================

CASE_TEST(algorithm_bit, bit_width_u8) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint8_t>(0)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint8_t>(1)));
  CASE_EXPECT_EQ(2, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint8_t>(2)));
  CASE_EXPECT_EQ(2, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint8_t>(3)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint8_t>(0x0F)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint8_t>(0xFF)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint8_t>(0x80)));
}

CASE_TEST(algorithm_bit, bit_width_u32) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint32_t>(0)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint32_t>(1)));
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint32_t>(0x0000FFFF)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint32_t>(0xFFFFFFFF)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint32_t>(0x80000000)));
}

CASE_TEST(algorithm_bit, bit_width_u64) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint64_t>(0)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint64_t>(1)));
  CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint64_t>(0x00000000FFFFFFFFULL)));
  CASE_EXPECT_EQ(64, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL)));
}

// ============================================================================
// bit_floor tests - Largest power of 2 not greater than x
// ============================================================================

CASE_TEST(algorithm_bit, bit_floor_u8) {
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint8_t>(0)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint8_t>(1)));
  CASE_EXPECT_EQ(2, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint8_t>(2)));
  CASE_EXPECT_EQ(2, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint8_t>(3)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint8_t>(5)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint8_t>(0x0F)));
  CASE_EXPECT_EQ(0x80, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint8_t>(0xFF)));
}

CASE_TEST(algorithm_bit, bit_floor_u32) {
  CASE_EXPECT_EQ(0U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint32_t>(0)));
  CASE_EXPECT_EQ(1U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint32_t>(1)));
  CASE_EXPECT_EQ(0x8000U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint32_t>(0x0000FFFF)));
  CASE_EXPECT_EQ(0x80000000U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint32_t>(0xFFFFFFFF)));
}

CASE_TEST(algorithm_bit, bit_floor_u64) {
  CASE_EXPECT_EQ(0ULL, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint64_t>(0)));
  CASE_EXPECT_EQ(1ULL, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint64_t>(1)));
  CASE_EXPECT_EQ(0x80000000ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint64_t>(0x00000000FFFFFFFFULL)));
  CASE_EXPECT_EQ(0x8000000000000000ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL)));
}

// ============================================================================
// bit_ceil tests - Smallest power of 2 not less than x
// ============================================================================

CASE_TEST(algorithm_bit, bit_ceil_u8) {
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint8_t>(0)));
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint8_t>(1)));
  CASE_EXPECT_EQ(2, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint8_t>(2)));
  CASE_EXPECT_EQ(4, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint8_t>(3)));
  CASE_EXPECT_EQ(8, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint8_t>(5)));
  CASE_EXPECT_EQ(16, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint8_t>(0x0F)));
  CASE_EXPECT_EQ(0x80, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint8_t>(0x80)));
}

CASE_TEST(algorithm_bit, bit_ceil_u32) {
  CASE_EXPECT_EQ(1U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint32_t>(0)));
  CASE_EXPECT_EQ(1U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint32_t>(1)));
  CASE_EXPECT_EQ(2U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint32_t>(2)));
  CASE_EXPECT_EQ(4U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint32_t>(3)));
  CASE_EXPECT_EQ(0x10000U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint32_t>(0x0000FFFF)));
  CASE_EXPECT_EQ(0x80000000U, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint32_t>(0x80000000)));
}

CASE_TEST(algorithm_bit, bit_ceil_u64) {
  CASE_EXPECT_EQ(1ULL, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint64_t>(0)));
  CASE_EXPECT_EQ(1ULL, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint64_t>(1)));
  CASE_EXPECT_EQ(0x100000000ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint64_t>(0x00000000FFFFFFFFULL)));
  CASE_EXPECT_EQ(0x8000000000000000ULL,
                 ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(static_cast<uint64_t>(0x8000000000000000ULL)));
}

// ============================================================================
// has_single_bit tests - Returns true if x is a power of 2
// ============================================================================

CASE_TEST(algorithm_bit, has_single_bit_u8) {
  CASE_EXPECT_FALSE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint8_t>(0)));
  CASE_EXPECT_TRUE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint8_t>(1)));
  CASE_EXPECT_TRUE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint8_t>(2)));
  CASE_EXPECT_FALSE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint8_t>(3)));
  CASE_EXPECT_TRUE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint8_t>(4)));
  CASE_EXPECT_FALSE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint8_t>(5)));
  CASE_EXPECT_TRUE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint8_t>(0x80)));
  CASE_EXPECT_FALSE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint8_t>(0xFF)));
}

CASE_TEST(algorithm_bit, has_single_bit_u32) {
  CASE_EXPECT_FALSE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint32_t>(0)));
  CASE_EXPECT_TRUE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint32_t>(1)));
  CASE_EXPECT_TRUE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint32_t>(0x80000000)));
  CASE_EXPECT_FALSE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint32_t>(0xFFFFFFFF)));
  CASE_EXPECT_FALSE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint32_t>(0x80000001)));
}

CASE_TEST(algorithm_bit, has_single_bit_u64) {
  CASE_EXPECT_FALSE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint64_t>(0)));
  CASE_EXPECT_TRUE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint64_t>(1)));
  CASE_EXPECT_TRUE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint64_t>(0x8000000000000000ULL)));
  CASE_EXPECT_FALSE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(static_cast<uint64_t>(0xFFFFFFFFFFFFFFFFULL)));
}

// ============================================================================
// Edge cases and comprehensive tests
// ============================================================================

CASE_TEST(algorithm_bit, edge_cases_powers_of_two) {
  // Test all powers of 2 for 32-bit
  for (int i = 0; i < 32; ++i) {
    uint32_t val = static_cast<uint32_t>(1) << i;
    CASE_EXPECT_TRUE(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::has_single_bit(val));
    CASE_EXPECT_EQ(i + 1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(val));
    CASE_EXPECT_EQ(val, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_floor(val));
    CASE_EXPECT_EQ(val, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_ceil(val));
    CASE_EXPECT_EQ(31 - i, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(val));
    CASE_EXPECT_EQ(i, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(val));
    CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::popcount(val));
  }
}

CASE_TEST(algorithm_bit, edge_cases_rotation_identity) {
  // Rotation by 0 should return the same value
  CASE_EXPECT_EQ(0xDEADBEEFU, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(0xDEADBEEFU, 0));
  CASE_EXPECT_EQ(0xDEADBEEFU, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(0xDEADBEEFU, 0));

  // Rotation by bit width should return the same value
  CASE_EXPECT_EQ(0xDEADBEEFU, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(0xDEADBEEFU, 32));
  CASE_EXPECT_EQ(0xDEADBEEFU, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(0xDEADBEEFU, 32));

  // rotl and rotr should be inverses
  uint32_t val = 0x12345678U;
  for (int i = 0; i < 32; ++i) {
    CASE_EXPECT_EQ(val,
                   ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(val, i), i));
    CASE_EXPECT_EQ(val,
                   ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotl(ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::rotr(val, i), i));
  }
}

CASE_TEST(algorithm_bit, edge_cases_count_consistency) {
  // countl_zero + bit_width should equal the bit count
  for (uint32_t val = 1; val != 0; val <<= 1) {
    CASE_EXPECT_EQ(32, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countl_zero(val) +
                           ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::bit_width(val));
  }

  // countr_zero + countr_one should make sense for alternating patterns
  uint32_t alternating = 0xAAAAAAAAU;  // 10101010...
  CASE_EXPECT_EQ(1, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_zero(alternating));
  CASE_EXPECT_EQ(0, ATFRAMEWORK_UTILS_NAMESPACE_ID::bit::countr_one(alternating));
}
