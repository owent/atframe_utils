// Copyright 2025 atframework
// Created by owent on 2024.12.26

#ifndef UTIL_ALGORITHM_CRYPTO_HMAC_H
#define UTIL_ALGORITHM_CRYPTO_HMAC_H

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
#  include <openssl/hmac.h>
#  include <openssl/kdf.h>

#  define ATFW_UTIL_MACRO_CRYPTO_HMAC_ENABLED 1

#elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)

#  include <mbedtls/hkdf.h>
#  include <mbedtls/md.h>

#  define ATFW_UTIL_MACRO_CRYPTO_HMAC_ENABLED 1

#endif

#ifdef ATFW_UTIL_MACRO_CRYPTO_HMAC_ENABLED

#  include <cstddef>
#  include <cstdint>
#  include <string>
#  include <vector>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace crypto {

/**
 * @brief Digest/Hash algorithm type enumeration
 */
enum class digest_type_t : uint8_t {
  kNone = 0,
  kSha1 = 1,
  kSha224 = 2,
  kSha256 = 3,
  kSha384 = 4,
  kSha512 = 5,
  kMd5 = 6,
};

/**
 * @brief HMAC error codes
 */
struct ATFRAMEWORK_UTILS_API hmac_error_code_t {
  enum type {
    kOk = 0,
    kInvalidParam = -1,
    kNotInitialized = -2,
    kAlreadyInitialized = -3,
    kDigestNotSupport = -4,
    kMalloc = -5,
    kOperation = -6,
    kOutputBufferTooSmall = -7,
    kDisabled = -8,
  };
};

/**
 * @brief Get the output length of a digest algorithm
 * @param type Digest algorithm type
 * @return Output length in bytes, or 0 if not supported
 */
ATFRAMEWORK_UTILS_API size_t get_digest_output_length(digest_type_t type) noexcept;

/**
 * @brief Get the digest name for a given digest type
 * @param type Digest algorithm type
 * @return Digest name string, or nullptr if not supported
 */
ATFRAMEWORK_UTILS_API const char* get_digest_name(digest_type_t type) noexcept;

/**
 * @brief HMAC (Hash-based Message Authentication Code) wrapper class
 *
 * This class provides a consistent interface for HMAC operations across different
 * crypto libraries (OpenSSL 1.x/3.x, BoringSSL, LibreSSL, mbedtls).
 *
 * @see https://en.wikipedia.org/wiki/HMAC
 * @see RFC 2104
 */
class hmac {
 public:
  ATFRAMEWORK_UTILS_API hmac();
  ATFRAMEWORK_UTILS_API ~hmac();

  // Non-copyable
  hmac(const hmac&) = delete;
  hmac& operator=(const hmac&) = delete;

  // Movable
  ATFRAMEWORK_UTILS_API hmac(hmac&& other) noexcept;
  ATFRAMEWORK_UTILS_API hmac& operator=(hmac&& other) noexcept;

  /**
   * @brief Initialize HMAC context with specified digest algorithm and key
   * @param type Digest algorithm type
   * @param key Key data
   * @param key_len Key length in bytes
   * @return 0 on success, or error code
   */
  ATFRAMEWORK_UTILS_API int init(digest_type_t type, const unsigned char* key, size_t key_len);
  ATFRAMEWORK_UTILS_API int init(digest_type_t type, gsl::span<const unsigned char> key);

  /**
   * @brief Close/reset the HMAC context
   * @return 0 on success, or error code
   */
  ATFRAMEWORK_UTILS_API int close();

  /**
   * @brief Update HMAC with additional data
   * @param input Input data
   * @param input_len Input data length in bytes
   * @return 0 on success, or error code
   */
  ATFRAMEWORK_UTILS_API int update(const unsigned char* input, size_t input_len);
  ATFRAMEWORK_UTILS_API int update(gsl::span<const unsigned char> input);

