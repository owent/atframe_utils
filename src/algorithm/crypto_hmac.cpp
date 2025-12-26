// Copyright 2025 atframework
// Created by owent on 2024.12.26

#include "algorithm/crypto_hmac.h"

#ifdef ATFW_UTIL_MACRO_CRYPTO_HMAC_ENABLED

#  include <common/string_oprs.h>

#  include <cstring>

/**
 * @note OpenSSL version compatibility notes:
 *       OpenSSL 1.0.x: HMAC() returns unsigned char*, HMAC_CTX on stack
 *       OpenSSL 1.1.0+: HMAC_CTX_new/free, EVP_MD_CTX_new/free, EVP_PKEY_HKDF
 *       OpenSSL 3.0+: Deprecated HMAC_* low-level APIs, use EVP_MAC API, EVP_KDF for HKDF
 *       BoringSSL: Similar to OpenSSL 1.1.0, native HKDF functions (HKDF, HKDF_extract, HKDF_expand)
 *       LibreSSL: Similar to OpenSSL 1.0.x/1.1.0, native HKDF functions (HKDF, HKDF_extract, HKDF_expand)
 *
 * @note mbedtls version compatibility notes:
 *       mbedtls 2.x/3.x: mbedtls_md_hmac_* and mbedtls_hkdf* APIs (compatible across versions)
 */

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

// Check for OpenSSL 3.0+ which has EVP_MAC API
#    if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L && \
        !defined(LIBRESSL_VERSION_NUMBER) && !defined(OPENSSL_IS_BORINGSSL)
#      define ATFW_CRYPTO_HMAC_USE_EVP_MAC 1
#      include <openssl/core_names.h>
#      include <openssl/params.h>
#    else
#      define ATFW_CRYPTO_HMAC_USE_EVP_MAC 0
#    endif

// Check for HKDF support
// BoringSSL and LibreSSL have native HKDF functions (HKDF, HKDF_extract, HKDF_expand)
// OpenSSL 3.0+ uses EVP_KDF API, OpenSSL 1.1.x uses EVP_PKEY_derive
#    if defined(OPENSSL_IS_BORINGSSL) || defined(LIBRESSL_VERSION_NUMBER)
#      define ATFW_CRYPTO_HKDF_USE_NATIVE 1
#      include <openssl/hkdf.h>
#    else
#      define ATFW_CRYPTO_HKDF_USE_NATIVE 0
#    endif

#    if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x10100000L && \
        !defined(LIBRESSL_VERSION_NUMBER) && !defined(OPENSSL_IS_BORINGSSL)
#      define ATFW_CRYPTO_HKDF_USE_EVP_PKEY 1
#    else
#      define ATFW_CRYPTO_HKDF_USE_EVP_PKEY 0
#    endif

#    if defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x30000000L && \
        !defined(LIBRESSL_VERSION_NUMBER) && !defined(OPENSSL_IS_BORINGSSL)
#      define ATFW_CRYPTO_HKDF_USE_EVP_KDF 1
#    else
#      define ATFW_CRYPTO_HKDF_USE_EVP_KDF 0
#    endif

// HMAC_CTX management for older OpenSSL
#    if (defined(OPENSSL_VERSION_NUMBER) && OPENSSL_VERSION_NUMBER >= 0x10100000L) || defined(OPENSSL_IS_BORINGSSL) || \
        (defined(LIBRESSL_VERSION_NUMBER) && LIBRESSL_VERSION_NUMBER >= 0x2070000fL)
#      define ATFW_CRYPTO_HMAC_CTX_NEW 1
#    else
#      define ATFW_CRYPTO_HMAC_CTX_NEW 0
#    endif

#  endif  // OpenSSL/LibreSSL/BoringSSL

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)
#    define ATFRAMEWORK_UTILS_CRYPTO_IGNORE_VERSION_WARNINGS 1
#  endif

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_IGNORE_VERSION_WARNINGS)
#    if defined(_MSC_VER)
#      pragma warning(push)
#      pragma warning(disable : 4244)
#    endif
#    if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#      pragma GCC diagnostic push
#      pragma GCC diagnostic ignored "-Wsign-conversion"
#    elif defined(__clang__) || defined(__apple_build_version__)
#      pragma clang diagnostic push
#      pragma clang diagnostic ignored "-Wsign-conversion"
#    endif
#  endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace crypto {

namespace details {

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

static const EVP_MD* get_evp_md_by_type(digest_type_t type) noexcept {
  switch (type) {
    case digest_type_t::kSha1:
      return EVP_sha1();
    case digest_type_t::kSha224:
      return EVP_sha224();
    case digest_type_t::kSha256:
      return EVP_sha256();
    case digest_type_t::kSha384:
      return EVP_sha384();
    case digest_type_t::kSha512:
      return EVP_sha512();
    case digest_type_t::kMd5:
      return EVP_md5();
    default:
      return nullptr;
  }
}

#    if ATFW_CRYPTO_HMAC_USE_EVP_MAC

struct hmac_evp_mac_context {
  EVP_MAC* mac;
  EVP_MAC_CTX* ctx;
  size_t output_length;
};

static hmac_evp_mac_context* create_hmac_context(digest_type_t type, const unsigned char* key, size_t key_len) {
  hmac_evp_mac_context* result = new (std::nothrow) hmac_evp_mac_context();
  if (result == nullptr) {
    return nullptr;
  }
  result->mac = nullptr;
  result->ctx = nullptr;
  result->output_length = 0;

  do {
    result->mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
    if (result->mac == nullptr) {
      break;
    }

    result->ctx = EVP_MAC_CTX_new(result->mac);
    if (result->ctx == nullptr) {
      break;
    }

    const char* digest_name = get_digest_name(type);
    if (digest_name == nullptr) {
      break;
    }

    OSSL_PARAM params[2];
    params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, const_cast<char*>(digest_name), 0);
    params[1] = OSSL_PARAM_construct_end();

    if (EVP_MAC_init(result->ctx, key, key_len, params) != 1) {
      break;
    }

    result->output_length = get_digest_output_length(type);
    return result;
  } while (false);

  // Cleanup on failure
  if (result->ctx != nullptr) {
    EVP_MAC_CTX_free(result->ctx);
  }
  if (result->mac != nullptr) {
    EVP_MAC_free(result->mac);
  }
  delete result;
  return nullptr;
}

static void free_hmac_context(void* ctx) {
  if (ctx == nullptr) {
    return;
  }
  hmac_evp_mac_context* context = static_cast<hmac_evp_mac_context*>(ctx);
  if (context->ctx != nullptr) {
    EVP_MAC_CTX_free(context->ctx);
  }
  if (context->mac != nullptr) {
    EVP_MAC_free(context->mac);
  }
  delete context;
}

#    else  // !ATFW_CRYPTO_HMAC_USE_EVP_MAC

struct hmac_legacy_context {
#      if ATFW_CRYPTO_HMAC_CTX_NEW
  HMAC_CTX* ctx;
#      else
  HMAC_CTX ctx;
  bool initialized;
#      endif
  size_t output_length;
};

static hmac_legacy_context* create_hmac_context(digest_type_t type, const unsigned char* key, size_t key_len) {
  const EVP_MD* md = get_evp_md_by_type(type);
  if (md == nullptr) {
    return nullptr;
  }

  hmac_legacy_context* result = new (std::nothrow) hmac_legacy_context();
  if (result == nullptr) {
    return nullptr;
  }

#      if ATFW_CRYPTO_HMAC_CTX_NEW
  result->ctx = HMAC_CTX_new();
  if (result->ctx == nullptr) {
    delete result;
    return nullptr;
  }

  if (HMAC_Init_ex(result->ctx, key, static_cast<int>(key_len), md, nullptr) != 1) {
    HMAC_CTX_free(result->ctx);
    delete result;
    return nullptr;
  }
#      else
  HMAC_CTX_init(&result->ctx);
  result->initialized = true;
  if (HMAC_Init_ex(&result->ctx, key, static_cast<int>(key_len), md, nullptr) != 1) {
    HMAC_CTX_cleanup(&result->ctx);
    delete result;
    return nullptr;
  }
#      endif

  result->output_length = get_digest_output_length(type);
  return result;
}

static void free_hmac_context(void* ctx) {
  if (ctx == nullptr) {
    return;
  }
  hmac_legacy_context* context = static_cast<hmac_legacy_context*>(ctx);
#      if ATFW_CRYPTO_HMAC_CTX_NEW
  if (context->ctx != nullptr) {
    HMAC_CTX_free(context->ctx);
  }
#      else
  if (context->initialized) {
    HMAC_CTX_cleanup(&context->ctx);
  }
#      endif
  delete context;
}

#    endif  // ATFW_CRYPTO_HMAC_USE_EVP_MAC

#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)

static const mbedtls_md_info_t* get_md_info_by_type(digest_type_t type) noexcept {
  switch (type) {
    case digest_type_t::kSha1:
      return mbedtls_md_info_from_type(MBEDTLS_MD_SHA1);
    case digest_type_t::kSha224:
      return mbedtls_md_info_from_type(MBEDTLS_MD_SHA224);
    case digest_type_t::kSha256:
      return mbedtls_md_info_from_type(MBEDTLS_MD_SHA256);
    case digest_type_t::kSha384:
      return mbedtls_md_info_from_type(MBEDTLS_MD_SHA384);
    case digest_type_t::kSha512:
      return mbedtls_md_info_from_type(MBEDTLS_MD_SHA512);
    case digest_type_t::kMd5:
      return mbedtls_md_info_from_type(MBEDTLS_MD_MD5);
    default:
      return nullptr;
  }
}

struct hmac_mbedtls_context {
  mbedtls_md_context_t ctx;
  bool initialized;
  size_t output_length;
};

