#include <common/string_oprs.h>
#include <cstring>

#include <algorithm/crypto_cipher.h>
#include <std/static_assert.h>

#ifdef CRYPTO_ENABLED

namespace util {
    namespace crypto {
        namespace details {
            static inline cipher::error_code_t::type setup_errorno(cipher &ci, int err, cipher::error_code_t::type ret) {
                ci.set_last_errno(err);
                return ret;
            }

            static const char *supported_ciphers[] = {
                "xxtea",
                "rc4",
                "aes-128-cfb",
                "aes-192-cfb",
                "aes-256-cfb",
                "aes-128-ctr",
                "aes-192-ctr",
                "aes-256-ctr",
                "bf-cfb",
                "camellia-128-cfb",
                "camellia-192-cfb",
                "camellia-256-cfb",
                "chacha20",          // only available on openssl 1.1.0 and upper
                "chacha20-poly1305", // only available on openssl 1.1.0 and upper
                NULL,                // end
            };

#if !(defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)) && defined(CRYPTO_USE_MBEDTLS)
            static const char *supported_ciphers_mbedtls[] = {
                "xxtea",
                "ARC4-128",
                "AES-128-CFB128",
                "AES-192-CFB128",
                "AES-256-CFB128",
                "AES-128-CTR",
                "AES-192-CTR",
                "AES-256-CTR",
                "BLOWFISH-CFB64",
                "CAMELLIA-128-CFB128",
                "CAMELLIA-192-CFB128",
                "CAMELLIA-256-CFB128",
                "CHACHA20",          // only available on later mbedtls version, @see https://github.com/ARMmbed/mbedtls/pull/485
                "CHACHA20-POLY1305", // only available on later mbedtls version, @see https://github.com/ARMmbed/mbedtls/pull/485
                NULL,                // end
            };

            STD_STATIC_ASSERT(sizeof(supported_ciphers) == sizeof(supported_ciphers_mbedtls));
#endif
        }

        cipher::cipher() : method_(method_t::EN_CMT_INVALID), cipher_kt_(NULL) {}
        cipher::~cipher() { close(); }

