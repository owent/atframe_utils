// Copyright 2026 atframework

#include <cstddef>
#include <cstdint>

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <std/explicit_declare.h>

#include "algorithm/murmur_hash.h"

//-----------------------------------------------------------------------------
// Platform-specific functions and macros

namespace {
ATFW_UTIL_FORCEINLINE static uint32_t rotl32(uint32_t x, int8_t r) noexcept { return (x << r) | (x >> (32 - r)); }

ATFW_UTIL_FORCEINLINE static uint64_t rotl64(uint64_t x, int8_t r) noexcept { return (x << r) | (x >> (64 - r)); }

#define ROTL32(x, y) rotl32((x), (y))
#define ROTL64(x, y) rotl64((x), (y))
#define BIG_CONSTANT(x) UINT64_C(x)

#if defined(_WIN32) || defined(__LITTLE_ENDIAN__) || \
    (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#  define ATFW_UTIL_MURMUR_HASH_COMPILETIME_LITTLE_ENDIAN 1
#else
#  define ATFW_UTIL_MURMUR_HASH_COMPILETIME_LITTLE_ENDIAN 0
#endif

#if defined(__i386__) || defined(__x86_64__) || defined(__aarch64__) || defined(_M_IX86) || defined(_M_X64) || \
    defined(_M_ARM64)
#  define ATFW_UTIL_MURMUR_HASH_HAS_FAST_UNALIGNED_LOAD 1
#else
#  define ATFW_UTIL_MURMUR_HASH_HAS_FAST_UNALIGNED_LOAD 0
#endif

#if ATFW_UTIL_MURMUR_HASH_COMPILETIME_LITTLE_ENDIAN && ATFW_UTIL_MURMUR_HASH_HAS_FAST_UNALIGNED_LOAD
#  define ATFW_UTIL_MURMUR_HASH_USE_FAST_UNALIGNED_LOAD 1
#else
#  define ATFW_UTIL_MURMUR_HASH_USE_FAST_UNALIGNED_LOAD 0
#endif

#if ATFW_UTIL_MURMUR_HASH_USE_FAST_UNALIGNED_LOAD
#  if defined(__clang__) || defined(__GNUC__)
using murmur_hash_alias_uint32_t = ATFW_EXPLICIT_MAY_ALIAS uint32_t;
using murmur_hash_alias_uint64_t = ATFW_EXPLICIT_MAY_ALIAS uint64_t;

struct __attribute__((__packed__)) murmur_hash_unaligned_uint32_t {
  murmur_hash_alias_uint32_t value;
};

struct __attribute__((__packed__)) murmur_hash_unaligned_uint64_t {
  murmur_hash_alias_uint64_t value;
};
#  elif defined(_MSC_VER)
#    pragma pack(push, 1)
struct murmur_hash_unaligned_uint32_t {
  uint32_t value;
};

struct murmur_hash_unaligned_uint64_t {
  uint64_t value;
};
#    pragma pack(pop)
#  endif
#endif

//-----------------------------------------------------------------------------
// Block read - if your platform needs to do endian-swapping or can only
// handle aligned reads, do the conversion here

#if !ATFW_UTIL_MURMUR_HASH_USE_FAST_UNALIGNED_LOAD
ATFW_UTIL_FORCEINLINE static uint32_t getblock32_fallback(const uint8_t *p) noexcept {
  return static_cast<uint32_t>(p[0]) | (static_cast<uint32_t>(p[1]) << 8U) | (static_cast<uint32_t>(p[2]) << 16U) |
         (static_cast<uint32_t>(p[3]) << 24U);
}

ATFW_UTIL_FORCEINLINE static uint64_t getblock64_fallback(const uint8_t *p) noexcept {
  return static_cast<uint64_t>(p[0]) | (static_cast<uint64_t>(p[1]) << 8U) | (static_cast<uint64_t>(p[2]) << 16U) |
         (static_cast<uint64_t>(p[3]) << 24U) | (static_cast<uint64_t>(p[4]) << 32U) |
         (static_cast<uint64_t>(p[5]) << 40U) | (static_cast<uint64_t>(p[6]) << 48U) |
         (static_cast<uint64_t>(p[7]) << 56U);
}
#endif

ATFW_UTIL_FORCEINLINE static uint32_t getblock32(const void *p) noexcept {
#if ATFW_UTIL_MURMUR_HASH_USE_FAST_UNALIGNED_LOAD
  return static_cast<const murmur_hash_unaligned_uint32_t *>(p)->value;
#else
  return getblock32_fallback(static_cast<const uint8_t *>(p));
#endif
}

ATFW_UTIL_FORCEINLINE static uint64_t getblock64(const void *p) noexcept {
#if ATFW_UTIL_MURMUR_HASH_USE_FAST_UNALIGNED_LOAD
  return static_cast<const murmur_hash_unaligned_uint64_t *>(p)->value;
#else
  return getblock64_fallback(static_cast<const uint8_t *>(p));
#endif
}

//-----------------------------------------------------------------------------
// Finalization mix - force all bits of a hash block to avalanche

ATFW_UTIL_FORCEINLINE static uint32_t fmix32(uint32_t h) noexcept {
  h ^= h >> 16;
  h *= 0x85ebca6b;
  h ^= h >> 13;
  h *= 0xc2b2ae35;
  h ^= h >> 16;

  return h;
}

//----------

ATFW_UTIL_FORCEINLINE static uint64_t fmix64(uint64_t k) noexcept {
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xff51afd7ed558ccd);
  k ^= k >> 33;
  k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
  k ^= k >> 33;

  return k;
}
}  // namespace