static hmac_mbedtls_context* create_hmac_context(digest_type_t type, const unsigned char* key, size_t key_len) {
  const mbedtls_md_info_t* md_info = get_md_info_by_type(type);
  if (md_info == nullptr) {
    return nullptr;
  }

  hmac_mbedtls_context* result = new (std::nothrow) hmac_mbedtls_context();
  if (result == nullptr) {
    return nullptr;
  }

  mbedtls_md_init(&result->ctx);
  result->initialized = true;

  int ret = mbedtls_md_setup(&result->ctx, md_info, 1);  // 1 = use HMAC
  if (ret != 0) {
    mbedtls_md_free(&result->ctx);
    delete result;
    return nullptr;
  }

  ret = mbedtls_md_hmac_starts(&result->ctx, key, key_len);
  if (ret != 0) {
    mbedtls_md_free(&result->ctx);
    delete result;
    return nullptr;
  }

  result->output_length = get_digest_output_length(type);
  return result;
}

static void free_hmac_context(void* ctx) {
  if (ctx == nullptr) {
    return;
  }
  hmac_mbedtls_context* context = static_cast<hmac_mbedtls_context*>(ctx);
  if (context->initialized) {
    mbedtls_md_free(&context->ctx);
  }
  delete context;
}

#  endif  // ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS

}  // namespace details

// ============================================================================
// Utility functions
// ============================================================================

ATFRAMEWORK_UTILS_API size_t get_digest_output_length(digest_type_t type) noexcept {
  switch (type) {
    case digest_type_t::kSha1:
      return 20;
    case digest_type_t::kSha224:
      return 28;
    case digest_type_t::kSha256:
      return 32;
    case digest_type_t::kSha384:
      return 48;
    case digest_type_t::kSha512:
      return 64;
    case digest_type_t::kMd5:
      return 16;
    default:
      return 0;
  }
}

ATFRAMEWORK_UTILS_API const char* get_digest_name(digest_type_t type) noexcept {
  switch (type) {
    case digest_type_t::kSha1:
      return "SHA1";
    case digest_type_t::kSha224:
      return "SHA224";
    case digest_type_t::kSha256:
      return "SHA256";
    case digest_type_t::kSha384:
      return "SHA384";
    case digest_type_t::kSha512:
      return "SHA512";
    case digest_type_t::kMd5:
      return "MD5";
    default:
      return nullptr;
  }
}

// ============================================================================
// HMAC class implementation
// ============================================================================

ATFRAMEWORK_UTILS_API hmac::hmac() : digest_type_(digest_type_t::kNone), last_errno_(0), context_(nullptr) {}

ATFRAMEWORK_UTILS_API hmac::~hmac() { close(); }

ATFRAMEWORK_UTILS_API hmac::hmac(hmac&& other) noexcept
    : digest_type_(other.digest_type_), last_errno_(other.last_errno_), context_(other.context_) {
  other.digest_type_ = digest_type_t::kNone;
  other.last_errno_ = 0;
  other.context_ = nullptr;
}

ATFRAMEWORK_UTILS_API hmac& hmac::operator=(hmac&& other) noexcept {
  if (this != &other) {
    close();
    digest_type_ = other.digest_type_;
    last_errno_ = other.last_errno_;
    context_ = other.context_;
    other.digest_type_ = digest_type_t::kNone;
    other.last_errno_ = 0;
    other.context_ = nullptr;
  }
  return *this;
}

ATFRAMEWORK_UTILS_API int hmac::init(digest_type_t type, const unsigned char* key, size_t key_len) {
  if (context_ != nullptr) {
    return hmac_error_code_t::kAlreadyInitialized;
  }

  if (key == nullptr && key_len > 0) {
    return hmac_error_code_t::kInvalidParam;
  }

  if (get_digest_output_length(type) == 0) {
    return hmac_error_code_t::kDigestNotSupport;
  }

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

#    if ATFW_CRYPTO_HMAC_USE_EVP_MAC
  context_ = details::create_hmac_context(type, key, key_len);
#    else
  context_ = details::create_hmac_context(type, key, key_len);
#    endif

  if (context_ == nullptr) {
    last_errno_ = static_cast<int64_t>(ERR_peek_error());
    return hmac_error_code_t::kMalloc;
  }

#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
  context_ = details::create_hmac_context(type, key, key_len);
  if (context_ == nullptr) {
    return hmac_error_code_t::kMalloc;
  }
#  endif

  digest_type_ = type;
  return hmac_error_code_t::kOk;
}

ATFRAMEWORK_UTILS_API int hmac::init(digest_type_t type, gsl::span<const unsigned char> key) {
  return init(type, key.data(), key.size());
}

ATFRAMEWORK_UTILS_API int hmac::close() {
  if (context_ != nullptr) {
    details::free_hmac_context(context_);
    context_ = nullptr;
  }
  digest_type_ = digest_type_t::kNone;
  last_errno_ = 0;
  return hmac_error_code_t::kOk;
}

ATFRAMEWORK_UTILS_API int hmac::update(const unsigned char* input, size_t input_len) {
  if (context_ == nullptr) {
    return hmac_error_code_t::kNotInitialized;
  }

  if (input == nullptr && input_len > 0) {
    return hmac_error_code_t::kInvalidParam;
  }

  if (input_len == 0) {
    return hmac_error_code_t::kOk;
  }

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

#    if ATFW_CRYPTO_HMAC_USE_EVP_MAC
  details::hmac_evp_mac_context* ctx = static_cast<details::hmac_evp_mac_context*>(context_);
  if (EVP_MAC_update(ctx->ctx, input, input_len) != 1) {
    last_errno_ = static_cast<int64_t>(ERR_peek_error());
    return hmac_error_code_t::kOperation;
  }
#    else
  details::hmac_legacy_context* ctx = static_cast<details::hmac_legacy_context*>(context_);
#      if ATFW_CRYPTO_HMAC_CTX_NEW
  if (HMAC_Update(ctx->ctx, input, input_len) != 1) {
#      else
  if (HMAC_Update(&ctx->ctx, input, input_len) != 1) {
#      endif
    last_errno_ = static_cast<int64_t>(ERR_peek_error());
    return hmac_error_code_t::kOperation;
  }
#    endif

#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
  details::hmac_mbedtls_context* ctx = static_cast<details::hmac_mbedtls_context*>(context_);
  int ret = mbedtls_md_hmac_update(&ctx->ctx, input, input_len);
  if (ret != 0) {
    last_errno_ = ret;
    return hmac_error_code_t::kOperation;
  }
#  endif

  return hmac_error_code_t::kOk;
}

ATFRAMEWORK_UTILS_API int hmac::update(gsl::span<const unsigned char> input) {
  return update(input.data(), input.size());
}

ATFRAMEWORK_UTILS_API int hmac::final(unsigned char* output, size_t* output_len) {
  if (context_ == nullptr) {
    return hmac_error_code_t::kNotInitialized;
  }

  if (output == nullptr || output_len == nullptr) {
    return hmac_error_code_t::kInvalidParam;
  }

  size_t expected_len = get_output_length();
  if (*output_len < expected_len) {
    *output_len = expected_len;
    return hmac_error_code_t::kOutputBufferTooSmall;
  }

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

#    if ATFW_CRYPTO_HMAC_USE_EVP_MAC
  details::hmac_evp_mac_context* ctx = static_cast<details::hmac_evp_mac_context*>(context_);
  size_t out_len = *output_len;
  if (EVP_MAC_final(ctx->ctx, output, &out_len, *output_len) != 1) {
    last_errno_ = static_cast<int64_t>(ERR_peek_error());
    return hmac_error_code_t::kOperation;
  }
  *output_len = out_len;
#    else
  details::hmac_legacy_context* ctx = static_cast<details::hmac_legacy_context*>(context_);
  unsigned int out_len = 0;
#      if ATFW_CRYPTO_HMAC_CTX_NEW
  if (HMAC_Final(ctx->ctx, output, &out_len) != 1) {
#      else
  if (HMAC_Final(&ctx->ctx, output, &out_len) != 1) {
#      endif
    last_errno_ = static_cast<int64_t>(ERR_peek_error());
    return hmac_error_code_t::kOperation;
  }
  *output_len = out_len;
#    endif

#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
  details::hmac_mbedtls_context* ctx = static_cast<details::hmac_mbedtls_context*>(context_);
  int ret = mbedtls_md_hmac_finish(&ctx->ctx, output);
  if (ret != 0) {
    last_errno_ = ret;
    return hmac_error_code_t::kOperation;
  }
  *output_len = expected_len;
#  endif

  return hmac_error_code_t::kOk;
}

ATFRAMEWORK_UTILS_API size_t hmac::get_output_length() const noexcept {
  if (context_ == nullptr) {
    return 0;
  }

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)
#    if ATFW_CRYPTO_HMAC_USE_EVP_MAC
  return static_cast<details::hmac_evp_mac_context*>(context_)->output_length;
#    else
  return static_cast<details::hmac_legacy_context*>(context_)->output_length;
#    endif
#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
  return static_cast<details::hmac_mbedtls_context*>(context_)->output_length;
#  else
  return 0;
#  endif
}

ATFRAMEWORK_UTILS_API bool hmac::is_valid() const noexcept { return context_ != nullptr; }

ATFRAMEWORK_UTILS_API int64_t hmac::get_last_errno() const noexcept { return last_errno_; }

ATFRAMEWORK_UTILS_API void hmac::set_last_errno(int64_t e) noexcept { last_errno_ = e; }

ATFRAMEWORK_UTILS_API int hmac::compute(digest_type_t type, const unsigned char* key, size_t key_len,
                                        const unsigned char* input, size_t input_len, unsigned char* output,
                                        size_t* output_len) {
  if (output == nullptr || output_len == nullptr) {
    return hmac_error_code_t::kInvalidParam;
  }

  size_t expected_len = get_digest_output_length(type);
  if (expected_len == 0) {
    return hmac_error_code_t::kDigestNotSupport;
  }

  if (*output_len < expected_len) {
    *output_len = expected_len;
    return hmac_error_code_t::kOutputBufferTooSmall;
  }

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

  const EVP_MD* md = details::get_evp_md_by_type(type);
  if (md == nullptr) {
    return hmac_error_code_t::kDigestNotSupport;
  }

#    if ATFW_CRYPTO_HMAC_USE_EVP_MAC
  // Use EVP_MAC API for OpenSSL 3.0+
  EVP_MAC* mac = EVP_MAC_fetch(nullptr, "HMAC", nullptr);
  if (mac == nullptr) {
    return hmac_error_code_t::kOperation;
  }

  EVP_MAC_CTX* ctx = EVP_MAC_CTX_new(mac);
  if (ctx == nullptr) {
    EVP_MAC_free(mac);
    return hmac_error_code_t::kMalloc;
  }

  const char* digest_name = get_digest_name(type);
  OSSL_PARAM params[2];
  params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, const_cast<char*>(digest_name), 0);
  params[1] = OSSL_PARAM_construct_end();

  int ret = hmac_error_code_t::kOk;
  if (EVP_MAC_init(ctx, key, key_len, params) != 1) {
    ret = hmac_error_code_t::kOperation;
  } else if (EVP_MAC_update(ctx, input, input_len) != 1) {
    ret = hmac_error_code_t::kOperation;
  } else {
    size_t out_len = *output_len;
    if (EVP_MAC_final(ctx, output, &out_len, *output_len) != 1) {
      ret = hmac_error_code_t::kOperation;
    } else {
      *output_len = out_len;
    }
  }

  EVP_MAC_CTX_free(ctx);
  EVP_MAC_free(mac);
  return ret;

#    else
  // Use HMAC() one-shot function for older OpenSSL
  unsigned int out_len = 0;
  unsigned char* result = HMAC(md, key, static_cast<int>(key_len), input, input_len, output, &out_len);
  if (result == nullptr) {
    return hmac_error_code_t::kOperation;
  }
  *output_len = out_len;
  return hmac_error_code_t::kOk;
#    endif

#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
  const mbedtls_md_info_t* md_info = details::get_md_info_by_type(type);
  if (md_info == nullptr) {
    return hmac_error_code_t::kDigestNotSupport;
  }

  int ret = mbedtls_md_hmac(md_info, key, key_len, input, input_len, output);
  if (ret != 0) {
    return hmac_error_code_t::kOperation;
  }
  *output_len = expected_len;
  return hmac_error_code_t::kOk;
#  endif
}

ATFRAMEWORK_UTILS_API int hmac::compute(digest_type_t type, gsl::span<const unsigned char> key,
                                        gsl::span<const unsigned char> input, unsigned char* output,
                                        size_t* output_len) {
  return compute(type, key.data(), key.size(), input.data(), input.size(), output, output_len);
}

ATFRAMEWORK_UTILS_API std::vector<unsigned char> hmac::compute_to_binary(digest_type_t type, const unsigned char* key,
                                                                         size_t key_len, const unsigned char* input,
                                                                         size_t input_len) {
  size_t output_len = get_digest_output_length(type);
  if (output_len == 0) {
    return std::vector<unsigned char>();
  }

  std::vector<unsigned char> result(output_len);
  if (compute(type, key, key_len, input, input_len, result.data(), &output_len) != hmac_error_code_t::kOk) {
    return std::vector<unsigned char>();
  }
  result.resize(output_len);
  return result;
}

ATFRAMEWORK_UTILS_API std::vector<unsigned char> hmac::compute_to_binary(digest_type_t type,
                                                                         gsl::span<const unsigned char> key,
                                                                         gsl::span<const unsigned char> input) {
  return compute_to_binary(type, key.data(), key.size(), input.data(), input.size());
}

ATFRAMEWORK_UTILS_API std::string hmac::compute_to_hex(digest_type_t type, const unsigned char* key, size_t key_len,
                                                       const unsigned char* input, size_t input_len, bool uppercase) {
  std::vector<unsigned char> binary = compute_to_binary(type, key, key_len, input, input_len);
  if (binary.empty()) {
    return std::string();
  }

  std::string result;
  result.resize(binary.size() * 2);
  ATFRAMEWORK_UTILS_NAMESPACE_ID::string::dumphex(binary.data(), binary.size(), &result[0], uppercase);
  return result;
}

ATFRAMEWORK_UTILS_API std::string hmac::compute_to_hex(digest_type_t type, gsl::span<const unsigned char> key,
                                                       gsl::span<const unsigned char> input, bool uppercase) {
  return compute_to_hex(type, key.data(), key.size(), input.data(), input.size(), uppercase);
}

// ============================================================================
// HKDF class implementation
// ============================================================================

ATFRAMEWORK_UTILS_API int hkdf::extract(digest_type_t type, const unsigned char* salt, size_t salt_len,
                                        const unsigned char* ikm, size_t ikm_len, unsigned char* prk, size_t* prk_len) {
  if (ikm == nullptr && ikm_len > 0) {
    return error_code_t::kInvalidParam;
  }
  if (prk == nullptr || prk_len == nullptr) {
    return error_code_t::kInvalidParam;
  }

  size_t hash_len = get_digest_output_length(type);
  if (hash_len == 0) {
    return error_code_t::kDigestNotSupport;
  }

  if (*prk_len < hash_len) {
    *prk_len = hash_len;
    return hmac_error_code_t::kOutputBufferTooSmall;
  }

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

#    if ATFW_CRYPTO_HKDF_USE_NATIVE
  // Use native HKDF_extract for BoringSSL and LibreSSL
  const EVP_MD* md = details::get_evp_md_by_type(type);
  if (md == nullptr) {
    return error_code_t::kDigestNotSupport;
  }

  if (HKDF_extract(prk, prk_len, md, ikm, ikm_len, salt, salt_len) != 1) {
    return error_code_t::kOperation;
  }
  return error_code_t::kOk;
#    else
  // Default OpenSSL implementation: HKDF-Extract is essentially HMAC(salt, ikm)
  // If salt is not provided, use a string of hash_len zeros
  std::vector<unsigned char> zero_salt;
  if (salt == nullptr || salt_len == 0) {
    zero_salt.resize(hash_len, 0);
    salt = zero_salt.data();
    salt_len = zero_salt.size();
  }

  int ret = hmac::compute(type, salt, salt_len, ikm, ikm_len, prk, prk_len);
  if (ret != hmac_error_code_t::kOk) {
    return error_code_t::kOperation;
  }

  return error_code_t::kOk;
#    endif

#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
  const mbedtls_md_info_t* md_info = details::get_md_info_by_type(type);
  if (md_info == nullptr) {
    return error_code_t::kDigestNotSupport;
  }

  int ret = mbedtls_hkdf_extract(md_info, salt, salt_len, ikm, ikm_len, prk);
  if (ret != 0) {
    return error_code_t::kOperation;
  }
  *prk_len = hash_len;
  return error_code_t::kOk;
#  endif
}

ATFRAMEWORK_UTILS_API int hkdf::extract(digest_type_t type, gsl::span<const unsigned char> salt,
                                        gsl::span<const unsigned char> ikm, unsigned char* prk, size_t* prk_len) {
  return extract(type, salt.data(), salt.size(), ikm.data(), ikm.size(), prk, prk_len);
}

ATFRAMEWORK_UTILS_API int hkdf::expand(digest_type_t type, const unsigned char* prk, size_t prk_len,
                                       const unsigned char* info, size_t info_len, unsigned char* okm, size_t okm_len) {
  if (prk == nullptr && prk_len > 0) {
    return error_code_t::kInvalidParam;
  }
  if (okm == nullptr && okm_len > 0) {
    return error_code_t::kInvalidParam;
  }

  size_t hash_len = get_digest_output_length(type);
  if (hash_len == 0) {
    return error_code_t::kDigestNotSupport;
  }

  // RFC 5869: L <= 255 * HashLen
  if (okm_len > 255 * hash_len) {
    return error_code_t::kOutputLengthTooLarge;
  }

  if (okm_len == 0) {
    return error_code_t::kOk;
  }

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

#    if ATFW_CRYPTO_HKDF_USE_EVP_KDF
  // Use EVP_KDF for OpenSSL 3.0+
  EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "HKDF", nullptr);
  if (kdf == nullptr) {
    return error_code_t::kOperation;
  }

  EVP_KDF_CTX* kctx = EVP_KDF_CTX_new(kdf);
  EVP_KDF_free(kdf);
  if (kctx == nullptr) {
    return error_code_t::kOperation;
  }

  const char* digest_name = get_digest_name(type);
  OSSL_PARAM params[5];
  int idx = 0;
  int hkdf_mode = EVP_KDF_HKDF_MODE_EXPAND_ONLY;
  params[idx++] = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, const_cast<char*>(digest_name), 0);
  params[idx++] = OSSL_PARAM_construct_int(OSSL_KDF_PARAM_MODE, &hkdf_mode);
  params[idx++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, const_cast<unsigned char*>(prk), prk_len);
  if (info != nullptr && info_len > 0) {
    params[idx++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, const_cast<unsigned char*>(info), info_len);
  }
  params[idx] = OSSL_PARAM_construct_end();

  int ret = error_code_t::kOk;
  if (EVP_KDF_derive(kctx, okm, okm_len, params) != 1) {
    ret = error_code_t::kOperation;
  }

  EVP_KDF_CTX_free(kctx);
  return ret;

#    elif ATFW_CRYPTO_HKDF_USE_EVP_PKEY
  // Use EVP_PKEY_derive for OpenSSL 1.1.0+
  EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
  if (pctx == nullptr) {
    return error_code_t::kOperation;
  }

  int ret = error_code_t::kOk;
  const EVP_MD* md = details::get_evp_md_by_type(type);

  do {
    if (EVP_PKEY_derive_init(pctx) <= 0) {
      ret = error_code_t::kOperation;
      break;
    }
    if (EVP_PKEY_CTX_hkdf_mode(pctx, EVP_PKEY_HKDEF_MODE_EXPAND_ONLY) <= 0) {
      ret = error_code_t::kOperation;
      break;
    }
    if (EVP_PKEY_CTX_set_hkdf_md(pctx, md) <= 0) {
      ret = error_code_t::kOperation;
      break;
    }
    if (EVP_PKEY_CTX_set1_hkdf_key(pctx, prk, static_cast<int>(prk_len)) <= 0) {
      ret = error_code_t::kOperation;
      break;
    }
    if (info != nullptr && info_len > 0) {
      if (EVP_PKEY_CTX_add1_hkdf_info(pctx, info, static_cast<int>(info_len)) <= 0) {
        ret = error_code_t::kOperation;
        break;
      }
    }
    size_t out_len = okm_len;
    if (EVP_PKEY_derive(pctx, okm, &out_len) <= 0) {
      ret = error_code_t::kOperation;
      break;
    }
  } while (false);

  EVP_PKEY_CTX_free(pctx);
  return ret;

#    elif ATFW_CRYPTO_HKDF_USE_NATIVE
  // Use native HKDF_expand for BoringSSL and LibreSSL
  const EVP_MD* md = details::get_evp_md_by_type(type);
  if (md == nullptr) {
    return error_code_t::kDigestNotSupport;
  }

  if (HKDF_expand(okm, okm_len, md, prk, prk_len, info, info_len) != 1) {
    return error_code_t::kOperation;
  }
  return error_code_t::kOk;

#    else
  // Manual implementation for older OpenSSL
  // T(0) = empty string
  // T(i) = HMAC-Hash(PRK, T(i-1) | info | i)
  // OKM = first L bytes of T(1) | T(2) | ... | T(N)

  size_t n = (okm_len + hash_len - 1) / hash_len;
  std::vector<unsigned char> t_prev;
  std::vector<unsigned char> t_input;
  size_t offset = 0;

  for (size_t i = 1; i <= n && offset < okm_len; ++i) {
    // Build input: T(i-1) | info | i
    t_input.clear();
    t_input.insert(t_input.end(), t_prev.begin(), t_prev.end());
    if (info != nullptr && info_len > 0) {
      t_input.insert(t_input.end(), info, info + info_len);
    }
    t_input.push_back(static_cast<unsigned char>(i));

    // Compute T(i) = HMAC(PRK, T(i-1) | info | i)
    std::vector<unsigned char> t_curr(hash_len);
    size_t t_len = hash_len;
    int ret = hmac::compute(type, prk, prk_len, t_input.data(), t_input.size(), t_curr.data(), &t_len);
    if (ret != hmac_error_code_t::kOk) {
      return error_code_t::kOperation;
    }

    // Copy to output
    size_t copy_len = (okm_len - offset < hash_len) ? (okm_len - offset) : hash_len;
    memcpy(okm + offset, t_curr.data(), copy_len);
    offset += copy_len;

    t_prev = std::move(t_curr);
  }

  return error_code_t::kOk;
#    endif

#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
  const mbedtls_md_info_t* md_info = details::get_md_info_by_type(type);
  if (md_info == nullptr) {
    return error_code_t::kDigestNotSupport;
  }

  int ret = mbedtls_hkdf_expand(md_info, prk, prk_len, info, info_len, okm, okm_len);
  if (ret != 0) {
    return error_code_t::kOperation;
  }
  return error_code_t::kOk;
#  endif
}

ATFRAMEWORK_UTILS_API int hkdf::expand(digest_type_t type, gsl::span<const unsigned char> prk,
                                       gsl::span<const unsigned char> info, unsigned char* okm, size_t okm_len) {
  return expand(type, prk.data(), prk.size(), info.data(), info.size(), okm, okm_len);
}

ATFRAMEWORK_UTILS_API int hkdf::derive(digest_type_t type, const unsigned char* salt, size_t salt_len,
                                       const unsigned char* ikm, size_t ikm_len, const unsigned char* info,
                                       size_t info_len, unsigned char* okm, size_t okm_len) {
  size_t hash_len = get_digest_output_length(type);
  if (hash_len == 0) {
    return error_code_t::kDigestNotSupport;
  }

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_USE_OPENSSL) || defined(ATFRAMEWORK_UTILS_CRYPTO_USE_LIBRESSL) || \
      defined(ATFRAMEWORK_UTILS_CRYPTO_USE_BORINGSSL)

#    if ATFW_CRYPTO_HKDF_USE_EVP_KDF
  // Use EVP_KDF for OpenSSL 3.0+ (full HKDF mode)
  EVP_KDF* kdf = EVP_KDF_fetch(nullptr, "HKDF", nullptr);
  if (kdf == nullptr) {
    return error_code_t::kOperation;
  }

  EVP_KDF_CTX* kctx = EVP_KDF_CTX_new(kdf);
  EVP_KDF_free(kdf);
  if (kctx == nullptr) {
    return error_code_t::kOperation;
  }

  const char* digest_name = get_digest_name(type);
  OSSL_PARAM params[6];
  int idx = 0;
  params[idx++] = OSSL_PARAM_construct_utf8_string(OSSL_KDF_PARAM_DIGEST, const_cast<char*>(digest_name), 0);
  params[idx++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_KEY, const_cast<unsigned char*>(ikm), ikm_len);
  if (salt != nullptr && salt_len > 0) {
    params[idx++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_SALT, const_cast<unsigned char*>(salt), salt_len);
  }
  if (info != nullptr && info_len > 0) {
    params[idx++] = OSSL_PARAM_construct_octet_string(OSSL_KDF_PARAM_INFO, const_cast<unsigned char*>(info), info_len);
  }
  params[idx] = OSSL_PARAM_construct_end();

  int ret = error_code_t::kOk;
  if (EVP_KDF_derive(kctx, okm, okm_len, params) != 1) {
    ret = error_code_t::kOperation;
  }

  EVP_KDF_CTX_free(kctx);
  return ret;

