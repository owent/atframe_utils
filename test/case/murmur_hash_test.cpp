// Copyright 2026 atframework

#include <cstdint>
#include <cstring>
#include <string>
#include <vector>

#include "algorithm/murmur_hash.h"
#include "frame/test_macros.h"

CASE_TEST(murmur_hash, hash2_basic) {
  const char *key = "hello world";
  int len = static_cast<int>(strlen(key));
  uint32_t seed = 0;

  uint32_t h1 = atfw::util::hash::murmur_hash2(key, len, seed);
  uint32_t h2 = atfw::util::hash::murmur_hash2(key, len, seed);
  CASE_EXPECT_EQ(h1, h2);

  // Different seed should produce different hash
  uint32_t h3 = atfw::util::hash::murmur_hash2(key, len, 42);
  CASE_EXPECT_NE(h1, h3);

  // Different key should produce different hash
  uint32_t h4 = atfw::util::hash::murmur_hash2("hello world!", 12, seed);
  CASE_EXPECT_NE(h1, h4);
}

CASE_TEST(murmur_hash, hash2_empty) {
  uint32_t h = atfw::util::hash::murmur_hash2("", 0, 0);
  // Empty input should still produce a valid hash
  (void)h;
}

CASE_TEST(murmur_hash, hash2_various_lengths) {
  uint32_t seed = 12345;

  // Test with lengths that exercise different code paths (mod 4)
  const char *data = "abcdefghijklmnop";

  uint32_t h1 = atfw::util::hash::murmur_hash2(data, 1, seed);  // len % 4 == 1
  uint32_t h2 = atfw::util::hash::murmur_hash2(data, 2, seed);  // len % 4 == 2
  uint32_t h3 = atfw::util::hash::murmur_hash2(data, 3, seed);  // len % 4 == 3
  uint32_t h4 = atfw::util::hash::murmur_hash2(data, 4, seed);  // len % 4 == 0
  uint32_t h5 = atfw::util::hash::murmur_hash2(data, 5, seed);  // len % 4 == 1
  uint32_t h8 = atfw::util::hash::murmur_hash2(data, 8, seed);  // len % 4 == 0
  uint32_t h16 = atfw::util::hash::murmur_hash2(data, 16, seed);

  // All should produce unique hashes
  CASE_EXPECT_NE(h1, h2);
  CASE_EXPECT_NE(h2, h3);
  CASE_EXPECT_NE(h3, h4);
  CASE_EXPECT_NE(h4, h5);
  CASE_EXPECT_NE(h5, h8);
  CASE_EXPECT_NE(h8, h16);
}

CASE_TEST(murmur_hash, hash2_64a_basic) {
  const char *key = "hello world";
  int len = static_cast<int>(strlen(key));
  uint64_t seed = 0;

  uint64_t h1 = atfw::util::hash::murmur_hash2_64a(key, len, seed);
  uint64_t h2 = atfw::util::hash::murmur_hash2_64a(key, len, seed);
  CASE_EXPECT_EQ(h1, h2);

  uint64_t h3 = atfw::util::hash::murmur_hash2_64a(key, len, 42);
  CASE_EXPECT_NE(h1, h3);
}

CASE_TEST(murmur_hash, hash2_64a_various_lengths) {
  uint64_t seed = 12345;
  const char *data = "abcdefghijklmnop";

  // Test with lengths that exercise different code paths (mod 8)
  uint64_t h1 = atfw::util::hash::murmur_hash2_64a(data, 1, seed);
  uint64_t h2 = atfw::util::hash::murmur_hash2_64a(data, 3, seed);
  uint64_t h4 = atfw::util::hash::murmur_hash2_64a(data, 4, seed);
  uint64_t h5 = atfw::util::hash::murmur_hash2_64a(data, 5, seed);
  uint64_t h7 = atfw::util::hash::murmur_hash2_64a(data, 7, seed);
  uint64_t h8 = atfw::util::hash::murmur_hash2_64a(data, 8, seed);
  uint64_t h16 = atfw::util::hash::murmur_hash2_64a(data, 16, seed);

  CASE_EXPECT_NE(h1, h2);
  CASE_EXPECT_NE(h4, h5);
  CASE_EXPECT_NE(h7, h8);
  CASE_EXPECT_NE(h8, h16);
}

