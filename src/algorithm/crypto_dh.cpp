#include <cstring>

#include <common/file_system.h>
#include <common/string_oprs.h>

#include <algorithm/crypto_dh.h>
#include <std/static_assert.h>

#include <common/compiler_message.h>
#include <config/compiler_features.h>
#include <std/explicit_declare.h>

#include <design_pattern/nomovable.h>
#include <design_pattern/noncopyable.h>

#if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#include <type_traits>
#endif

#ifdef CRYPTO_DH_ENABLED

// define max key cache length, the same as MBEDTLS_SSL_MAX_CONTENT_LEN
#define CRYPTO_DH_MAX_KEY_LEN 1024

#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)

#include <openssl/ec.h>
#include <openssl/rand.h>

// copy from ssl_locl.h
#ifndef n2s
#define n2s(c, s) ((s = (((unsigned int)(c[0])) << 8) | (((unsigned int)(c[1])))), c += 2)
#endif

// copy from ssl_locl.h
#ifndef s2n
#define s2n(s, c) ((c[0] = (unsigned char)(((s) >> 8) & 0xff), c[1] = (unsigned char)(((s)) & 0xff)), c += 2)
#endif

// copy from ssl_locl.h
#ifndef NAMED_CURVE_TYPE
#define NAMED_CURVE_TYPE 3
#endif

// copy from t1_lib.c of openssl 1.1.0
struct tls_curve_info {
    int          nid;     /* Curve NID */
    int          secbits; /* Bits of security (from SP800-57) */
    unsigned int flags;   /* Flags: currently just field type */
};

#ifndef TLS_CURVE_CHAR2
#define TLS_CURVE_CHAR2 0x1
#endif

#ifndef TLS_CURVE_PRIME
#define TLS_CURVE_PRIME 0x0
#endif

#ifndef TLS_CURVE_CUSTOM
#define TLS_CURVE_CUSTOM 0x2
#endif

/**
 * Table of curve information.
 * Do not delete entries or reorder this array! It is used as a lookup
 * table: the index of each entry is one less than the TLS curve id.
 * @see t1_lib.c in openssl source tree for more details
 */