//-----------------------------------------------------------------------------

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace hash {

// ===================== MurmurHash2 =====================
//-----------------------------------------------------------------------------

ATFRAMEWORK_UTILS_API uint32_t murmur_hash2(const void *key, int len, uint32_t seed) {
  // 'm' and 'r' are mixing constants generated offline.
  // They're not really 'magic', they just happen to work well.

  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  // Initialize the hash to a 'random' value

  uint32_t h = seed ^ static_cast<uint32_t>(len);

  // Mix 4 bytes at a time into the hash

  const auto *data = static_cast<const uint8_t *>(key);

  while (len >= 4) {
    uint32_t k = getblock32(data);

    k *= m;
    k ^= k >> r;
    k *= m;

    h *= m;
    h ^= k;

    data += 4;
    len -= 4;
  }

  // Handle the last few bytes of the input array

  switch (len) {
    case 3:
      h ^= static_cast<uint32_t>(data[2]) << 16;
      ATFW_EXPLICIT_FALLTHROUGH
    case 2:
      h ^= static_cast<uint32_t>(data[1]) << 8;
      ATFW_EXPLICIT_FALLTHROUGH
    case 1:
      h ^= static_cast<uint32_t>(data[0]);
      h *= m;
  }

  // Do a few final mixes of the hash to ensure the last few
  // bytes are well-incorporated.

  h ^= h >> 13;
  h *= m;
  h ^= h >> 15;

  return h;
}

//-----------------------------------------------------------------------------
// MurmurHash2, 64-bit versions, by Austin Appleby

// The same caveats as 32-bit MurmurHash2 apply here - beware of alignment
// and endian-ness issues if used across multiple platforms.

// 64-bit hash for 64-bit platforms

ATFRAMEWORK_UTILS_API uint64_t murmur_hash2_64a(const void *key, int len, uint64_t seed) {
  const uint64_t m = BIG_CONSTANT(0xc6a4a7935bd1e995);
  const int r = 47;

  uint64_t h = seed ^ (static_cast<uint64_t>(len) * m);

  const auto *data = static_cast<const uint8_t *>(key);
  const auto *end = data + (static_cast<size_t>(len / 8) * 8U);

  while (data != end) {
    uint64_t k = getblock64(data);
    data += 8U;

    k *= m;
    k ^= k >> r;
    k *= m;

    h ^= k;
    h *= m;
  }

  const auto *data2 = data;

  switch (len & 7) {
    case 7:
      h ^= static_cast<uint64_t>(data2[6]) << 48U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 6:
      h ^= static_cast<uint64_t>(data2[5]) << 40U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 5:
      h ^= static_cast<uint64_t>(data2[4]) << 32U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 4:
      h ^= static_cast<uint64_t>(data2[3]) << 24U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 3:
      h ^= static_cast<uint64_t>(data2[2]) << 16U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 2:
      h ^= static_cast<uint64_t>(data2[1]) << 8U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 1:
      h ^= static_cast<uint64_t>(data2[0]);
      h *= m;
  }

  h ^= h >> r;
  h *= m;
  h ^= h >> r;

  return h;
}

// 64-bit hash for 32-bit platforms

ATFRAMEWORK_UTILS_API uint64_t murmur_hash2_64b(const void *key, int len, uint64_t seed) {
  const uint32_t m = 0x5bd1e995;
  const int r = 24;

  uint32_t h1 = static_cast<uint32_t>(seed) ^ static_cast<uint32_t>(len);
  uint32_t h2 = static_cast<uint32_t>(seed >> 32U);

  const auto *data = static_cast<const uint8_t *>(key);

  while (len >= 8) {
    uint32_t k1 = getblock32(data);
    data += 4U;
    k1 *= m;
    k1 ^= k1 >> r;
    k1 *= m;
    h1 *= m;
    h1 ^= k1;
    len -= 4;

    uint32_t k2 = getblock32(data);
    data += 4U;
    k2 *= m;
    k2 ^= k2 >> r;
    k2 *= m;
    h2 *= m;
    h2 ^= k2;
    len -= 4;
  }

  if (len >= 4) {
    uint32_t k1 = getblock32(data);
    data += 4U;
    k1 *= m;
    k1 ^= k1 >> r;
    k1 *= m;
    h1 *= m;
    h1 ^= k1;
    len -= 4;
  }

  switch (len) {
    case 3:
      h2 ^= static_cast<uint32_t>(data[2]) << 16U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 2:
      h2 ^= static_cast<uint32_t>(data[1]) << 8U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 1:
      h2 ^= static_cast<uint32_t>(data[0]);
      h2 *= m;
  }

  h1 ^= h2 >> 18;
  h1 *= m;
  h2 ^= h1 >> 22;
  h2 *= m;
  h1 ^= h2 >> 17;
  h1 *= m;
  h2 ^= h1 >> 19;
  h2 *= m;

  uint64_t h = h1;

  h = (h << 32) | h2;

  return h;
}

// ===================== MurmurHash3 =====================
ATFRAMEWORK_UTILS_API uint32_t murmur_hash3_x86_32(const void *key, int len, uint32_t seed) {
  const auto *data = static_cast<const uint8_t *>(key);
  const int nblocks = len / 4;

  uint32_t h1 = seed;

  const uint32_t c1 = 0xcc9e2d51;
  const uint32_t c2 = 0x1b873593;

  //----------
  // body

  const uint8_t *blocks = data;
  for (int i = 0; i < nblocks; ++i) {
    uint32_t k1 = getblock32(blocks);
    blocks += 4U;

    k1 *= c1;
    k1 = ROTL32(k1, 15);
    k1 *= c2;

    h1 ^= k1;
    h1 = ROTL32(h1, 13);
    h1 = (h1 * 5U) + 0xe6546b64U;
  }

  //----------
  // tail

  const auto *tail = data + (static_cast<size_t>(nblocks) * 4U);

  uint32_t k1 = 0;

  switch (len & 3) {
    case 3:
      k1 ^= static_cast<uint32_t>(tail[2]) << 16;
      ATFW_EXPLICIT_FALLTHROUGH
    case 2:
      k1 ^= static_cast<uint32_t>(tail[1]) << 8;
      ATFW_EXPLICIT_FALLTHROUGH
    case 1:
      k1 ^= static_cast<uint32_t>(tail[0]);
      k1 *= c1;
      k1 = ROTL32(k1, 15);
      k1 *= c2;
      h1 ^= k1;
  }

  //----------
  // finalization

  h1 ^= static_cast<uint32_t>(len);

  h1 = fmix32(h1);

  return h1;
}

//-----------------------------------------------------------------------------

ATFRAMEWORK_UTILS_API void murmur_hash3_x86_128(const void *key, const int len, uint32_t seed, uint32_t out[4]) {
  const auto *data = static_cast<const uint8_t *>(key);
  const int nblocks = len / 16;

  uint32_t h1 = seed;
  uint32_t h2 = seed;
  uint32_t h3 = seed;
  uint32_t h4 = seed;

  const uint32_t c1 = 0x239b961b;
  const uint32_t c2 = 0xab0e9789;
  const uint32_t c3 = 0x38b34ae5;
  const uint32_t c4 = 0xa1e38b93;

  //----------
  // body

  const uint8_t *blocks = data;
  for (int i = 0; i < nblocks; ++i) {
    uint32_t k1 = getblock32(blocks);
    uint32_t k2 = getblock32(blocks + 4U);
    uint32_t k3 = getblock32(blocks + 8U);
    uint32_t k4 = getblock32(blocks + 12U);
    blocks += 16U;

    k1 *= c1;
    k1 = ROTL32(k1, 15);
    k1 *= c2;
    h1 ^= k1;

    h1 = ROTL32(h1, 19);
    h1 += h2;
    h1 = (h1 * 5U) + 0x561ccd1bU;

    k2 *= c2;
    k2 = ROTL32(k2, 16);
    k2 *= c3;
    h2 ^= k2;

    h2 = ROTL32(h2, 17);
    h2 += h3;
    h2 = (h2 * 5U) + 0x0bcaa747U;

    k3 *= c3;
    k3 = ROTL32(k3, 17);
    k3 *= c4;
    h3 ^= k3;

    h3 = ROTL32(h3, 15);
    h3 += h4;
    h3 = (h3 * 5U) + 0x96cd1c35U;

    k4 *= c4;
    k4 = ROTL32(k4, 18);
    k4 *= c1;
    h4 ^= k4;

    h4 = ROTL32(h4, 13);
    h4 += h1;
    h4 = (h4 * 5U) + 0x32ac3b17U;
  }

  //----------
  // tail

  const auto *tail = data + (static_cast<size_t>(nblocks) * 16U);

  uint32_t k1 = 0;
  uint32_t k2 = 0;
  uint32_t k3 = 0;
  uint32_t k4 = 0;

  switch (len & 15) {
    case 15:
      k4 ^= static_cast<uint32_t>(tail[14]) << 16;
      ATFW_EXPLICIT_FALLTHROUGH
    case 14:
      k4 ^= static_cast<uint32_t>(tail[13]) << 8;
      ATFW_EXPLICIT_FALLTHROUGH
    case 13:
      k4 ^= static_cast<uint32_t>(tail[12]) << 0;
      k4 *= c4;
      k4 = ROTL32(k4, 18);
      k4 *= c1;
      h4 ^= k4;
      ATFW_EXPLICIT_FALLTHROUGH
    case 12:
      k3 ^= static_cast<uint32_t>(tail[11]) << 24;
      ATFW_EXPLICIT_FALLTHROUGH
    case 11:
      k3 ^= static_cast<uint32_t>(tail[10]) << 16;
      ATFW_EXPLICIT_FALLTHROUGH
    case 10:
      k3 ^= static_cast<uint32_t>(tail[9]) << 8;
      ATFW_EXPLICIT_FALLTHROUGH
    case 9:
      k3 ^= static_cast<uint32_t>(tail[8]) << 0;
      k3 *= c3;
      k3 = ROTL32(k3, 17);
      k3 *= c4;
      h3 ^= k3;
      ATFW_EXPLICIT_FALLTHROUGH
    case 8:
      k2 ^= static_cast<uint32_t>(tail[7]) << 24;
      ATFW_EXPLICIT_FALLTHROUGH
    case 7:
      k2 ^= static_cast<uint32_t>(tail[6]) << 16;
      ATFW_EXPLICIT_FALLTHROUGH
    case 6:
      k2 ^= static_cast<uint32_t>(tail[5]) << 8;
      ATFW_EXPLICIT_FALLTHROUGH
    case 5:
      k2 ^= static_cast<uint32_t>(tail[4]) << 0;
      k2 *= c2;
      k2 = ROTL32(k2, 16);
      k2 *= c3;
      h2 ^= k2;
      ATFW_EXPLICIT_FALLTHROUGH
    case 4:
      k1 ^= static_cast<uint32_t>(tail[3]) << 24;
      ATFW_EXPLICIT_FALLTHROUGH
    case 3:
      k1 ^= static_cast<uint32_t>(tail[2]) << 16;
      ATFW_EXPLICIT_FALLTHROUGH
    case 2:
      k1 ^= static_cast<uint32_t>(tail[1]) << 8;
      ATFW_EXPLICIT_FALLTHROUGH
    case 1:
      k1 ^= static_cast<uint32_t>(tail[0]) << 0;
      k1 *= c1;
      k1 = ROTL32(k1, 15);
      k1 *= c2;
      h1 ^= k1;
  }

  //----------
  // finalization

  h1 ^= static_cast<uint32_t>(len);
  h2 ^= static_cast<uint32_t>(len);
  h3 ^= static_cast<uint32_t>(len);
  h4 ^= static_cast<uint32_t>(len);

  h1 += h2;
  h1 += h3;
  h1 += h4;
  h2 += h1;
  h3 += h1;
  h4 += h1;

  h1 = fmix32(h1);
  h2 = fmix32(h2);
  h3 = fmix32(h3);
  h4 = fmix32(h4);

  h1 += h2;
  h1 += h3;
  h1 += h4;
  h2 += h1;
  h3 += h1;
  h4 += h1;

  out[0] = h1;
  out[1] = h2;
  out[2] = h3;
  out[3] = h4;
}

//-----------------------------------------------------------------------------

ATFRAMEWORK_UTILS_API void murmur_hash3_x64_128(const void *key, const int len, const uint32_t seed, uint64_t out[2]) {
  const auto *data = static_cast<const uint8_t *>(key);
  const int nblocks = len / 16;

  uint64_t h1 = seed;
  uint64_t h2 = seed;

  const uint64_t c1 = BIG_CONSTANT(0x87c37b91114253d5);
  const uint64_t c2 = BIG_CONSTANT(0x4cf5ad432745937f);

  //----------
  // body

  const uint8_t *blocks = data;
  for (int i = 0; i < nblocks; ++i) {
    uint64_t k1 = getblock64(blocks);
    uint64_t k2 = getblock64(blocks + 8U);
    blocks += 16U;

    k1 *= c1;
    k1 = ROTL64(k1, 31);
    k1 *= c2;
    h1 ^= k1;

    h1 = ROTL64(h1, 27);
    h1 += h2;
    h1 = (h1 * UINT64_C(5)) + UINT64_C(0x52dce729);

    k2 *= c2;
    k2 = ROTL64(k2, 33);
    k2 *= c1;
    h2 ^= k2;

    h2 = ROTL64(h2, 31);
    h2 += h1;
    h2 = (h2 * UINT64_C(5)) + UINT64_C(0x38495ab5);
  }

  //----------
  // tail

  const auto *tail = data + (static_cast<size_t>(nblocks) * 16U);

  uint64_t k1 = 0;
  uint64_t k2 = 0;

  switch (len & 15) {
    case 15:
      k2 ^= static_cast<uint64_t>(tail[14]) << 48U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 14:
      k2 ^= static_cast<uint64_t>(tail[13]) << 40U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 13:
      k2 ^= static_cast<uint64_t>(tail[12]) << 32U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 12:
      k2 ^= static_cast<uint64_t>(tail[11]) << 24U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 11:
      k2 ^= static_cast<uint64_t>(tail[10]) << 16U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 10:
      k2 ^= static_cast<uint64_t>(tail[9]) << 8U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 9:
      k2 ^= static_cast<uint64_t>(tail[8]);
      k2 *= c2;
      k2 = ROTL64(k2, 33);
      k2 *= c1;
      h2 ^= k2;
      ATFW_EXPLICIT_FALLTHROUGH
    case 8:
      k1 ^= static_cast<uint64_t>(tail[7]) << 56U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 7:
      k1 ^= static_cast<uint64_t>(tail[6]) << 48U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 6:
      k1 ^= static_cast<uint64_t>(tail[5]) << 40U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 5:
      k1 ^= static_cast<uint64_t>(tail[4]) << 32U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 4:
      k1 ^= static_cast<uint64_t>(tail[3]) << 24U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 3:
      k1 ^= static_cast<uint64_t>(tail[2]) << 16U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 2:
      k1 ^= static_cast<uint64_t>(tail[1]) << 8U;
      ATFW_EXPLICIT_FALLTHROUGH
    case 1:
      k1 ^= static_cast<uint64_t>(tail[0]);
      k1 *= c1;
      k1 = ROTL64(k1, 31);
      k1 *= c2;
      h1 ^= k1;
  };

  //----------
  // finalization

  h1 ^= static_cast<uint64_t>(len);
  h2 ^= static_cast<uint64_t>(len);

  h1 += h2;
  h2 += h1;

  h1 = fmix64(h1);
  h2 = fmix64(h2);

  h1 += h2;
  h2 += h1;

  out[0] = h1;
  out[1] = h2;
}

//-----------------------------------------------------------------------------
}  // namespace hash
ATFRAMEWORK_UTILS_NAMESPACE_END
