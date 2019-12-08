/**
 * @file crypto_dh.h
 * @brief DH/ECDH算法适配,大数随机算法适配
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2017.09.19
 *
 * @history
 *
 *
 */

#ifndef UTIL_ALGORITHM_CRYPTO_DH_H
#define UTIL_ALGORITHM_CRYPTO_DH_H

#pragma once

// CRYPTO_USE_OPENSSL, CRYPTO_USE_LIBRESSL,CRYPTO_USE_BORINGSSL, CRYPTO_USE_MBEDTLS

#include <config/atframe_utils_build_feature.h>

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)

#include <openssl/bio.h>
#include <openssl/bn.h>
#include <openssl/dh.h>
#include <openssl/ecdh.h>
#include <openssl/err.h>
#include <openssl/pem.h>

#define CRYPTO_DH_ENABLED 1

#elif defined(CRYPTO_USE_MBEDTLS)

#include "mbedtls/platform.h"
// "mbedtls/platform.h" must be the first
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/dhm.h"
#include "mbedtls/ecdh.h"
#include "mbedtls/ecp.h"
#include "mbedtls/entropy.h"

#define CRYPTO_DH_ENABLED 1

#endif

#ifdef CRYPTO_DH_ENABLED

#include <string>
#include <vector>

#include "std/smart_ptr.h"

namespace util {
    namespace crypto {
        class dh {
        public:
            struct LIBATFRAME_UTILS_API method_t {
                enum type {
                    EN_CDT_INVALID = 0, // inner
                    EN_CDT_DH      = 1, // dh algorithm
                    EN_CDT_ECDH         // ecdh algorithm
                };
            };

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            struct LIBATFRAME_UTILS_API dh_context_t {
                union {
                    DH *    openssl_dh_ptr_;
                    EC_KEY *openssl_ecdh_ptr_;
                };
                union {
                    BIGNUM *  peer_pubkey_;
                    EC_POINT *peer_ecpoint_;
                };
            };
#elif defined(CRYPTO_USE_MBEDTLS)
            struct LIBATFRAME_UTILS_API dh_context_t {
                union {
                    mbedtls_dhm_context  mbedtls_dh_ctx_;
                    mbedtls_ecdh_context mbedtls_ecdh_ctx_;
                };
            };
#endif

            struct LIBATFRAME_UTILS_API error_code_t {
                enum type {
                    OK                      = 0,
                    INVALID_PARAM           = -1,
                    NOT_INITED              = -2,
                    ALREADY_INITED          = -3,
                    MALLOC                  = -4,
                    DISABLED                = -11,
                    NOT_SUPPORT             = -12,
                    OPERATION               = -13,
                    INIT_RANDOM_ENGINE      = -14,
                    READ_DHPARAM_FILE       = -21,
                    INIT_DHPARAM            = -22,
                    INIT_DH_READ_PARAM      = -23,
                    INIT_DH_GENERATE_KEY    = -24,
                    INIT_DH_READ_KEY        = -25,
                    INIT_DH_GENERATE_SECRET = -26,
                };
            };

            class shared_context {
            public:
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                typedef struct {
                    BIO *                      param;
                    std::vector<unsigned char> param_buffer;
                    int                        ecp_id;
                } dh_param_t;

                typedef struct {
                } random_engine_t;

#elif defined(CRYPTO_USE_MBEDTLS)
                typedef struct {
                    std::string          param;
                    mbedtls_ecp_group_id ecp_id;
                } dh_param_t;

                // move mbedtls_ctr_drbg_context and mbedtls_entropy_context here
                typedef struct {
                    mbedtls_ctr_drbg_context ctr_drbg;
                    mbedtls_entropy_context  entropy;
                } random_engine_t;
#endif

                typedef std::shared_ptr<shared_context> ptr_t;

            private:
                struct LIBATFRAME_UTILS_API creator_helper {};

                LIBATFRAME_UTILS_API shared_context();

            public:
                LIBATFRAME_UTILS_API shared_context(creator_helper &helper);
                LIBATFRAME_UTILS_API ~shared_context();
                LIBATFRAME_UTILS_API static ptr_t create();

