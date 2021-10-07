// Copyright 2021 atframework
// Create by owent

#include "algorithm/crypto_dh.h"

#include <common/file_system.h>
#include <common/string_oprs.h>

#include <std/static_assert.h>

#include <common/compiler_message.h>
#include <config/compiler_features.h>
#include <std/explicit_declare.h>

#include <design_pattern/nomovable.h>
#include <design_pattern/noncopyable.h>

#include <assert.h>
#include <cstring>
#include <iostream>

#if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#  include <type_traits>
#endif

#ifdef CRYPTO_DH_ENABLED

// define max key cache length, the same as MBEDTLS_SSL_MAX_CONTENT_LEN
#  define CRYPTO_DH_MAX_KEY_LEN 1024

#  ifndef UNUSED
#    define UNUSED(x) ((void)x)
#  endif

#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)

#    include <openssl/ec.h>
#    include <openssl/evp.h>
#    include <openssl/rand.h>

#    ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
#      include <openssl/core_names.h>
#      include <openssl/decoder.h>
#      include <openssl/encoder.h>
#      include <openssl/param_build.h>
#    endif

// copy from ssl_locl.h
#    ifndef s2n
#      define s2n(s, c) ((c[0] = (unsigned char)(((s) >> 8) & 0xff), c[1] = (unsigned char)(((s)) & 0xff)), c += 2)
#    endif

// copy from ssl_locl.h
#    ifndef NAMED_CURVE_TYPE
#      define NAMED_CURVE_TYPE 3
#    endif

// copy from t1_lib.c of openssl 1.1.0
struct tls_curve_info {
  int nid;            /* Curve NID */
  int secbits;        /* Bits of security (from SP800-57) */
  unsigned int flags; /* Flags: currently just field type */
                      // uint16_t     group_id; // TODO Update with openssl 3.x.x declare
};

#    ifndef TLS_CURVE_CHAR2
#      define TLS_CURVE_CHAR2 0x1
#    endif

#    ifndef TLS_CURVE_PRIME
#      define TLS_CURVE_PRIME 0x0
#    endif

#    ifndef TLS_CURVE_CUSTOM
#      define TLS_CURVE_CUSTOM 0x2
#    endif

/**
 * Table of curve information.
 * Do not delete entries or reorder this array! It is used as a lookup
 * table: the index of each entry is one less than the TLS curve id.
 * @see t1_lib.c in openssl source tree for more details
 */