  /**
   * @brief Finalize HMAC computation and get the result
   * @param output Output buffer for HMAC result
   * @param output_len On input: size of output buffer. On output: actual HMAC length
   * @return 0 on success, or error code
   */
  ATFRAMEWORK_UTILS_API int final(unsigned char* output, size_t* output_len);

  /**
   * @brief Get the output length of the HMAC
   * @return Output length in bytes
   */
  ATFRAMEWORK_UTILS_API size_t get_output_length() const noexcept;

  /**
   * @brief Check if HMAC context is initialized
   * @return true if initialized
   */
  ATFRAMEWORK_UTILS_API bool is_valid() const noexcept;

  /**
   * @brief Get last error code from underlying crypto library
   * @return Error code
   */
  ATFRAMEWORK_UTILS_API int64_t get_last_errno() const noexcept;

  /**
   * @brief Set last error code
   * @param e Error code
   */
  ATFRAMEWORK_UTILS_API void set_last_errno(int64_t e) noexcept;

  /**
   * @brief One-shot HMAC computation
   * @param type Digest algorithm type
   * @param key Key data
   * @param key_len Key length
   * @param input Input data
   * @param input_len Input data length
   * @param output Output buffer
   * @param output_len On input: buffer size. On output: actual HMAC length
   * @return 0 on success, or error code
   */
  static ATFRAMEWORK_UTILS_API int compute(digest_type_t type, const unsigned char* key, size_t key_len,
                                           const unsigned char* input, size_t input_len, unsigned char* output,
                                           size_t* output_len);

  static ATFRAMEWORK_UTILS_API int compute(digest_type_t type, gsl::span<const unsigned char> key,
                                           gsl::span<const unsigned char> input, unsigned char* output,
                                           size_t* output_len);

  /**
   * @brief One-shot HMAC computation returning result as vector
   * @param type Digest algorithm type
   * @param key Key data
   * @param key_len Key length
   * @param input Input data
   * @param input_len Input data length
   * @return HMAC result as vector, empty on error
   */
  static ATFRAMEWORK_UTILS_API std::vector<unsigned char> compute_to_binary(digest_type_t type,
                                                                            const unsigned char* key, size_t key_len,
                                                                            const unsigned char* input,
                                                                            size_t input_len);

  static ATFRAMEWORK_UTILS_API std::vector<unsigned char> compute_to_binary(digest_type_t type,
                                                                            gsl::span<const unsigned char> key,
                                                                            gsl::span<const unsigned char> input);

  /**
   * @brief One-shot HMAC computation returning result as hex string
   * @param type Digest algorithm type
   * @param key Key data
   * @param key_len Key length
   * @param input Input data
   * @param input_len Input data length
   * @param uppercase Use uppercase hex characters
   * @return HMAC result as hex string, empty on error
   */
  static ATFRAMEWORK_UTILS_API std::string compute_to_hex(digest_type_t type, const unsigned char* key, size_t key_len,
                                                          const unsigned char* input, size_t input_len,
                                                          bool uppercase = false);

  static ATFRAMEWORK_UTILS_API std::string compute_to_hex(digest_type_t type, gsl::span<const unsigned char> key,
                                                          gsl::span<const unsigned char> input, bool uppercase = false);

 private:
  digest_type_t digest_type_;
  int64_t last_errno_;
  void* context_;
};

/**
 * @brief HKDF (HMAC-based Key Derivation Function) wrapper class
 *
 * This class provides a consistent interface for HKDF operations across different
 * crypto libraries (OpenSSL 1.x/3.x, BoringSSL, LibreSSL, mbedtls).
 *
 * @see https://en.wikipedia.org/wiki/HKDF
 * @see RFC 5869
 */
class hkdf {
 public:
  /**
   * @brief HKDF error codes
   */
  struct ATFRAMEWORK_UTILS_API error_code_t {
    enum type {
      kOk = 0,
      kInvalidParam = -1,
      kDigestNotSupport = -2,
      kOperation = -3,
      kOutputLengthTooLarge = -4,
      kDisabled = -5,
    };
  };

