/**
 * @file sha.h
 * @brief sha算法适配,如果没有openssl和mbedtls则使用内置的软实现
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2019.12.20
 *
 * @history
 *
 *
 */

#ifndef UTIL_ALGORITHM_HASH_SHA_H
#define UTIL_ALGORITHM_HASH_SHA_H

#pragma once

// ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL, ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL,ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL,
// ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS

#include <config/atframe_utils_build_feature.h>

#if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
    defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

#  include <openssl/evp.h>

#elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)

#  include "mbedtls/platform.h"
// "mbedtls/platform.h" must be the first
#  include "mbedtls/sha1.h"
#  include "mbedtls/sha256.h"
#  include "mbedtls/sha512.h"

#endif

#include <design_pattern/noncopyable.h>

#include <algorithm/base64.h>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace hash {
class sha {
 public:
  enum type {
    EN_ALGORITHM_UNINITED = 0,
    EN_ALGORITHM_SHA1,
    EN_ALGORITHM_SHA224,
    EN_ALGORITHM_SHA256,
    EN_ALGORITHM_SHA384,
    EN_ALGORITHM_SHA512,
  };

  UTIL_DESIGN_PATTERN_NOCOPYABLE(sha)

 public:
  ATFRAMEWORK_UTILS_API sha();
  ATFRAMEWORK_UTILS_API ~sha();

  ATFRAMEWORK_UTILS_API sha(sha&&);
  ATFRAMEWORK_UTILS_API sha& operator=(sha&&);

  ATFRAMEWORK_UTILS_API bool init(type);
  ATFRAMEWORK_UTILS_API void close();
  ATFRAMEWORK_UTILS_API void swap(sha& other);

  ATFRAMEWORK_UTILS_API bool update(const unsigned char* in, size_t inlen);
  ATFRAMEWORK_UTILS_API bool final();

  ATFRAMEWORK_UTILS_API size_t get_output_length() const;

  static ATFRAMEWORK_UTILS_API size_t get_output_length(type bt);

  ATFRAMEWORK_UTILS_API const unsigned char* get_output() const;

  ATFRAMEWORK_UTILS_API std::string get_output_hex(bool is_uppercase = false) const;

  ATFRAMEWORK_UTILS_API std::string get_output_base64(
      ATFRAMEWORK_UTILS_NAMESPACE_ID::base64_mode_t::type bt =
          ATFRAMEWORK_UTILS_NAMESPACE_ID::base64_mode_t::EN_BMT_STANDARD) const;

  static ATFRAMEWORK_UTILS_API std::string hash_to_binary(type t, const void* in, size_t inlen);
  static ATFRAMEWORK_UTILS_API std::string hash_to_hex(type t, const void* in, size_t inlen, bool is_uppercase = false);
  static ATFRAMEWORK_UTILS_API std::string hash_to_base64(
      type t, const void* in, size_t inlen,
      ATFRAMEWORK_UTILS_NAMESPACE_ID::base64_mode_t::type bt =
          ATFRAMEWORK_UTILS_NAMESPACE_ID::base64_mode_t::EN_BMT_STANDARD);

 private:
  type hash_type_;
  void* private_raw_data_;
};
}  // namespace hash
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif
