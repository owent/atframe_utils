/**
 * @file crypto_cipher.h
 * @brief 加密算法库适配
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2017.07.25
 *
 * @see https://en.wikipedia.org/wiki/XXTEA
 * @history
 *
 *
 */

#ifndef UTIL_ALGORITHM_CRYPTO_CIPHER_H
#define UTIL_ALGORITHM_CRYPTO_CIPHER_H

#pragma once

// CRYPTO_USE_OPENSSL, CRYPTO_USE_LIBRESSL,CRYPTO_USE_BORINGSSL, CRYPTO_USE_MBEDTLS

#include <config/atframe_utils_build_feature.h>

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)

#include <openssl/evp.h>
#include <openssl/md5.h>
#include <openssl/sha.h>

#define CRYPTO_CIPHER_ENABLED 1

#elif defined(CRYPTO_USE_MBEDTLS)

#include <mbedtls/cipher.h>
#include <mbedtls/md.h>

#define CRYPTO_CIPHER_ENABLED 1

#endif

#ifdef CRYPTO_CIPHER_ENABLED

#include <string>
#include <vector>

#include "xxtea.h"

namespace util {
    namespace crypto {
        class cipher {
        public:
            struct method_t {
                enum type {
                    EN_CMT_INVALID = 0, // inner
                    EN_CMT_XXTEA = 1,   // inner
                    EN_CMT_CIPHER
                };
            };

            struct mode_t {
                enum type { EN_CMODE_ENCRYPT = 0x01, EN_CMODE_DECRYPT = 0x02 };
            };

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            typedef EVP_CIPHER cipher_kt_t;
            typedef EVP_CIPHER_CTX cipher_evp_t;
            typedef EVP_MD digest_type_t;
            enum {
                MAX_KEY_LENGTH = EVP_MAX_KEY_LENGTH, //
                MAX_IV_LENGTH = EVP_MAX_IV_LENGTH,   //
                MAX_MD_SIZE = EVP_MAX_MD_SIZE
            };

#elif defined(CRYPTO_USE_MBEDTLS)
            typedef mbedtls_cipher_info_t cipher_kt_t;
            typedef mbedtls_cipher_context_t cipher_evp_t;
            typedef mbedtls_md_info_t digest_type_t;
            enum {
                MAX_KEY_LENGTH = 64, //
                MAX_IV_LENGTH = MBEDTLS_MAX_IV_LENGTH,
                MAX_MD_SIZE = MBEDTLS_MD_MAX_SIZE
            };
#endif

            struct error_code_t {
                enum type {
                    OK = 0,
                    INVALID_PARAM = -1,
                    NOT_INITED = -2,
                    ALREADY_INITED = -3,
                    MALLOC = -4,
                    CIPHER_DISABLED = -11,
                    CIPHER_NOT_SUPPORT = -12,
                    CIPHER_OPERATION = -13,
                };
            };

        public:
            cipher();
            ~cipher();

            int init(const char *name, int mode = mode_t::EN_CMODE_ENCRYPT | mode_t::EN_CMODE_DECRYPT);
            int close();

            /**
             * @brief set last error returned by crypto library
             * @param err error code returned by crypto library
             */
            inline void set_last_errno(int e) { last_errorno_ = e; }
            /**
             * @brief get last error returned by crypto library
             * @return last error code returned by crypto library
             */
            inline int get_last_errno() const { return last_errorno_; }

            /**
             * @brief               get iv size in crypt library
             * @return              iv length in bytes
             */
            uint32_t get_iv_size() const;

            /**
             * @brief               get key length in bits
             * @return              key length in bits
             */
            uint32_t get_key_bits() const;

            /**
             * @brief               get block size in crypt library
             * @return              block length in bytes
             */
            uint32_t get_block_size() const;

            /**
             * @brief               set key
             * @param key           key
             * @param key_bitlen    key length to use, in bits. must equal or greater to get_key_bits()
             * @return              0 or error code less than 0
             */
            int set_key(const unsigned char *key, uint32_t key_bitlen);

            /**
             * @brief               set key
             * @param iv            iv value
             * @param iv_len        length of iv, in bytes
             * @return              0 or error code less than 0
             */
            int set_iv(const unsigned char *iv, size_t iv_len);

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
            int encrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen);

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
            int decrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen);

        public:
            static const cipher_kt_t *get_cipher_by_name(const char *name);
            /**
             * @biref               split cipher names by space, comma, semicolon or colon
             * @param in            string contain some cipher names
             * @return              begin(first) and end(second) address of a cipher name, both NULL if not found.
             *                      you can use second pointer as the paramter of next call, just like strtok
             */
            static std::pair<const char *, const char *> ciphertok(const char *in);
            static const std::vector<std::string> &get_all_cipher_names();

            static int init_global_algorithm();
            static int cleanup_global_algorithm();

        private:
            method_t::type method_;
            int last_errorno_;
            const cipher_kt_t *cipher_kt_;
            std::vector<unsigned char> iv_;
            typedef struct {
                ::util::xxtea_key key;
            } xxtea_context_t;
            typedef struct {
                cipher_evp_t *enc; // used for encrypt
                cipher_evp_t *dec; // used for decrypt
            } cipher_context_t;
            union {
                cipher_context_t cipher_context_;
                xxtea_context_t xxtea_context_;
            };
        };
    } // namespace crypto
} // namespace util

#endif

#endif