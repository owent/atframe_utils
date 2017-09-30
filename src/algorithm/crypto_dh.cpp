#include <common/string_oprs.h>
#include <cstring>

#include <algorithm/crypto_dh.h>
#include <std/static_assert.h>

#ifdef CRYPTO_DH_ENABLED

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
                NID_secp521r1,        // see nist_curves in ec_curve.c
                NID_secp384r1,        // see nist_curves in ec_curve.c
                NID_X9_62_prime256v1, // see nist_curves in ec_curve.c
                NID_secp224r1,        // see nist_curves in ec_curve.c
                NID_X9_62_prime192v1, // see nist_curves in ec_curve.c
                NID_secp256k1,        // see curve_list in ec_curve.c
                NID_secp224k1,        // see curve_list in ec_curve.c
                NID_secp192k1,        // see curve_list in ec_curve.c
                NID_brainpoolP512r1,  // see curve_list in ec_curve.c
                NID_brainpoolP384r1,  // see curve_list in ec_curve.c
                NID_brainpoolP256r1,  // see curve_list in ec_curve.c
                -1,                   // end
            };

            STD_STATIC_ASSERT(sizeof(supported_dh_curves) / sizeof(const char *) == sizeof(supported_dh_curves_openssl) / sizeof(int));
#endif
        } // namespace details

        // =============== shared context ===============
        dh::shared_context::shared_context() {}
        dh::shared_context::~shared_context() {}

        int dh::shared_context::init(const char *name) { return 0; }

        void dh::shared_context::reset() {}

        int dh::shared_context::random(void *output, size_t *olen) { return error_code_t::OK; }

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

        const char **dh::get_all_curve_names() { return &details::supported_dh_curves[1]; }
    } // namespace crypto
} // namespace util

#endif