        int cipher::init(const char *name, int mode) {
            if (method_ != method_t::EN_CMT_INVALID) {
                return details::setup_errorno(*this, -1, error_code_t::ALREADY_INITED);
            }

            if (NULL == name) {
                return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
            }

            if (0 == UTIL_STRFUNC_STRCASE_CMP(name, "xxtea")) {
                method_ = method_t::EN_CMT_XXTEA;
                return details::setup_errorno(*this, 0, error_code_t::OK);
            }

            cipher_kt_ = get_cipher_by_name(name);
            if (NULL == cipher_kt_) {
                return details::setup_errorno(*this, -1, error_code_t::CIPHER_NOT_SUPPORT);
            }

            int ret = error_code_t::OK;
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            do {
                if (mode & mode_t::EN_CMODE_ENCRYPT) {
                    cipher_context_.enc = EVP_CIPHER_CTX_new();

                    if (NULL == cipher_context_.enc) {
                        ret = details::setup_errorno(*this, -1, error_code_t::MALLOC);
                        break;
                    }
                    if (!(EVP_CipherInit_ex(cipher_context_.enc, cipher_kt_, NULL, NULL, NULL, 1))) {
                        ret = details::setup_errorno(*this, -1, error_code_t::CIPHER_OPERATION);
                        break;
                    }
                } else {
                    cipher_context_.enc = NULL;
                }

                if (mode & mode_t::EN_CMODE_DECRYPT) {
                    cipher_context_.dec = EVP_CIPHER_CTX_new();

                    if (NULL == cipher_context_.dec) {
                        ret = details::setup_errorno(*this, -1, error_code_t::MALLOC);
                        break;
                    }

                    if (!(EVP_CipherInit_ex(cipher_context_.dec, cipher_kt_, NULL, NULL, NULL, 0))) {
                        ret = details::setup_errorno(*this, -1, error_code_t::CIPHER_OPERATION);
                        break;
                    }
                } else {
                    cipher_context_.dec = NULL;
                }

            } while (false);

            if (error_code_t::OK != ret) {
                if ((mode & mode_t::EN_CMODE_ENCRYPT) && NULL != cipher_context_.enc) {
                    EVP_CIPHER_CTX_free(cipher_context_.enc);
                    cipher_context_.enc = NULL;
                }

                if ((mode & mode_t::EN_CMODE_DECRYPT) && NULL != cipher_context_.dec) {
                    EVP_CIPHER_CTX_free(cipher_context_.dec);
                    cipher_context_.dec = NULL;
                }
            }

#elif defined(CRYPTO_USE_MBEDTLS)
            do {
                if (mode & mode_t::EN_CMODE_ENCRYPT) {
                    cipher_context_.enc = (cipher_evp_t *)malloc(sizeof(cipher_evp_t));

                    if (NULL == cipher_context_.enc) {
                        ret = details::setup_errorno(*this, -1, error_code_t::MALLOC);
                        break;
                    }

                    memset(cipher_context_.enc, 0, sizeof(cipher_evp_t));
                    mbedtls_cipher_init(cipher_context_.enc);
                    int res;
                    if ((res = mbedtls_cipher_setup(cipher_context_.enc, cipher_kt_)) != 0) {
                        ret = details::setup_errorno(*this, res, error_code_t::CIPHER_OPERATION);
                        break;
                    }
                } else {
                    cipher_context_.enc = NULL;
                }

                if (mode & mode_t::EN_CMODE_DECRYPT) {
                    cipher_context_.dec = (cipher_evp_t *)malloc(sizeof(cipher_evp_t));

                    if (NULL == cipher_context_.dec) {
                        ret = details::setup_errorno(*this, -1, error_code_t::MALLOC);
                        break;
                    }

                    memset(cipher_context_.dec, 0, sizeof(cipher_evp_t));
                    mbedtls_cipher_init(cipher_context_.dec);
                    int res;
                    if ((res = mbedtls_cipher_setup(cipher_context_.dec, cipher_kt_)) != 0) {
                        ret = details::setup_errorno(*this, res, error_code_t::CIPHER_OPERATION);
                        break;
                    }
                } else {
                    cipher_context_.dec = NULL;
                }

            } while (false);

            if (error_code_t::OK != ret) {
                if ((mode & mode_t::EN_CMODE_ENCRYPT) && NULL != cipher_context_.enc) {
                    mbedtls_cipher_free(cipher_context_.enc);
                    free(cipher_context_.enc);
                    cipher_context_.enc = NULL;
                }

                if ((mode & mode_t::EN_CMODE_DECRYPT) && NULL != cipher_context_.dec) {
                    mbedtls_cipher_free(cipher_context_.dec);
                    free(cipher_context_.dec);
                    cipher_context_.dec = NULL;
                }
            }
#endif
            if (error_code_t::OK == ret) {
                method_ = method_t::EN_CMT_CIPHER;
            }

            return ret;
        }

        int cipher::close() {

            do {
                if (method_ == method_t::EN_CMT_INVALID) {
                    return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
                } else if (method_ == method_t::EN_CMT_XXTEA) {
                    // just do nothing when using xxtea
                    break;
                }

// cipher cleanup
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (NULL != cipher_context_.enc) {
                    EVP_CIPHER_CTX_free(cipher_context_.enc);
                    cipher_context_.enc = NULL;
                }

                if (NULL != cipher_context_.dec) {
                    EVP_CIPHER_CTX_free(cipher_context_.dec);
                    cipher_context_.dec = NULL;
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                if (NULL != cipher_context_.enc) {
                    mbedtls_cipher_free(cipher_context_.enc);
                    free(cipher_context_.enc);
                    cipher_context_.enc = NULL;
                }

                if (NULL != cipher_context_.dec) {
                    mbedtls_cipher_free(cipher_context_.dec);
                    free(cipher_context_.dec);
                    cipher_context_.dec = NULL;
                }
#endif
            } while (false);

            method_ = method_t::EN_CMT_INVALID;
            return details::setup_errorno(*this, 0, error_code_t::OK);
        }

        uint32_t cipher::get_iv_size() const {
            switch (method_) {
            case method_t::EN_CMT_INVALID:
            case method_t::EN_CMT_XXTEA:
                return 0;
            case method_t::EN_CMT_CIPHER:
                if (NULL != cipher_context_.enc) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t>(EVP_CIPHER_CTX_iv_length(cipher_context_.enc));
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t>(mbedtls_cipher_get_iv_size(cipher_context_.enc));
#endif
                } else if (NULL != cipher_context_.dec) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t>(EVP_CIPHER_CTX_iv_length(cipher_context_.dec));
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t>(mbedtls_cipher_get_iv_size(cipher_context_.dec));
#endif
                } else {
                    return 0;
                }
            default:
                return 0;
            }
        }

