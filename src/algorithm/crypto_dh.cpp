#include <cstring>

#include <common/file_system.h>
#include <common/string_oprs.h>

#include <algorithm/crypto_dh.h>
#include <std/static_assert.h>

#ifdef CRYPTO_DH_ENABLED


#ifndef UNUSED
#define UNUSED(x) ((void)x)
#endif

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)

#include <openssl/rand.h>

// copy from ssl_locl.h
#ifndef n2s
#define n2s(c, s) ((s = (((unsigned int)(c[0])) << 8) | (((unsigned int)(c[1])))), c += 2)
#endif

// copy from ssl_locl.h
#ifndef s2n
#define s2n(s, c) ((c[0] = (unsigned char)(((s) >> 8) & 0xff), c[1] = (unsigned char)(((s)) & 0xff)), c += 2)
#endif

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
static inline int DH_set0_key(DH *dh, BIGNUM *pub_key, BIGNUM *priv_key) {
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

            STD_STATIC_ASSERT(sizeof(supported_dh_curves) / sizeof(const char *) == sizeof(supported_dh_curves_openssl) / sizeof(int));
#endif
        } // namespace details

        // =============== shared context ===============
        dh::shared_context::shared_context() : method_(method_t::EN_CDT_INVALID) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            dh_param_.param = NULL;
#elif defined(CRYPTO_USE_MBEDTLS)
#endif

            memset(&random_engine_, 0, sizeof(random_engine_));
        }
        dh::shared_context::~shared_context() { reset(); }

        int dh::shared_context::init(const char *name) {
            if (NULL == name) {
                return error_code_t::INVALID_PARAM;
            }

            method_t::type method = method_t::EN_CDT_DH;
            if (0 == UTIL_STRFUNC_STRNCASE_CMP("ecdh:", name, 5)) {
                method = method_t::EN_CDT_ECDH;
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
                UNUSED(pem_file_e);
                if (NULL == pem) {
                    ret = error_code_t::READ_DHPARAM_FILE;
                    break;
                }
                fseek(pem, 0, SEEK_END);
                size_t pem_sz = static_cast<size_t>(ftell(pem));
                fseek(pem, 0, SEEK_SET);

// Read from pem file
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                do {
                    unsigned char *pem_buf = reinterpret_cast<unsigned char *>(calloc(pem_sz, sizeof(unsigned char)));
                    if (!pem_buf) {
                        ret = error_code_t::MALLOC;
                        break;
                    }
                    fread(pem_buf, sizeof(unsigned char), pem_sz, pem);
                    dh_param_.param = BIO_new_mem_buf(pem_buf, static_cast<int>(pem_sz));

                    // check
                    DH *test_dh_ctx = PEM_read_bio_DHparams(dh_param_.param, NULL, NULL, NULL);
                    if (NULL == test_dh_ctx) {
                        ret = error_code_t::READ_DHPARAM_FILE;
                    } else {
                        int errcode = 0;
                        DH_check(test_dh_ctx, &errcode);
                        if (((DH_CHECK_P_NOT_SAFE_PRIME | DH_NOT_SUITABLE_GENERATOR | DH_UNABLE_TO_CHECK_GENERATOR) & errcode)) {
                            ret = error_code_t::READ_DHPARAM_FILE;
                        }
                        DH_free(test_dh_ctx);
                    }
                } while (false);

                if (error_code_t::OK != ret) {
                    if (NULL != dh_param_.param) {
                        BIO_free(dh_param_.param);
                        dh_param_.param = NULL;
                    }
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                // mbedtls_dhm_read_params must has last character to be zero, so add one zero to the end
                dh_param_.param.resize(pem_sz * sizeof(unsigned char) + 1, 0);
                fread(&dh_param_.param[0], sizeof(unsigned char), pem_sz, pem);

                // test
                mbedtls_dhm_context test_dh_ctx;
                mbedtls_dhm_init(&test_dh_ctx);
                if (0 != mbedtls_dhm_parse_dhm(&test_dh_ctx, reinterpret_cast<const unsigned char *>(dh_param_.param.data()), pem_sz + 1)) {
                    ret = error_code_t::INIT_DHPARAM;
                } else {
                    mbedtls_dhm_free(&test_dh_ctx);
                }

                if (error_code_t::OK != ret) {
                    dh_param_.param.clear();
                }
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
// TODO Check if algorithm available
// TODO setup ecp
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
#elif defined(CRYPTO_USE_MBEDTLS)
#endif
                break;
            }
            default: { return error_code_t::NOT_SUPPORT; }
            }

            if (ret < 0) {
                reset();
            }

            return ret;
        }

        int dh::shared_context::init(method_t::type method) {
            if (method_t::EN_CDT_INVALID != method_) {
                return error_code_t::ALREADY_INITED;
            }

// random engine
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
#elif defined(CRYPTO_USE_MBEDTLS)
            mbedtls_ctr_drbg_init(&random_engine_.ctr_drbg);
            mbedtls_entropy_init(&random_engine_.entropy);

            ret = mbedtls_ctr_drbg_seed(&random_engine_.ctr_drbg, mbedtls_entropy_func, &random_engine_.entropy, NULL, 0);
            if (0 != ret) {
                // clear DH or ECDH data
                dh_param_.param.clear();
                return error_code_t::INIT_RANDOM_ENGINE;
            }
#endif
            method_ = method;

            return error_code_t::OK;
        }

        void dh::shared_context::reset() {
            if (method_t::EN_CDT_INVALID == method_) {
                return;
            }

            switch (method_) {
            case method_t::EN_CDT_DH: {
// clear pem file buffer
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL != dh_param_.param) {
                    BIO_free(dh_param_.param);
                    dh_param_.param = NULL;
                }
#elif defined(CRYPTO_USE_MBEDTLS)
                dh_param_.param.clear();
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
// TODO clear ecp
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
#elif defined(CRYPTO_USE_MBEDTLS)
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
#elif defined(CRYPTO_USE_MBEDTLS)
            mbedtls_ctr_drbg_free(&random_engine_.ctr_drbg);
            mbedtls_entropy_free(&random_engine_.entropy);
#endif
        }

        int dh::shared_context::random(void *output, size_t output_sz) {
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

        bool dh::shared_context::is_dh_client_mode() const {
            if (method_t::EN_CDT_DH != method_) {
                return false;
            }

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            return NULL == dh_param_.param;
#elif defined(CRYPTO_USE_MBEDTLS)
            return dh_param_.param.empty();
#endif
        }

        // --------------- shared context ---------------

        dh::dh() : last_errorno_(0) { memset(&dh_context_, 0, sizeof(dh_context_)); }
        dh::~dh() { close(); }

        int dh::init(shared_context::ptr_t shared_context) {
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
                    if (shared_context->is_dh_client_mode()) {
                        dh_context_.openssl_dh_ptr_ = DH_new();
                    } else {
                        UNUSED(BIO_reset(shared_context->get_dh_parameter().param));
                        dh_context_.openssl_dh_ptr_ = PEM_read_bio_DHparams(shared_context->get_dh_parameter().param, NULL, NULL, NULL);
                    }

                    if (!dh_context_.openssl_dh_ptr_) {
                        ret = error_code_t::INIT_DHPARAM;
                        break;
                    }

                    // if (!shared_context->is_dh_client_mode() && !DH_generate_key(dh_context_.openssl_dh_ptr_)) {
                    //     ret = error_code_t::INIT_DH_GENERATE_KEY;
                    //     break;
                    // }
                } while (false);

                if (0 != ret) {
                    if (NULL != dh_context_.peer_pubkey_) {
                        BN_free(dh_context_.peer_pubkey_);
                        dh_context_.peer_pubkey_ = NULL;
                    }

                    if (NULL != dh_context_.openssl_dh_ptr_) {
                        DH_free(dh_context_.openssl_dh_ptr_);
                        dh_context_.openssl_dh_ptr_ = NULL;
                    }
                }
#elif defined(CRYPTO_USE_MBEDTLS)
                // mbedtls_dhm_read_params
                do {
                    mbedtls_dhm_init(&dh_context_.mbedtls_dh_ctx_);
                    dh_context_.dh_param_cache_.clear();

                    // client mode, just init , do not read PEM file
                    if (false == shared_context->is_dh_client_mode()) {
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
                ret = details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
                // TODO
                break;
            }
            default: { details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT); }
            }

            if (0 != ret) {
                return ret;
            }

            shared_context_ = shared_context;
            return details::setup_errorno(*this, 0, error_code_t::OK);
        }

        int dh::close() {
            if (!shared_context_) {
                return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
            }

            shared_context::ptr_t shared_context;
            shared_context.swap(shared_context_);

            switch (shared_context->get_method()) {
            case method_t::EN_CDT_DH: {
// init DH param file
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL != dh_context_.peer_pubkey_) {
                    BN_free(dh_context_.peer_pubkey_);
                    dh_context_.peer_pubkey_ = NULL;
                }

                if (NULL != dh_context_.openssl_dh_ptr_) {
                    DH_free(dh_context_.openssl_dh_ptr_);
                    dh_context_.openssl_dh_ptr_ = NULL;
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                mbedtls_dhm_free(&dh_context_.mbedtls_dh_ctx_);
                dh_context_.dh_param_cache_.clear();
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
                return details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
                // TODO
                // break;
            }
            default: { details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT); }
            }

            return details::setup_errorno(*this, 0, error_code_t::OK);
        }

        int dh::make_params(std::vector<unsigned char> &param) {
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
                int errcode = 0;
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

                    size_t olen = 0;
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
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
                ret = details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
                // TODO
                break;
            }
            default: { details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT); }
            }

            return ret;
        }

        int dh::read_params(const unsigned char *input, size_t ilen) {
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

                BIGNUM *DH_p = NULL;
                BIGNUM *DH_g = NULL;
                do {
                    // ===============================================
                    // import P,G,GY
                    // @see int ssl3_get_key_exchange(SSL *s) in s3_clnt.c                                          -- openssl 1.0.x
                    // @see int tls_process_ske_dhe(SSL *s, PACKET *pkt, EVP_PKEY **pkey, int *al) in statem_clnt.c -- openssl 1.1.x
                    {
                        unsigned int i = 0, param_len = 2, n = static_cast<unsigned int>(ilen);
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
                        if (!(dh_context_.peer_pubkey_ = BN_bin2bn(p, i, NULL))) {
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
                int res = mbedtls_dhm_read_params(&dh_context_.mbedtls_dh_ctx_, &dh_params_beg, dh_params_beg + ilen);
                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_READ_PARAM);
                    break;
                }
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
                ret = details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
                // TODO
                break;
            }
            default: { details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT); }
            }

            return ret;
        }

        int dh::make_public(std::vector<unsigned char> &param) {
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
                    ret = details::setup_errorno(*this, ERR_get_error(), error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }

                int errcode = 0;
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
                                                  mbedtls_ctr_drbg_random, &&shared_context_->get_random_engine().ctr_drbg);

                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
                ret = details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
                // TODO
                break;
            }
            default: { details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT); }
            }

            return ret;
        }

        int dh::read_public(const unsigned char *input, size_t ilen) {
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

                if (NULL != dh_context_.peer_pubkey_) {
                    BN_free(dh_context_.peer_pubkey_);
                }
                dh_context_.peer_pubkey_ = BN_bin2bn(input, ilen, NULL);
                if (NULL == dh_context_.peer_pubkey_) {
                    ret = details::setup_errorno(*this, 0, error_code_t::INIT_DH_READ_KEY);
                    break;
                };

#elif defined(CRYPTO_USE_MBEDTLS)
                int res = mbedtls_dhm_read_public(&dh_context_.mbedtls_dh_ctx_, input, ilen);
                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_KEY);
                    break;
                }
