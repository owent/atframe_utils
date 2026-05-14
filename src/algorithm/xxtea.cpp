// Copyright 2026 atframework

// Namespace/API macros are provided by the public header and intentionally used through it here.
// NOLINTBEGIN(misc-include-cleaner)

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <limits>

#include "config/compile_optimize.h"

#include "algorithm/xxtea.h"

#define XXTEA_DELTA 0x9e3779b9
#define XXTEA_MX (((z >> 5 ^ y << 2) + (y >> 3 ^ z << 4)) ^ ((sum ^ y) + (key->data[(p & 3) ^ e] ^ z)))
/*
 * 32-bit integer manipulation macros (big endian)
 */
#ifndef XXTEA_GET_UINT32_BE
#  define XXTEA_GET_UINT32_BE(n, b, i)                                                                    \
                                                                                                          \
    {                                                                                                     \
      (n) = ((uint32_t)(b)[(i)] << 24) | ((uint32_t)(b)[(i) + 1] << 16) | ((uint32_t)(b)[(i) + 2] << 8) | \
            ((uint32_t)(b)[(i) + 3]);                                                                     \
    }
#endif

#ifdef max
#  undef max
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN

namespace {
template <bool CHECK_ENABLE>
struct ATFW_UTIL_SYMBOL_LOCAL xxtea_check_length;

template <>
struct ATFW_UTIL_SYMBOL_LOCAL xxtea_check_length<true> {
  static bool check_protect(size_t len) {
    return len > (static_cast<size_t>(std::numeric_limits<uint32_t>::max()) << 2);
  }
};

template <bool CHECK_ENABLE>
struct ATFW_UTIL_SYMBOL_LOCAL xxtea_check_length {
  static bool check_protect(size_t) { return false; }
};

template <typename Ty>
struct ATFW_UTIL_SYMBOL_LOCAL xxtea_check_length_delegate {
  static constexpr const bool value = sizeof(Ty) > sizeof(uint32_t);
};
}  // namespace

ATFRAMEWORK_UTILS_API void xxtea_setup(xxtea_key *k, const unsigned char filled[4 * sizeof(uint32_t)]) {
  int i = 0;

  memset(k->data, 0, sizeof(k->data));

  for (i = 0; i < 4; i++) {
    XXTEA_GET_UINT32_BE(k->data[i], filled, i << 2);
  }
}

ATFRAMEWORK_UTILS_API void xxtea_encrypt(const xxtea_key *key, void *buffer, size_t len) {
  if (len & 0x03) {
    std::abort();
  }

  if (xxtea_check_length<xxtea_check_length_delegate<size_t>::value>::check_protect(len)) {
    std::abort();
  }

  if (nullptr == key || nullptr == buffer || 0 == len) {
    return;
  }

  uint32_t *v = reinterpret_cast<uint32_t *>(buffer);
  uint32_t n = static_cast<uint32_t>(len >> 2);

  uint32_t y = 0, z = 0, sum = 0;
  uint32_t p = 0, rounds = 0, e = 0;

  rounds = 6 + (52 / n);
  sum = 0;
  z = v[n - 1];
  do {
    sum += XXTEA_DELTA;
    e = (sum >> 2) & 3;
    for (p = 0; p < n - 1; p++) {
      y = v[p + 1];
      z = v[p] += XXTEA_MX;
    }
    y = v[0];
    z = v[n - 1] += XXTEA_MX;
  } while (--rounds);
}

ATFRAMEWORK_UTILS_API void xxtea_encrypt(const xxtea_key *key, const void *input, size_t ilen, void *output,
                                         size_t *olen) {
  bool is_success = false;
  do {
    if (nullptr == key || input == nullptr || ilen <= 0 || output == nullptr || nullptr == olen) {
      break;
    }

    size_t real_olen = ((ilen - 1) | 0x03) + 1;
    if (*olen < real_olen) {
      break;
    }

    if (input != output) {
      memcpy(output, input, ilen);
    }

    if (real_olen > ilen) {
      memset(reinterpret_cast<char *>(output) + ilen, 0, real_olen - ilen);
    }

    *olen = real_olen;
    ATFRAMEWORK_UTILS_NAMESPACE_ID::xxtea_encrypt(key, output, *olen);

    is_success = true;
  } while (false);

  if (!is_success && nullptr != olen) {
    *olen = 0;
  }
}

ATFRAMEWORK_UTILS_API void xxtea_decrypt(const xxtea_key *key, void *buffer, size_t len) {
  if (len & 0x03) {
    std::abort();
  }

  if (xxtea_check_length<xxtea_check_length_delegate<size_t>::value>::check_protect(len)) {
    std::abort();
  }

  if (nullptr == key || nullptr == buffer || 0 == len) {
    return;
  }

  uint32_t *v = reinterpret_cast<uint32_t *>(buffer);
  uint32_t n = static_cast<uint32_t>(len >> 2);

  uint32_t y = 0, z = 0, sum = 0;
  uint32_t p = 0, rounds = 0, e = 0;

  rounds = 6 + (52 / n);
  sum = rounds * XXTEA_DELTA;
  y = v[0];
  do {
    e = (sum >> 2) & 3;
    for (p = n - 1; p > 0; p--) {
      z = v[p - 1];
      y = v[p] -= XXTEA_MX;
    }
    z = v[n - 1];
    y = v[0] -= XXTEA_MX;
    sum -= XXTEA_DELTA;
  } while (--rounds);
}

ATFRAMEWORK_UTILS_API void xxtea_decrypt(const xxtea_key *key, const void *input, size_t ilen, void *output,
                                         size_t *olen) {
  bool is_success = false;
  do {
    if (nullptr == key || input == nullptr || ilen <= 0 || output == nullptr || nullptr == olen) {
      break;
    }

    size_t real_olen = ((ilen - 1) | 0x03) + 1;
    if (*olen < real_olen) {
      break;
    }

    if (input != output) {
      memcpy(output, input, ilen);
    }

    if (real_olen > ilen) {
      memset(reinterpret_cast<char *>(output) + ilen, 0, real_olen - ilen);
    }

    *olen = real_olen;
    ATFRAMEWORK_UTILS_NAMESPACE_ID::xxtea_decrypt(key, output, *olen);

    is_success = true;
  } while (false);

  if (!is_success && nullptr != olen) {
    *olen = 0;
  }
}
ATFRAMEWORK_UTILS_NAMESPACE_END

// NOLINTEND(misc-include-cleaner)