CASE_TEST(murmur_hash, hash2_64b_basic) {
  const char *key = "hello world";
  int len = static_cast<int>(strlen(key));
  uint64_t seed = 0;

  uint64_t h1 = atfw::util::hash::murmur_hash2_64b(key, len, seed);
  uint64_t h2 = atfw::util::hash::murmur_hash2_64b(key, len, seed);
  CASE_EXPECT_EQ(h1, h2);

  uint64_t h3 = atfw::util::hash::murmur_hash2_64b(key, len, 42);
  CASE_EXPECT_NE(h1, h3);
}

CASE_TEST(murmur_hash, hash2_64b_various_lengths) {
  uint64_t seed = 12345;
  const char *data = "abcdefghijklmnop";

  uint64_t h1 = atfw::util::hash::murmur_hash2_64b(data, 1, seed);
  uint64_t h2 = atfw::util::hash::murmur_hash2_64b(data, 2, seed);
  uint64_t h3 = atfw::util::hash::murmur_hash2_64b(data, 3, seed);
  uint64_t h4 = atfw::util::hash::murmur_hash2_64b(data, 4, seed);
  uint64_t h8 = atfw::util::hash::murmur_hash2_64b(data, 8, seed);
  uint64_t h16 = atfw::util::hash::murmur_hash2_64b(data, 16, seed);

  CASE_EXPECT_NE(h1, h2);
  CASE_EXPECT_NE(h2, h3);
  CASE_EXPECT_NE(h3, h4);
  CASE_EXPECT_NE(h4, h8);
  CASE_EXPECT_NE(h8, h16);
}

CASE_TEST(murmur_hash, hash3_x86_32_basic) {
  const char *key = "hello world";
  int len = static_cast<int>(strlen(key));
  uint32_t seed = 0;

  uint32_t h1 = atfw::util::hash::murmur_hash3_x86_32(key, len, seed);
  uint32_t h2 = atfw::util::hash::murmur_hash3_x86_32(key, len, seed);
  CASE_EXPECT_EQ(h1, h2);

  uint32_t h3 = atfw::util::hash::murmur_hash3_x86_32(key, len, 42);
  CASE_EXPECT_NE(h1, h3);
}

CASE_TEST(murmur_hash, hash3_x86_32_various_lengths) {
  uint32_t seed = 12345;
  const char *data = "abcdefghijklmnop";

  // MurmurHash3 processes 4 bytes at a time, so test different remainders
  uint32_t h1 = atfw::util::hash::murmur_hash3_x86_32(data, 1, seed);
  uint32_t h2 = atfw::util::hash::murmur_hash3_x86_32(data, 2, seed);
  uint32_t h3 = atfw::util::hash::murmur_hash3_x86_32(data, 3, seed);
  uint32_t h4 = atfw::util::hash::murmur_hash3_x86_32(data, 4, seed);
  uint32_t h5 = atfw::util::hash::murmur_hash3_x86_32(data, 5, seed);

  CASE_EXPECT_NE(h1, h2);
  CASE_EXPECT_NE(h2, h3);
  CASE_EXPECT_NE(h3, h4);
  CASE_EXPECT_NE(h4, h5);
}

CASE_TEST(murmur_hash, hash3_x86_128_basic) {
  const char *key = "hello world";
  int len = static_cast<int>(strlen(key));
  uint32_t seed = 0;

  uint32_t out1[4] = {0};
  uint32_t out2[4] = {0};
  atfw::util::hash::murmur_hash3_x86_128(key, len, seed, out1);
  atfw::util::hash::murmur_hash3_x86_128(key, len, seed, out2);
  CASE_EXPECT_EQ(0, memcmp(out1, out2, sizeof(out1)));

  // Different seed
  uint32_t out3[4] = {0};
  atfw::util::hash::murmur_hash3_x86_128(key, len, 42, out3);
  CASE_EXPECT_NE(0, memcmp(out1, out3, sizeof(out1)));
}

CASE_TEST(murmur_hash, hash3_x86_128_various_lengths) {
  uint32_t seed = 12345;
  const char *data = "abcdefghijklmnopqrstuvwxyz";

  // MurmurHash3_x86_128 processes 16 bytes at a time
  uint32_t out1[4], out2[4], out3[4], out4[4], out5[4];
  atfw::util::hash::murmur_hash3_x86_128(data, 1, seed, out1);
  atfw::util::hash::murmur_hash3_x86_128(data, 5, seed, out2);
  atfw::util::hash::murmur_hash3_x86_128(data, 10, seed, out3);
  atfw::util::hash::murmur_hash3_x86_128(data, 15, seed, out4);
  atfw::util::hash::murmur_hash3_x86_128(data, 16, seed, out5);

  CASE_EXPECT_NE(0, memcmp(out1, out2, sizeof(out1)));
  CASE_EXPECT_NE(0, memcmp(out2, out3, sizeof(out1)));
  CASE_EXPECT_NE(0, memcmp(out3, out4, sizeof(out1)));
  CASE_EXPECT_NE(0, memcmp(out4, out5, sizeof(out1)));
}