                /**
                 * @brief initialize a shared context for server mode
                 * @param name algorithm name, ecdh:[ECDH algorithm name] or the path of dh parameter PEM file
                 * @note using RFC 4492 for ECDH algorithm
                 * @return 0 or error code
                 */
                LIBATFRAME_UTILS_API int init(const char *name);

                /**
                 * @brief initialize a shared context for client mode
                 * @param method algorithm method
                 * @return 0 or error code
                 */
                LIBATFRAME_UTILS_API int init(method_t::type method);

                /**
                 * @brief reset shared resource
                 */
                LIBATFRAME_UTILS_API void reset();

                /**
                 * @brief random buffer
                 * @return 0 or error code
                 */
                LIBATFRAME_UTILS_API int random(void *output, size_t output_sz);

                LIBATFRAME_UTILS_API bool is_client_mode() const;

                LIBATFRAME_UTILS_API method_t::type get_method() const;

                LIBATFRAME_UTILS_API const dh_param_t &get_dh_parameter() const;
                LIBATFRAME_UTILS_API const random_engine_t &get_random_engine() const;
                LIBATFRAME_UTILS_API random_engine_t &get_random_engine();

            private:
                method_t::type  method_;
                dh_param_t      dh_param_;
                random_engine_t random_engine_;
            };

        public:
            LIBATFRAME_UTILS_API dh();
            LIBATFRAME_UTILS_API ~dh();

            /**
             * @brief initialize
             * @param shared_context shared context
             * @return 0 or error code
             */
            LIBATFRAME_UTILS_API int init(shared_context::ptr_t shared_context);

            /**
             * @brief release all resources
             * @return 0 or error code
             */
            LIBATFRAME_UTILS_API int close();

            /**
             * @brief set last error returned by crypto library
             * @param err error code returned by crypto library
             */
            LIBATFRAME_UTILS_API void set_last_errno(int e);

            /**
             * @brief get last error returned by crypto library
             * @return last error code returned by crypto library
             */
            LIBATFRAME_UTILS_API int get_last_errno() const;

            /**
             * @brief          Setup and write the ServerKeyExchange parameters
             *
             * @param param    destination buffer
             *
             * @note           This function assumes that ctx->P and ctx->G
             *                 have already been properly set
             *
             * @note           server process: make_params->read_public->calc_secret
             * @return         0 if successful, or error code
             */
            LIBATFRAME_UTILS_API int make_params(std::vector<unsigned char> &param);

            /**
             * @brief          Parse the ServerKeyExchange parameters
             *
             * @param input    input buffer
             * @param ilen     size of buffer
             *
             * @note           client process: read_params->make_public->calc_secret
             * @return         0 if successful, or error code
             */
            LIBATFRAME_UTILS_API int read_params(const unsigned char *input, size_t ilen);

            /**
             * @brief          Create own private value X and export G^X
             *
             * @param param    destination buffer
             *
             * @note           client process: read_params->make_public->calc_secret
             * @return         0 if successful, or error code
             */
            LIBATFRAME_UTILS_API int make_public(std::vector<unsigned char> &param);

            /**
             * @brief          Import the peer's public value G^Y
             *
             * @param input    input buffer
             * @param ilen     size of buffer
             *
             * @note           server process: make_params->read_public->calc_secret
             * @return         0 if successful, or error code
             */
            LIBATFRAME_UTILS_API int read_public(const unsigned char *input, size_t ilen);

            /**
             * @brief          Derive and export the shared secret (G^Y)^X mod P
             *
             * @param output   destination buffer
             *
             * @return         0 if successful, or error code
             *
             */
            LIBATFRAME_UTILS_API int calc_secret(std::vector<unsigned char> &output);

        public:
            static LIBATFRAME_UTILS_API const std::vector<std::string> &get_all_curve_names();

        private:
            int                   last_errorno_;
            shared_context::ptr_t shared_context_;
            dh_context_t          dh_context_;
        };
    } // namespace crypto
} // namespace util

#endif

#endif