static constexpr const tls_curve_info nid_list[] = {
    {
#    ifdef NID_sect163k1
        NID_sect163k1
#    else
        0
#    endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect163k1 (1) */
    {
#    ifdef NID_sect163r1
        NID_sect163r1
#    else
        0
#    endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect163r1 (2) */
    {
#    ifdef NID_sect163r2
        NID_sect163r2
#    else
        0
#    endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect163r2 (3) */
    {
#    ifdef NID_sect193r1
        NID_sect193r1
#    else
        0
#    endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect193r1 (4) */
    {
#    ifdef NID_sect193r2
        NID_sect193r2
#    else
        0
#    endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect193r2 (5) */
    {
#    ifdef NID_sect233k1
        NID_sect233k1
#    else
        0
#    endif
        ,
        112, TLS_CURVE_CHAR2}, /* sect233k1 (6) */
    {
#    ifdef NID_sect233r1
        NID_sect233r1
#    else
        0
#    endif
        ,
        112, TLS_CURVE_CHAR2}, /* sect233r1 (7) */
    {
#    ifdef NID_sect239k1
        NID_sect239k1
#    else
        0
#    endif
        ,
        112, TLS_CURVE_CHAR2}, /* sect239k1 (8) */
    {
#    ifdef NID_sect283k1
        NID_sect283k1
#    else
        0
#    endif
        ,
        128, TLS_CURVE_CHAR2}, /* sect283k1 (9) */
    {
#    ifdef NID_sect283r1
        NID_sect283r1
#    else
        0
#    endif
        ,
        128, TLS_CURVE_CHAR2}, /* sect283r1 (10) */
    {
#    ifdef NID_sect409k1
        NID_sect409k1
#    else
        0
#    endif
        ,
        192, TLS_CURVE_CHAR2}, /* sect409k1 (11) */
    {
#    ifdef NID_sect409r1
        NID_sect409r1
#    else
        0
#    endif
        ,
        192, TLS_CURVE_CHAR2}, /* sect409r1 (12) */
    {
#    ifdef NID_sect571k1
        NID_sect571k1
#    else
        0
#    endif
        ,
        256, TLS_CURVE_CHAR2}, /* sect571k1 (13) */
    {
#    ifdef NID_sect571r1
        NID_sect571r1
#    else
        0
#    endif
        ,
        256, TLS_CURVE_CHAR2}, /* sect571r1 (14) */
    {
#    ifdef NID_secp160k1
        NID_secp160k1
#    else
        0
#    endif
        ,
        80, TLS_CURVE_PRIME}, /* secp160k1 (15) */
    {
#    ifdef NID_secp160r1
        NID_secp160r1
#    else
        0
#    endif
        ,
        80, TLS_CURVE_PRIME}, /* secp160r1 (16) */
    {
#    ifdef NID_secp160r2
        NID_secp160r2
#    else
        0
#    endif
        ,
        80, TLS_CURVE_PRIME}, /* secp160r2 (17) */
    {
#    ifdef NID_secp192k1
        NID_secp192k1
#    else
        0
#    endif
        ,
        80, TLS_CURVE_PRIME}, /* secp192k1 (18) */
    {
#    ifdef NID_X9_62_prime192v1
        NID_X9_62_prime192v1
#    else
        0
#    endif
        ,
        80, TLS_CURVE_PRIME}, /* secp192r1 (19) */
    {
#    ifdef NID_secp224k1
        NID_secp224k1
#    else
        0
#    endif
        ,
        112, TLS_CURVE_PRIME}, /* secp224k1 (20) */
    {
#    ifdef NID_secp224r1
        NID_secp224r1
#    else
        0
#    endif
        ,
        112, TLS_CURVE_PRIME}, /* secp224r1 (21) */
    {
#    ifdef NID_secp256k1
        NID_secp256k1
#    else
        0
#    endif
        ,
        128, TLS_CURVE_PRIME}, /* secp256k1 (22) */
    {
#    ifdef NID_X9_62_prime256v1
        NID_X9_62_prime256v1
#    else
        0
#    endif
        ,
        128, TLS_CURVE_PRIME}, /* secp256r1 (23) */
    {
#    ifdef NID_secp384r1
        NID_secp384r1
#    else
        0
#    endif
        ,
        192, TLS_CURVE_PRIME}, /* secp384r1 (24) */
    {
#    ifdef NID_secp521r1
        NID_secp521r1
#    else
        0
#    endif
        ,
        256, TLS_CURVE_PRIME}, /* secp521r1 (25) */
    {
#    ifdef NID_brainpoolP256r1
        NID_brainpoolP256r1
#    else
        0
#    endif
        ,
        128, TLS_CURVE_PRIME}, /* brainpoolP256r1 (26) */
    {
#    ifdef NID_brainpoolP384r1
        NID_brainpoolP384r1
#    else
        0
#    endif
        ,
        192, TLS_CURVE_PRIME}, /* brainpoolP384r1 (27) */
    {
#    ifdef NID_brainpoolP512r1
        NID_brainpoolP512r1
#    else
        0
#    endif
        ,
        256, TLS_CURVE_PRIME}, /* brainpool512r1 (28) */
    {
#    ifdef NID_X25519
        NID_X25519
#    else
        0
#    endif
        ,
        128, TLS_CURVE_CUSTOM}, /* X25519 (29) */
    {
#    ifdef NID_X448
        NID_X448
#    else
        0
#    endif
        ,
        224, TLS_CURVE_CUSTOM}, /* X448 (30) */
};

#    define OSSL_NELEM(x) (sizeof(x) / sizeof(x[0]))

static int tls1_ec_group_id2nid(int group_id, unsigned int *pflags) {
  const tls_curve_info *cinfo;
  /* ECC curves from RFC 4492 and RFC 7027 */
  if ((group_id < 1) || ((unsigned int)group_id > OSSL_NELEM(nid_list))) return 0;
  cinfo = nid_list + group_id - 1;
  if (pflags) *pflags = cinfo->flags;
  return cinfo->nid;
}

static int tls1_nid2group_id(int nid) {
  int i;
  for (i = 0; i < static_cast<int>(OSSL_NELEM(nid_list)); i++) {
    if (nid_list[i].nid == nid) {
      return i + 1;
    }
  }
  return 0;
}

// adaptor for openssl 1.0.x -> openssl 1.1.x
#    if OPENSSL_VERSION_NUMBER < 0x10100000L

/**
 * @see crypto/dh/dh_lib.c in openssl 1.1.x
 */
static inline void DH_get0_pqg(const DH *dh, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g) {
  if (p != nullptr) *p = dh->p;
  if (q != nullptr) *q = dh->q;
  if (g != nullptr) *g = dh->g;
}

/**
 * @see crypto/dh/dh_lib.c in openssl 1.1.x
 */
static inline int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g) {
  /* If the fields p and g in d are nullptr, the corresponding input
   * parameters MUST be non-nullptr.  q may remain nullptr.
   */
  if ((dh->p == nullptr && p == nullptr) || (dh->g == nullptr && g == nullptr)) return 0;

  if (p != nullptr) {
    BN_free(dh->p);
    dh->p = p;
  }
  if (q != nullptr) {
    BN_free(dh->q);
    dh->q = q;
  }
  if (g != nullptr) {
    BN_free(dh->g);
    dh->g = g;
  }

  if (q != nullptr) {
    dh->length = BN_num_bits(q);
  }

  return 1;
}

/**
 * @see crypto/dh/dh_lib.c in openssl 1.1.x or upper
 */
static inline void DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key) {
  if (pub_key != nullptr) *pub_key = dh->pub_key;
  if (priv_key != nullptr) *priv_key = dh->priv_key;
}

static DH *EVP_PKEY_get0_DH(EVP_PKEY *pkey) {
  DH *ret = EVP_PKEY_get1_DH(pkey);
  if (nullptr != ret) {
    DH_free(ret);
  }
  return ret;
}

/**
 * @see crypto/dh/dh_lib.c in openssl 1.1.x or upper
 */
EXPLICIT_UNUSED_ATTR static inline int DH_set0_key(DH *dh, BIGNUM *pub_key, BIGNUM *priv_key) {
  /* If the field pub_key in dh is nullptr, the corresponding input
   * parameters MUST be non-nullptr.  The priv_key field may
   * be left nullptr.
   */
  if (dh->pub_key == nullptr && pub_key == nullptr) return 0;

  if (pub_key != nullptr) {
    BN_free(dh->pub_key);
    dh->pub_key = pub_key;
  }
  if (priv_key != nullptr) {
    BN_free(dh->priv_key);
    dh->priv_key = priv_key;
  }

  return 1;
}

#    endif

#    if defined(LIBRESSL_VERSION_NUMBER) || OPENSSL_VERSION_NUMBER < 0x10100000L

#      ifndef ASN1_PKEY_CTRL_SET1_TLS_ENCPT
#        define ASN1_PKEY_CTRL_SET1_TLS_ENCPT 0x9
#      endif

#      ifndef ASN1_PKEY_CTRL_GET1_TLS_ENCPT
#        define ASN1_PKEY_CTRL_GET1_TLS_ENCPT 0xa
#      endif

static int EC_KEY_oct2key(EC_KEY *key, const unsigned char *buf, size_t len, BN_CTX *ctx) {
  if (key == nullptr) return 0;
  const EC_GROUP *group = EC_KEY_get0_group(key);
  if (group == nullptr) return 0;
  EC_POINT *point = EC_POINT_new(group);
  if (nullptr == point) {
    return 0;
  }
  if (EC_POINT_oct2point(group, point, buf, len, ctx) <= 0) {
    EC_POINT_free(point);
    return 0;
  }

  EC_KEY_set_public_key(key, point);
  EC_POINT_free(point);
  return 1;
}

static size_t EC_POINT_point2buf(const EC_GROUP *group, const EC_POINT *point, point_conversion_form_t form,
                                 unsigned char **pbuf, BN_CTX *ctx) {
  size_t len;
  unsigned char *buf;

  len = EC_POINT_point2oct(group, point, form, nullptr, 0, nullptr);
  if (len == 0) return 0;
  if ((buf = (unsigned char *)OPENSSL_malloc(len)) == nullptr) {
    ECerr(281 /*EC_F_EC_POINT_POINT2BUF*/, ERR_R_MALLOC_FAILURE);
    return 0;
  }
  len = EC_POINT_point2oct(group, point, form, buf, len, ctx);
  if (len == 0) {
    OPENSSL_free(buf);
    return 0;
  }
  *pbuf = buf;
  return len;
}

static size_t EC_KEY_key2buf(const EC_KEY *key, point_conversion_form_t form, unsigned char **pbuf, BN_CTX *ctx) {
  if (key == nullptr || EC_KEY_get0_public_key(key) == nullptr || EC_KEY_get0_group(key) == nullptr) return 0;
  return EC_POINT_point2buf(EC_KEY_get0_group(key), EC_KEY_get0_public_key(key), form, pbuf, ctx);
}

// Just like crypto/ec/ec_ameth.c, openssl < 1.1.0 do not support x25519/x448 and ECX_KEY, just skip it
static int evp_pkey_asn1_ctrl(EVP_PKEY *pkey, int op, int arg1, void *arg2) {
  if (EVP_PKEY_get0_asn1(pkey) == nullptr) return -2;
  // openssl < 1.1.0 do not has API of EVP_PKEY_get0_EC_KEY(key)
  // So we can only use EVP_PKEY_get1_EC_KEY and free it later
  EC_KEY *ec_key = EVP_PKEY_get1_EC_KEY(pkey);
  int ret;
  switch (op) {
    case ASN1_PKEY_CTRL_SET1_TLS_ENCPT:
      ret = EC_KEY_oct2key(ec_key, (const unsigned char *)arg2, (size_t)arg1, nullptr);
      break;
    case ASN1_PKEY_CTRL_GET1_TLS_ENCPT:
      ret = (int)EC_KEY_key2buf(ec_key, POINT_CONVERSION_UNCOMPRESSED, (unsigned char **)arg2, nullptr);
      break;
    default:
      ret = -2;
      break;
  }
  if (nullptr != ec_key) {
    EC_KEY_free(ec_key);
  }
  return ret;
}

static size_t EVP_PKEY_get1_tls_encodedpoint(EVP_PKEY *pkey, unsigned char **ppt) {
  int rv;
  rv = evp_pkey_asn1_ctrl(pkey, ASN1_PKEY_CTRL_GET1_TLS_ENCPT, 0, (void *)ppt);
  if (rv <= 0) return 0;
  return (size_t)rv;
}

static int EVP_PKEY_set1_tls_encodedpoint(EVP_PKEY *pkey, const unsigned char *pt, size_t ptlen) {
  if (ptlen > INT_MAX) return 0;
  if (evp_pkey_asn1_ctrl(pkey, ASN1_PKEY_CTRL_SET1_TLS_ENCPT, (int)ptlen, (void *)pt) <= 0) return 0;
  return 1;
}

#    endif

static size_t crypto_dh_EVP_PKEY_get1_tls_encodedpoint(EVP_PKEY *pkey, unsigned char **ppt) {
#    if defined(CRYPTO_USE_BORINGSSL)
  EC_KEY *ec_key = EVP_PKEY_get0_EC_KEY(pkey);
  return EC_KEY_key2buf(ec_key, POINT_CONVERSION_UNCOMPRESSED, ppt, nullptr);
#    elif (defined(OPENSSL_API_COMPAT) && OPENSSL_API_COMPAT >= 0x30000000L) ||  \
        (defined(OPENSSL_API_LEVEL) && OPENSSL_API_LEVEL >= 30000) ||            \
        (!defined(LIBRESSL_VERSION_NUMBER) && defined(OPENSSL_VERSION_NUMBER) && \
         OPENSSL_VERSION_NUMBER >= 0x30000000L)
  return EVP_PKEY_get1_encoded_public_key(pkey, ppt);
#    else
  return EVP_PKEY_get1_tls_encodedpoint(pkey, ppt);
#    endif
}

#    if defined(CRYPTO_USE_BORINGSSL)
static int crypto_dh_EC_KEY_oct2key(EC_KEY *key, const unsigned char *buf, size_t len, BN_CTX *ctx) {
  if (key == nullptr) return 0;
  const EC_GROUP *group = EC_KEY_get0_group(key);
  if (group == nullptr) return 0;
  EC_POINT *point = EC_POINT_new(group);
  if (nullptr == point) {
    return 0;
  }
  if (EC_POINT_oct2point(group, point, buf, len, ctx) <= 0) {
    EC_POINT_free(point);
    return 0;
  }

  EC_KEY_set_public_key(key, point);
  EC_POINT_free(point);
  return 1;
}
#    endif

static size_t crypto_dh_EVP_PKEY_set1_tls_encodedpoint(EVP_PKEY *pkey, const unsigned char *pt, size_t ptlen) {
#    if defined(CRYPTO_USE_BORINGSSL)
  EC_KEY *ec_key = EVP_PKEY_get0_EC_KEY(pkey);
  if (crypto_dh_EC_KEY_oct2key(ec_key, pt, ptlen, nullptr) <= 0) {
    return 0;
  }
  return 1;
#    elif (defined(OPENSSL_API_COMPAT) && OPENSSL_API_COMPAT >= 0x30000000L) ||  \
        (defined(OPENSSL_API_LEVEL) && OPENSSL_API_LEVEL >= 30000) ||            \
        (!defined(LIBRESSL_VERSION_NUMBER) && defined(OPENSSL_VERSION_NUMBER) && \
         OPENSSL_VERSION_NUMBER >= 0x30000000L)
  return EVP_PKEY_set1_encoded_public_key(pkey, pt, ptlen);
#    else
  return EVP_PKEY_set1_tls_encodedpoint(pkey, pt, ptlen);
#    endif
}

#  endif

#  ifdef max
#    undef max
#  endif

namespace util {
namespace crypto {
namespace details {
static inline dh::error_code_t::type setup_errorno(dh &ci, int err, dh::error_code_t::type ret) {
  ci.set_last_errno(err);
  return ret;
}

static constexpr const char *supported_dh_curves[] = {
    "",
    "x25519",           // see ecp_supported_curves in ecp.c of mbedtls
    "x448",             // mbedtls don't support right now but maybe support it in the future
    "secp521r1",        // see ecp_supported_curves in ecp.c of mbedtls
    "secp384r1",        // see ecp_supported_curves in ecp.c of mbedtls
    "secp256r1",        // see ecp_supported_curves in ecp.c of mbedtls
    "secp224r1",        // see ecp_supported_curves in ecp.c of mbedtls
    "secp192r1",        // see ecp_supported_curves in ecp.c of mbedtls
    "secp256k1",        // see ecp_supported_curves in ecp.c of mbedtls
    "secp224k1",        // see ecp_supported_curves in ecp.c of mbedtls
    "secp192k1",        // see ecp_supported_curves in ecp.c of mbedtls
    "brainpoolP512r1",  // see ecp_supported_curves in ecp.c of mbedtls
    "brainpoolP384r1",  // see ecp_supported_curves in ecp.c of mbedtls
    "brainpoolP256r1",  // see ecp_supported_curves in ecp.c of mbedtls
    nullptr,            // end
};

#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
#    if !defined(CRYPTO_USE_BORINGSSL)
// Just like PACKET_peek_net_2() but with raw pointer
static inline void openssl_peek_net_2(const unsigned char *data, unsigned int &length) {
  length = ((unsigned int)(*data)) << 8;
  length |= *(data + 1);
}

// Just like WPACKET_put_bytes__() but with raw pointer and without WPACKET_allocate_bytes
static inline void openssl_set_net_2(unsigned char *data, unsigned int length) {
  *data = (unsigned char)((length >> 8) & 0xff);
  *(data + 1) = (unsigned char)(length & 0xff);
}

static inline BIGNUM *openssl_get_dh_point(const unsigned char *&input, size_t &left_size) {
  if (nullptr == input || left_size < 2) {
    return nullptr;
  }

  unsigned int point_size;
  openssl_peek_net_2(input, point_size);
  if (left_size < point_size + 2) {
    return nullptr;
  }

  BIGNUM *ret = BN_bin2bn(input + 2, point_size, nullptr);
  if (nullptr != ret) {
    input += 2 + point_size;
    left_size -= 2 + point_size;
  }

  return ret;
}

static inline bool openssl_put_dh_point(const BIGNUM *input, unsigned char *&output, size_t &left_size) {
  if (nullptr == output || nullptr == input) {
    return false;
  }

  unsigned int point_size = BN_num_bytes(input);
  if (left_size < 2 + point_size) {
    return false;
  }

  openssl_set_net_2(output, point_size);
  left_size -= 2;
  output += 2;

  BN_bn2bin(input, output);
  output += point_size;
  left_size -= point_size;
  return true;
}
#    endif

// see ec_list_element in ec_curve.c of openssl
static constexpr const int supported_dh_curves_openssl[] = {
    0,
#    ifdef NID_X25519
    NID_X25519,
#    else
    0,
#    endif
#    ifdef NID_X448
    NID_X448,
#    else
    0,
#    endif
#    ifdef NID_secp521r1
    NID_secp521r1,         // see nist_curves in ec_curve.c
    NID_secp384r1,         // see nist_curves in ec_curve.c
    NID_X9_62_prime256v1,  // see nist_curves in ec_curve.c
    NID_secp224r1,         // see nist_curves in ec_curve.c
    NID_X9_62_prime192v1,  // see nist_curves in ec_curve.c
#    else
    0,  0, 0, 0, 0,
#    endif
#    ifdef NID_secp256k1
    NID_secp256k1,  // see curve_list in ec_curve.c
    NID_secp224k1,  // see curve_list in ec_curve.c
    NID_secp192k1,  // see curve_list in ec_curve.c
#    else
    0,  0, 0,
#    endif
#    ifdef NID_brainpoolP512r1
    NID_brainpoolP512r1,  // see curve_list in ec_curve.c
    NID_brainpoolP384r1,  // see curve_list in ec_curve.c
    NID_brainpoolP256r1,  // see curve_list in ec_curve.c
#    else
    0,  0, 0,
#    endif
    -1,  // end
};

STD_STATIC_ASSERT(sizeof(supported_dh_curves) / sizeof(supported_dh_curves[0]) ==
                  sizeof(supported_dh_curves_openssl) / sizeof(supported_dh_curves_openssl[0]));

static inline void reset(EVP_PKEY *&pkey) {
  if (nullptr == pkey) {
    return;
  }

  EVP_PKEY_free(pkey);
  pkey = nullptr;
}

static inline void reset(EVP_PKEY_CTX *&ctx) {
  if (nullptr == ctx) {
    return;
  }

  EVP_PKEY_CTX_free(ctx);
  ctx = nullptr;
}

static inline void reset(BIGNUM *&bn) {
  if (nullptr == bn) {
    return;
  }

  BN_free(bn);
  bn = nullptr;
}

#    ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
static inline void reset(OSSL_DECODER_CTX *&ctx) {
  if (nullptr == ctx) {
    return;
  }

  OSSL_DECODER_CTX_free(ctx);
  ctx = nullptr;
}

static inline void reset(OSSL_PARAM_BLD *&ossl_bld) {
  if (nullptr == ossl_bld) {
    return;
  }

  OSSL_PARAM_BLD_free(ossl_bld);
  ossl_bld = nullptr;
}

static inline void reset(OSSL_PARAM *&ossl) {
  if (nullptr == ossl) {
    return;
  }

  OSSL_PARAM_free(ossl);
  ossl = nullptr;
}
#    else
static inline void reset(DH *&dh) {
  if (nullptr == dh) {
    return;
  }

  DH_free(dh);
  dh = nullptr;
}
#    endif
/*
static inline void reset(EC_KEY *&ec_key) {
    if (nullptr == ec_key) {
        return;
    }

    EC_KEY_free(ec_key);
    ec_key = nullptr;
}

static inline void reset(EC_POINT *&point) {
    if (nullptr == point) {
        return;
    }

    EC_POINT_free(point);
    point = nullptr;
}

static inline void reset(BN_CTX *&bn) {
    if (nullptr == bn) {
        return;
    }

    BN_CTX_free(bn);
    bn = nullptr;
}
*/

static inline void reset(BIO *&bio) {
  if (nullptr == bio) {
    return;
  }

  BIO_free(bio);
  bio = nullptr;
}

template <class TPTR>
class openssl_raii {
  UTIL_DESIGN_PATTERN_NOMOVABLE(openssl_raii)
  UTIL_DESIGN_PATTERN_NOCOPYABLE(openssl_raii)

 public:
  inline openssl_raii(TPTR *in) : data_(in) {}
  inline ~openssl_raii() { reset(); }

  inline void reset() { ::util::crypto::details::reset(data_); }

  inline operator bool() const { return !!data_; }

  inline const TPTR *operator->() const noexcept { return data_; }
  inline TPTR *operator->() noexcept { return data_; }

  inline const TPTR *get() const noexcept { return data_; }
  inline TPTR *get() noexcept { return data_; }

  inline const TPTR *&ref() const noexcept { return data_; }
  inline TPTR *&ref() noexcept { return data_; }

 private:
  TPTR *data_;
};

static EVP_PKEY_CTX *initialize_pkey_ctx_by_group_id(int group_id, bool init_keygen, bool init_paramgen) {
  EVP_PKEY_CTX *ret = nullptr;
  unsigned int gtype = 0;
  int curve_nid = tls1_ec_group_id2nid(group_id, &gtype);
  if (TLS_CURVE_CUSTOM == gtype) {
    ret = EVP_PKEY_CTX_new_id(curve_nid, nullptr);
  } else {
    ret = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr);
  }

  if (nullptr == ret) {
    return nullptr;
  }

  if (init_keygen && EVP_PKEY_keygen_init(ret) <= 0) {
    reset(ret);
    return ret;
  }

  if (init_paramgen) {
    if (EVP_PKEY_paramgen_init(ret) <= 0 &&
        EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE != ERR_GET_REASON(ERR_peek_error())) {
      reset(ret);
      return ret;
    }
  }

  if (TLS_CURVE_CUSTOM != gtype && init_paramgen && EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ret, curve_nid) <= 0) {
    reset(ret);
  }

  return ret;
}

static EVP_PKEY_CTX *initialize_pkey_ctx_by_pkey(EVP_PKEY *params_key, bool init_keygen, bool init_paramgen) {
  if (nullptr == params_key) {
    return nullptr;
  }

  EVP_PKEY_CTX *ret = EVP_PKEY_CTX_new(params_key, nullptr);
  if (nullptr == ret) {
    return nullptr;
  }

  if (init_keygen && EVP_PKEY_keygen_init(ret) <= 0) {
    reset(ret);
    return ret;
  }

  if (init_paramgen && EVP_PKEY_paramgen_init(ret) <= 0) {
    reset(ret);
    return ret;
  }

  return ret;
}
#  endif
}  // namespace details

// =============== shared context ===============
LIBATFRAME_UTILS_API dh::shared_context::shared_context() : flags_(flags_t::NONE), method_(method_t::EN_CDT_INVALID) {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  dh_param_.param = nullptr;
  dh_param_.group_id = 0;
  dh_param_.keygen_ctx = nullptr;
#  elif defined(CRYPTO_USE_MBEDTLS)
  dh_param_.group_id = MBEDTLS_ECP_DP_NONE;
#  endif

#  if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#    if (defined(__cplusplus) && __cplusplus >= 201402L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L))
  UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<random_engine_t>::value);
#    elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
  UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<random_engine_t>::value);
#    else
  UTIL_CONFIG_STATIC_ASSERT(std::is_pod<random_engine_t>::value);
#    endif
#  endif

  memset(&random_engine_, 0, sizeof(random_engine_));
}
LIBATFRAME_UTILS_API dh::shared_context::shared_context(creator_helper &)
    : flags_(flags_t::NONE), method_(method_t::EN_CDT_INVALID) {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  dh_param_.param = nullptr;
  dh_param_.group_id = 0;
  dh_param_.keygen_ctx = nullptr;
#  elif defined(CRYPTO_USE_MBEDTLS)
  dh_param_.group_id = MBEDTLS_ECP_DP_NONE;
#  endif

#  if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#    if (defined(__cplusplus) && __cplusplus >= 201402L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L))
  UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<random_engine_t>::value);
#    elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
  UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<random_engine_t>::value);
