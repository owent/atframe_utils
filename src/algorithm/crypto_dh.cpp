#include <cstring>

#include <common/file_system.h>
#include <common/string_oprs.h>

#include <algorithm/crypto_dh.h>
#include <std/static_assert.h>

#ifdef CRYPTO_DH_ENABLED

#ifndef UNUSED
#define UNUSED(x) ((void)x)
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
        dh::shared_context::~shared_context() {}

        int dh::shared_context::init(const char *name) {
            if (method_t::EN_CDT_INVALID != method_) {
                return error_code_t::ALREADY_INITED;
            }

            method_t::type method = method_t::EN_CDT_DH;
            if (0 == UTIL_STRFUNC_STRNCASE_CMP("ecdh:", name, 5)) {
                method = method;
            }

            int ret = error_code_t::OK;
            switch (method) {
            case method_t::EN_CDT_DH: {
                // do nothing in client mode
                if (NULL == name) {
                    break;
                }

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

// TODO Read from pem file
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

            if (0 != ret) {
                return ret;
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
            return ret;
        }

        void dh::shared_context::reset() {
            if (method_t::EN_CDT_INVALID == method_) {
                return;
            }

            switch (method_) {
            case method_t::EN_CDT_DH: {
// TODO clear pem file buffer
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

        int dh::shared_context::random(void *output, size_t *olen) { return error_code_t::OK; }

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

        int dh::init(shared_context::ptr_t shared_context) { return details::setup_errorno(*this, 0, error_code_t::OK); }

        int dh::close() {
            do {
// cipher cleanup
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
#elif defined(CRYPTO_USE_MBEDTLS)
#endif
            } while (false);

            return details::setup_errorno(*this, 0, error_code_t::OK);
        }

        int dh::make_params(std::vector<unsigned char> &param) { return details::setup_errorno(*this, 0, error_code_t::OK); }

        int dh::make_params(const unsigned char *param, size_t *plen) { return details::setup_errorno(*this, 0, error_code_t::OK); }

        int dh::read_params(const unsigned char *input, size_t ilen) { return details::setup_errorno(*this, 0, error_code_t::OK); }

        int dh::make_public(std::vector<unsigned char> &param) { return details::setup_errorno(*this, 0, error_code_t::OK); }

        int dh::make_public(const unsigned char *param, size_t *plen) { return details::setup_errorno(*this, 0, error_code_t::OK); }

        int dh::read_public(const unsigned char *input, size_t ilen) { return details::setup_errorno(*this, 0, error_code_t::OK); }

        int dh::calc_secret(unsigned char *output, size_t output_size, size_t *olen) {
            return details::setup_errorno(*this, 0, error_code_t::OK);
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