static const tls_curve_info nid_list[] = {
    {
#ifdef NID_sect163k1
        NID_sect163k1
#else
        0
#endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect163k1 (1) */
    {
#ifdef NID_sect163r1
        NID_sect163r1
#else
        0
#endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect163r1 (2) */
    {
#ifdef NID_sect163r2
        NID_sect163r2
#else
        0
#endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect163r2 (3) */
    {
#ifdef NID_sect193r1
        NID_sect193r1
#else
        0
#endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect193r1 (4) */
    {
#ifdef NID_sect193r2
        NID_sect193r2
#else
        0
#endif
        ,
        80, TLS_CURVE_CHAR2}, /* sect193r2 (5) */
    {
#ifdef NID_sect233k1
        NID_sect233k1
#else
        0
#endif
        ,
        112, TLS_CURVE_CHAR2}, /* sect233k1 (6) */
    {
#ifdef NID_sect233r1
        NID_sect233r1
#else
        0
#endif
        ,
        112, TLS_CURVE_CHAR2}, /* sect233r1 (7) */
    {
#ifdef NID_sect239k1
        NID_sect239k1
#else
        0
#endif
        ,
        112, TLS_CURVE_CHAR2}, /* sect239k1 (8) */
    {
#ifdef NID_sect283k1
        NID_sect283k1
#else
        0
#endif
        ,
        128, TLS_CURVE_CHAR2}, /* sect283k1 (9) */
    {
#ifdef NID_sect283r1
        NID_sect283r1
#else
        0
#endif
        ,
        128, TLS_CURVE_CHAR2}, /* sect283r1 (10) */
    {
#ifdef NID_sect409k1
        NID_sect409k1
#else
        0
#endif
        ,
        192, TLS_CURVE_CHAR2}, /* sect409k1 (11) */
    {
#ifdef NID_sect409r1
        NID_sect409r1
#else
        0
#endif
        ,
        192, TLS_CURVE_CHAR2}, /* sect409r1 (12) */
    {
#ifdef NID_sect571k1
        NID_sect571k1
#else
        0
#endif
        ,
        256, TLS_CURVE_CHAR2}, /* sect571k1 (13) */
    {
#ifdef NID_sect571r1
        NID_sect571r1
#else
        0
#endif
        ,
        256, TLS_CURVE_CHAR2}, /* sect571r1 (14) */
    {
#ifdef NID_secp160k1
        NID_secp160k1
#else
        0
#endif
        ,
        80, TLS_CURVE_PRIME}, /* secp160k1 (15) */
    {
#ifdef NID_secp160r1
        NID_secp160r1
#else
        0
#endif
        ,
        80, TLS_CURVE_PRIME}, /* secp160r1 (16) */
    {
#ifdef NID_secp160r2
        NID_secp160r2
#else
        0
#endif
        ,
        80, TLS_CURVE_PRIME}, /* secp160r2 (17) */
    {
#ifdef NID_secp192k1
        NID_secp192k1
#else
        0
#endif
        ,
        80, TLS_CURVE_PRIME}, /* secp192k1 (18) */
    {
#ifdef NID_X9_62_prime192v1
        NID_X9_62_prime192v1
#else
        0
#endif
        ,
        80, TLS_CURVE_PRIME}, /* secp192r1 (19) */
    {
#ifdef NID_secp224k1
        NID_secp224k1
#else
        0
#endif
        ,
        112, TLS_CURVE_PRIME}, /* secp224k1 (20) */
    {
#ifdef NID_secp224r1
        NID_secp224r1
#else
        0
#endif
        ,
        112, TLS_CURVE_PRIME}, /* secp224r1 (21) */
    {
#ifdef NID_secp256k1
        NID_secp256k1
#else
        0
#endif
        ,
        128, TLS_CURVE_PRIME}, /* secp256k1 (22) */
    {
#ifdef NID_X9_62_prime256v1
        NID_X9_62_prime256v1
#else
        0
#endif
        ,
        128, TLS_CURVE_PRIME}, /* secp256r1 (23) */
    {
#ifdef NID_secp384r1
        NID_secp384r1
#else
        0
#endif
        ,
        192, TLS_CURVE_PRIME}, /* secp384r1 (24) */
    {
#ifdef NID_secp521r1
        NID_secp521r1
#else
        0
#endif
        ,
        256, TLS_CURVE_PRIME}, /* secp521r1 (25) */
    {
#ifdef NID_brainpoolP256r1
        NID_brainpoolP256r1
#else
        0
#endif
        ,
        128, TLS_CURVE_PRIME}, /* brainpoolP256r1 (26) */
    {
#ifdef NID_brainpoolP384r1
        NID_brainpoolP384r1
#else
        0
#endif
        ,
        192, TLS_CURVE_PRIME}, /* brainpoolP384r1 (27) */
    {
#ifdef NID_brainpoolP512r1
        NID_brainpoolP512r1
#else
        0
#endif
        ,
        256, TLS_CURVE_PRIME}, /* brainpool512r1 (28) */
    {
#ifdef NID_X25519
        NID_X25519
#else
        0
#endif
        ,
        128, TLS_CURVE_CUSTOM}, /* X25519 (29) */
    {
#ifdef NID_X448
        NID_X448
#else
        0
#endif
        ,
        224, TLS_CURVE_CUSTOM}, /* X448 (30) */
};

#define OSSL_NELEM(x) (sizeof(x) / sizeof(x[0]))

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
#if OPENSSL_VERSION_NUMBER < 0x10100000L

/**
 * @see crypto/dh/dh_lib.c in openssl 1.1.x
 */
static inline void DH_get0_pqg(const DH *dh, const BIGNUM **p, const BIGNUM **q, const BIGNUM **g) {
    if (p != NULL) *p = dh->p;
    if (q != NULL) *q = dh->q;
    if (g != NULL) *g = dh->g;
}

/**
 * @see crypto/dh/dh_lib.c in openssl 1.1.x
 */
static inline int DH_set0_pqg(DH *dh, BIGNUM *p, BIGNUM *q, BIGNUM *g) {
    /* If the fields p and g in d are NULL, the corresponding input
     * parameters MUST be non-NULL.  q may remain NULL.
     */
    if ((dh->p == NULL && p == NULL) || (dh->g == NULL && g == NULL)) return 0;

    if (p != NULL) {
        BN_free(dh->p);
        dh->p = p;
    }
    if (q != NULL) {
        BN_free(dh->q);
        dh->q = q;
    }
    if (g != NULL) {
        BN_free(dh->g);
        dh->g = g;
    }

    if (q != NULL) {
        dh->length = BN_num_bits(q);
    }

    return 1;
}

/**
 * @see crypto/dh/dh_lib.c in openssl 1.1.x
 */
static inline void DH_get0_key(const DH *dh, const BIGNUM **pub_key, const BIGNUM **priv_key) {
    if (pub_key != NULL) *pub_key = dh->pub_key;
    if (priv_key != NULL) *priv_key = dh->priv_key;
}

/**
 * @see crypto/dh/dh_lib.c in openssl 1.1.x
 */
EXPLICIT_UNUSED_ATTR static inline int DH_set0_key(DH *dh, BIGNUM *pub_key, BIGNUM *priv_key) {
    /* If the field pub_key in dh is NULL, the corresponding input
     * parameters MUST be non-NULL.  The priv_key field may
     * be left NULL.
     */
    if (dh->pub_key == NULL && pub_key == NULL) return 0;

    if (pub_key != NULL) {
        BN_free(dh->pub_key);
        dh->pub_key = pub_key;
    }
    if (priv_key != NULL) {
        BN_free(dh->priv_key);
        dh->priv_key = priv_key;
    }

    return 1;
}

#ifndef ASN1_PKEY_CTRL_SET1_TLS_ENCPT
#define ASN1_PKEY_CTRL_SET1_TLS_ENCPT 0x9
#endif

#ifndef ASN1_PKEY_CTRL_GET1_TLS_ENCPT
#define ASN1_PKEY_CTRL_GET1_TLS_ENCPT 0xa
#endif


static int EC_KEY_oct2key(EC_KEY *key, const unsigned char *buf, size_t len, BN_CTX *ctx) {
    if (key == NULL) return 0;
    const EC_GROUP *group = EC_KEY_get0_group(key);
    if (group == NULL) return 0;
    EC_POINT *point = EC_POINT_new(group);
    if (NULL == point) {
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

static size_t EC_POINT_point2buf(const EC_GROUP *group, const EC_POINT *point, point_conversion_form_t form, unsigned char **pbuf,
                                 BN_CTX *ctx) {
    size_t         len;
    unsigned char *buf;

    len = EC_POINT_point2oct(group, point, form, NULL, 0, NULL);
    if (len == 0) return 0;
    if ((buf = (unsigned char *)OPENSSL_malloc(len)) == NULL) {
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
    if (key == NULL || EC_KEY_get0_public_key(key) == NULL || EC_KEY_get0_group(key) == NULL) return 0;
    return EC_POINT_point2buf(EC_KEY_get0_group(key), EC_KEY_get0_public_key(key), form, pbuf, ctx);
}

// Just like crypto/ec/ec_ameth.c, openssl < 1.1.0 do not support x25519/x448 and ECX_KEY, just skip it
static int evp_pkey_asn1_ctrl(EVP_PKEY *pkey, int op, int arg1, void *arg2) {
    if (EVP_PKEY_get0_asn1(pkey) == NULL) return -2;
    // openssl < 1.1.0 do not has API of EVP_PKEY_get0_EC_KEY(key)
    // So we can only use EVP_PKEY_get1_EC_KEY and free it later
    EC_KEY *ec_key = EVP_PKEY_get1_EC_KEY(pkey);
    int     ret;
    switch (op) {
    case ASN1_PKEY_CTRL_SET1_TLS_ENCPT:
        ret = EC_KEY_oct2key(ec_key, arg2, arg1, NULL);
        break;
    case ASN1_PKEY_CTRL_GET1_TLS_ENCPT:
        ret = EC_KEY_key2buf(ec_key, POINT_CONVERSION_UNCOMPRESSED, arg2, NULL);
        break;
    default:
        ret = -2;
        break;
    }
    if (NULL != ec_key) {
        EC_KEY_free(ec_key);
    }
    return ret;
}

static size_t EVP_PKEY_get1_tls_encodedpoint(EVP_PKEY *pkey, unsigned char **ppt) {
    int rv;
    rv = evp_pkey_asn1_ctrl(pkey, ASN1_PKEY_CTRL_GET1_TLS_ENCPT, 0, (void *)ppt);
    if (rv <= 0) return 0;
    return rv;
}

static int EVP_PKEY_set1_tls_encodedpoint(EVP_PKEY *pkey, const unsigned char *pt, size_t ptlen) {
    if (ptlen > INT_MAX) return 0;
    if (evp_pkey_asn1_ctrl(pkey, ASN1_PKEY_CTRL_SET1_TLS_ENCPT, (int)ptlen, (void *)pt) <= 0) return 0;
    return 1;
}
#endif

#endif


#ifdef max
#undef max
#endif

namespace util {
    namespace crypto {
        namespace details {
            static inline dh::error_code_t::type setup_errorno(dh &ci, int err, dh::error_code_t::type ret) {
                ci.set_last_errno(err);
                return ret;
            }

            static const char *supported_dh_curves[] = {
                "",
                "x25519",          // see ecp_supported_curves in ecp.c of mbedtls
                "x448",            // mbedtls don't support right now but maybe support it in the future
                "secp521r1",       // see ecp_supported_curves in ecp.c of mbedtls
                "secp384r1",       // see ecp_supported_curves in ecp.c of mbedtls
                "secp256r1",       // see ecp_supported_curves in ecp.c of mbedtls
                "secp224r1",       // see ecp_supported_curves in ecp.c of mbedtls
                "secp192r1",       // see ecp_supported_curves in ecp.c of mbedtls
                "secp256k1",       // see ecp_supported_curves in ecp.c of mbedtls
                "secp224k1",       // see ecp_supported_curves in ecp.c of mbedtls
                "secp192k1",       // see ecp_supported_curves in ecp.c of mbedtls
                "brainpoolP512r1", // see ecp_supported_curves in ecp.c of mbedtls
                "brainpoolP384r1", // see ecp_supported_curves in ecp.c of mbedtls
                "brainpoolP256r1", // see ecp_supported_curves in ecp.c of mbedtls
                NULL,              // end
            };

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            // see ec_list_element in ec_curve.c of openssl
            static const int supported_dh_curves_openssl[] = {
                0,
#ifdef NID_X25519
                NID_X25519,
#else
                0,
#endif
#ifdef NID_X448
                NID_X448,
#else
                0,
#endif
#ifdef NID_secp521r1
                NID_secp521r1,        // see nist_curves in ec_curve.c
                NID_secp384r1,        // see nist_curves in ec_curve.c
                NID_X9_62_prime256v1, // see nist_curves in ec_curve.c
                NID_secp224r1,        // see nist_curves in ec_curve.c
                NID_X9_62_prime192v1, // see nist_curves in ec_curve.c
#else
                0,  0, 0, 0, 0,
#endif
#ifdef NID_secp256k1
                NID_secp256k1, // see curve_list in ec_curve.c
                NID_secp224k1, // see curve_list in ec_curve.c
                NID_secp192k1, // see curve_list in ec_curve.c
#else
                0,  0, 0,
#endif
#ifdef NID_brainpoolP512r1
                NID_brainpoolP512r1, // see curve_list in ec_curve.c
                NID_brainpoolP384r1, // see curve_list in ec_curve.c
                NID_brainpoolP256r1, // see curve_list in ec_curve.c
#else
                0,  0, 0,
#endif
                -1, // end
            };

            STD_STATIC_ASSERT(sizeof(supported_dh_curves) / sizeof(supported_dh_curves[0]) ==
                              sizeof(supported_dh_curves_openssl) / sizeof(supported_dh_curves_openssl[0]));

            static inline void reset(EVP_PKEY *&pkey) {
                if (NULL == pkey) {
                    return;
                }

                EVP_PKEY_free(pkey);
                pkey = NULL;
            }

            static inline void reset(EVP_PKEY_CTX *&ctx) {
                if (NULL == ctx) {
                    return;
                }

                EVP_PKEY_CTX_free(ctx);
                ctx = NULL;
            }

            static inline void reset(DH *&dh) {
                if (NULL == dh) {
                    return;
                }

                DH_free(dh);
                dh = NULL;
            }

            /*
            static inline void reset(EC_KEY *&ec_key) {
                if (NULL == ec_key) {
                    return;
                }

                EC_KEY_free(ec_key);
                ec_key = NULL;
            }

            static inline void reset(EC_POINT *&point) {
                if (NULL == point) {
                    return;
                }

                EC_POINT_free(point);
                point = NULL;
            }
            */

            static inline void reset(BN_CTX *&bn) {
                if (NULL == bn) {
                    return;
                }

                BN_CTX_free(bn);
                bn = NULL;
            }

            static inline void reset(BIO *&bio) {
                if (NULL == bio) {
                    return;
                }

                BIO_free(bio);
                bio = NULL;
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

                inline const TPTR *operator->() const { return data_; }
                inline TPTR *      operator->() { return data_; }

                inline const TPTR *get() const { return data_; }
                inline TPTR *      get() { return data_; }

            private:
                TPTR *data_;
            };

            static EVP_PKEY_CTX *initialize_pkey_ctx_by_group_id(int group_id, bool init_keygen, bool init_paramgen) {
                EVP_PKEY_CTX *ret       = NULL;
                unsigned int  gtype     = 0;
                int           curve_nid = tls1_ec_group_id2nid(group_id, &gtype);
                if (TLS_CURVE_CUSTOM == gtype) {
                    ret = EVP_PKEY_CTX_new_id(curve_nid, NULL);
                } else {
                    ret = EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL);
                }

                if (NULL == ret) {
                    return NULL;
                }

                if (init_keygen && EVP_PKEY_keygen_init(ret) <= 0) {
                    reset(ret);
                    return ret;
                }

                if (init_paramgen && EVP_PKEY_paramgen_init(ret) <= 0) {
                    reset(ret);
                    return ret;
                }

                if (TLS_CURVE_CUSTOM != gtype && EVP_PKEY_CTX_set_ec_paramgen_curve_nid(ret, curve_nid) <= 0) {
                    reset(ret);
                }

                return ret;
            }
#endif
        } // namespace details

        // =============== shared context ===============
        LIBATFRAME_UTILS_API dh::shared_context::shared_context() : flags_(flags_t::NONE), method_(method_t::EN_CDT_INVALID) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            dh_param_.param        = NULL;
            dh_param_.group_id     = 0;
            dh_param_.paramgen_ctx = NULL;
            dh_param_.keygen_ctx   = NULL;
            dh_param_.params_key   = NULL;
#elif defined(CRYPTO_USE_MBEDTLS)
            dh_param_.group_id = MBEDTLS_ECP_DP_NONE;
#endif

#if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#if (defined(__cplusplus) && __cplusplus >= 201402L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L))
            UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<random_engine_t>::value);
#elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
            UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<random_engine_t>::value);
#else
            UTIL_CONFIG_STATIC_ASSERT(std::is_pod<random_engine_t>::value);
#endif
#endif

            memset(&random_engine_, 0, sizeof(random_engine_));
        }
        LIBATFRAME_UTILS_API dh::shared_context::shared_context(creator_helper &)
            : flags_(flags_t::NONE), method_(method_t::EN_CDT_INVALID) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            dh_param_.param        = NULL;
            dh_param_.group_id     = 0;
            dh_param_.paramgen_ctx = NULL;
            dh_param_.keygen_ctx   = NULL;
            dh_param_.params_key   = NULL;
#elif defined(CRYPTO_USE_MBEDTLS)
            dh_param_.group_id = MBEDTLS_ECP_DP_NONE;
#endif

#if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#if (defined(__cplusplus) && __cplusplus >= 201402L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L))
            UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<random_engine_t>::value);
#elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
            UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<random_engine_t>::value);
#else
            UTIL_CONFIG_STATIC_ASSERT(std::is_pod<random_engine_t>::value);
#endif
#endif
            memset(&random_engine_, 0, sizeof(random_engine_));
        }
        LIBATFRAME_UTILS_API dh::shared_context::~shared_context() { reset(); }

        LIBATFRAME_UTILS_API dh::shared_context::ptr_t dh::shared_context::create() {
            creator_helper h;
            return std::make_shared<dh::shared_context>(h);
        }

        LIBATFRAME_UTILS_API int dh::shared_context::init(const char *name) {
            if (NULL == name) {
                return error_code_t::INVALID_PARAM;
            }

            int            ecp_idx = 1;
            method_t::type method  = method_t::EN_CDT_DH;
            if (0 == UTIL_STRFUNC_STRNCASE_CMP("ecdh:", name, 5)) {
                method = method_t::EN_CDT_ECDH;

                while (NULL != details::supported_dh_curves[ecp_idx]) {
                    if (0 == UTIL_STRFUNC_STRCASE_CMP(name + 5, details::supported_dh_curves[ecp_idx])) {
                        break;
                    }
                    ++ecp_idx;
                }

                if (NULL == details::supported_dh_curves[ecp_idx]) {
                    return error_code_t::NOT_SUPPORT;
                }

// check if it's available
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (0 == details::supported_dh_curves_openssl[ecp_idx]) {
                    return error_code_t::NOT_SUPPORT;
                }
#endif
            }

            int ret = init(method);
            // init failed
            if (ret < 0) {
                return ret;
            }

            switch (method) {
            case method_t::EN_CDT_DH: {
                // do nothing in client mode
                FILE *pem = NULL;
                UTIL_FS_OPEN(pem_file_e, pem, name, "r");
                COMPILER_UNUSED(pem_file_e);
                if (NULL == pem) {
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
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                do {
                    dh_param_.param_buffer.resize(pem_sz);
                    if (0 == fread(&dh_param_.param_buffer[0], sizeof(unsigned char), pem_sz, pem)) {
                        ret = error_code_t::READ_DHPARAM_FILE;
                        break;
                    }
                    dh_param_.param = BIO_new_mem_buf(&dh_param_.param_buffer[0], static_cast<int>(pem_sz));

                    // check
                    details::openssl_raii<DH> test_dh_ctx(PEM_read_bio_DHparams(dh_param_.param, NULL, NULL, NULL));
                    if (!test_dh_ctx) {
                        ret = error_code_t::READ_DHPARAM_FILE;
                    } else {
                        int errcode = 0;
                        DH_check(test_dh_ctx.get(), &errcode);
                        if (((DH_CHECK_P_NOT_SAFE_PRIME | DH_NOT_SUITABLE_GENERATOR | DH_UNABLE_TO_CHECK_GENERATOR) & errcode)) {
                            ret = error_code_t::READ_DHPARAM_FILE;
                        }
                    }
                } while (false);

                if (error_code_t::OK != ret) {
                    details::reset(dh_param_.param);
                    dh_param_.param_buffer.clear();
                }

#elif defined(CRYPTO_USE_MBEDTLS)
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
                    if (0 !=
                        mbedtls_dhm_parse_dhm(&test_dh_ctx, reinterpret_cast<const unsigned char *>(dh_param_.param.data()), pem_sz + 1)) {
                        ret = error_code_t::INIT_DHPARAM;
                    } else {
                        mbedtls_dhm_free(&test_dh_ctx);
                    }

                    if (error_code_t::OK != ret) {
                        dh_param_.param.clear();
                    }
                } while (false);
#endif

                UTIL_FS_CLOSE(pem);
                break;
            }
            case method_t::EN_CDT_ECDH: {
// check if it's available
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                // https://github.com/prithuadhikary/OPENSSL_EVP_ECDH_EXAMPLE/blob/master/main.c
                dh_param_.group_id = tls1_nid2group_id(details::supported_dh_curves_openssl[ecp_idx]);
                unsigned int gtype = 0;

                tls1_ec_group_id2nid(dh_param_.group_id, &gtype);
                if (TLS_CURVE_CUSTOM == gtype) {
                    dh_param_.keygen_ctx = details::initialize_pkey_ctx_by_group_id(dh_param_.group_id, true, false);
                    if (NULL == dh_param_.keygen_ctx) {
                        ret = error_code_t::NOT_SUPPORT;
                    }
                } else {
                    details::reset(dh_param_.paramgen_ctx);
                    dh_param_.paramgen_ctx = details::initialize_pkey_ctx_by_group_id(dh_param_.group_id, false, true);
                    if (NULL == dh_param_.paramgen_ctx) {
                        dh_param_.group_id = 0;
                        ret                = error_code_t::NOT_SUPPORT;
                        break;
                    }

                    do {
                        details::reset(dh_param_.params_key);
                        EVP_PKEY_paramgen(dh_param_.paramgen_ctx, &dh_param_.params_key);
                        if (NULL == dh_param_.params_key) {
                            break;
                        }
                        details::reset(dh_param_.keygen_ctx);
                        dh_param_.keygen_ctx = EVP_PKEY_CTX_new(dh_param_.params_key, NULL);
                        if (NULL == dh_param_.keygen_ctx) {
                            break;
                        }

                        if (EVP_PKEY_keygen_init(dh_param_.keygen_ctx) <= 0) {
                            details::reset(dh_param_.keygen_ctx);
                        }
                    } while (false);
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                const mbedtls_ecp_curve_info *curve = mbedtls_ecp_curve_info_from_name(details::supported_dh_curves[ecp_idx]);
                if (NULL == curve) {
                    ret = error_code_t::NOT_SUPPORT;
                    break;
                }
                dh_param_.group_id = curve->grp_id;
#endif
                break;
            }
            default: {
                return error_code_t::NOT_SUPPORT;
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
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
#elif defined(CRYPTO_USE_MBEDTLS)
            mbedtls_ctr_drbg_init(&random_engine_.ctr_drbg);
            mbedtls_entropy_init(&random_engine_.entropy);

            int res = mbedtls_ctr_drbg_seed(&random_engine_.ctr_drbg, mbedtls_entropy_func, &random_engine_.entropy, NULL, 0);
            if (0 != res) {
                // clear DH or ECDH data
                dh_param_.param.clear();
                return error_code_t::INIT_RANDOM_ENGINE;
            }
#endif
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
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                // clear dh pem buffer
                if (NULL != dh_param_.param) {
                    details::reset(dh_param_.param);
                    dh_param_.param_buffer.clear();
                }
                // clear ecp
                dh_param_.group_id = 0;
#elif defined(CRYPTO_USE_MBEDTLS)
                // clear dh pem buffer
                dh_param_.param.clear();
                // clear ecp
                dh_param_.group_id = MBEDTLS_ECP_DP_NONE;
#endif
                break;
            }
            default: {
                // do nothing
                break;
            }
            }

            method_ = method_t::EN_CDT_INVALID;
// random engine
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            details::reset(dh_param_.params_key);
            details::reset(dh_param_.paramgen_ctx);
            details::reset(dh_param_.keygen_ctx);
#elif defined(CRYPTO_USE_MBEDTLS)
            mbedtls_ctr_drbg_free(&random_engine_.ctr_drbg);
            mbedtls_entropy_free(&random_engine_.entropy);
#endif
        }

        LIBATFRAME_UTILS_API int dh::shared_context::random(void *output, size_t output_sz) {
            if (method_t::EN_CDT_INVALID == method_) {
                return error_code_t::NOT_INITED;
            }

            if (NULL == output || output_sz <= 0) {
                return error_code_t::INVALID_PARAM;
            }

            int ret = error_code_t::OK;
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            if (!RAND_bytes(reinterpret_cast<unsigned char *>(output), static_cast<int>(output_sz))) {
                ret = static_cast<int>(ERR_get_error());
            }

#elif defined(LIBATFRAME_ATGATEWAY_ENABLE_MBEDTLS)
            ret = mbedtls_ctr_drbg_random(&random_engine_.ctr_drbg, reinterpret_cast<unsigned char *>(output), output_sz);
#endif
            return ret;
        }

        LIBATFRAME_UTILS_API bool dh::shared_context::is_client_mode() const { return 0 != (flags_ & flags_t::CLIENT_MODE); }

        LIBATFRAME_UTILS_API dh::method_t::type dh::shared_context::get_method() const { return method_; }
        LIBATFRAME_UTILS_API const dh::shared_context::dh_param_t &dh::shared_context::get_dh_parameter() const { return dh_param_; }
        LIBATFRAME_UTILS_API const dh::shared_context::random_engine_t &dh::shared_context::get_random_engine() const {
            return random_engine_;
        }
        LIBATFRAME_UTILS_API dh::shared_context::random_engine_t &dh::shared_context::get_random_engine() { return random_engine_; }

        LIBATFRAME_UTILS_API bool dh::shared_context::check_or_setup_ecp_id(int group_id) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            if (0 != dh_param_.group_id && dh_param_.group_id != group_id) {
                return false;
            }

            if (NULL == dh_param_.keygen_ctx) {
                dh_param_.keygen_ctx = details::initialize_pkey_ctx_by_group_id(group_id, true, false);
                if (NULL == dh_param_.keygen_ctx) {
                    return false;
                }

                dh_param_.group_id = group_id;
            }
            return true;

#elif defined(CRYPTO_USE_MBEDTLS)
            if (MBEDTLS_ECP_DP_NONE != dh_param_.group_id) {
                return dh_param_.group_id == group_id;
            }
            return true;
#endif
        }

        // --------------- shared context ---------------

        LIBATFRAME_UTILS_API dh::dh() : last_errorno_(0) {
            memset(&dh_context_, 0, sizeof(dh_context_));
#if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#if (defined(__cplusplus) && __cplusplus >= 201402L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L))
            UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<dh_context_t>::value);
#elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
            UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<dh_context_t>::value);
#else
            UTIL_CONFIG_STATIC_ASSERT(std::is_pod<dh_context_t>::value);
#endif
#endif
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
            case method_t::EN_CDT_DH: {
// init DH param file
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                do {
                    if (shared_context->is_client_mode()) {
                        dh_context_.openssl_dh_ptr_ = DH_new();
                    } else {
                        if (NULL != shared_context->get_dh_parameter().param) {
                            UNUSED(BIO_reset(shared_context->get_dh_parameter().param));
                        }
                        dh_context_.openssl_dh_ptr_ = PEM_read_bio_DHparams(shared_context->get_dh_parameter().param, NULL, NULL, NULL);
                    }

                    if (!dh_context_.openssl_dh_ptr_) {
                        ret = error_code_t::INIT_DHPARAM;
                        break;
                    }

                } while (false);

                if (0 != ret) {
                    if (NULL != dh_context_.peer_pubkey_) {
                        BN_free(dh_context_.peer_pubkey_);
                        dh_context_.peer_pubkey_ = NULL;
                    }

                    details::reset(dh_context_.openssl_dh_ptr_);
                }
#elif defined(CRYPTO_USE_MBEDTLS)
                // mbedtls_dhm_read_params
                do {
                    mbedtls_dhm_init(&dh_context_.mbedtls_dh_ctx_);

                    // client mode, just init , do not read PEM file
                    if (false == shared_context->is_client_mode()) {
                        int res =
                            mbedtls_dhm_parse_dhm(&dh_context_.mbedtls_dh_ctx_,
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
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
// init DH param file
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (false == shared_context->is_client_mode()) {
                    if (NULL == shared_context->get_dh_parameter().keygen_ctx) {
                        ret = error_code_t::NOT_SERVER_MODE;
                    }
                }
#elif defined(CRYPTO_USE_MBEDTLS)
                // mbedtls_dhm_read_params
                do {
                    mbedtls_ecdh_init(&dh_context_.mbedtls_ecdh_ctx_);

                    if (false == shared_context->is_client_mode()) {
                        int res = mbedtls_ecp_group_load(&dh_context_.mbedtls_ecdh_ctx_.grp, shared_context->get_dh_parameter().group_id);
                        if (0 != res) {
                            ret = details::setup_errorno(*this, res, error_code_t::INIT_DHPARAM);
                            break;
                        }
                    }
                } while (false);

                if (0 != ret) {
                    mbedtls_ecdh_free(&dh_context_.mbedtls_ecdh_ctx_);
                }
#endif
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

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            details::reset(dh_context_.openssl_pkey_ctx_);
#endif

            switch (shared_context->get_method()) {
            case method_t::EN_CDT_DH: {
// clear DH param file and cache
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL != dh_context_.peer_pubkey_) {
                    BN_free(dh_context_.peer_pubkey_);
                    dh_context_.peer_pubkey_ = NULL;
                }

                details::reset(dh_context_.openssl_dh_ptr_);
#elif defined(CRYPTO_USE_MBEDTLS)
                mbedtls_dhm_free(&dh_context_.mbedtls_dh_ctx_);
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
// clear ecdh key and cache
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                details::reset(dh_context_.openssl_ecdh_peer_key_);
                details::reset(dh_context_.openssl_ecdh_pkey_);
#elif defined(CRYPTO_USE_MBEDTLS)
                mbedtls_ecdh_free(&dh_context_.mbedtls_ecdh_ctx_);
#endif
                break;
            }
            default: {
                details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
            }
            }

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
            case method_t::EN_CDT_DH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL == dh_context_.openssl_dh_ptr_) {
                    ret = error_code_t::NOT_SUPPORT;
                    break;
                }

                // ===============================================
                int res = DH_generate_key(dh_context_.openssl_dh_ptr_);
                if (1 != res) {
                    ret = error_code_t::INIT_DH_GENERATE_KEY;
                    break;
                }
                int           errcode     = 0;
                const BIGNUM *self_pubkey = NULL;
                DH_get0_key(dh_context_.openssl_dh_ptr_, &self_pubkey, NULL);
                res = DH_check_pub_key(dh_context_.openssl_dh_ptr_, self_pubkey, &errcode);
                if (1 != res) {
                    ret = details::setup_errorno(*this, errcode, error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }

                // write big number into buffer, the size must be no less than BN_num_bytes()
                // @see https://www.openssl.org/docs/manmaster/crypto/BN_bn2bin.html
                // dump P,G,GX
                // @see int ssl3_send_server_key_exchange(SSL *s) in s3_srvr.c          -- openssl 1.0.x
                // @see int tls_construct_server_key_exchange(SSL *s) in statem_srvr.c  -- openssl 1.1.x
                {
                    const BIGNUM *r[4] = {NULL, NULL, NULL, NULL};
                    DH_get0_pqg(dh_context_.openssl_dh_ptr_, &r[0], NULL, &r[1]);
                    DH_get0_key(dh_context_.openssl_dh_ptr_, &r[2], NULL);

                    size_t       olen  = 0;
                    unsigned int nr[4] = {0};
                    for (int i = 0; i < 4 && r[i] != NULL; i++) {
                        nr[i] = BN_num_bytes(r[i]);
                        // DHM_MPI_EXPORT in mbedtls/polarssl use 2 byte to store length, so openssl/libressl/boringssl should use
                        // OPENSSL_NO_SRP
                        olen += static_cast<size_t>(nr[i] + 2);
                    }

                    param.resize(olen, 0);
                    unsigned char *p = &param[0];
                    for (int i = 0; i < 4 && r[i] != NULL; i++) {
                        s2n(nr[i], p);
                        BN_bn2bin(r[i], p);
                        p += nr[i];
                    }
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                // size is P,G,GX
                size_t psz  = mbedtls_mpi_size(&dh_context_.mbedtls_dh_ctx_.P);
                size_t gsz  = mbedtls_mpi_size(&dh_context_.mbedtls_dh_ctx_.G);
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
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                do {
                    if (!shared_context_ || NULL == shared_context_->get_dh_parameter().keygen_ctx) {
                        ret = details::setup_errorno(*this, 0, error_code_t::INIT_DHPARAM);
                        break;
                    }

                    details::reset(dh_context_.openssl_ecdh_pkey_);
                    if (EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_ecdh_pkey_) <= 0) {
                        ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DHPARAM);
                    }

                    if (NULL == dh_context_.openssl_ecdh_pkey_) {
                        ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DHPARAM);
                        break;
                    }
                } while (false);

                if (0 != ret) {
                    details::reset(dh_context_.openssl_ecdh_pkey_);
                    break;
                }

                if (NULL == dh_context_.openssl_ecdh_pkey_) {
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
                unsigned char *point_data = NULL;
                size_t         encode_len = EVP_PKEY_get1_tls_encodedpoint(dh_context_.openssl_ecdh_pkey_, &point_data);
                if (NULL == point_data) {
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_KEY);
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

#elif defined(CRYPTO_USE_MBEDTLS)
                unsigned char buf[CRYPTO_DH_MAX_KEY_LEN];
                // size is ecp group(3byte) + point(unknown size)
                size_t olen = 0;
                // @see mbedtls_ecdh_make_params, output group and point
                int res = mbedtls_ecdh_make_params(&dh_context_.mbedtls_ecdh_ctx_, &olen, buf, sizeof(buf), mbedtls_ctr_drbg_random,
                                                   &shared_context_->get_random_engine().ctr_drbg);
                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }
                param.assign(buf, buf + olen);
#endif
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

            if (NULL == input || ilen == 0) {
                return details::setup_errorno(*this, 0, error_code_t::INVALID_PARAM);
            }

            int ret = details::setup_errorno(*this, 0, error_code_t::OK);
            switch (shared_context_->get_method()) {
            case method_t::EN_CDT_DH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL == dh_context_.openssl_dh_ptr_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
                    break;
                }

                BIGNUM *DH_p = NULL;
                BIGNUM *DH_g = NULL;
                do {
                    // ===============================================
                    // import P,G,GY
                    // @see int ssl3_get_key_exchange(SSL *s) in s3_clnt.c                                          -- openssl 1.0.x
                    // @see int tls_process_ske_dhe(SSL *s, PACKET *pkt, EVP_PKEY **pkey, int *al) in statem_clnt.c -- openssl 1.1.x
                    {
                        unsigned int         i = 0, param_len = 2, n = static_cast<unsigned int>(ilen);
                        const unsigned char *p = reinterpret_cast<const unsigned char *>(input);

                        // P
                        if (param_len > n) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
                            break;
                        }
                        n2s(p, i);

                        if (i > n - param_len) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
                            break;
                        }

                        param_len += i;
                        DH_p = BN_bin2bn(p, i, NULL);
                        p += i;

                        if (BN_is_zero(DH_p)) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
                            break;
                        }

                        // G
                        param_len += 2;
                        if (param_len > n) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
                            break;
                        }
                        n2s(p, i);

                        if (i > n - param_len) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
                            break;
                        }

                        param_len += i;
                        DH_g = BN_bin2bn(p, i, NULL);
                        p += i;

                        if (BN_is_zero(DH_g)) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
                            break;
                        }

                        // Set P, G
                        if (!DH_set0_pqg(dh_context_.openssl_dh_ptr_, DH_p, NULL, DH_g)) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
                            break;
                        }
                        DH_p = NULL;
                        DH_g = NULL;

                        // GY
                        param_len += 2;
                        if (param_len > n) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
                            break;
                        }
                        n2s(p, i);

                        if (i > n - param_len) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
                            break;
                        }

                        // param_len += i; // do not use **param_len** any more, it will cause static analysis to report a warning
                        dh_context_.peer_pubkey_ = BN_bin2bn(p, i, NULL);
                        if (!dh_context_.peer_pubkey_) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
                            break;
                        }
                        // p += i; // do not use **p** any more, it will cause static analysis to report a warning

                        if (BN_is_zero(dh_context_.peer_pubkey_)) {
                            ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
                            break;
                        }
                    }
                } while (false);

                if (0 != ret && NULL != dh_context_.peer_pubkey_) {
                    BN_free(dh_context_.peer_pubkey_);
                    dh_context_.peer_pubkey_ = NULL;
                }

                if (NULL != DH_p) {
                    BN_free(DH_p);
                    DH_p = NULL;
                }

                if (NULL != DH_g) {
                    BN_free(DH_g);
                    DH_g = NULL;
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                unsigned char *dh_params_beg = const_cast<unsigned char *>(input);
                int            res           = mbedtls_dhm_read_params(&dh_context_.mbedtls_dh_ctx_, &dh_params_beg, dh_params_beg + ilen);
                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_READ_PARAM);
                    break;
                }
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
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
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_READ_PARAM);
                    break;
                }

                int group_id = static_cast<int>(input[1] << 8) | static_cast<int>(input[2]);
                if (!shared_context_ || !shared_context_->check_or_setup_ecp_id(group_id)) {
                    ret = details::setup_errorno(*this, 0, error_code_t::ALGORITHM_MISMATCH);
                    break;
                }

                if (!shared_context_ || NULL == shared_context_->get_dh_parameter().keygen_ctx) {
                    ret = details::setup_errorno(*this, 0, error_code_t::NOT_CLIENT_MODE);
                    break;
                }

                details::reset(dh_context_.openssl_ecdh_pkey_);
                if (EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_ecdh_pkey_) <= 0) {
                    details::reset(dh_context_.openssl_ecdh_pkey_);
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }
                if (NULL == dh_context_.openssl_ecdh_pkey_) {
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }

                if (NULL == dh_context_.openssl_ecdh_peer_key_) {
                    EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_ecdh_peer_key_);
                }
                if (NULL == dh_context_.openssl_ecdh_peer_key_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::MALLOC);
                    break;
                }

                // int type_id = EVP_PKEY_id(dh_context_.openssl_ecdh_pkey_);
                //     Still missing nid information if type_id == EVP_PKEY_EC
                // if (EVP_PKEY_set_type(dh_context_.openssl_ecdh_peer_key_, type_id) <= 0) {
                //     ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::NOT_SUPPORT);
                //     details::reset(dh_context_.openssl_ecdh_peer_key_);
                //     break;
                // }

                if (EVP_PKEY_set1_tls_encodedpoint(dh_context_.openssl_ecdh_peer_key_, &input[curve_grp_len], encoded_pt_len) <= 0) {
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::NOT_SUPPORT);
                    details::reset(dh_context_.openssl_ecdh_peer_key_);
                    break;
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                const unsigned char *dh_params_beg = input;
                int                  res = mbedtls_ecdh_read_params(&dh_context_.mbedtls_ecdh_ctx_, &dh_params_beg, dh_params_beg + ilen);
                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_READ_PARAM);
                    break;
                }