#    else
  UTIL_CONFIG_STATIC_ASSERT(std::is_pod<random_engine_t>::value);
#    endif
#  endif
  memset(&random_engine_, 0, sizeof(random_engine_));
}
LIBATFRAME_UTILS_API dh::shared_context::~shared_context() { reset(); }

LIBATFRAME_UTILS_API dh::shared_context::ptr_t dh::shared_context::create() {
  creator_helper h;
  return std::make_shared<dh::shared_context>(h);
}

LIBATFRAME_UTILS_API int dh::shared_context::init(const char *name) {
  if (nullptr == name) {
    return error_code_t::INVALID_PARAM;
  }

  int ecp_idx = 1;
  method_t::type method = method_t::EN_CDT_DH;
  if (0 == UTIL_STRFUNC_STRNCASE_CMP("ecdh:", name, 5)) {
    method = method_t::EN_CDT_ECDH;

    while (nullptr != details::supported_dh_curves[ecp_idx]) {
      if (0 == UTIL_STRFUNC_STRCASE_CMP(name + 5, details::supported_dh_curves[ecp_idx])) {
        break;
      }
      ++ecp_idx;
    }

    if (nullptr == details::supported_dh_curves[ecp_idx]) {
      return error_code_t::NOT_SUPPORT;
    }

// check if it's available
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
    if (0 == details::supported_dh_curves_openssl[ecp_idx]) {
      return error_code_t::NOT_SUPPORT;
    }
#  endif
  }

  int ret = init(method);
  // init failed
  if (ret < 0) {
    return ret;
  }

  switch (method) {
#  if !defined(CRYPTO_USE_BORINGSSL)
    case method_t::EN_CDT_DH: {
      // do nothing in client mode
      FILE *pem = nullptr;
      UTIL_FS_OPEN(pem_file_e, pem, name, "r");
      COMPILER_UNUSED(pem_file_e);
      if (nullptr == pem) {
        ret = error_code_t::READ_DHPARAM_FILE;
        break;
      }
      fseek(pem, 0, SEEK_END);
      size_t pem_sz = 0;
      {
        long pem_sz_l = ftell(pem);
        if (pem_sz_l >= 0) {
          pem_sz = static_cast<size_t>(pem_sz_l);
        }
      }
      fseek(pem, 0, SEEK_SET);

// Read from pem file
#    if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL)
      do {
        dh_param_.param_buffer.resize(pem_sz);
        if (0 == fread(&dh_param_.param_buffer[0], sizeof(unsigned char), pem_sz, pem)) {
          ret = error_code_t::READ_DHPARAM_FILE;
          break;
        }
        details::reset(dh_param_.param);
        dh_param_.param = BIO_new_mem_buf(&dh_param_.param_buffer[0], static_cast<int>(pem_sz));

        details::openssl_raii<EVP_PKEY> params_key{EVP_PKEY_new()};
        if (nullptr == params_key.get()) {
          ret = error_code_t::MALLOC;
          break;
        }

#      ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
        // Just like codes in <openssl-3.0.0>/apps/dhparam.c
        details::openssl_raii<OSSL_DECODER_CTX> test_decoder_ctx{OSSL_DECODER_CTX_new_for_pkey(
            &params_key.ref(), nullptr, nullptr, "DH", OSSL_KEYMGMT_SELECT_DOMAIN_PARAMETERS, nullptr, nullptr)};
        if (!test_decoder_ctx) {
          ret = error_code_t::READ_DHPARAM_FILE;
          break;
        }
        if (!OSSL_DECODER_from_bio(test_decoder_ctx.get(), dh_param_.param)) {
          ret = error_code_t::READ_DHPARAM_FILE;
          break;
        }
#      else
        // check
        details::openssl_raii<DH> test_dh_ctx(PEM_read_bio_DHparams(dh_param_.param, nullptr, nullptr, nullptr));
        if (!test_dh_ctx) {
          ret = error_code_t::READ_DHPARAM_FILE;
          break;
        }

        // Maybe use EVP_PKEY_assign and DH_up_ref instead of EVP_PKEY_set1_DH, there is a BUG on openssl 1.1.1 and
        // fixed in 1.1.1f
        // @see https://github.com/openssl/openssl/issues/10592
        if (1 != EVP_PKEY_set1_DH(params_key.get(), test_dh_ctx.get())) {
          ret = error_code_t::NOT_SUPPORT;
          break;
        }
#      endif

        details::openssl_raii<EVP_PKEY_CTX> paramgen_ctx{
            details::initialize_pkey_ctx_by_pkey(params_key.get(), false, true)};
        if (nullptr == paramgen_ctx.get()) {
          ret = error_code_t::NOT_SUPPORT;
          break;
        }

#      if (defined(OPENSSL_API_COMPAT) && OPENSSL_API_COMPAT >= 0x10101000L) ||    \
          (defined(OPENSSL_API_LEVEL) && OPENSSL_API_LEVEL >= 10101) ||            \
          (!defined(LIBRESSL_VERSION_NUMBER) && defined(OPENSSL_VERSION_NUMBER) && \
           OPENSSL_VERSION_NUMBER >= 0x10101000L)
        if (1 != EVP_PKEY_param_check(paramgen_ctx.get())) {
          ret = error_code_t::NOT_SUPPORT;
          break;
        }
#      endif

        details::reset(dh_param_.keygen_ctx);
        dh_param_.keygen_ctx = details::initialize_pkey_ctx_by_pkey(params_key.get(), true, false);
      } while (false);

      if (error_code_t::OK != ret) {
        details::reset(dh_param_.param);
        dh_param_.param_buffer.clear();
      }

#    elif defined(CRYPTO_USE_MBEDTLS)
      // mbedtls_dhm_read_params must has last character to be zero, so add one zero to the end
      dh_param_.param.resize(pem_sz * sizeof(unsigned char) + 1, 0);
      do {
        if (0 == fread(&dh_param_.param[0], sizeof(unsigned char), pem_sz, pem)) {
          ret = error_code_t::READ_DHPARAM_FILE;
          break;
        }

        // test
        mbedtls_dhm_context test_dh_ctx;
        mbedtls_dhm_init(&test_dh_ctx);
        if (0 != mbedtls_dhm_parse_dhm(&test_dh_ctx, reinterpret_cast<const unsigned char *>(dh_param_.param.data()),
                                       pem_sz + 1)) {
          ret = error_code_t::INIT_DHPARAM;
        } else {
          mbedtls_dhm_free(&test_dh_ctx);
        }

        if (error_code_t::OK != ret) {
          dh_param_.param.clear();
        }
      } while (false);
#    endif

      UTIL_FS_CLOSE(pem);
      break;
    }
#  endif
    case method_t::EN_CDT_ECDH: {
// check if it's available
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      // https://github.com/prithuadhikary/OPENSSL_EVP_ECDH_EXAMPLE/blob/master/main.c
      dh_param_.group_id = tls1_nid2group_id(details::supported_dh_curves_openssl[ecp_idx]);
      details::openssl_raii<EVP_PKEY_CTX> paramgen_ctx{
          details::initialize_pkey_ctx_by_group_id(dh_param_.group_id, false, true)};
      if (nullptr == paramgen_ctx.get()) {
        dh_param_.group_id = 0;
        ret = error_code_t::NOT_SUPPORT;
        break;
      }

      do {
        details::openssl_raii<EVP_PKEY> params_key{nullptr};
        EVP_PKEY_paramgen(paramgen_ctx.get(), &params_key.ref());
        // openssl 1.1.1 will report EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE for x25519 and x448
        if (nullptr == params_key.get() &&
            EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE != ERR_GET_REASON(ERR_peek_error())) {
          break;
        }
        details::reset(dh_param_.keygen_ctx);
        if (nullptr == paramgen_ctx.get()) {
          dh_param_.keygen_ctx = details::initialize_pkey_ctx_by_group_id(dh_param_.group_id, true, false);
        } else {
          dh_param_.keygen_ctx = details::initialize_pkey_ctx_by_pkey(params_key.get(), true, false);
        }
      } while (false);
#  elif defined(CRYPTO_USE_MBEDTLS)
      const mbedtls_ecp_curve_info *curve = mbedtls_ecp_curve_info_from_name(details::supported_dh_curves[ecp_idx]);
      if (nullptr == curve) {
        ret = error_code_t::NOT_SUPPORT;
        break;
      }
      dh_param_.group_id = curve->grp_id;
#  endif
      break;
    }
    default: {
      ret = error_code_t::NOT_SUPPORT;
      break;
    }
  }

  if (ret < 0) {
    reset();
  }

  flags_ |= flags_t::SERVER_MODE;
  flags_ &= ~static_cast<uint32_t>(flags_t::CLIENT_MODE);
  return ret;
}