        uint32_t cipher::get_key_bits() const {
            switch (method_) {
            case method_t::EN_CMT_INVALID:
                return 0;
            case method_t::EN_CMT_XXTEA:
                return sizeof(::util::xxtea_key) * 8;
            case method_t::EN_CMT_CIPHER:
                if (NULL != cipher_context_.enc) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t>(EVP_CIPHER_CTX_key_length(cipher_context_.enc) * 8);
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t>(mbedtls_cipher_get_key_bitlen(cipher_context_.enc));
#endif
                } else if (NULL != cipher_context_.dec) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t>(EVP_CIPHER_CTX_key_length(cipher_context_.dec) * 8);
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t>(mbedtls_cipher_get_key_bitlen(cipher_context_.dec));
#endif
                } else {
                    return 0;
                }
            default:
                return 0;
            }
        }

        uint32_t cipher::get_block_size() const {
            switch (method_) {
            case method_t::EN_CMT_INVALID:
                return 0;
            case method_t::EN_CMT_XXTEA:
                return 0x04;
            case method_t::EN_CMT_CIPHER:
                if (NULL != cipher_context_.enc) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t>(EVP_CIPHER_CTX_block_size(cipher_context_.enc));
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t>(mbedtls_cipher_get_block_size(cipher_context_.enc));
#endif
                } else if (NULL != cipher_context_.dec) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t>(EVP_CIPHER_CTX_block_size(cipher_context_.dec));
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t>(mbedtls_cipher_get_block_size(cipher_context_.dec));
#endif
                } else {
                    return 0;
                }
            default:
                return 0;
            }
        }

        int cipher::set_key(const unsigned char *key, uint32_t key_bitlen) {
            switch (method_) {
            case method_t::EN_CMT_INVALID:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            case method_t::EN_CMT_XXTEA: {
                unsigned char secret[4 * sizeof(uint32_t)] = {0};
                if (key_bitlen >= sizeof(secret) * 8) {
                    memcpy(secret, key, sizeof(secret));
                } else {
                    memcpy(secret, key, key_bitlen / 8);
                }
                util::xxtea_setup(&xxtea_context_.key, secret);
                return details::setup_errorno(*this, 0, error_code_t::OK);
            }
            case method_t::EN_CMT_CIPHER: {
                int res = 0;
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (get_key_bits() < key_bitlen) {
                    return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
                }

                if (NULL != cipher_context_.enc) {
                    if (!EVP_CipherInit_ex(cipher_context_.enc, NULL, NULL, key, NULL, 1)) {
                        res = -1;
                    }
                }

                if (NULL != cipher_context_.dec) {
                    if (!EVP_CipherInit_ex(cipher_context_.dec, NULL, NULL, key, NULL, 0)) {
                        res = -1;
                    }
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                if (NULL != cipher_context_.enc) {
                    res = mbedtls_cipher_setkey(cipher_context_.enc, key, static_cast<int>(key_bitlen), MBEDTLS_ENCRYPT);
                }

                if (NULL != cipher_context_.dec) {
                    res = mbedtls_cipher_setkey(cipher_context_.dec, key, static_cast<int>(key_bitlen), MBEDTLS_DECRYPT);
                }
#endif
                if (res != 0) {
                    return details::setup_errorno(*this, res, error_code_t::CIPHER_OPERATION);
                }
                return details::setup_errorno(*this, res, error_code_t::OK);
            }
            default:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            }
        }

        int cipher::set_iv(const unsigned char *iv, size_t iv_len) {
            switch (method_) {
            case method_t::EN_CMT_INVALID:
            case method_t::EN_CMT_XXTEA:
                return error_code_t::OK;

            case method_t::EN_CMT_CIPHER: {
                int res = 0;
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (get_iv_size() > iv_len) {
                    return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
                }

                if (NULL != cipher_context_.enc) {
                    if (!EVP_CipherInit_ex(cipher_context_.enc, NULL, NULL, NULL, iv, 1)) {
                        res = -1;
                    }
                }

                if (NULL != cipher_context_.dec) {
                    if (!EVP_CipherInit_ex(cipher_context_.dec, NULL, NULL, NULL, iv, 0)) {
                        res = -1;
                    }
                }

#elif defined(CRYPTO_USE_MBEDTLS)
                if (NULL != cipher_context_.enc) {
                    res = mbedtls_cipher_set_iv(cipher_context_.enc, iv, iv_len);
                }

                if (NULL != cipher_context_.dec) {
                    res = mbedtls_cipher_set_iv(cipher_context_.dec, iv, iv_len);
                }
#endif
                if (res != 0) {
                    return details::setup_errorno(*this, res, error_code_t::CIPHER_OPERATION);
                }
                return details::setup_errorno(*this, res, error_code_t::OK);
            }
            default:
                return error_code_t::OK;
            }

            return error_code_t::OK;
        }

        int cipher::encrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen) {
            if (input == NULL || ilen <= 0 || output == NULL || NULL == olen || olen <= 0 || *olen < ilen + get_block_size()) {
                return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
            }

            switch (method_) {
            case method_t::EN_CMT_INVALID:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            case method_t::EN_CMT_XXTEA: {
                memcpy(output, input, ilen);
                *olen = ((ilen - 1) | 0x03) + 1;
                if (*olen > ilen) {
                    memset(output + ilen, 0, *olen - ilen);
                }
                util::xxtea_encrypt(&xxtea_context_.key, output, *olen);
                return details::setup_errorno(*this, 0, error_code_t::OK);
            }
            case method_t::EN_CMT_CIPHER: {
                if (NULL == cipher_context_.enc) {
                    return details::setup_errorno(*this, 0, error_code_t::CIPHER_DISABLED);
                }
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                int outl, finish_olen;
                if (!(EVP_CipherUpdate(cipher_context_.enc, output, &outl, input, ilen))) {
                    return details::setup_errorno(*this, -1, error_code_t::CIPHER_OPERATION);
                }

                if (!(EVP_CipherFinal_ex(cipher_context_.enc, output + outl, &finish_olen))) {
                    return details::setup_errorno(*this, -1, error_code_t::CIPHER_OPERATION);
                }

                *olen = static_cast<size_t>(outl + finish_olen);
                return error_code_t::OK;

#elif defined(CRYPTO_USE_MBEDTLS)
                size_t finish_olen;
                if ((last_errorno_ = mbedtls_cipher_reset(cipher_context_.enc)) != 0) {
                    return error_code_t::CIPHER_OPERATION;
                }

                if ((last_errorno_ = mbedtls_cipher_update(cipher_context_.enc, input, ilen, output, olen)) != 0) {
                    return error_code_t::CIPHER_OPERATION;
                }

                if ((last_errorno_ = mbedtls_cipher_finish(cipher_context_.enc, output + *olen, &finish_olen)) != 0) {
                    return error_code_t::CIPHER_OPERATION;
                }

                *olen += finish_olen;
                return error_code_t::OK;
#endif
            }
            default:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            }
        }

        int cipher::decrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen) {
            if (input == NULL || ilen <= 0 || output == NULL || NULL == olen || olen <= 0 || *olen < ilen + get_block_size()) {
                return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
            }

            switch (method_) {
            case method_t::EN_CMT_INVALID:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            case method_t::EN_CMT_XXTEA: {
                memcpy(output, input, ilen);
                *olen = ((ilen - 1) | 0x03) + 1;
                if (*olen > ilen) {
                    memset(reinterpret_cast<char *>(output) + ilen, 0, *olen - ilen);
                }
                util::xxtea_decrypt(&xxtea_context_.key, output, *olen);
                return details::setup_errorno(*this, 0, error_code_t::OK);
            }
            case method_t::EN_CMT_CIPHER: {
                if (NULL == cipher_context_.dec) {
                    return details::setup_errorno(*this, 0, error_code_t::CIPHER_DISABLED);
                }
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                int outl, finish_olen;
                if (!(EVP_CipherUpdate(cipher_context_.dec, output, &outl, input, ilen))) {
                    return details::setup_errorno(*this, -1, error_code_t::CIPHER_OPERATION);
                }

                if (!(EVP_CipherFinal_ex(cipher_context_.dec, output + outl, &finish_olen))) {
                    return details::setup_errorno(*this, -1, error_code_t::CIPHER_OPERATION);
                }

                *olen = static_cast<size_t>(outl + finish_olen);

                return error_code_t::OK;
#elif defined(CRYPTO_USE_MBEDTLS)
                size_t finish_olen;
                if ((last_errorno_ = mbedtls_cipher_reset(cipher_context_.dec)) != 0) {
                    return error_code_t::CIPHER_OPERATION;
                }

                if ((last_errorno_ = mbedtls_cipher_update(cipher_context_.dec, input, ilen, output, olen)) != 0) {
                    return error_code_t::CIPHER_OPERATION;
                }

                if ((last_errorno_ = mbedtls_cipher_finish(cipher_context_.dec, output + *olen, &finish_olen)) != 0) {
                    return error_code_t::CIPHER_OPERATION;
                }

                *olen += finish_olen;
                return error_code_t::OK;
#endif
            }
            default:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            }
        }

        const cipher::cipher_kt_t *cipher::get_cipher_by_name(const char *name) {
            size_t index = 0;
            for (; NULL != details::supported_ciphers[index]; ++index) {
                if (0 == UTIL_STRFUNC_STRCASE_CMP(name, details::supported_ciphers[index])) {
                    break;
                }
            }

            if (NULL == details::supported_ciphers[index]) {
                return NULL;
            }

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            return EVP_get_cipherbyname(details::supported_ciphers[index]);
#elif defined(CRYPTO_USE_MBEDTLS)
            return mbedtls_cipher_info_from_string(details::supported_ciphers_mbedtls[index]);
#endif
        }

        std::pair<const char *, const char *> cipher::ciphertok(const char *in) {
            std::pair<const char *, const char *> ret;
            ret.first = NULL;
            ret.second = NULL;
            if (NULL == in) {
                return ret;
            }

            const char *b = in;
            const char *e;
            // skip \r\n\t and space
            while (0 != *b && (*b == ' ' || *b == '\t' || *b == '\r' || *b == '\n' || *b == ';' || *b == ',' || *b == ':')) {
                ++b;
            }

            for (e = b; 0 != *e; ++e) {
                if (*e == ' ' || *e == '\t' || *e == '\r' || *e == '\n' || *e == ';' || *e == ',' || *e == ':') {
                    break;
                }
            }

            if (e <= b) {
                return ret;
            }

            ret.first = b;
            ret.second = e;
            return ret;
        }

        const std::vector<std::string> &cipher::get_all_cipher_names() {
            static std::vector<std::string> ret;
            if (ret.empty()) {
                const size_t inner_algorithm_sz = 1;
                for (size_t i = 0; i < inner_algorithm_sz; ++i) {
                    ret.push_back(details::supported_ciphers[i]);
                }

                for (size_t i = inner_algorithm_sz; NULL != details::supported_ciphers[i]; ++i) {
                    if (NULL != get_cipher_by_name(details::supported_ciphers[i])) {
                        ret.push_back(details::supported_ciphers[i]);
                    }
                }
            }
            return ret;
        }

        int cipher::init_global_algorithm() {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            OpenSSL_add_all_algorithms();
#endif
            return 0;
        }
    }
}

#endif