// Copyright 2021 atframework
// Create by owent on 2017.07.25
// @see https://en.wikipedia.org/wiki/XXTEA

#ifndef UTIL_ALGORITHM_CRYPTO_CIPHER_H
#define UTIL_ALGORITHM_CRYPTO_CIPHER_H

#pragma once

// ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL, ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL,ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL,
// ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS

#include <config/atframe_utils_build_feature.h>

#include <gsl/select-gsl.h>

#if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
    defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

#  include <openssl/crypto.h>
#  include <openssl/err.h>
#  include <openssl/evp.h>

#  define ATFW_UTIL_MACRO_CRYPTO_CIPHER_ENABLED 1

#elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)

#  include <mbedtls/cipher.h>
#  include <mbedtls/md.h>

#  define ATFW_UTIL_MACRO_CRYPTO_CIPHER_ENABLED 1

#endif

#ifdef ATFW_UTIL_MACRO_CRYPTO_CIPHER_ENABLED

#  ifndef CRYPTO_CIPHER_ENABLED
#    define CRYPTO_CIPHER_ENABLED ATFW_UTIL_MACRO_CRYPTO_CIPHER_ENABLED
#  endif

#  include <string>
#  include <vector>

#  include "xxtea.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace crypto {
struct cipher_interface_info_t;

class cipher {
 public:
  struct ATFRAMEWORK_UTILS_API mode_t {
    enum type { EN_CMODE_ENCRYPT = 0x01, EN_CMODE_DECRYPT = 0x02 };
  };

  enum iv_roll_policy_t : uint8_t {
    IV_ROLL_NONE = 0,
    IV_ROLL_AEAD_INC1_BE,
    IV_ROLL_CTR_INC_BLOCKS_BE,
    IV_ROLL_CHAIN_CIPHERTEXT,
    IV_ROLL_SODIUM_STREAM_COUNTER_LE64,
  };

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)
  using cipher_kt_t = EVP_CIPHER;
  using cipher_evp_t = EVP_CIPHER_CTX;
  using digest_type_t = EVP_MD;
  enum {
    MAX_KEY_LENGTH = EVP_MAX_KEY_LENGTH,  //
    MAX_IV_LENGTH = EVP_MAX_IV_LENGTH,    //
    MAX_MD_SIZE = EVP_MAX_MD_SIZE
  };

#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
  using cipher_kt_t = mbedtls_cipher_info_t;
  using cipher_evp_t = mbedtls_cipher_context_t;
  using digest_type_t = mbedtls_md_info_t;
  enum {
    MAX_KEY_LENGTH = 64,  //
    MAX_IV_LENGTH = MBEDTLS_MAX_IV_LENGTH,
    MAX_MD_SIZE = MBEDTLS_MD_MAX_SIZE
  };