LIBATFRAME_UTILS_API int dh::shared_context::init(method_t::type method) {
  if (method_t::EN_CDT_INVALID != method_) {
    return error_code_t::ALREADY_INITED;
  }

  if (method_t::EN_CDT_INVALID == method) {
    return error_code_t::INVALID_PARAM;
  }

// random engine
#  if defined(CRYPTO_USE_BORINGSSL)
  if (method_t::EN_CDT_DH == method) {
    return error_code_t::NOT_SUPPORT;
  }
#  elif defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL)
#  elif defined(CRYPTO_USE_MBEDTLS)
  mbedtls_ctr_drbg_init(&random_engine_.ctr_drbg);
  mbedtls_entropy_init(&random_engine_.entropy);

  int res = mbedtls_ctr_drbg_seed(&random_engine_.ctr_drbg, mbedtls_entropy_func, &random_engine_.entropy, nullptr, 0);
  if (0 != res) {
    // clear DH or ECDH data
    dh_param_.param.clear();
    return error_code_t::INIT_RANDOM_ENGINE;
  }
#  endif
  method_ = method;

  flags_ |= flags_t::CLIENT_MODE;
  return error_code_t::OK;
}

LIBATFRAME_UTILS_API void dh::shared_context::reset() {
  flags_ = flags_t::NONE;
  if (method_t::EN_CDT_INVALID == method_) {
    return;
  }

  switch (method_) {
    case method_t::EN_CDT_DH:
    case method_t::EN_CDT_ECDH: {
// clear pem file buffer
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      // clear dh pem buffer
      if (nullptr != dh_param_.param) {
        details::reset(dh_param_.param);
        dh_param_.param_buffer.clear();
      }
      // clear ecp
      dh_param_.group_id = 0;
#  elif defined(CRYPTO_USE_MBEDTLS)
      // clear dh pem buffer
      dh_param_.param.clear();
      // clear ecp
      dh_param_.group_id = MBEDTLS_ECP_DP_NONE;
#  endif
      break;
    }
    default: {
      // do nothing
      break;
    }
  }

  method_ = method_t::EN_CDT_INVALID;
// random engine
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  details::reset(dh_param_.param);
  details::reset(dh_param_.keygen_ctx);
#  elif defined(CRYPTO_USE_MBEDTLS)
  mbedtls_ctr_drbg_free(&random_engine_.ctr_drbg);
  mbedtls_entropy_free(&random_engine_.entropy);
#  endif
}

LIBATFRAME_UTILS_API int dh::shared_context::random(void *output, size_t output_sz) {
  if (method_t::EN_CDT_INVALID == method_) {
    return error_code_t::NOT_INITED;
  }

  if (nullptr == output || output_sz <= 0) {
    return error_code_t::INVALID_PARAM;
  }

  int ret = error_code_t::OK;
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  if (!RAND_bytes(reinterpret_cast<unsigned char *>(output), static_cast<int>(output_sz))) {
    ret = static_cast<int>(ERR_peek_error());
  }

#  elif defined(LIBATFRAME_ATGATEWAY_ENABLE_MBEDTLS)
  ret = mbedtls_ctr_drbg_random(&random_engine_.ctr_drbg, reinterpret_cast<unsigned char *>(output), output_sz);
#  endif
  return ret;
}

LIBATFRAME_UTILS_API bool dh::shared_context::is_client_mode() const { return 0 != (flags_ & flags_t::CLIENT_MODE); }

LIBATFRAME_UTILS_API dh::method_t::type dh::shared_context::get_method() const { return method_; }
LIBATFRAME_UTILS_API const dh::shared_context::dh_param_t &dh::shared_context::get_dh_parameter() const {
  return dh_param_;
}
LIBATFRAME_UTILS_API const dh::shared_context::random_engine_t &dh::shared_context::get_random_engine() const {
  return random_engine_;
}
LIBATFRAME_UTILS_API dh::shared_context::random_engine_t &dh::shared_context::get_random_engine() {
  return random_engine_;
}

#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
LIBATFRAME_UTILS_API int dh::shared_context::try_reset_ecp_id(int group_id) {
  if (0 != dh_param_.group_id && dh_param_.group_id != group_id) {
    return error_code_t::ALGORITHM_MISMATCH;
  }

  dh_param_.group_id = group_id;
  details::openssl_raii<EVP_PKEY_CTX> paramgen_ctx{
      details::initialize_pkey_ctx_by_group_id(dh_param_.group_id, false, true)};
  if (nullptr == paramgen_ctx.get()) {
    dh_param_.group_id = 0;
    return error_code_t::NOT_SUPPORT;
  }

  details::openssl_raii<EVP_PKEY> params_key{nullptr};
  EVP_PKEY_paramgen(paramgen_ctx.get(), &params_key.ref());
  // openssl 1.1.1 will report EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE for x25519 and x448
  if (nullptr == params_key.get() &&
      EVP_R_OPERATION_NOT_SUPPORTED_FOR_THIS_KEYTYPE != ERR_GET_REASON(ERR_peek_error())) {
    return error_code_t::MALLOC;
  }
  details::reset(dh_param_.keygen_ctx);
  if (nullptr == params_key.get()) {
    dh_param_.keygen_ctx = details::initialize_pkey_ctx_by_group_id(dh_param_.group_id, true, false);
  } else {
    dh_param_.keygen_ctx = details::initialize_pkey_ctx_by_pkey(params_key.get(), true, false);
  }

  return error_code_t::OK;
}