#    elif ATFW_CRYPTO_HKDF_USE_EVP_PKEY
  // Use EVP_PKEY_derive for OpenSSL 1.1.0+
  EVP_PKEY_CTX* pctx = EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr);
  if (pctx == nullptr) {
    return error_code_t::kOperation;
  }

  int ret = error_code_t::kOk;
  const EVP_MD* md = details::get_evp_md_by_type(type);

  do {
    if (EVP_PKEY_derive_init(pctx) <= 0) {
      ret = error_code_t::kOperation;
      break;
    }
    if (EVP_PKEY_CTX_set_hkdf_md(pctx, md) <= 0) {
      ret = error_code_t::kOperation;
      break;
    }
    if (salt != nullptr && salt_len > 0) {
      if (EVP_PKEY_CTX_set1_hkdf_salt(pctx, salt, static_cast<int>(salt_len)) <= 0) {
        ret = error_code_t::kOperation;
        break;
      }
    }
    if (EVP_PKEY_CTX_set1_hkdf_key(pctx, ikm, static_cast<int>(ikm_len)) <= 0) {
      ret = error_code_t::kOperation;
      break;
    }
    if (info != nullptr && info_len > 0) {
      if (EVP_PKEY_CTX_add1_hkdf_info(pctx, info, static_cast<int>(info_len)) <= 0) {
        ret = error_code_t::kOperation;
        break;
      }
    }
    size_t out_len = okm_len;
    if (EVP_PKEY_derive(pctx, okm, &out_len) <= 0) {
      ret = error_code_t::kOperation;
      break;
    }
  } while (false);

  EVP_PKEY_CTX_free(pctx);
  return ret;

#    elif ATFW_CRYPTO_HKDF_USE_NATIVE
  // Use native HKDF for BoringSSL and LibreSSL
  const EVP_MD* md = details::get_evp_md_by_type(type);
  if (md == nullptr) {
    return error_code_t::kDigestNotSupport;
  }

  if (HKDF(okm, okm_len, md, ikm, ikm_len, salt, salt_len, info, info_len) != 1) {
    return error_code_t::kOperation;
  }
  return error_code_t::kOk;

#    else
  // Manual implementation: extract then expand
  std::vector<unsigned char> prk(hash_len);
  size_t prk_len = hash_len;
  int ret = extract(type, salt, salt_len, ikm, ikm_len, prk.data(), &prk_len);
  if (ret != error_code_t::kOk) {
    return ret;
  }

  return expand(type, prk.data(), prk_len, info, info_len, okm, okm_len);
#    endif

#  elif defined(ATFRAMEWORK_UTILS_CRYPTO_USE_MBEDTLS)
  const mbedtls_md_info_t* md_info = details::get_md_info_by_type(type);
  if (md_info == nullptr) {
    return error_code_t::kDigestNotSupport;
  }

  int ret = mbedtls_hkdf(md_info, salt, salt_len, ikm, ikm_len, info, info_len, okm, okm_len);
  if (ret != 0) {
    return error_code_t::kOperation;
  }
  return error_code_t::kOk;
#  endif
}

ATFRAMEWORK_UTILS_API int hkdf::derive(digest_type_t type, gsl::span<const unsigned char> salt,
                                       gsl::span<const unsigned char> ikm, gsl::span<const unsigned char> info,
                                       unsigned char* okm, size_t okm_len) {
  return derive(type, salt.data(), salt.size(), ikm.data(), ikm.size(), info.data(), info.size(), okm, okm_len);
}

ATFRAMEWORK_UTILS_API std::vector<unsigned char> hkdf::derive_to_binary(digest_type_t type,
                                                                        gsl::span<const unsigned char> salt,
                                                                        gsl::span<const unsigned char> ikm,
                                                                        gsl::span<const unsigned char> info,
                                                                        size_t okm_len) {
  if (okm_len == 0) {
    return std::vector<unsigned char>();
  }

  std::vector<unsigned char> result(okm_len);
  int ret = derive(type, salt, ikm, info, result.data(), okm_len);
  if (ret != error_code_t::kOk) {
    return std::vector<unsigned char>();
  }
  return result;
}

}  // namespace crypto
ATFRAMEWORK_UTILS_NAMESPACE_END

#  if defined(ATFRAMEWORK_UTILS_CRYPTO_IGNORE_VERSION_WARNINGS)
#    if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#      if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#        pragma GCC diagnostic pop
#      endif
#    elif defined(__clang__) || defined(__apple_build_version__)
#      pragma clang diagnostic pop
#    endif

#    if defined(_MSC_VER)
#      pragma warning(pop)
#    endif
#  endif

#endif  // ATFW_UTIL_MACRO_CRYPTO_HMAC_ENABLED
