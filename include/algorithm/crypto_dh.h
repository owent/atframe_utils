// Copyright 2026 atframework
//
// Create by owent on 2017.09.19

#ifndef UTIL_ALGORITHM_CRYPTO_DH_H
#define UTIL_ALGORITHM_CRYPTO_DH_H

#pragma once

// ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL, ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL,ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL,
// ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS

#include <config/atframe_utils_build_feature.h>

#include <nostd/string_view.h>

#if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
    defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
#  define CRYPTO_DH_ENABLED 1
#endif

#ifdef CRYPTO_DH_ENABLED

#  include <cstdint>
#  include <memory>
#  include <string>
#  include <vector>

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)
// Forward declarations for OpenSSL/LibreSSL/BoringSSL types used in private helpers.
// Declaring these here lets us avoid pulling <openssl/bn.h> into the public header.
extern "C" {
struct bignum_st;
}
#  endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace crypto {

/**
 * @brief DH and ECDH progress
 * @note TLS handshake:
 *         server process: shared_context::init(curve name)->make_params->read_public->calc_secret
 *         client process: shared_context::init(method_t::kDh/method_t::kEcdh)->read_params->make_public->calc_secret
 * @note Static configure:
 *         server1 process: shared_context::init(curve name)->make_params->make_public->read_public->calc_secret
 *         server2 process: shared_context::init(curve name)->make_params->make_public->read_public->calc_secret
 */
class dh {
 public:
  enum class method_t : int32_t {
    kInvalid = 0,  // inner
    kDh = 1,       // dh algorithm
    kEcdh = 2,     // ecdh algorithm
  };

  enum class error_code_t : int32_t {
    kOk = 0,
    kInvalidParam = -1,
    kNotInited = -2,
    kAlreadyInited = -3,
    kMalloc = -4,
    kDisabled = -11,
    kNotSupport = -12,
    kOperation = -13,
    kInitRandomEngine = -14,
    kNotClientMode = -15,
    kNotServerMode = -16,
    kAlgorithmMismatch = -17,
    kReadDhparamFile = -21,
    kInitDhparam = -22,
    kInitDhReadParam = -23,
    kInitDhGenerateKey = -24,
    kInitDhReadKey = -25,
    kInitDhGenerateSecret = -26,
  };

  // Comparison operators with int (legacy callers compare against 0). Allows
  // `dh.init(...) != 0`, `dh.init(...) < 0`, etc. without sprinkling
  // static_cast<int> at every call site.
  friend constexpr bool operator==(error_code_t lhs, int rhs) noexcept { return static_cast<int>(lhs) == rhs; }
  friend constexpr bool operator==(int lhs, error_code_t rhs) noexcept { return lhs == static_cast<int>(rhs); }
  friend constexpr bool operator!=(error_code_t lhs, int rhs) noexcept { return static_cast<int>(lhs) != rhs; }
  friend constexpr bool operator!=(int lhs, error_code_t rhs) noexcept { return lhs != static_cast<int>(rhs); }
  friend constexpr bool operator<(error_code_t lhs, int rhs) noexcept { return static_cast<int>(lhs) < rhs; }
  friend constexpr bool operator<(int lhs, error_code_t rhs) noexcept { return lhs < static_cast<int>(rhs); }
  friend constexpr bool operator<=(error_code_t lhs, int rhs) noexcept { return static_cast<int>(lhs) <= rhs; }
  friend constexpr bool operator<=(int lhs, error_code_t rhs) noexcept { return lhs <= static_cast<int>(rhs); }
  friend constexpr bool operator>(error_code_t lhs, int rhs) noexcept { return static_cast<int>(lhs) > rhs; }
  friend constexpr bool operator>(int lhs, error_code_t rhs) noexcept { return lhs > static_cast<int>(rhs); }
  friend constexpr bool operator>=(error_code_t lhs, int rhs) noexcept { return static_cast<int>(lhs) >= rhs; }
  friend constexpr bool operator>=(int lhs, error_code_t rhs) noexcept { return lhs >= static_cast<int>(rhs); }

  enum class flags_t : uint32_t {
    kNone = 0,
    kServerMode = 0x01,
    kClientMode = 0x02,
  };

  // Forward declarations: definitions live in crypto_dh.cpp so that this
  // public header does not depend on OpenSSL/mbedtls headers.
  struct dh_context_t;

  class shared_context {
   public:
    struct dh_param_t;
    struct random_engine_t;

    using ptr_t = std::shared_ptr<shared_context>;

   private:
    struct ATFRAMEWORK_UTILS_API creator_helper {};

    ATFRAMEWORK_UTILS_API shared_context();

   public:
    ATFRAMEWORK_UTILS_API shared_context(creator_helper &helper);
    ATFRAMEWORK_UTILS_API ~shared_context();
    ATFRAMEWORK_UTILS_API static ptr_t create();

    /**
     * @brief initialize a shared context for server mode
     * @param name algorithm name, ecdh:[ECDH algorithm name] or the path of dh parameter PEM file
     * @note using RFC 4492 for ECDH algorithm
     * @return error_code_t::kOk or error code
     */
    ATFRAMEWORK_UTILS_API error_code_t init(nostd::string_view name);