#endif
                break;
            }
            default: {
                details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
            }
            }

            return ret;
        }

        LIBATFRAME_UTILS_API int dh::make_public(std::vector<unsigned char> &param) {
            if (!shared_context_) {
                return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
            }

            int ret = details::setup_errorno(*this, 0, error_code_t::OK);
            switch (shared_context_->get_method()) {
            case method_t::EN_CDT_DH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL == dh_context_.openssl_dh_ptr_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
                    break;
                }

                int res = DH_generate_key(dh_context_.openssl_dh_ptr_);
                if (1 != res) {
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }

                int           errcode     = 0;
                const BIGNUM *self_pubkey = NULL;
                DH_get0_key(dh_context_.openssl_dh_ptr_, &self_pubkey, NULL);
                res = DH_check_pub_key(dh_context_.openssl_dh_ptr_, self_pubkey, &errcode);
                if (1 != res) {
                    ret = details::setup_errorno(*this, errcode, error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }

                // write big number into buffer, the size must be no less than BN_num_bytes()
                // @see https://www.openssl.org/docs/manmaster/crypto/BN_bn2bin.html
                size_t dhparam_bnsz = BN_num_bytes(self_pubkey);
                param.resize(dhparam_bnsz, 0);
                BN_bn2bin(self_pubkey, &param[0]);

#elif defined(CRYPTO_USE_MBEDTLS)
                size_t psz = dh_context_.mbedtls_dh_ctx_.len;
                param.resize(psz, 0);
                int res = mbedtls_dhm_make_public(&dh_context_.mbedtls_dh_ctx_, static_cast<int>(psz), &param[0], psz,
                                                  mbedtls_ctr_drbg_random, &shared_context_->get_random_engine().ctr_drbg);

                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL == dh_context_.openssl_ecdh_pkey_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_PARAM);
                    break;
                }

                unsigned char *point_data = NULL;
                size_t         encode_len = EVP_PKEY_get1_tls_encodedpoint(dh_context_.openssl_ecdh_pkey_, &point_data);
                if (NULL == point_data) {
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }

                param.resize(encode_len + 1, 0);
                memcpy(&param[1], point_data, encode_len);
                OPENSSL_free(point_data);
                param[0] = static_cast<unsigned char>(encode_len);