CASE_TEST(murmur_hash, hash3_x64_128_basic) {
  const char *key = "hello world";
  int len = static_cast<int>(strlen(key));
  uint32_t seed = 0;

  uint64_t out1[2] = {0};
  uint64_t out2[2] = {0};
  atfw::util::hash::murmur_hash3_x64_128(key, len, seed, out1);
  atfw::util::hash::murmur_hash3_x64_128(key, len, seed, out2);
  CASE_EXPECT_EQ(out1[0], out2[0]);
  CASE_EXPECT_EQ(out1[1], out2[1]);

  uint64_t out3[2] = {0};
  atfw::util::hash::murmur_hash3_x64_128(key, len, 42, out3);
  CASE_EXPECT_TRUE(out1[0] != out3[0] || out1[1] != out3[1]);
}

CASE_TEST(murmur_hash, hash3_x64_128_various_lengths) {
  uint32_t seed = 12345;
  const char *data = "abcdefghijklmnopqrstuvwxyz";

  // MurmurHash3_x64_128 processes 16 bytes at a time
  uint64_t out1[2], out2[2], out3[2], out4[2], out5[2];
  atfw::util::hash::murmur_hash3_x64_128(data, 1, seed, out1);
  atfw::util::hash::murmur_hash3_x64_128(data, 7, seed, out2);
  atfw::util::hash::murmur_hash3_x64_128(data, 8, seed, out3);
  atfw::util::hash::murmur_hash3_x64_128(data, 15, seed, out4);
  atfw::util::hash::murmur_hash3_x64_128(data, 16, seed, out5);

  CASE_EXPECT_TRUE(out1[0] != out2[0] || out1[1] != out2[1]);
  CASE_EXPECT_TRUE(out2[0] != out3[0] || out2[1] != out3[1]);
  CASE_EXPECT_TRUE(out3[0] != out4[0] || out3[1] != out4[1]);
  CASE_EXPECT_TRUE(out4[0] != out5[0] || out4[1] != out5[1]);
}

CASE_TEST(murmur_hash, hash3_x86_32_empty) {
  uint32_t h = atfw::util::hash::murmur_hash3_x86_32("", 0, 0);
  (void)h;
}

CASE_TEST(murmur_hash, hash3_x86_128_empty) {
  uint32_t out[4] = {0};
  atfw::util::hash::murmur_hash3_x86_128("", 0, 0, out);
  // Should not crash
}

CASE_TEST(murmur_hash, hash3_x64_128_empty) {
  uint64_t out[2] = {0};
  atfw::util::hash::murmur_hash3_x64_128("", 0, 0, out);
  // Should not crash
}

CASE_TEST(murmur_hash, known_reference_vectors) {
  // These known-answer values are compatible with Austin Appleby's SMHasher reference implementation.
  // The MurmurHash3_x86_32 vector is also published by scikit-learn:
  // murmurhash3_32(b"Hello World!", seed=42) == 3565178 == 0x0036667a.
  static constexpr char kPayload[] = "Hello World!";
  static constexpr int kPayloadLength = static_cast<int>(sizeof(kPayload) - 1U);
  static constexpr uint32_t kSeed32 = 42U;
  static constexpr uint64_t kSeed64 = 42U;

  CASE_EXPECT_EQ(UINT32_C(0x3d9fc202), atfw::util::hash::murmur_hash2(kPayload, kPayloadLength, kSeed32));
  CASE_EXPECT_EQ(UINT64_C(0x22e9f6747266b120), atfw::util::hash::murmur_hash2_64a(kPayload, kPayloadLength, kSeed64));
  CASE_EXPECT_EQ(UINT64_C(0x57236b9bcf30d7cc), atfw::util::hash::murmur_hash2_64b(kPayload, kPayloadLength, kSeed64));
  CASE_EXPECT_EQ(UINT32_C(0x0036667a), atfw::util::hash::murmur_hash3_x86_32(kPayload, kPayloadLength, kSeed32));

  uint32_t x86_128[4] = {0};
  atfw::util::hash::murmur_hash3_x86_128(kPayload, kPayloadLength, kSeed32, x86_128);
  CASE_EXPECT_EQ(UINT32_C(0x7f188edf), x86_128[0]);
  CASE_EXPECT_EQ(UINT32_C(0x42a77b58), x86_128[1]);
  CASE_EXPECT_EQ(UINT32_C(0xb6161cc7), x86_128[2]);
  CASE_EXPECT_EQ(UINT32_C(0x9359780e), x86_128[3]);

  uint64_t x64_128[2] = {0};
  atfw::util::hash::murmur_hash3_x64_128(kPayload, kPayloadLength, kSeed32, x64_128);
  CASE_EXPECT_EQ(UINT64_C(0xa7bc102ca798f8dc), x64_128[0]);
  CASE_EXPECT_EQ(UINT64_C(0x3c957070a5f9fa3e), x64_128[1]);
}