#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
                ret = details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
                // TODO
                break;
            }
            default: { details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT); }
            }

            return ret;
        }

        int dh::calc_secret(std::vector<unsigned char> &output) {
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
                //  if (shared_context_->is_dh_client_mode()) {
                mbedtls_dhm_calc_secret(&dh_context_.mbedtls_dh_ctx_, &output[0], psz, &psz, mbedtls_ctr_drbg_random,
                                        &shared_context_->get_random_engine().ctr_drbg);
                // } else {
                // mbedtls_dhm_calc_secret(&dh_context_.mbedtls_dh_ctx_, &output[0], psz, &psz, NULL, NULL);
                // }
                if (0 != res) {
                    ret = details::setup_errorno(*this, res, error_code_t::INIT_DH_GENERATE_SECRET);
                    break;
                }

#endif
                break;
            }
            case method_t::EN_CDT_ECDH: {
                ret = details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT);
                // TODO
                break;
            }
            default: { details::setup_errorno(*this, 0, error_code_t::NOT_SUPPORT); }
            }

            return ret;
        }

        const std::vector<std::string> &dh::get_all_curve_names() {
            static std::vector<std::string> ret;
            if (ret.empty()) {
                for (int i = 1; details::supported_dh_curves[i] != NULL; ++i) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    if (0 != details::supported_dh_curves_openssl[i]) {
                        ret.push_back(details::supported_dh_curves[i]);
                    }
#else
                    ret.push_back(details::supported_dh_curves[i]);
#endif
                }
            }

            return ret;
        }
    } // namespace crypto
} // namespace util

#endif