#elif defined(CRYPTO_USE_MBEDTLS)
                unsigned char buf[CRYPTO_DH_MAX_KEY_LEN];
                // size is point(unknown size)
                size_t olen = 0;
                // @see mbedtls_ecdh_make_public, output group and point
                int res = mbedtls_ecdh_make_public(&dh_context_.mbedtls_ecdh_ctx_, &olen, buf, sizeof(buf), mbedtls_ctr_drbg_random,
                                                   &shared_context_->get_random_engine().ctr_drbg);

                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }

                param.assign(buf, buf + olen);
#endif
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

            if (NULL == input || ilen == 0) {
                return details::setup_errorno(*this, 0, error_code_t::INVALID_PARAM);
            }

            int ret = details::setup_errorno(*this, 0, error_code_t::OK);
            switch (shared_context_->get_method()) {
            case method_t::EN_CDT_DH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL == dh_context_.openssl_dh_ptr_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
                    break;
                }

                if (NULL != dh_context_.peer_pubkey_) {
                    BN_free(dh_context_.peer_pubkey_);
                }
                dh_context_.peer_pubkey_ = BN_bin2bn(input, static_cast<int>(ilen), NULL);
                if (NULL == dh_context_.peer_pubkey_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
                    break;
                };

#elif defined(CRYPTO_USE_MBEDTLS)
                int res = mbedtls_dhm_read_public(&dh_context_.mbedtls_dh_ctx_, input, ilen);
                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_READ_KEY);
                    break;
                }
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL == dh_context_.openssl_ecdh_pkey_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
                    break;
                }

                /* Get encoded point length */
                size_t point_len = input[0];
                if (point_len + 1 != ilen) {
                    ret = details::setup_errorno(*this, 0, error_code_t::INVALID_PARAM);
                    break;
                }

                if (NULL == dh_context_.openssl_ecdh_peer_key_) {
                    EVP_PKEY_keygen(shared_context_->get_dh_parameter().keygen_ctx, &dh_context_.openssl_ecdh_peer_key_);
                }
                if (NULL == dh_context_.openssl_ecdh_peer_key_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::MALLOC);
                    break;
                }

                // int type_id = EVP_PKEY_id(dh_context_.openssl_ecdh_pkey_);
                //     Still missing nid information if type_id == EVP_PKEY_EC
                // if (EVP_PKEY_set_type(dh_context_.openssl_ecdh_peer_key_, type_id) <= 0) {
                //     ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::NOT_SUPPORT);
                //     details::reset(dh_context_.openssl_ecdh_peer_key_);
                //     break;
                // }

                if (EVP_PKEY_set1_tls_encodedpoint(dh_context_.openssl_ecdh_peer_key_, &input[1], point_len) <= 0) {
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::NOT_SUPPORT);
                    details::reset(dh_context_.openssl_ecdh_peer_key_);
                    break;
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                int res = mbedtls_ecdh_read_public(&dh_context_.mbedtls_ecdh_ctx_, input, ilen);
                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_READ_KEY);
                    break;
                }