  /**
   * @brief Perform HKDF-Extract step
   *
   * PRK = HKDF-Extract(salt, IKM)
   *
   * @param type Digest algorithm type
   * @param salt Optional salt value (can be nullptr, then uses zeros)
   * @param salt_len Salt length in bytes
   * @param ikm Input keying material
   * @param ikm_len IKM length in bytes
   * @param prk Output buffer for pseudorandom key
   * @param prk_len On input: buffer size. On output: actual PRK length
   * @return 0 on success, or error code
   */
  static ATFRAMEWORK_UTILS_API int extract(digest_type_t type, const unsigned char* salt, size_t salt_len,
                                           const unsigned char* ikm, size_t ikm_len, unsigned char* prk,
                                           size_t* prk_len);

  static ATFRAMEWORK_UTILS_API int extract(digest_type_t type, gsl::span<const unsigned char> salt,
                                           gsl::span<const unsigned char> ikm, unsigned char* prk, size_t* prk_len);

  /**
   * @brief Perform HKDF-Expand step
   *
   * OKM = HKDF-Expand(PRK, info, L)
   *
   * @param type Digest algorithm type
   * @param prk Pseudorandom key from extract step
   * @param prk_len PRK length in bytes
   * @param info Optional context/application specific info (can be nullptr)
   * @param info_len Info length in bytes
   * @param okm Output buffer for output keying material
   * @param okm_len Desired output length in bytes
   * @return 0 on success, or error code
   */
  static ATFRAMEWORK_UTILS_API int expand(digest_type_t type, const unsigned char* prk, size_t prk_len,
                                          const unsigned char* info, size_t info_len, unsigned char* okm,
                                          size_t okm_len);

  static ATFRAMEWORK_UTILS_API int expand(digest_type_t type, gsl::span<const unsigned char> prk,
                                          gsl::span<const unsigned char> info, unsigned char* okm, size_t okm_len);

  /**
   * @brief Perform full HKDF (Extract + Expand)
   *
   * @param type Digest algorithm type
   * @param salt Optional salt value (can be nullptr)
   * @param salt_len Salt length in bytes
   * @param ikm Input keying material
   * @param ikm_len IKM length in bytes
   * @param info Optional context info (can be nullptr)
   * @param info_len Info length in bytes
   * @param okm Output buffer for output keying material
   * @param okm_len Desired output length in bytes
   * @return 0 on success, or error code
   */
  static ATFRAMEWORK_UTILS_API int derive(digest_type_t type, const unsigned char* salt, size_t salt_len,
                                          const unsigned char* ikm, size_t ikm_len, const unsigned char* info,
                                          size_t info_len, unsigned char* okm, size_t okm_len);

  static ATFRAMEWORK_UTILS_API int derive(digest_type_t type, gsl::span<const unsigned char> salt,
                                          gsl::span<const unsigned char> ikm, gsl::span<const unsigned char> info,
                                          unsigned char* okm, size_t okm_len);

  /**
   * @brief Perform full HKDF and return result as vector
   *
   * @param type Digest algorithm type
   * @param salt Optional salt value
   * @param ikm Input keying material
   * @param info Optional context info
   * @param okm_len Desired output length in bytes
   * @return Output keying material as vector, empty on error
   */
  static ATFRAMEWORK_UTILS_API std::vector<unsigned char> derive_to_binary(digest_type_t type,
                                                                           gsl::span<const unsigned char> salt,
                                                                           gsl::span<const unsigned char> ikm,
                                                                           gsl::span<const unsigned char> info,
                                                                           size_t okm_len);

 private:
  hkdf() = delete;
  ~hkdf() = delete;
};

}  // namespace crypto
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif  // ATFW_UTIL_MACRO_CRYPTO_HMAC_ENABLED

#endif  // UTIL_ALGORITHM_CRYPTO_HMAC_H