CASE_TEST(murmur_hash, unaligned_input_matches_aligned) {
  static constexpr char kPayload[] = "0123456789abcdef0123456789abcdef";
  static constexpr int kPayloadLength = static_cast<int>(sizeof(kPayload) - 1U);

  alignas(8) unsigned char aligned_storage[sizeof(kPayload)] = {0};
  std::memcpy(aligned_storage, kPayload, static_cast<size_t>(kPayloadLength));

  std::vector<unsigned char> unaligned_storage(static_cast<size_t>(kPayloadLength) + 1U, 0);
  std::memcpy(unaligned_storage.data() + 1U, kPayload, static_cast<size_t>(kPayloadLength));

  const void *aligned_key = aligned_storage;
  const void *unaligned_key = unaligned_storage.data() + 1U;

  CASE_EXPECT_EQ(atfw::util::hash::murmur_hash2(aligned_key, kPayloadLength, 0),
                 atfw::util::hash::murmur_hash2(unaligned_key, kPayloadLength, 0));
  CASE_EXPECT_EQ(atfw::util::hash::murmur_hash2_64a(aligned_key, kPayloadLength, 0),
                 atfw::util::hash::murmur_hash2_64a(unaligned_key, kPayloadLength, 0));
  CASE_EXPECT_EQ(atfw::util::hash::murmur_hash2_64b(aligned_key, kPayloadLength, 0),
                 atfw::util::hash::murmur_hash2_64b(unaligned_key, kPayloadLength, 0));
  CASE_EXPECT_EQ(atfw::util::hash::murmur_hash3_x86_32(aligned_key, kPayloadLength, 0),
                 atfw::util::hash::murmur_hash3_x86_32(unaligned_key, kPayloadLength, 0));

  uint32_t aligned_x86_128[4] = {0};
  uint32_t unaligned_x86_128[4] = {0};
  atfw::util::hash::murmur_hash3_x86_128(aligned_key, kPayloadLength, 0, aligned_x86_128);
  atfw::util::hash::murmur_hash3_x86_128(unaligned_key, kPayloadLength, 0, unaligned_x86_128);
  CASE_EXPECT_EQ(0, std::memcmp(aligned_x86_128, unaligned_x86_128, sizeof(aligned_x86_128)));

  uint64_t aligned_x64_128[2] = {0};
  uint64_t unaligned_x64_128[2] = {0};
  atfw::util::hash::murmur_hash3_x64_128(aligned_key, kPayloadLength, 0, aligned_x64_128);
  atfw::util::hash::murmur_hash3_x64_128(unaligned_key, kPayloadLength, 0, unaligned_x64_128);
  CASE_EXPECT_EQ(0, std::memcmp(aligned_x64_128, unaligned_x64_128, sizeof(aligned_x64_128)));
}

CASE_TEST(murmur_hash, hash2_distribution) {
  // Test distribution by hashing sequential integers
  const int num_buckets = 64;
  int buckets[64] = {0};
  const int num_keys = 10000;

  for (int i = 0; i < num_keys; ++i) {
    uint32_t h = atfw::util::hash::murmur_hash2(&i, sizeof(i), 0);
    buckets[h % num_buckets]++;
  }

  // Each bucket should have roughly num_keys / num_buckets entries
  double expected = static_cast<double>(num_keys) / num_buckets;
  for (int i = 0; i < num_buckets; ++i) {
    // Allow 50% deviation for reasonable distribution
    CASE_EXPECT_GT(buckets[i], static_cast<int>(expected * 0.5));
    CASE_EXPECT_LT(buckets[i], static_cast<int>(expected * 1.5));
  }
}
