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

// CRYPTO_USE_OPENSSL, CRYPTO_USE_LIBRESSL,CRYPTO_USE_BORINGSSL, CRYPTO_USE_MBEDTLS

#include <config/atframe_utils_build_feature.h>

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)

#  include <openssl/evp.h>

#elif defined(CRYPTO_USE_MBEDTLS)

#  include "mbedtls/platform.h"
// "mbedtls/platform.h" must be the first
#  include "mbedtls/sha1.h"
#  include "mbedtls/sha256.h"
#  include "mbedtls/sha512.h"

#endif

#include <design_pattern/noncopyable.h>

#include <algorithm/base64.h>

namespace util {
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
  LIBATFRAME_UTILS_API sha();
  LIBATFRAME_UTILS_API ~sha();

#if defined(UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES) && UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES
  LIBATFRAME_UTILS_API sha(sha&&);
  LIBATFRAME_UTILS_API sha& operator=(sha&&);
#endif

  LIBATFRAME_UTILS_API bool init(type);
  LIBATFRAME_UTILS_API void close();
  LIBATFRAME_UTILS_API void swap(sha& other);

  LIBATFRAME_UTILS_API bool update(const unsigned char* in, size_t inlen);
  LIBATFRAME_UTILS_API bool final();

  LIBATFRAME_UTILS_API size_t get_output_length() const;

  static LIBATFRAME_UTILS_API size_t get_output_length(type bt);

  LIBATFRAME_UTILS_API const unsigned char* get_output() const;

  LIBATFRAME_UTILS_API std::string get_output_hex(bool is_uppercase = false) const;

  LIBATFRAME_UTILS_API std::string get_output_base64(
      ::util::base64_mode_t::type bt = ::util::base64_mode_t::EN_BMT_STANDARD) const;

  static LIBATFRAME_UTILS_API std::string hash_to_binary(type t, const void* in, size_t inlen);
  static LIBATFRAME_UTILS_API std::string hash_to_hex(type t, const void* in, size_t inlen, bool is_uppercase = false);
  static LIBATFRAME_UTILS_API std::string hash_to_base64(
      type t, const void* in, size_t inlen, ::util::base64_mode_t::type bt = ::util::base64_mode_t::EN_BMT_STANDARD);

 private:
  type hash_type_;
  void* private_raw_data_;
};
}  // namespace hash
}  // namespace util

#endif