#    if !defined(CRYPTO_USE_BORINGSSL)
LIBATFRAME_UTILS_API int dh::shared_context::try_reset_dh_params(BIGNUM *&DH_p, BIGNUM *&DH_g) {
  if (nullptr == DH_p || nullptr == DH_g) {
    return error_code_t::INVALID_PARAM;
  }
  int ret = error_code_t::OK;
#      ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
  details::openssl_raii<EVP_PKEY> params_key{nullptr};

  details::openssl_raii<OSSL_PARAM_BLD> ossl_param_bld{OSSL_PARAM_BLD_new()};
  if (ossl_param_bld.get() == nullptr || !OSSL_PARAM_BLD_push_BN(ossl_param_bld.get(), OSSL_PKEY_PARAM_FFC_P, DH_p) ||
      !OSSL_PARAM_BLD_push_BN(ossl_param_bld.get(), OSSL_PKEY_PARAM_FFC_G, DH_g)) {
    return error_code_t::INIT_DH_READ_PARAM;
  }
  details::openssl_raii<OSSL_PARAM> ossl_params{OSSL_PARAM_BLD_to_param(ossl_param_bld.get())};
  if (ossl_params.get() == nullptr) {
    return error_code_t::INIT_DH_READ_KEY;
  }

  details::openssl_raii<EVP_PKEY_CTX> paramgen_ctx{EVP_PKEY_CTX_new_from_name(nullptr, "DH", nullptr)};
  if (nullptr == paramgen_ctx.get() || EVP_PKEY_fromdata_init(paramgen_ctx.get()) <= 0 ||
      EVP_PKEY_fromdata(paramgen_ctx.get(), &params_key.ref(), EVP_PKEY_KEYPAIR, ossl_params.get()) <= 0) {
    return error_code_t::INIT_DH_READ_KEY;
  }

  details::reset(dh_param_.keygen_ctx);
  dh_param_.keygen_ctx = details::initialize_pkey_ctx_by_pkey(params_key.get(), true, false);
  if (dh_param_.keygen_ctx == nullptr || EVP_PKEY_param_check_quick(dh_param_.keygen_ctx) != 1) {
    return error_code_t::INIT_DH_READ_PARAM;
  }
#      else
  details::openssl_raii<EVP_PKEY> params_key{EVP_PKEY_new()};
  if (nullptr == params_key.get()) {
    return error_code_t::MALLOC;
  }

  DH *dh = DH_new();
  if (nullptr == dh) {
    return error_code_t::MALLOC;
  }
  if (!DH_set0_pqg(dh, DH_p, nullptr, DH_g)) {
    details::reset(dh);
    return error_code_t::INIT_DH_READ_PARAM;
  }
  // Move into DH object after call DH_set0_pqg successfully.
  DH_p = nullptr;
  DH_g = nullptr;

  // Maybe use EVP_PKEY_assign and DH_up_ref instead of EVP_PKEY_set1_DH, there is a BUG on openssl 1.1.1 and fixed
  // in 1.1.1f
  // @see https://github.com/openssl/openssl/issues/10592
  if (1 != EVP_PKEY_set1_DH(params_key.get(), dh)) {
    details::reset(dh);
    return error_code_t::OPERATION;
  }
  details::reset(dh);
  details::reset(dh_param_.keygen_ctx);
  dh_param_.keygen_ctx = details::initialize_pkey_ctx_by_pkey(params_key.get(), true, false);
  if (dh_param_.keygen_ctx == nullptr) {
    return error_code_t::INIT_DH_READ_PARAM;
  }
#      endif
  return ret;
}
#    endif
#  endif

// --------------- shared context ---------------

LIBATFRAME_UTILS_API dh::dh() : last_errorno_(0) {
  memset(&dh_context_, 0, sizeof(dh_context_));
#  if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#    if (defined(__cplusplus) && __cplusplus >= 201402L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L))
  UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<dh_context_t>::value);
#    elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
  UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<dh_context_t>::value);
#    else
  UTIL_CONFIG_STATIC_ASSERT(std::is_pod<dh_context_t>::value);
#    endif
#  endif
}
LIBATFRAME_UTILS_API dh::~dh() { close(); }

LIBATFRAME_UTILS_API int dh::init(shared_context::ptr_t shared_context) {
  if (!shared_context) {
    return details::setup_errorno(*this, 0, error_code_t::INVALID_PARAM);
  }

  // shared_context must be initialized
  if (method_t::EN_CDT_INVALID == shared_context->get_method()) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }

  int ret = 0;

  switch (shared_context->get_method()) {
#  if !defined(CRYPTO_USE_BORINGSSL)
    case method_t::EN_CDT_DH: {
// init DH param file
#    if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL)
      if (false == shared_context->is_client_mode()) {
        if (nullptr == shared_context->get_dh_parameter().keygen_ctx) {
          ret = error_code_t::NOT_SERVER_MODE;
          break;
        }
      }
#    elif defined(CRYPTO_USE_MBEDTLS)
      // mbedtls_dhm_read_params
      do {
        mbedtls_dhm_init(&dh_context_.mbedtls_dh_ctx_);

        // client mode, just init , do not read PEM file
        if (false == shared_context->is_client_mode()) {
          int res = mbedtls_dhm_parse_dhm(
              &dh_context_.mbedtls_dh_ctx_,
              reinterpret_cast<const unsigned char *>(shared_context->get_dh_parameter().param.data()),
              shared_context->get_dh_parameter().param.size());
          if (0 != res) {
            ret = details::setup_errorno(*this, res, error_code_t::INIT_DHPARAM);
            break;
          }
        }
      } while (false);

      if (0 != ret) {
        mbedtls_dhm_free(&dh_context_.mbedtls_dh_ctx_);
      }
#    endif
      break;
    }
#  endif
    case method_t::EN_CDT_ECDH: {
// init DH param file
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      if (false == shared_context->is_client_mode()) {
        if (nullptr == shared_context->get_dh_parameter().keygen_ctx) {
          ret = error_code_t::NOT_SERVER_MODE;
          break;
        }
      }
#  elif defined(CRYPTO_USE_MBEDTLS)
      // mbedtls_dhm_read_params
      do {
        mbedtls_ecdh_init(&dh_context_.mbedtls_ecdh_ctx_);

        if (false == shared_context->is_client_mode()) {
          int res = mbedtls_ecdh_setup(&dh_context_.mbedtls_ecdh_ctx_, shared_context->get_dh_parameter().group_id);
          if (0 != res) {
            ret = details::setup_errorno(*this, res, error_code_t::INIT_DHPARAM);
            break;
          }
        }
      } while (false);

      if (0 != ret) {
        mbedtls_ecdh_free(&dh_context_.mbedtls_ecdh_ctx_);
      }
#  endif
      break;
    }
    default: {
      details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
    }
  }

  if (0 != ret) {
    return ret;
  }

  shared_context_ = shared_context;
  return details::setup_errorno(*this, 0, error_code_t::OK);
}

LIBATFRAME_UTILS_API int dh::close() {
  if (!shared_context_) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }

  shared_context::ptr_t shared_context;
  shared_context.swap(shared_context_);

  switch (shared_context->get_method()) {
    case method_t::EN_CDT_DH: {
// clear DH param file and cache
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      details::reset(dh_context_.openssl_dh_peer_key_);
      details::reset(dh_context_.openssl_dh_pkey_);
#  elif defined(CRYPTO_USE_MBEDTLS)
      mbedtls_dhm_free(&dh_context_.mbedtls_dh_ctx_);
#  endif
      break;
    }
    case method_t::EN_CDT_ECDH: {
// clear ecdh key and cache
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      details::reset(dh_context_.openssl_ecdh_peer_key_);
      details::reset(dh_context_.openssl_ecdh_pkey_);
#  elif defined(CRYPTO_USE_MBEDTLS)
      mbedtls_ecdh_free(&dh_context_.mbedtls_ecdh_ctx_);
#  endif
      break;
    }
    default: {
      details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
    }
  }

#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  details::reset(dh_context_.openssl_pkey_ctx_);
#  endif

  return details::setup_errorno(*this, 0, error_code_t::OK);
}

LIBATFRAME_UTILS_API void dh::set_last_errno(int e) { last_errorno_ = e; }

LIBATFRAME_UTILS_API int dh::get_last_errno() const { return last_errorno_; }

LIBATFRAME_UTILS_API int dh::make_params(std::vector<unsigned char> &param) {
  if (!shared_context_) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }

  int ret = details::setup_errorno(*this, 0, error_code_t::OK);
  switch (shared_context_->get_method()) {
#  if !defined(CRYPTO_USE_BORINGSSL)
    case method_t::EN_CDT_DH: {
#    if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL)
      do {
        if (nullptr == shared_context_->get_dh_parameter().keygen_ctx) {
          ret = details::setup_errorno(*this, 0, error_code_t::INIT_DHPARAM);
          break;
        }

        details::reset(dh_context_.openssl_dh_pkey_);
        if (EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_dh_pkey_) <= 0) {
          ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DHPARAM);
        }

        if (nullptr == dh_context_.openssl_dh_pkey_) {
          ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DHPARAM);
          break;
        }
      } while (false);

      if (0 != ret) {
        details::reset(dh_context_.openssl_dh_pkey_);
        break;
      }

      if (nullptr == dh_context_.openssl_dh_pkey_) {
        ret = details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
        break;
      }

      // write big number into buffer, the size must be no less than BN_num_bytes()
      // @see https://www.openssl.org/docs/manmaster/crypto/BN_bn2bin.html
      // dump P,G,GX
      // @see int ssl3_send_server_key_exchange(SSL *s) in s3_srvr.c          -- openssl 1.0.x
      // @see int tls_construct_server_key_exchange(SSL *s) in statem_srvr.c  -- openssl 1.1.x/3.0.0
#      ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
      BIGNUM *r[4] = {nullptr, nullptr, nullptr, nullptr};
      if (!EVP_PKEY_get_bn_param(dh_context_.openssl_dh_pkey_, OSSL_PKEY_PARAM_FFC_P, &r[0]) ||
          !EVP_PKEY_get_bn_param(dh_context_.openssl_dh_pkey_, OSSL_PKEY_PARAM_FFC_G, &r[1]) ||
          !EVP_PKEY_get_bn_param(dh_context_.openssl_dh_pkey_, OSSL_PKEY_PARAM_PUB_KEY, &r[2])) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        for (auto &bn : r) {
          details::reset(bn);
        }
        break;
      }
