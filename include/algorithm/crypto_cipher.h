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

#include <openssl/err.h>
#include <openssl/evp.h>

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
        struct cipher_interface_info_t;

        class cipher {
        public:
            struct LIBATFRAME_UTILS_API mode_t {
                enum type { EN_CMODE_ENCRYPT = 0x01, EN_CMODE_DECRYPT = 0x02 };
            };

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            typedef EVP_CIPHER     cipher_kt_t;
            typedef EVP_CIPHER_CTX cipher_evp_t;
            typedef EVP_MD         digest_type_t;
            enum {
                MAX_KEY_LENGTH = EVP_MAX_KEY_LENGTH, //
                MAX_IV_LENGTH  = EVP_MAX_IV_LENGTH,  //
                MAX_MD_SIZE    = EVP_MAX_MD_SIZE
            };

#elif defined(CRYPTO_USE_MBEDTLS)
            typedef mbedtls_cipher_info_t    cipher_kt_t;
            typedef mbedtls_cipher_context_t cipher_evp_t;
            typedef mbedtls_md_info_t        digest_type_t;
            enum {
                MAX_KEY_LENGTH = 64, //
                MAX_IV_LENGTH  = MBEDTLS_MAX_IV_LENGTH,
                MAX_MD_SIZE    = MBEDTLS_MD_MAX_SIZE
            };
#endif

            struct LIBATFRAME_UTILS_API error_code_t {
                enum type {
                    OK                          = 0,
                    INVALID_PARAM               = -1,
                    NOT_INITED                  = -2,
                    ALREADY_INITED              = -3,
                    MALLOC                      = -4,
                    CIPHER_DISABLED             = -11,
                    CIPHER_NOT_SUPPORT          = -12,
                    CIPHER_OPERATION            = -13,
                    CIPHER_OPERATION_SET_IV     = -14,
                    LIBSODIUM_OPERATION         = -15,
                    LIBSODIUM_OPERATION_TAG_LEN = -16,
                    MUST_CALL_AEAD_API          = -21,
                    MUST_NOT_CALL_AEAD_API      = -22,
                };
            };

        public:
            LIBATFRAME_UTILS_API cipher();
            LIBATFRAME_UTILS_API ~cipher();

            LIBATFRAME_UTILS_API int init(const char *name, int mode = mode_t::EN_CMODE_ENCRYPT | mode_t::EN_CMODE_DECRYPT);
            LIBATFRAME_UTILS_API int close();

            /**
             * @brief set last error returned by crypto library
             * @param err error code returned by crypto library
             */
            LIBATFRAME_UTILS_API void set_last_errno(int64_t e);
            /**
             * @brief get last error returned by crypto library
             * @return last error code returned by crypto library
             */
            LIBATFRAME_UTILS_API int64_t get_last_errno() const;

            /**
             * @brief if it's a AEAD cipher
             * @see https://en.wikipedia.org/wiki/Authenticated_encryption
             * @return if it's AEAD cipher, return true
             */
            LIBATFRAME_UTILS_API bool is_aead() const;

            /**
             * @brief               get iv size in crypt library
             * @return              iv length in bytes
             */
            LIBATFRAME_UTILS_API uint32_t get_iv_size() const;

            /**libsodium_context_
             * @brief               get key length in bits
             * @return              key length in bits
             */
            LIBATFRAME_UTILS_API uint32_t get_key_bits() const;

            /**
             * @brief               get block size in crypt library
             * @return              block length in bytes
             */
            LIBATFRAME_UTILS_API uint32_t get_block_size() const;

            /**
             * @brief               set key
             * @param key           key
             * @param key_bitlen    key length to use, in bits. must equal or greater to get_key_bits()
             * @return              0 or error code less than 0
             */
            LIBATFRAME_UTILS_API int set_key(const unsigned char *key, uint32_t key_bitlen);

            /**
             * @brief               selibsodium_context_t initialization vector
             * @param iv            iv value
             * @param iv_len        length of iv, in bytes, can not be greater than 16 when using mbedtls
             * @note                if using chacha20,chacha20-ietf,xchacha20,salsa20 or xsalsa20, the first
             *                      8 bytes is the counter, and the rest is the nonce.
             * @return              0 or error code less than 0
             */
            LIBATFRAME_UTILS_API int set_iv(const unsigned char *iv, size_t iv_len);

            /**
             * @brief               clear initialization vector
             * @return              0 or error code less than 0
             */
            LIBATFRAME_UTILS_API void clear_iv();

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
            LIBATFRAME_UTILS_API int encrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen);

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
            LIBATFRAME_UTILS_API int decrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen);


            /**
             * @biref               encrypt data
             * @param input         buffer holding the input data
             * @param ilen          length of the input data
             * @param output        buffer for the output data. Should be able to hold at
             *                      least ilen + block_size. Cannot be the same buffer as
             *                      input!
             * @param olen          length of the output data, will be filled with the
             *                      actual number of bytes written.
             * @param ad            Additional data to authenticate.
             * @param ad_len        Length of ad. ad_len must not be greater than 0xFF00
             * @param tag           buffer for the authentication tag
             * @param tag_len       desired tag length, tag_len must between [4, 16] and tag_len % 2 == 0
             * @return              0 or error code
             */
            LIBATFRAME_UTILS_API int encrypt_aead(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen,
                                                  const unsigned char *ad, size_t ad_len, unsigned char *tag, size_t tag_len);

            /**
             * @biref               decrypt data
             * @param input         buffer holding the input data
             * @param ilen          length of the input data
             * @param output        buffer for the output data. Should be able to hold at
             *                      least ilen + block_size. Cannot be the same buffer as
             *                      input!
             * @param olen          length of the output data, will be filled with the
             *                      actual number of bytes written.
             * @param ad            Additional data to be authenticated.
             * @param ad_len        Length of ad. ad_len must not be greater than 0xFF00
             * @param tag           buffer holding the authentication tag
             * @param tag_len       length of the authentication tag
             * @return              0 or error code
             */
            LIBATFRAME_UTILS_API int decrypt_aead(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen,
                                                  const unsigned char *ad, size_t ad_len, const unsigned char *tag, size_t tag_len);

        public:
            static LIBATFRAME_UTILS_API const cipher_kt_t *get_cipher_by_name(const char *name);
            /**
             * @biref               split cipher names by space, comma, semicolon or colon
             * @param in            string contain some cipher names
             * @return              begin(first) and end(second) address of a cipher name, both NULL if not found.
             *                      you can use second pointer as the paramter of next call, just like strtok
             */
            static LIBATFRAME_UTILS_API std::pair<const char *, const char *> ciphertok(const char *in);
            static LIBATFRAME_UTILS_API const std::vector<std::string> &get_all_cipher_names();

            static LIBATFRAME_UTILS_API int init_global_algorithm();
            static LIBATFRAME_UTILS_API int cleanup_global_algorithm();


        private:
            UTIL_SYMBOL_HIDDEN int init_with_cipher(const cipher_interface_info_t *interface, int mode);
            UTIL_SYMBOL_HIDDEN int close_with_cipher();

        private:
            const cipher_interface_info_t *interface_;
            int64_t                        last_errorno_;
            const cipher_kt_t *            cipher_kt_;
            std::vector<unsigned char>     iv_;
            struct xxtea_context_t {
                ::util::xxtea_key key;
            };
            struct libsodium_context_t {
                unsigned char key[32];
            };
            struct cipher_context_t {
                cipher_evp_t *enc; // used for encrypt
                cipher_evp_t *dec; // used for decrypt
            };
            union {
                cipher_context_t    cipher_context_;
                xxtea_context_t     xxtea_context_;
                libsodium_context_t libsodium_context_;
            };
        };
    } // namespace crypto
} // namespace util

#endif

#endif