    /**
     * @brief initialize a shared context for client mode
     * @param method algorithm method
     * @return error_code_t::kOk or error code
     */
    ATFRAMEWORK_UTILS_API error_code_t init(method_t method);

    /**
     * @brief reset shared resource
     */
    ATFRAMEWORK_UTILS_API void reset();

    /**
     * @brief random buffer
     * @return error_code_t::kOk or error code
     */
    ATFRAMEWORK_UTILS_API error_code_t random(void *output, size_t output_sz);

    ATFRAMEWORK_UTILS_API bool is_client_mode() const;

    ATFRAMEWORK_UTILS_API method_t get_method() const;

   private:
    friend class dh;

    ATFRAMEWORK_UTILS_API const dh_param_t &get_dh_parameter() const;
    ATFRAMEWORK_UTILS_API const random_engine_t &get_random_engine() const;
    ATFRAMEWORK_UTILS_API random_engine_t &get_random_engine();

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)
    ATFRAMEWORK_UTILS_API error_code_t try_reset_ecp_id(int group_id);
    /**
     * @brief Try to reset DH Params of P,G
     *
     * @param DH_p INOUT new P
     * @param DH_g INOUT new G
     * @return error_code_t::kOk or error code
     * @note DH_p and DH_g will be set to nullptr when moved in, user must free them if they are still not nullptr
     */
    ATFRAMEWORK_UTILS_API error_code_t try_reset_dh_params(struct bignum_st *&DH_p, struct bignum_st *&DH_g);
#  endif

    uint32_t flags_;
    method_t method_;
    std::unique_ptr<dh_param_t> dh_param_;
    std::unique_ptr<random_engine_t> random_engine_;
  };

 public:
  ATFRAMEWORK_UTILS_API dh();
  ATFRAMEWORK_UTILS_API ~dh();

  /**
   * @brief initialize
   * @param shared_context shared context
   * @return error_code_t::kOk or error code
   */
  ATFRAMEWORK_UTILS_API error_code_t init(shared_context::ptr_t shared_context);

  /**
   * @brief release all resources
   * @return error_code_t::kOk or error code
   */
  ATFRAMEWORK_UTILS_API error_code_t close();

  /**
   * @brief set last error returned by crypto library
   * @param err error code returned by crypto library
   */
  ATFRAMEWORK_UTILS_API void set_last_errno(int e);

  /**
   * @brief get last error returned by crypto library
   * @return last error code returned by crypto library
   */
  ATFRAMEWORK_UTILS_API int get_last_errno() const;

  /**
   * @brief          Setup and write the ServerKeyExchange parameters
   *
   * @param param    destination buffer
   *
   * @note           This function assumes that ctx->P and ctx->G
   *                 have already been properly set
   *
   * @note           server process: make_params->read_public->calc_secret
   * @return         error_code_t::kOk if successful, or error code
   */
  ATFRAMEWORK_UTILS_API error_code_t make_params(std::vector<unsigned char> &param);

  /**
   * @brief          Parse the ServerKeyExchange parameters
   *
   * @param input    input buffer
   * @param ilen     size of buffer
   *
   * @note           client process: read_params->make_public->calc_secret
   * @return         error_code_t::kOk if successful, or error code
   */
  ATFRAMEWORK_UTILS_API error_code_t read_params(const unsigned char *input, size_t ilen);

  /**
   * @brief          Create own private value X and export G^X
   *
   * @param param    destination buffer
   *
   * @note           client process: read_params->make_public->calc_secret
   * @return         error_code_t::kOk if successful, or error code
   */
  ATFRAMEWORK_UTILS_API error_code_t make_public(std::vector<unsigned char> &param);

  /**
   * @brief          Import the peer's public value G^Y
   *
   * @param input    input buffer
   * @param ilen     size of buffer
   *
   * @note           server process: make_params->read_public->calc_secret
   * @return         error_code_t::kOk if successful, or error code
   */
  ATFRAMEWORK_UTILS_API error_code_t read_public(const unsigned char *input, size_t ilen);

  /**
   * @brief          Derive and export the shared secret (G^Y)^X mod P
   *
   * @param output   destination buffer
   *
   * @return         error_code_t::kOk if successful, or error code
   *
   */
  ATFRAMEWORK_UTILS_API error_code_t calc_secret(std::vector<unsigned char> &output);

 public:
  static ATFRAMEWORK_UTILS_API const std::vector<std::string> &get_all_curve_names();

 private:
#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)
  error_code_t check_or_setup_ecp_id(int group_id);
  error_code_t check_or_setup_dh_pg_gy(struct bignum_st *&DH_p, struct bignum_st *&DH_g, struct bignum_st *&DH_gy);
#  endif

  int last_errorno_;
  shared_context::ptr_t shared_context_;
  std::unique_ptr<dh_context_t> dh_context_;
};
}  // namespace crypto
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif

#endif