#      else
      const BIGNUM *r[4] = {nullptr, nullptr, nullptr, nullptr};

      DH *dh = EVP_PKEY_get0_DH(dh_context_.openssl_dh_pkey_);
      if (nullptr == dh) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }

      const BIGNUM *self_pubkey = nullptr;
      DH_get0_key(dh, &self_pubkey, nullptr);
      if (nullptr == self_pubkey) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }

      DH_get0_pqg(dh, &r[0], nullptr, &r[1]);
      DH_get0_key(dh, &r[2], nullptr);
#      endif
      // puts("make_params");
      // BN_print_fp(stdout, r[0]);
      // BN_print_fp(stdout, r[1]);
      // puts("r2");
      // BN_print_fp(stdout, r[2]);
      // puts("end");
      {
        size_t olen = 0;
        unsigned int nr[4] = {0};
        for (int i = 0; i < 4 && r[i] != nullptr; i++) {
          nr[i] = BN_num_bytes(r[i]);
          // DHM_MPI_EXPORT in mbedtls/polarssl use 2 byte to store length, so openssl/libressl/boringssl should use
          // OPENSSL_NO_SRP
          olen += static_cast<size_t>(nr[i] + 2);
        }

        param.resize(olen, 0);
        unsigned char *p = &param[0];
        for (int i = 0; i < 4 && r[i] != nullptr; i++) {
          details::openssl_put_dh_point(r[i], p, olen);
        }
      }
#      ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
      for (auto &bn : r) {
        details::reset(bn);
      }
#      endif

#    elif defined(CRYPTO_USE_MBEDTLS)
      // size is P,G,GX
      size_t psz = mbedtls_mpi_size(&dh_context_.mbedtls_dh_ctx_.P);
      size_t gsz = mbedtls_mpi_size(&dh_context_.mbedtls_dh_ctx_.G);
      size_t olen = 0;
      // @see mbedtls_dhm_make_params, output P,G,GX. GX is smaller than P
      // each big number has 2 byte length
      param.resize(psz + psz + gsz + 6, 0);
      int res = mbedtls_dhm_make_params(&dh_context_.mbedtls_dh_ctx_, static_cast<int>(psz),
                                        reinterpret_cast<unsigned char *>(&param[0]), &olen, mbedtls_ctr_drbg_random,
                                        &shared_context_->get_random_engine().ctr_drbg);
      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }

      // resize if P,G,GX is small than param
      assert(olen <= psz + psz + gsz + 6);
      if (olen < param.size()) {
        param.resize(olen);
      }
#    endif
      break;
    }
#  endif
    case method_t::EN_CDT_ECDH: {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      do {
        if (nullptr == shared_context_->get_dh_parameter().keygen_ctx) {
          ret = details::setup_errorno(*this, 0, error_code_t::INIT_DHPARAM);
          break;
        }

        details::reset(dh_context_.openssl_ecdh_pkey_);
        if (EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_ecdh_pkey_) <= 0) {
          ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DHPARAM);
        }

        if (nullptr == dh_context_.openssl_ecdh_pkey_) {
          ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DHPARAM);
          break;
        }
      } while (false);

      if (0 != ret) {
        details::reset(dh_context_.openssl_ecdh_pkey_);
        break;
      }

      if (nullptr == dh_context_.openssl_ecdh_pkey_) {
        ret = details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
        break;
      }

      // pub key:
      //   EVP_PKEY_get1_tls_encodedpoint()
      //   EVP_PKEY_set1_tls_encodedpoint()
      // param:
      //   EVP_PKEY_id()
      //   pkey_set_type()
      //   EVP_PKEY_print_params()
      //   EVP_PKEY_print_params()
      unsigned char *point_data = nullptr;
      size_t encode_len = crypto_dh_EVP_PKEY_get1_tls_encodedpoint(dh_context_.openssl_ecdh_pkey_, &point_data);
      if (nullptr == point_data) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }
      int group_id = shared_context_->get_dh_parameter().group_id;
      // {
      //     int type_id = EVP_PKEY_id(dh_context_.openssl_ecdh_pkey_);
      //     if (EVP_PKEY_EC == type_id) {
      //         group_id = tls1_nid2group_id(...);
      //     } else {
      //         group_id = tls1_nid2group_id(type_id);
      //     }
      // }
      param.resize(encode_len + 4, 0);
      memcpy(&param[4], point_data, encode_len);
      OPENSSL_free(point_data);

      // Write data
      param[0] = NAMED_CURVE_TYPE;
      param[1] = static_cast<unsigned char>(group_id >> 8);
      param[2] = static_cast<unsigned char>(group_id & 0xFF);
      param[3] = static_cast<unsigned char>(encode_len);

#  elif defined(CRYPTO_USE_MBEDTLS)
      unsigned char buf[CRYPTO_DH_MAX_KEY_LEN];
      // size is ecp group(3byte) + point(unknown size)
      size_t olen = 0;
      // @see mbedtls_ecdh_make_params, output group and point
      int res = mbedtls_ecdh_make_params(&dh_context_.mbedtls_ecdh_ctx_, &olen, buf, sizeof(buf),
                                         mbedtls_ctr_drbg_random, &shared_context_->get_random_engine().ctr_drbg);
      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }
      param.assign(buf, buf + olen);
#  endif
      break;
    }
    default: {
      details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
    }
  }

  return ret;
}

LIBATFRAME_UTILS_API int dh::read_params(const unsigned char *input, size_t ilen) {
  if (!shared_context_) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }

  if (nullptr == input || ilen == 0) {
    return details::setup_errorno(*this, 0, error_code_t::INVALID_PARAM);
  }

  int ret = details::setup_errorno(*this, 0, error_code_t::OK);
  switch (shared_context_->get_method()) {
#  if !defined(CRYPTO_USE_BORINGSSL)
    case method_t::EN_CDT_DH: {
#    if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL)
      // @see int ssl3_get_key_exchange(SSL *s) in s3_clnt.c                                          -- openssl 1.0.x
      // @see int tls_process_ske_dhe(SSL *s, PACKET *pkt, EVP_PKEY **pkey, int *al) in statem_clnt.c --
      // openssl 1.1.x/3.x.x
      details::openssl_raii<BIGNUM> DH_p{details::openssl_get_dh_point(input, ilen)};
      details::openssl_raii<BIGNUM> DH_g{details::openssl_get_dh_point(input, ilen)};
      details::openssl_raii<BIGNUM> DH_gy{details::openssl_get_dh_point(input, ilen)};
      if (nullptr == DH_p.get() || nullptr == DH_g.get()) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_READ_PARAM);
        break;
      }
      if (nullptr == DH_gy.get()) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_READ_KEY);
        break;
      }
      ret = check_or_setup_dh_pg_gy(DH_p.ref(), DH_g.ref(), DH_gy.ref());
#    elif defined(CRYPTO_USE_MBEDTLS)
      unsigned char *dh_params_beg = const_cast<unsigned char *>(input);
      int res = mbedtls_dhm_read_params(&dh_context_.mbedtls_dh_ctx_, &dh_params_beg, dh_params_beg + ilen);
      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_READ_PARAM);
        break;
      }
#    endif
      break;
    }
#  endif
    case method_t::EN_CDT_ECDH: {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      /*
       * XXX: For now, we only support named (not generic) curves. In
       * this situation, the serverKeyExchange message has: [1 byte
       * CurveType], [2 byte CurveName] [1 byte length of encoded
       * point], followed by the actual encoded point itself
       */
      size_t curve_grp_len = 4;
      if (ilen < curve_grp_len) {
        ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
        break;
      }

      size_t encoded_pt_len = input[3];
      if (encoded_pt_len > ilen - curve_grp_len) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_READ_PARAM);
        break;
      }

      int group_id = static_cast<int>(input[1] << 8) | static_cast<int>(input[2]);
      ret = check_or_setup_ecp_id(group_id);
      if (error_code_t::OK != ret) {
        ret = details::setup_errorno(*this, 0, static_cast<error_code_t::type>(ret));
        break;
      }

      if (nullptr == shared_context_->get_dh_parameter().keygen_ctx) {
        ret = details::setup_errorno(*this, 0, error_code_t::NOT_CLIENT_MODE);
        break;
      }

      details::reset(dh_context_.openssl_ecdh_pkey_);
      if (EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_ecdh_pkey_) <= 0) {
        details::reset(dh_context_.openssl_ecdh_pkey_);
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }
      if (nullptr == dh_context_.openssl_ecdh_pkey_) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }

      if (nullptr == dh_context_.openssl_ecdh_peer_key_) {
        EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_ecdh_peer_key_);
      }
      if (nullptr == dh_context_.openssl_ecdh_peer_key_) {
        ret = details::setup_errorno(*this, 0, error_code_t::MALLOC);
        break;
      }

      // int type_id = EVP_PKEY_id(dh_context_.openssl_ecdh_pkey_);
      //     Still missing nid information if type_id == EVP_PKEY_EC
      // if (EVP_PKEY_set_type(dh_context_.openssl_ecdh_peer_key_, type_id) <= 0) {
      //     ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::NOT_SUPPORT);
      //     details::reset(dh_context_.openssl_ecdh_peer_key_);
      //     break;
      // }

      if (crypto_dh_EVP_PKEY_set1_tls_encodedpoint(dh_context_.openssl_ecdh_peer_key_, &input[curve_grp_len],
                                                   encoded_pt_len) <= 0) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::NOT_SUPPORT);
        details::reset(dh_context_.openssl_ecdh_peer_key_);
        break;
      }

#  elif defined(CRYPTO_USE_MBEDTLS)
      const unsigned char *dh_params_beg = input;
      int res = mbedtls_ecdh_read_params(&dh_context_.mbedtls_ecdh_ctx_, &dh_params_beg, dh_params_beg + ilen);
      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_READ_PARAM);
        break;
      }
#  endif
      break;
    }
    default: {
      details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
    }
  }

  return ret;
}  // namespace crypto

