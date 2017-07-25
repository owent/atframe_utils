
#include <common/string_oprs.h>

#include <algorithm/crypto_cipher.h>

#ifdef CRYPTO_ENABLED

namespace util {
    namespace crypto {
        namespace details {
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
                "chacha20",         // only available on openssl 1.1.0 and upper
                "chacha20-poly1305" // only available on openssl 1.1.0 and upper
            };

#if defined(CRYPTO_USE_MBEDTLS)
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
                "CHACHA20",         // only available on later mbedtls version, @see https://github.com/ARMmbed/mbedtls/pull/485
                "CHACHA20-POLY1305" // only available on later mbedtls version, @see https://github.com/ARMmbed/mbedtls/pull/485
            };
#endif
        }

        cipher::cipher() : method_(method_t::EN_CMT_INVALID), cipher_kt_(NULL) {}
        cipher::~cipher() { close(); }

        bool cipher::init(const char *name, int mode) {
            if (method_ != method_t::EN_CMT_INVALID) {
                return false;
            }

            if (NULL == name) {
                return false;
            }

            if (0 == UTIL_STRFUNC_STRCASE_CMP(name, "xxtea")) {
                method_ = method_t::EN_CMT_XXTEA;
                return true;
            }

            cipher_kt_ = get_cipher_by_name(name);
            if (NULL == cipher_kt_) {
                return false;
            }

            bool ret = true;
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            do {
                if (mode & mode_t::EN_CMODE_ENCRYPT) {
                    cipher_context_.enc = EVP_CIPHER_CTX_new();

                    if (NULL == cipher_context_.enc) {
                        ret = false;
                        break;
                    }
                    if (!EVP_CipherInit_ex(cipher_context_.enc, cipher_kt_, NULL, NULL, NULL, 1)) {
                        ret = false;
                        break;
                    }
                } else {
                    cipher_context_.enc = NULL;
                }

                if (mode & mode_t::EN_CMODE_DECRYPT) {
                    cipher_context_.dec = EVP_CIPHER_CTX_new();

                    if (NULL == cipher_context_.dec) {
                        ret = false;
                        break;
                    }

                    if (!EVP_CipherInit_ex(cipher_context_.dec, cipher_kt_, NULL, NULL, NULL, 0)) {
                        ret = false;
                        break;
                    }
                } else {
                    cipher_context_.dec = NULL;
                }

            } while (false);

            if (false == ret) {
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
                        ret = false;
                        break;
                    }

                    memset(cipher_context_.enc, 0, sizeof(cipher_evp_t));
                    mbedtls_cipher_init(cipher_context_.enc);
                    if (mbedtls_cipher_setup(cipher_context_.enc, cipher_kt_) != 0) {
                        ret = false;
                        break;
                    }
                } else {
                    cipher_context_.enc = NULL;
                }

                if (mode & mode_t::EN_CMODE_DECRYPT) {
                    cipher_context_.dec = (cipher_evp_t *)malloc(sizeof(cipher_evp_t));

                    if (NULL == cipher_context_.dec) {
                        ret = false;
                        break;
                    }

                    memset(cipher_context_.dec, 0, sizeof(cipher_evp_t));
                    mbedtls_cipher_init(cipher_context_.dec);
                    if (mbedtls_cipher_setup(cipher_context_.dec, cipher_kt_) != 0) {
                        ret = false;
                        break;
                    }
                } else {
                    cipher_context_.dec = NULL;
                }

            } while (false);

            if (false == ret) {
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
            if (ret) {
                method_ = method_t::EN_CMT_CIPHER;
            }
            return ret;
        }

        bool cipher::close() {

            do {
                if (method_ == method_t::EN_CMT_INVALID) {
                    return false;
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
            return true;
        }

        uint32_t cipher::get_iv_size() const {
            switch (method_) {
            case method_t::EN_CMT_INVALID:
            case method_t::EN_CMT_XXTEA:
                return 0;
            case method_t::EN_CMT_CIPHER:
                if (NULL != cipher_context_.enc) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t> EVP_CIPHER_CTX_iv_length(cipher_context_.enc);
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t> mbedtls_cipher_get_iv_size(cipher_context_.enc);
#endif
                } else if (NULL != cipher_context_.dec) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t> EVP_CIPHER_CTX_iv_length(cipher_context_.dec);
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t> mbedtls_cipher_get_iv_size(cipher_context_.dec);
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
                    return static_cast<uint32_t> EVP_CIPHER_CTX_key_length(cipher_context_.enc) * 8;
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t> mbedtls_cipher_get_key_bitlen(cipher_context_.enc);
#endif
                } else if (NULL != cipher_context_.dec) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t> EVP_CIPHER_CTX_key_length(cipher_context_.dec) * 8;
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t> mbedtls_cipher_get_key_bitlen(cipher_context_.dec);
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
                    return static_cast<uint32_t> EVP_CIPHER_CTX_block_size(cipher_context_.enc);
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t> mbedtls_cipher_get_block_size(cipher_context_.enc);
#endif
                } else if (NULL != cipher_context_.dec) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                    return static_cast<uint32_t> EVP_CIPHER_CTX_block_size(cipher_context_.dec);
#elif defined(CRYPTO_USE_MBEDTLS)
                    return static_cast<uint32_t> mbedtls_cipher_get_block_size(cipher_context_.dec);
#endif
                } else {
                    return 0;
                }
            default:
                return 0;
            }
        }

        int cipher::encrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen) {
            if (input == NULL || ilen <= 0 || output == NULL || olen <= 0 || *olen < ilen + get_block_size()) {
                return -1;
            }

            switch (method_) {
            case method_t::EN_CMT_INVALID:
                return -2;
            case method_t::EN_CMT_XXTEA: {
                memcpy(output, input, ilen);
                *olen = ((ilen - 1) | 0x03) + 1;
                if (*olen > ilen) {
                    memset(reinterpret_cast<char *>(buffer) + insz, 0, *olen - insz);
                }
                util::xxtea_encrypt(&xxtea_context_.key, output, *olen);
                return 0;
            }
            case method_t::EN_CMT_CIPHER:
                if (NULL == cipher_context_.enc) {
                    return -3;
                }
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
#elif defined(CRYPTO_USE_MBEDTLS)
#endif
            default:
                return -2;
            }
        }

        int cipher::decrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen) {
            if (input == NULL || ilen <= 0 || output == NULL || olen <= 0 || *olen < ilen + get_block_size()) {
                return -1;
            }

            switch (method_) {
            case method_t::EN_CMT_INVALID:
                return -2;
            case method_t::EN_CMT_XXTEA: {
                memcpy(output, input, ilen);
                *olen = ((ilen - 1) | 0x03) + 1;
                if (*olen > ilen) {
                    memset(reinterpret_cast<char *>(buffer) + insz, 0, *olen - insz);
                }
                util::xxtea_decrypt(&xxtea_context_.key, output, *olen);
                return 0;
            }
            case method_t::EN_CMT_CIPHER:
                if (NULL == cipher_context_.enc) {
                    return -3;
                }
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
#elif defined(CRYPTO_USE_MBEDTLS)
#endif
            default:
                return -2;
            }
        }

        const cipher::cipher_kt_t *cipher::get_cipher_by_name(const char *name) {
            size_t sz = sizeof(details::supported_ciphers) / sizeof(const char *);
            size_t index = 0;
            for (; index < sz; ++index) {
                if (0 == UTIL_STRFUNC_STRCASE_CMP(name, details::supported_ciphers[index])) {
                    break;
                }
            }

            if (index >= sz) {
                return NULL;
            }

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            return EVP_get_cipherbyname(details::supported_ciphers[index]);
#elif defined(CRYPTO_USE_MBEDTLS)
            return mbedtls_cipher_info_from_string(details::supported_ciphers_mbedtls[index]);
#endif
        }

        void cipher::split_ciphers(const std::string &in, std::vector<std::string> &out) {
            size_t b = 0, e;

            while (b < in.size()) {
                // skip \r\n\t and space
                if (in[b] == ' ' || in[b] == '\t' || in[b] == '\r' || in[b] == '\n' || in[b] == ';') {
                    ++b;
                    continue;
                }

                for (e = b + 1; e < in.size(); ++e) {
                    char c = in[e];
                    if (c == ' ' || c == '\t' || c == '\r' || c == '\n' || c == ';') {
                        break;
                    }
                }

                out.push_back(in.substr(b, e - b));
                b = e;
            }
        }
    }
}

#endif