#endif
                break;
            }
            default: {
                details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
            }
            }

            return ret;
        }

        LIBATFRAME_UTILS_API int dh::calc_secret(std::vector<unsigned char> &output) {
            if (!shared_context_) {
                return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
            }

            int ret = details::setup_errorno(*this, 0, error_code_t::OK);
            switch (shared_context_->get_method()) {
            case method_t::EN_CDT_DH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL == dh_context_.openssl_dh_ptr_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
                    break;
                }

                if (NULL == dh_context_.peer_pubkey_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
                    break;
                }

                output.resize(static_cast<size_t>(sizeof(unsigned char) * (DH_size(dh_context_.openssl_dh_ptr_))), 0);
                int secret_len = DH_compute_key(&output[0], dh_context_.peer_pubkey_, dh_context_.openssl_dh_ptr_);
                if (secret_len < 0) {
                    ret = details::setup_errorno(*this, secret_len, error_code_t::INIT_DH_GENERATE_SECRET);
                    break;
                }

#elif defined(CRYPTO_USE_MBEDTLS)
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

#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL == dh_context_.openssl_ecdh_pkey_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
                    break;
                }

                if (NULL == dh_context_.openssl_ecdh_peer_key_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
                    break;
                }

                if (NULL != dh_context_.openssl_pkey_ctx_) {
                    if (dh_context_.openssl_ecdh_pkey_ != EVP_PKEY_CTX_get0_pkey(dh_context_.openssl_pkey_ctx_)) {
                        details::reset(dh_context_.openssl_pkey_ctx_);
                    }
                }

                if (NULL == dh_context_.openssl_pkey_ctx_) {
                    dh_context_.openssl_pkey_ctx_ = EVP_PKEY_CTX_new(dh_context_.openssl_ecdh_pkey_, NULL);
                    if (NULL != dh_context_.openssl_pkey_ctx_) {
                        if (EVP_PKEY_derive_init(dh_context_.openssl_pkey_ctx_) <= 0) {
                            ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_SECRET);
                            details::reset(dh_context_.openssl_pkey_ctx_);
                        }
                    }
                }
                if (NULL == dh_context_.openssl_pkey_ctx_) {
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_SECRET);
                    break;
                }

                if (dh_context_.openssl_ecdh_peer_key_ != EVP_PKEY_CTX_get0_peerkey(dh_context_.openssl_pkey_ctx_)) {
                    if (EVP_PKEY_derive_set_peer(dh_context_.openssl_pkey_ctx_, dh_context_.openssl_ecdh_peer_key_) <= 0) {
                        ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_SECRET);
                        break;
                    }
                }

                size_t secret_len = 0;
                if (EVP_PKEY_derive(dh_context_.openssl_pkey_ctx_, NULL, &secret_len) <= 0) {
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_SECRET);
                    break;
                }

                output.resize(static_cast<size_t>((secret_len + 7) / 8) * 8, 0);
                if ((EVP_PKEY_derive(dh_context_.openssl_pkey_ctx_, &output[0], &secret_len)) <= 0) {
                    ret = details::setup_errorno(*this, static_cast<int>(ERR_get_error()), error_code_t::INIT_DH_GENERATE_SECRET);
                    break;
                }
                output.resize(static_cast<size_t>(secret_len));