LIBATFRAME_UTILS_API int dh::make_public(std::vector<unsigned char> &param) {
  if (!shared_context_) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }

  int ret = details::setup_errorno(*this, 0, error_code_t::OK);
  switch (shared_context_->get_method()) {
#  if !defined(CRYPTO_USE_BORINGSSL)
    case method_t::EN_CDT_DH: {
#    if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL)
      if (nullptr == dh_context_.openssl_dh_pkey_) {
        ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
        break;
      }

#      ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
      BIGNUM *self_pubkey = nullptr;
      if (!EVP_PKEY_get_bn_param(dh_context_.openssl_dh_pkey_, OSSL_PKEY_PARAM_PUB_KEY, &self_pubkey)) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        details::reset(self_pubkey);
        break;
      }
#      else
      DH *dh = EVP_PKEY_get0_DH(dh_context_.openssl_dh_pkey_);
      if (nullptr == dh) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_READ_PARAM);
        break;
      }

      int errcode = 0;
      const BIGNUM *self_pubkey = nullptr;
      DH_get0_key(dh, &self_pubkey, nullptr);
      if (nullptr == self_pubkey) {
        ret = details::setup_errorno(*this, errcode, error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }
#      endif
      // write big number into buffer, the size must be no less than BN_num_bytes()
      // @see https://www.openssl.org/docs/manmaster/crypto/BN_bn2bin.html
      size_t dhparam_bnsz = BN_num_bytes(self_pubkey);
      param.resize(dhparam_bnsz, 0);
      BN_bn2bin(self_pubkey, &param[0]);
#      ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
      details::reset(self_pubkey);
#      endif
#    elif defined(CRYPTO_USE_MBEDTLS)
      size_t psz = dh_context_.mbedtls_dh_ctx_.len;
      param.resize(psz, 0);
      int res = mbedtls_dhm_make_public(&dh_context_.mbedtls_dh_ctx_, static_cast<int>(psz), &param[0], psz,
                                        mbedtls_ctr_drbg_random, &shared_context_->get_random_engine().ctr_drbg);

      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }
#    endif
      break;
    }
#  endif
    case method_t::EN_CDT_ECDH: {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      if (nullptr == dh_context_.openssl_ecdh_pkey_) {
        ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
        break;
      }

      unsigned char *point_data = nullptr;
      size_t encode_len = crypto_dh_EVP_PKEY_get1_tls_encodedpoint(dh_context_.openssl_ecdh_pkey_, &point_data);
      if (nullptr == point_data) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }

      param.resize(encode_len + 1, 0);
      memcpy(&param[1], point_data, encode_len);
      OPENSSL_free(point_data);
      param[0] = static_cast<unsigned char>(encode_len);

#  elif defined(CRYPTO_USE_MBEDTLS)
      unsigned char buf[CRYPTO_DH_MAX_KEY_LEN];
      // size is point(unknown size)
      size_t olen = 0;
      // @see mbedtls_ecdh_make_public, output group and point
      int res = mbedtls_ecdh_make_public(&dh_context_.mbedtls_ecdh_ctx_, &olen, buf, sizeof(buf),
                                         mbedtls_ctr_drbg_random, &shared_context_->get_random_engine().ctr_drbg);

      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }

      param.assign(buf, buf + olen);
#  endif
      break;
    }
    default: {
      details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
    }
  }

  return ret;
}

LIBATFRAME_UTILS_API int dh::read_public(const unsigned char *input, size_t ilen) {
  if (!shared_context_) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }

#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
  if (nullptr == shared_context_->get_dh_parameter().keygen_ctx) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }
#  endif

  if (nullptr == input || ilen == 0) {
    return details::setup_errorno(*this, 0, error_code_t::INVALID_PARAM);
  }

  int ret = details::setup_errorno(*this, 0, error_code_t::OK);
  switch (shared_context_->get_method()) {
#  if !defined(CRYPTO_USE_BORINGSSL)
    case method_t::EN_CDT_DH: {
#    if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL)
      if (nullptr == dh_context_.openssl_dh_pkey_) {
        ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
        break;
      }

      details::openssl_raii<BIGNUM> pub_key{BN_bin2bn(input, static_cast<int>(ilen), nullptr)};
      if (nullptr == pub_key.get()) {
        ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
        break;
      }

      if (nullptr == dh_context_.openssl_dh_peer_key_) {
        EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_dh_peer_key_);
      }
      if (nullptr == dh_context_.openssl_dh_peer_key_) {
        ret = details::setup_errorno(*this, 0, error_code_t::MALLOC);
        break;
      }

#      ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
      if (!EVP_PKEY_set_bn_param(dh_context_.openssl_dh_peer_key_, OSSL_PKEY_PARAM_PUB_KEY, pub_key.get())) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }
#      else
      DH *dh = EVP_PKEY_get0_DH(dh_context_.openssl_dh_peer_key_);
      if (nullptr == dh) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
        break;
      }

      if (DH_set0_key(dh, pub_key.get(), nullptr)) {
        pub_key.ref() = nullptr;
      }
#      endif

#    elif defined(CRYPTO_USE_MBEDTLS)
      int res = mbedtls_dhm_read_public(&dh_context_.mbedtls_dh_ctx_, input, ilen);
      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_READ_KEY);
        break;
      }
#    endif
      break;
    }
#  endif
    case method_t::EN_CDT_ECDH: {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      if (nullptr == dh_context_.openssl_ecdh_pkey_) {
        ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
        break;
      }

      /* Get encoded point length */
      size_t point_len = input[0];
      if (point_len + 1 != ilen) {
        ret = details::setup_errorno(*this, 0, error_code_t::INVALID_PARAM);
        break;
      }

      if (nullptr == dh_context_.openssl_ecdh_peer_key_) {
        EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_ecdh_peer_key_);
      }
      if (nullptr == dh_context_.openssl_ecdh_peer_key_) {
        ret = details::setup_errorno(*this, 0, error_code_t::MALLOC);
        break;
      }

      if (crypto_dh_EVP_PKEY_set1_tls_encodedpoint(dh_context_.openssl_ecdh_peer_key_, &input[1], point_len) <= 0) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::NOT_SUPPORT);
        details::reset(dh_context_.openssl_ecdh_peer_key_);
        break;
      }

#  elif defined(CRYPTO_USE_MBEDTLS)
      int res = mbedtls_ecdh_read_public(&dh_context_.mbedtls_ecdh_ctx_, input, ilen);
      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_READ_KEY);
        break;
      }
#  endif
      break;
    }
    default: {
      details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
    }
  }

  return ret;
}  // namespace util

LIBATFRAME_UTILS_API int dh::calc_secret(std::vector<unsigned char> &output) {
  if (!shared_context_) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }

  int ret = details::setup_errorno(*this, 0, error_code_t::OK);
  switch (shared_context_->get_method()) {
#  if !defined(CRYPTO_USE_BORINGSSL)
    case method_t::EN_CDT_DH: {
#    if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL)
      if (nullptr == dh_context_.openssl_dh_pkey_) {
        ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
        break;
      }

      if (nullptr == dh_context_.openssl_dh_peer_key_) {
        ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
        break;
      }

      if (nullptr != dh_context_.openssl_pkey_ctx_) {
        if (dh_context_.openssl_dh_pkey_ != EVP_PKEY_CTX_get0_pkey(dh_context_.openssl_pkey_ctx_)) {
          details::reset(dh_context_.openssl_pkey_ctx_);
        }
      }

      if (nullptr == dh_context_.openssl_pkey_ctx_) {
        dh_context_.openssl_pkey_ctx_ = EVP_PKEY_CTX_new(dh_context_.openssl_dh_pkey_, nullptr);
        if (nullptr != dh_context_.openssl_pkey_ctx_) {
          if (EVP_PKEY_derive_init(dh_context_.openssl_pkey_ctx_) <= 0) {
            ret =
                details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_SECRET);
            details::reset(dh_context_.openssl_pkey_ctx_);
          }
        }
      }
      if (nullptr == dh_context_.openssl_pkey_ctx_) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_SECRET);
        break;
      }

#      if !defined(CRYPTO_USE_BORINGSSL)
      if (dh_context_.openssl_dh_peer_key_ != EVP_PKEY_CTX_get0_peerkey(dh_context_.openssl_pkey_ctx_)) {
#      endif
        if (EVP_PKEY_derive_set_peer(dh_context_.openssl_pkey_ctx_, dh_context_.openssl_dh_peer_key_) <= 0) {
          ret =
              details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_SECRET);
          break;
        }
#      if !defined(CRYPTO_USE_BORINGSSL)
      }
#      endif

      // puts("pkey: params");
      // EVP_PKEY_print_params_fp(stdout, dh_context_.openssl_dh_pkey_, 2, nullptr);
      // puts("pkey: public");
      // EVP_PKEY_print_public_fp(stdout, dh_context_.openssl_dh_pkey_, 2, nullptr);
      // puts("pkey: private");
      // EVP_PKEY_print_private_fp(stdout, dh_context_.openssl_dh_pkey_, 2, nullptr);
      // puts("peer_key: params");
      // EVP_PKEY_print_params_fp(stdout, dh_context_.openssl_dh_peer_key_, 2, nullptr);
      // puts("peer_key: public");
      // EVP_PKEY_print_public_fp(stdout, dh_context_.openssl_dh_peer_key_, 2, nullptr);
      // puts("peer_key: private");
      // EVP_PKEY_print_private_fp(stdout, dh_context_.openssl_dh_peer_key_, 2, nullptr);

      size_t secret_len = 0;
      if (EVP_PKEY_derive(dh_context_.openssl_pkey_ctx_, nullptr, &secret_len) <= 0) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_SECRET);
        break;
      }

      output.resize(static_cast<size_t>((secret_len + 7) / 8) * 8, 0);
      if ((EVP_PKEY_derive(dh_context_.openssl_pkey_ctx_, &output[0], &secret_len)) <= 0) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_SECRET);
        break;
      }
      output.resize(static_cast<size_t>(secret_len));

#    elif defined(CRYPTO_USE_MBEDTLS)
      size_t psz = dh_context_.mbedtls_dh_ctx_.len;
      // generate next_secret
      output.resize(psz, 0);
      int res;
      res = mbedtls_dhm_calc_secret(&dh_context_.mbedtls_dh_ctx_, &output[0], psz, &psz, mbedtls_ctr_drbg_random,
                                    &shared_context_->get_random_engine().ctr_drbg);
      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_SECRET);
        break;
      }

#    endif
      break;
    }
#  endif
    case method_t::EN_CDT_ECDH: {
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      if (nullptr == dh_context_.openssl_ecdh_pkey_) {
        ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
        break;
      }

      if (nullptr == dh_context_.openssl_ecdh_peer_key_) {
        ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
        break;
      }

      if (nullptr != dh_context_.openssl_pkey_ctx_) {
        if (dh_context_.openssl_ecdh_pkey_ != EVP_PKEY_CTX_get0_pkey(dh_context_.openssl_pkey_ctx_)) {
          details::reset(dh_context_.openssl_pkey_ctx_);
        }
      }

      if (nullptr == dh_context_.openssl_pkey_ctx_) {
        dh_context_.openssl_pkey_ctx_ = EVP_PKEY_CTX_new(dh_context_.openssl_ecdh_pkey_, nullptr);
        if (nullptr != dh_context_.openssl_pkey_ctx_) {
          if (EVP_PKEY_derive_init(dh_context_.openssl_pkey_ctx_) <= 0) {
            ret =
                details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_SECRET);
            details::reset(dh_context_.openssl_pkey_ctx_);
          }
        }
      }

      if (nullptr == dh_context_.openssl_pkey_ctx_) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_SECRET);
        break;
      }