#  endif

  struct ATFRAMEWORK_UTILS_API error_code_t {
    enum type {
      OK = 0,
      INVALID_PARAM = -1,
      NOT_INITED = -2,
      ALREADY_INITED = -3,
      MALLOC = -4,
      CIPHER_DISABLED = -11,
      CIPHER_NOT_SUPPORT = -12,
      CIPHER_OPERATION = -13,
      CIPHER_OPERATION_SET_IV = -14,
      LIBSODIUM_OPERATION = -15,
      LIBSODIUM_OPERATION_TAG_LEN = -16,
      MUST_CALL_AEAD_API = -21,
      MUST_NOT_CALL_AEAD_API = -22,
    };
  };

 public:
  ATFRAMEWORK_UTILS_API cipher();
  ATFRAMEWORK_UTILS_API ~cipher();

  ATFRAMEWORK_UTILS_API int init(const char *name, int mode = mode_t::EN_CMODE_ENCRYPT | mode_t::EN_CMODE_DECRYPT);
  ATFRAMEWORK_UTILS_API int close();

  /**
   * @brief set last error returned by crypto library
   * @param err error code returned by crypto library
   */
  ATFRAMEWORK_UTILS_API void set_last_errno(int64_t e);
  /**
   * @brief get last error returned by crypto library
   * @return last error code returned by crypto library
   */
  ATFRAMEWORK_UTILS_API int64_t get_last_errno() const;

  /**
   * @brief if it's a AEAD cipher
   * @see https://en.wikipedia.org/wiki/Authenticated_encryption
   * @return if it's AEAD cipher, return true
   */
  ATFRAMEWORK_UTILS_API bool is_aead() const;

  /**
   * @brief               get iv size in crypt library
   * @return              iv length in bytes
   */
  ATFRAMEWORK_UTILS_API uint32_t get_iv_size() const;

  /**libsodium_context_
   * @brief               get key length in bits
   * @return              key length in bits
   */
  ATFRAMEWORK_UTILS_API uint32_t get_key_bits() const;

  /**
   * @brief               get block size in crypt library
   * @return              block length in bytes
   */
  ATFRAMEWORK_UTILS_API uint32_t get_block_size() const;

  /**
   * @brief               get tag size in crypt library
   * @return              tag length in bytes
   */
  ATFRAMEWORK_UTILS_API uint32_t get_tag_size() const;

  /**
   * @brief               set tag size in crypt library
   * @param tag_size      tag length in bytes
   * @note                only for AEAD cipher, tag size should between [4, 16] and tag_size % 2 == 0 and match the
   *                      algorithm. For chacha20-poly1305-ietf and xchacha20-poly1305-ietf, tag size must be 16.
   *                      For GCM mode, tag size can be 4, 6, 8, 10, 12, 14, 16, and 16 is recommended.
   */
  ATFRAMEWORK_UTILS_API void set_tag_size(uint32_t tag_size);

  /**
   * @brief               set key
   * @param key           key
   * @param key_bitlen    key length to use, in bits. must equal or greater to get_key_bits()
   * @return              0 or error code less than 0
   */
  ATFRAMEWORK_UTILS_API int set_key(const unsigned char *key, uint32_t key_bitlen);
  ATFRAMEWORK_UTILS_API int set_key(gsl::span<const unsigned char> key);

  /**
   * @brief               selibsodium_context_t initialization vector
   * @param iv            iv value
   * @param iv_len        length of iv, in bytes, can not be greater than 16 when using mbedtls
   * @note                if using chacha20,chacha20-ietf,xchacha20,salsa20 or xsalsa20, the first
   *                      8 bytes is the counter, and the rest is the nonce.
   * @return              0 or error code less than 0
   */
  ATFRAMEWORK_UTILS_API int set_iv(const unsigned char *iv, size_t iv_len);
  ATFRAMEWORK_UTILS_API int set_iv(gsl::span<const unsigned char> iv);

  /**
   * @brief               clear initialization vector
   * @return              0 or error code less than 0
   */
  ATFRAMEWORK_UTILS_API void clear_iv();

  ATFRAMEWORK_UTILS_API gsl::span<const unsigned char> get_iv() const noexcept;

  /**
   * @biref               encrypt data
   * @param input         buffer holding the input data
   * @param ilen          length of the input data
   * @param output        buffer for the output data. Should be able to hold at
   *                      least ilen + block_size. Cannot be the same buffer as
   *                      input!
   * @param olen          length of the output data, will be filled with the
   *                      actual number of bytes written.
   * @return              0 or error code
   */
  ATFRAMEWORK_UTILS_API int encrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen);

  /**
   * @biref               decrypt data
   * @param input         buffer holding the input data
   * @param ilen          length of the input data
   * @param output        buffer for the output data. Should be able to hold at
   *                      least ilen + block_size. Cannot be the same buffer as
   *                      input!
   * @param olen          length of the output data, will be filled with the
   *                      actual number of bytes written.
   * @return              0 or error code
   */
  ATFRAMEWORK_UTILS_API int decrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen);

  /**
   * @biref               encrypt data
   * @param input         buffer holding the input data
   * @param ilen          length of the input data
   * @param output        buffer for the output data. Should be able to hold at
   *                      least ilen + block_size. Cannot be the same buffer as
   *                      input, tag are append to the end of output+ilen!
   * @param olen          length of the output data, will be filled with the
   *                      actual number of bytes written.
   * @param ad            Additional data to authenticate.
   * @param ad_len        Length of ad. ad_len must not be greater than 0xFF00
   * @return              0 or error code
   */
  ATFRAMEWORK_UTILS_API int encrypt_aead(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen,
                                         const unsigned char *ad, size_t ad_len);

  /**
   * @biref               decrypt data
   * @param input         buffer holding the input data, the tag are stored at input+ilen-tag_len
   * @param ilen          length of the input data
   * @param output        buffer for the output data. Should be able to hold at
   *                      least ilen + block_size. Cannot be the same buffer as
   *                      input!
   * @param olen          length of the output data, will be filled with the
   *                      actual number of bytes written.
   * @param ad            Additional data to be authenticated.
   * @param ad_len        Length of ad. ad_len must not be greater than 0xFF00
   * @return              0 or error code
   */
  ATFRAMEWORK_UTILS_API int decrypt_aead(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen,
                                         const unsigned char *ad, size_t ad_len);

 public:
  static ATFRAMEWORK_UTILS_API const cipher_kt_t *get_cipher_by_name(const char *name);
  /**
   * @biref               split cipher names by space, comma, semicolon or colon
   * @param in            string contain some cipher names
   * @return              begin(first) and end(second) address of a cipher name, both nullptr if not found.
   *                      you can use second pointer as the paramter of next call, just like strtok
   */
  static ATFRAMEWORK_UTILS_API std::pair<const char *, const char *> ciphertok(const char *in);
  static ATFRAMEWORK_UTILS_API const std::vector<std::string> &get_all_cipher_names();

  static ATFRAMEWORK_UTILS_API int init_global_algorithm();
  static ATFRAMEWORK_UTILS_API int cleanup_global_algorithm();

 private:
  int init_with_cipher(const cipher_interface_info_t *, int mode);
  int close_with_cipher();

 private:
  const cipher_interface_info_t *interface_;
  int64_t last_errorno_;
  uint32_t tag_length_;
  bool iv_is_set_;
  iv_roll_policy_t iv_roll_policy_;
  const cipher_kt_t *cipher_kt_;

  std::vector<unsigned char> iv_;
  struct xxtea_context_t {
    ATFRAMEWORK_UTILS_NAMESPACE_ID::xxtea_key key;
  };
  struct libsodium_context_t {
    unsigned char key[32];
  };
  struct cipher_context_t {
    cipher_evp_t *enc;  // used for encrypt
    cipher_evp_t *dec;  // used for decrypt
  };
  union {
    cipher_context_t cipher_context_;
    xxtea_context_t xxtea_context_;
    libsodium_context_t libsodium_context_;
  };
};
}  // namespace crypto
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif

#endif