#elif defined(CRYPTO_USE_MBEDTLS)
                unsigned char buf[CRYPTO_DH_MAX_KEY_LEN];
                // usually is group size
                size_t olen = 0;
                int    res;
                res = mbedtls_ecdh_calc_secret(&dh_context_.mbedtls_ecdh_ctx_, &olen, buf, sizeof(buf), mbedtls_ctr_drbg_random,
                                               &shared_context_->get_random_engine().ctr_drbg);
                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_SECRET);
                    break;
                }

                output.assign(buf, buf + olen);
#endif
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
                for (int i = 1; details::supported_dh_curves[i] != NULL; ++i) {
                    if (0 == strlen(details::supported_dh_curves[i])) {
                        continue;
                    }
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    if (0 != details::supported_dh_curves_openssl[i]) {
                        unsigned int gtype    = 0;
                        int          group_id = tls1_nid2group_id(details::supported_dh_curves_openssl[i]);
                        if (0 == group_id) {
                            continue;
                        }
                        int nid = tls1_ec_group_id2nid(group_id, &gtype);

                        if (gtype == TLS_CURVE_CUSTOM) {
                            details::openssl_raii<EVP_PKEY_CTX> pctx(EVP_PKEY_CTX_new_id(nid, NULL));
                            if (!pctx) {
                                continue;
                            }

                            if (EVP_PKEY_keygen_init(pctx.get()) <= 0) {
                                continue;
                            }
                        } else {
                            details::openssl_raii<EVP_PKEY_CTX> pctx(EVP_PKEY_CTX_new_id(EVP_PKEY_EC, NULL));
                            if (!pctx) {
                                continue;
                            }
                            if (EVP_PKEY_keygen_init(pctx.get()) <= 0) {
                                continue;
                            }

                            if (EVP_PKEY_CTX_set_ec_paramgen_curve_nid(pctx.get(), nid) <= 0) {
                                continue;
                            }
                        }

                        ret.push_back(std::string("ecdh:") + details::supported_dh_curves[i]);
                    }
#else
                    if (NULL != mbedtls_ecp_curve_info_from_name(details::supported_dh_curves[i])) {
                        ret.push_back(std::string("ecdh:") + details::supported_dh_curves[i]);
                    }
#endif
                }
            }

            return ret;
        }
    } // namespace crypto
} // namespace util

#endif