#    if !defined(CRYPTO_USE_BORINGSSL)
      if (dh_context_.openssl_ecdh_peer_key_ != EVP_PKEY_CTX_get0_peerkey(dh_context_.openssl_pkey_ctx_)) {
#    endif
        if (EVP_PKEY_derive_set_peer(dh_context_.openssl_pkey_ctx_, dh_context_.openssl_ecdh_peer_key_) <= 0) {
          ret =
              details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_SECRET);
          break;
        }
#    if !defined(CRYPTO_USE_BORINGSSL)
      }
#    endif

      size_t secret_len = 0;
      if (EVP_PKEY_derive(dh_context_.openssl_pkey_ctx_, nullptr, &secret_len) <= 0) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_SECRET);
        break;
      }

      output.resize(static_cast<size_t>((secret_len + 7) / 8) * 8, 0);
      if ((EVP_PKEY_derive(dh_context_.openssl_pkey_ctx_, &output[0], &secret_len)) <= 0) {
        ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_SECRET);
        break;
      }
      output.resize(static_cast<size_t>(secret_len));

#  elif defined(CRYPTO_USE_MBEDTLS)
      unsigned char buf[CRYPTO_DH_MAX_KEY_LEN];
      // usually is group size
      size_t olen = 0;
      int res;
      res = mbedtls_ecdh_calc_secret(&dh_context_.mbedtls_ecdh_ctx_, &olen, buf, sizeof(buf), mbedtls_ctr_drbg_random,
                                     &shared_context_->get_random_engine().ctr_drbg);
      if (0 != res) {
        ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_SECRET);
        break;
      }

      output.assign(buf, buf + olen);
#  endif
      break;
    }
    default: {
      details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
    }
  }

  return ret;
}

LIBATFRAME_UTILS_API const std::vector<std::string> &dh::get_all_curve_names() {
  static std::vector<std::string> ret;
  if (ret.empty()) {
    for (int i = 1; details::supported_dh_curves[i] != nullptr; ++i) {
      if (0 == strlen(details::supported_dh_curves[i])) {
        continue;
      }
#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
      if (0 != details::supported_dh_curves_openssl[i]) {
        unsigned int gtype = 0;
        int group_id = tls1_nid2group_id(details::supported_dh_curves_openssl[i]);
        if (0 == group_id) {
          continue;
        }
        int nid = tls1_ec_group_id2nid(group_id, &gtype);

        if (gtype == TLS_CURVE_CUSTOM) {
          details::openssl_raii<EVP_PKEY_CTX> pctx_keygen(EVP_PKEY_CTX_new_id(nid, nullptr));
          if (!pctx_keygen) {
            continue;
          }

          if (EVP_PKEY_keygen_init(pctx_keygen.get()) <= 0) {
            continue;
          }

          details::openssl_raii<EVP_PKEY_CTX> pctx_paramgen(EVP_PKEY_CTX_new_id(nid, nullptr));
          if (!pctx_paramgen) {
            continue;
          }

          if (EVP_PKEY_paramgen_init(pctx_paramgen.get()) <= 0) {
            continue;
          }
        } else {
          details::openssl_raii<EVP_PKEY_CTX> pctx_keygen(EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr));
          if (!pctx_keygen) {
            continue;
          }
          if (EVP_PKEY_keygen_init(pctx_keygen.get()) <= 0) {
            continue;
          }

          if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx_keygen.get(), nid) <= 0) {
            continue;
          }

          details::openssl_raii<EVP_PKEY_CTX> pctx_paramgen(EVP_PKEY_CTX_new_id(EVP_PKEY_EC, nullptr));
          if (!pctx_paramgen) {
            continue;
          }

          if (EVP_PKEY_paramgen_init(pctx_paramgen.get()) <= 0) {
            continue;
          }

          if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx_paramgen.get(), nid) <= 0) {
            continue;
          }
        }

        ret.push_back(std::string("ecdh:") + details::supported_dh_curves[i]);
      }
#  else
      if (nullptr != mbedtls_ecp_curve_info_from_name(details::supported_dh_curves[i])) {
        ret.push_back(std::string("ecdh:") + details::supported_dh_curves[i]);
      }
#  endif
    }
  }

  return ret;
}

#  if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
int dh::check_or_setup_ecp_id(int group_id) {
  if (!shared_context_) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }

  int ret = shared_context_->try_reset_ecp_id(group_id);
  if (error_code_t::OK != ret) {
    return details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), static_cast<error_code_t::type>(ret));
  }

  if (nullptr == shared_context_->get_dh_parameter().keygen_ctx) {
    return details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
  }

  return ret;
}

#    if !defined(CRYPTO_USE_BORINGSSL)
int dh::check_or_setup_dh_pg_gy(BIGNUM *&DH_p, BIGNUM *&DH_g, BIGNUM *&DH_gy) {
  if (!shared_context_) {
    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
  }

  if (nullptr != dh_context_.openssl_pkey_ctx_) {
    return error_code_t::ALREADY_INITED;
  }

  // import P,G,GY
  // @see int ssl3_get_key_exchange(SSL *s) in s3_clnt.c                                          -- openssl 1.0.x
  // @see int tls_process_ske_dhe(SSL *s, PACKET *pkt, EVP_PKEY **pkey, int *al) in statem_clnt.c -- openssl 1.1.x/3.x.x

  // puts("check_or_setup_dh_pg_gy");
  // BN_print_fp(stdout, DH_p);
  // BN_print_fp(stdout, DH_g);
  // BN_print_fp(stdout, DH_gy);

  int ret = error_code_t::OK;
  do {
    ret = shared_context_->try_reset_dh_params(DH_p, DH_g);
    if (error_code_t::OK != ret) {
      break;
    }

    if (nullptr == shared_context_->get_dh_parameter().keygen_ctx) {
      ret = error_code_t::INIT_DH_GENERATE_KEY;
      break;
    }

    details::reset(dh_context_.openssl_dh_pkey_);
    if (EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_dh_pkey_) <= 0) {
      details::reset(dh_context_.openssl_dh_pkey_);
      ret = details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), error_code_t::INIT_DH_GENERATE_KEY);
      break;
    }

    do {
      // import GY(optional)
      if (nullptr == DH_gy) {
        break;
      }

#      ifdef CRYPTO_USE_OPENSSL_WITH_OSSL_APIS
      details::openssl_raii<OSSL_PARAM_BLD> ossl_param_bld{OSSL_PARAM_BLD_new()};
      if (nullptr == ossl_param_bld.get() ||
          !OSSL_PARAM_BLD_push_BN(ossl_param_bld.get(), OSSL_PKEY_PARAM_FFC_P, DH_p) ||
          !OSSL_PARAM_BLD_push_BN(ossl_param_bld.get(), OSSL_PKEY_PARAM_FFC_G, DH_g) ||
          !OSSL_PARAM_BLD_push_BN(ossl_param_bld.get(), OSSL_PKEY_PARAM_PUB_KEY, DH_gy)) {
        ret = error_code_t::INIT_DH_READ_KEY;
        break;
      }

      details::openssl_raii<OSSL_PARAM> ossl_params{OSSL_PARAM_BLD_to_param(ossl_param_bld.get())};
      if (ossl_params.get() == nullptr) {
        ret = error_code_t::INIT_DH_READ_KEY;
        break;
      }

      details::reset(dh_context_.openssl_dh_peer_key_);
      if (!EVP_PKEY_fromdata_init(shared_context_->get_dh_parameter().keygen_ctx) ||
          !EVP_PKEY_fromdata(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_dh_peer_key_,
                             EVP_PKEY_KEYPAIR, ossl_params.get())) {
        ret = error_code_t::INIT_DH_READ_KEY;
        break;
      }

      // if (!EVP_PKEY_set_bn_param(dh_context_.openssl_dh_peer_key_, OSSL_PKEY_PARAM_PUB_KEY, DH_gy)) {
      //   ret = error_code_t::INIT_DH_READ_KEY;
      //   break;
      // }

      // @see test_fromdata_dh_named_group in <openssl-3.0.0>/test/evp_pkey_provided_test.c
      printf("\nbefore P: ");
      BN_print_fp(stdout, DH_p);
      printf("\nbefore G: ");
      BN_print_fp(stdout, DH_g);
      printf("\nbefore pubkey: ");
      BN_print_fp(stdout, DH_gy);
      {
        BIGNUM *debug_bn[3] = {nullptr, nullptr, nullptr};
        EVP_PKEY_get_bn_param(dh_context_.openssl_dh_peer_key_, OSSL_PKEY_PARAM_FFC_P, &debug_bn[0]);
        EVP_PKEY_get_bn_param(dh_context_.openssl_dh_peer_key_, OSSL_PKEY_PARAM_FFC_G, &debug_bn[1]);
        EVP_PKEY_get_bn_param(dh_context_.openssl_dh_peer_key_, OSSL_PKEY_PARAM_PUB_KEY, &debug_bn[2]);
        printf("\n after P: ");
        BN_print_fp(stdout, debug_bn[0]);
        printf("\n after G: ");
        BN_print_fp(stdout, debug_bn[1]);
        printf("\n after pubkey: ");
        BN_print_fp(stdout, debug_bn[2]);
        BN_free(debug_bn[0]);
        BN_free(debug_bn[1]);
        BN_free(debug_bn[2]);
      }

#      else
      if (nullptr == dh_context_.openssl_dh_peer_key_) {
        EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_dh_peer_key_);
      }
      if (nullptr == dh_context_.openssl_dh_peer_key_) {
        ret = error_code_t::INIT_DH_GENERATE_KEY;
        break;
      }

      DH *peer_dh = EVP_PKEY_get0_DH(dh_context_.openssl_dh_peer_key_);
      if (nullptr == peer_dh) {
        ret = error_code_t::INIT_DH_GENERATE_KEY;
        break;
      }

      if (!DH_set0_key(peer_dh, DH_gy, nullptr)) {
        ret = error_code_t::INIT_DH_READ_KEY;
        break;
      }
      // Move out here
      DH_gy = nullptr;
#      endif
    } while (false);

  } while (false);
  return details::setup_errorno(*this, static_cast<int>(ERR_peek_error()), static_cast<error_code_t::type>(ret));
}
#    endif
#  endif

}  // namespace crypto
}  // namespace util

#endif
