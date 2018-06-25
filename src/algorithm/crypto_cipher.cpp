#include <common/string_oprs.h>
#include <cstring>

#include <algorithm/crypto_cipher.h>
#include <std/static_assert.h>

#ifdef CRYPTO_CIPHER_ENABLED

namespace util {
    namespace crypto {
        enum cipher_interface_method_t {
            EN_CIMT_INVALID = 0, // inner
            EN_CIMT_XXTEA = 1,   // inner
            EN_CIMT_INNER,       // inner bound
            EN_CIMT_CIPHER,      // using openssl/libressl/boringssl/mbedtls
            EN_CIMT_LIBSODIUM,   // using libsodium
        };

        /**
         * @note boringssl macros
         *       OPENSSL_IS_BORINGSSL
         *       BORINGSSL_API_VERSION
         */


        enum cipher_interface_flags_t {
            EN_CIFT_NONE = 0,                    // using inner algorithm
            EN_CIFT_AEAD = 0x01,                 // is aead cipher
            EN_CIFT_DECRYPT_NO_PADDING = 0x0100, // is aead cipher
            EN_CIFT_ENCRYPT_NO_PADDING = 0x0200, // is aead cipher

            EN_CIFT_MBEDTLS_NO_FINISH = 0x010000, // is aead cipher
        };

        struct cipher_interface_info_t {
            const char *name;
            cipher_interface_method_t method;
            const char *mbedtls_name;
            uint32_t flags;
        };

        namespace details {
            static inline cipher::error_code_t::type setup_errorno(cipher &ci, int64_t err, cipher::error_code_t::type ret) {
                ci.set_last_errno(err);
                return ret;
            }

            // openssl/libressl/boringssl   @see crypto/objects/obj_dat.h or crypto/obj/obj_dat.h
            // mbedtls                      @see library/cipher_wrap.c
            static const cipher_interface_info_t supported_ciphers[] = {
                {"xxtea", EN_CIMT_XXTEA, "xxtea", EN_CIFT_NONE},
                {"rc4", EN_CIMT_CIPHER, "ARC4-128", EN_CIFT_NONE},
                {"aes-128-cfb", EN_CIMT_CIPHER, "AES-128-CFB128", EN_CIFT_NONE},
                {"aes-192-cfb", EN_CIMT_CIPHER, "AES-192-CFB128", EN_CIFT_NONE},
                {"aes-256-cfb", EN_CIMT_CIPHER, "AES-256-CFB128", EN_CIFT_NONE},
                {"aes-128-ctr", EN_CIMT_CIPHER, "AES-128-CTR", EN_CIFT_NONE},
                {"aes-192-ctr", EN_CIMT_CIPHER, "AES-192-CTR", EN_CIFT_NONE},
                {"aes-256-ctr", EN_CIMT_CIPHER, "AES-256-CTR", EN_CIFT_NONE},
                {"aes-128-ecb", EN_CIMT_CIPHER, "AES-128-ECB", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"aes-192-ecb", EN_CIMT_CIPHER, "AES-192-ECB", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"aes-256-ecb", EN_CIMT_CIPHER, "AES-256-ECB", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"aes-128-cbc", EN_CIMT_CIPHER, "AES-128-CBC", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"aes-192-cbc", EN_CIMT_CIPHER, "AES-192-CBC", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"aes-256-cbc", EN_CIMT_CIPHER, "AES-256-CBC", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"des-ecb", EN_CIMT_CIPHER, "DES-ECB", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"des-cbc", EN_CIMT_CIPHER, "DES-CBC", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"des-ede", EN_CIMT_CIPHER, "DES-EDE-ECB", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"des-ede-cbc", EN_CIMT_CIPHER, "DES-EDE-CBC", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"des-ede3", EN_CIMT_CIPHER, "DES-EDE3-ECB", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"des-ede3-cbc", EN_CIMT_CIPHER, "DES-EDE3-CBC", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                // {"bf-ecb", EN_CIMT_CIPHER, "BLOWFISH-ECB", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                // this unit test of this cipher can not passed in all libraries
                {"bf-cbc", EN_CIMT_CIPHER, "BLOWFISH-CBC", EN_CIFT_ENCRYPT_NO_PADDING | EN_CIFT_DECRYPT_NO_PADDING},
                {"bf-cfb", EN_CIMT_CIPHER, "BLOWFISH-CFB64", EN_CIFT_NONE},
                {"camellia-128-cfb", EN_CIMT_CIPHER, "CAMELLIA-128-CFB128", EN_CIFT_NONE},
                {"camellia-192-cfb", EN_CIMT_CIPHER, "CAMELLIA-192-CFB128", EN_CIFT_NONE},
                {"camellia-256-cfb", EN_CIMT_CIPHER, "CAMELLIA-256-CFB128", EN_CIFT_NONE},
                {"chacha20", // only available on openssl 1.1.0 and upper
                 EN_CIMT_CIPHER,
                 "CHACHA20", // only available on later mbedtls version, @see https://github.com/ARMmbed/mbedtls/pull/485
                 EN_CIFT_NONE},

                {"aes-128-gcm", EN_CIMT_CIPHER, "AES-128-GCM", EN_CIFT_AEAD},
                {"aes-192-gcm", EN_CIMT_CIPHER, "AES-192-GCM", EN_CIFT_AEAD},
                {"aes-256-gcm", EN_CIMT_CIPHER, "AES-256-GCM", EN_CIFT_AEAD},
                {"aes-128-ccm", EN_CIMT_CIPHER, "AES-128-CCM", EN_CIFT_AEAD},
                {"aes-192-ccm", EN_CIMT_CIPHER, "AES-192-CCM", EN_CIFT_AEAD},
                {"aes-256-ccm", EN_CIMT_CIPHER, "AES-256-CCM", EN_CIFT_AEAD},
                {"chacha20-poly1305", // only available on openssl 1.1.0 and upper or boringssl
                 EN_CIMT_CIPHER,
                 "CHACHA20-POLY1305", // only available on later mbedtls version, @see https://github.com/ARMmbed/mbedtls/pull/485
                 EN_CIFT_AEAD},
                {NULL, EN_CIMT_INVALID, NULL, false}, // end
            };

            static const cipher_interface_info_t *get_cipher_interface_by_name(const char *name) {
                if (NULL == name) {
                    return NULL;
                }

                for (size_t i = 0; NULL != details::supported_ciphers[i].name; ++i) {
                    if (0 == UTIL_STRFUNC_STRCASE_CMP(name, details::supported_ciphers[i].name)) {
                        return &details::supported_ciphers[i];
                    }
                }

                return NULL;
            }
        } // namespace details

        cipher::cipher() : interface_(NULL), last_errorno_(0), cipher_kt_(NULL) {}
        cipher::~cipher() { close(); }

        int cipher::init(const char *name, int mode) {
            if (NULL != interface_ && interface_->method != EN_CIMT_INVALID) {
                return details::setup_errorno(*this, -1, error_code_t::ALREADY_INITED);
            }

            if (NULL == name) {
                return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
            }

            const cipher_interface_info_t *interface = details::get_cipher_interface_by_name(name);
            if (NULL == interface) {
                return details::setup_errorno(*this, -1, error_code_t::CIPHER_NOT_SUPPORT);
            }

            int ret = error_code_t::OK;

            switch (interface->method) {
            case EN_CIMT_XXTEA:
                break;
            case EN_CIMT_CIPHER:
                ret = init_with_cipher(interface, mode);
                break;
            default:
                ret = details::setup_errorno(*this, -1, error_code_t::CIPHER_NOT_SUPPORT);
                break;
            }

            if (error_code_t::OK == ret) {
                interface_ = interface;
            }

            return ret;
        }

        int cipher::init_with_cipher(const cipher_interface_info_t *interface, int mode) {
            if (NULL == interface) {
                return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
            }

            if (interface->method != EN_CIMT_CIPHER) {
                return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
            }

            cipher_kt_ = get_cipher_by_name(interface->name);
            if (NULL == cipher_kt_) {
                return details::setup_errorno(*this, -1, error_code_t::CIPHER_NOT_SUPPORT);
            }

            int ret = error_code_t::OK;
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            do {
                if (mode & mode_t::EN_CMODE_ENCRYPT) {
                    cipher_context_.enc = EVP_CIPHER_CTX_new();

                    if (NULL == cipher_context_.enc) {
                        ret = details::setup_errorno(*this, ERR_peek_error(), error_code_t::MALLOC);
                        break;
                    }
                    if (!(EVP_CipherInit_ex(cipher_context_.enc, cipher_kt_, NULL, NULL, NULL, 1))) {
                        ret = details::setup_errorno(*this, ERR_peek_error(), error_code_t::CIPHER_OPERATION);
                        break;
                    }
                } else {
                    cipher_context_.enc = NULL;
                }

                if (mode & mode_t::EN_CMODE_DECRYPT) {
                    cipher_context_.dec = EVP_CIPHER_CTX_new();

                    if (NULL == cipher_context_.dec) {
                        ret = details::setup_errorno(*this, ERR_peek_error(), error_code_t::MALLOC);
                        break;
                    }

                    if (!(EVP_CipherInit_ex(cipher_context_.dec, cipher_kt_, NULL, NULL, NULL, 0))) {
                        ret = details::setup_errorno(*this, ERR_peek_error(), error_code_t::CIPHER_OPERATION);
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

                    // memset(cipher_context_.enc, 0, sizeof(cipher_evp_t)); // call memset in mbedtls_cipher_init
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

                    // memset(cipher_context_.dec, 0, sizeof(cipher_evp_t)); // call memset in mbedtls_cipher_init
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
#else
            return details::setup_errorno(*this, -1, error_code_t::CIPHER_NOT_SUPPORT);
#endif
            return ret;
        }

        int cipher::close() {
            if (NULL == interface_ || interface_->method == EN_CIMT_INVALID) {
                return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
            }

            int ret = error_code_t::OK;
            switch (interface_->method) {
            case EN_CIMT_XXTEA:
                // just do nothing when using xxtea
                ret = details::setup_errorno(*this, 0, error_code_t::OK);
                break;

            case EN_CIMT_CIPHER:
                ret = close_with_cipher();
                break;

            default:
                ret = details::setup_errorno(*this, 0, error_code_t::CIPHER_NOT_SUPPORT);
                break;
            }

            interface_ = NULL;
            return ret;
        }

        int cipher::close_with_cipher() {
            if (NULL == interface_) {
                return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
            }

            if (interface_->method != EN_CIMT_CIPHER) {
                return details::setup_errorno(*this, 0, error_code_t::INVALID_PARAM);
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

            return details::setup_errorno(*this, 0, error_code_t::OK);
        }

        bool cipher::is_aead() const {
            if (NULL == interface_) {
                return false;
            }

            return 0 != (interface_->flags & EN_CIFT_AEAD);
        }

        uint32_t cipher::get_iv_size() const {
            if (NULL == interface_) {
                return 0;
            }

            switch (interface_->method) {
            case EN_CIMT_INVALID:
            case EN_CIMT_XXTEA:
                return 0;
            case EN_CIMT_CIPHER:
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
            if (NULL == interface_) {
                return 0;
            }

            switch (interface_->method) {
            case EN_CIMT_INVALID:
                return 0;
            case EN_CIMT_XXTEA:
                return sizeof(::util::xxtea_key) * 8;
            case EN_CIMT_CIPHER:
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
            if (NULL == interface_) {
                return 0;
            }

            switch (interface_->method) {
            case EN_CIMT_INVALID:
                return 0;
            case EN_CIMT_XXTEA:
                return 0x04;
            case EN_CIMT_CIPHER:
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
            if (NULL == interface_) {
                return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
            }

            switch (interface_->method) {
            case EN_CIMT_INVALID:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            case EN_CIMT_XXTEA: {
                unsigned char secret[4 * sizeof(uint32_t)] = {0};
                if (key_bitlen >= sizeof(secret) * 8) {
                    memcpy(secret, key, sizeof(secret));
                } else {
                    memcpy(secret, key, key_bitlen / 8);
                }
                util::xxtea_setup(&xxtea_context_.key, secret);
                return details::setup_errorno(*this, 0, error_code_t::OK);
            }
            case EN_CIMT_CIPHER: {
                int res = 0;
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                if (get_key_bits() > key_bitlen) {
                    return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
                }

                if (NULL != cipher_context_.enc) {
                    if (!EVP_CipherInit_ex(cipher_context_.enc, NULL, NULL, key, NULL, -1)) {
                        res = ERR_peek_error();
                    }
                }

                if (NULL != cipher_context_.dec) {
                    if (!EVP_CipherInit_ex(cipher_context_.dec, NULL, NULL, key, NULL, -1)) {
                        res = ERR_peek_error();
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
            if (NULL == interface_ || interface_->method == EN_CIMT_INVALID) {
                return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
            }

            switch (interface_->method) {
            case EN_CIMT_INVALID:
            case EN_CIMT_XXTEA:
                return error_code_t::OK;

            case EN_CIMT_CIPHER: {
                int res = 0;
                if (get_iv_size() > iv_len) {
                    return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
                }

                iv_.assign(iv, iv + iv_len);
                return details::setup_errorno(*this, res, error_code_t::OK);
            }
            default:
                return error_code_t::OK;
            }
        }

        void cipher::clear_iv() { iv_.clear(); }

        int cipher::encrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen) {
            if (NULL == interface_ || interface_->method == EN_CIMT_INVALID) {
                return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
            }

            if (input == NULL || ilen <= 0 || output == NULL || NULL == olen || *olen <= 0 || *olen < ilen + get_block_size()) {
                return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
            }

            switch (interface_->method) {
            case EN_CIMT_INVALID:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            case EN_CIMT_XXTEA: {
                util::xxtea_encrypt(&xxtea_context_.key, reinterpret_cast<const void *>(input), ilen, reinterpret_cast<void *>(output),
                                    olen);
                return details::setup_errorno(*this, 0, error_code_t::OK);
            }
            case EN_CIMT_CIPHER: {
                if (NULL == cipher_context_.enc) {
                    return details::setup_errorno(*this, 0, error_code_t::CIPHER_DISABLED);
                }

                if (iv_.empty()) {
                    if (0 != get_iv_size()) {
                        iv_.resize(get_iv_size(), 0);
                    }
                }

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                int outl, finish_olen;

                if (!iv_.empty()) {
                    if (!EVP_CipherInit_ex(cipher_context_.enc, NULL, NULL, NULL, &iv_[0], -1)) {
                        return details::setup_errorno(*this, ERR_peek_error(), error_code_t::CIPHER_OPERATION);
                    }
                }

                if (0 != (interface_->flags & EN_CIFT_ENCRYPT_NO_PADDING)) {
                    EVP_CIPHER_CTX_set_padding(cipher_context_.enc, 0);
                }

                if (!(EVP_CipherUpdate(cipher_context_.enc, output, &outl, input, static_cast<int>(ilen)))) {
                    return details::setup_errorno(*this, ERR_peek_error(), error_code_t::CIPHER_OPERATION);
                }

                if (!(EVP_CipherFinal_ex(cipher_context_.enc, output + outl, &finish_olen))) {
                    return details::setup_errorno(*this, ERR_peek_error(), error_code_t::CIPHER_OPERATION);
                }

                *olen = static_cast<size_t>(outl + finish_olen);
                return error_code_t::OK;

#elif defined(CRYPTO_USE_MBEDTLS)
                if (0 != (interface_->flags & EN_CIFT_ENCRYPT_NO_PADDING) && MBEDTLS_MODE_CBC == cipher_context_.enc->cipher_info->mode) {
                    if ((last_errorno_ = mbedtls_cipher_set_padding_mode(cipher_context_.enc, MBEDTLS_PADDING_NONE)) != 0) {
                        return error_code_t::CIPHER_OPERATION;
                    }
                }

                unsigned char empty_iv[MBEDTLS_MAX_IV_LENGTH] = {0};
                if ((last_errorno_ = mbedtls_cipher_crypt(cipher_context_.enc, iv_.empty() ? empty_iv : &iv_[0], iv_.size(), input, ilen,
                                                          output, olen)) != 0) {
                    return error_code_t::CIPHER_OPERATION;
                }
                return error_code_t::OK;
#endif
            }
            default:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            }
        }

        int cipher::decrypt(const unsigned char *input, size_t ilen, unsigned char *output, size_t *olen) {
            if (NULL == interface_ || interface_->method == EN_CIMT_INVALID) {
                return details::setup_errorno(*this, 0, error_code_t::NOT_INITED);
            }

            if (input == NULL || ilen <= 0 || output == NULL || NULL == olen || *olen <= 0 || *olen < ilen + get_block_size()) {
                return details::setup_errorno(*this, -1, error_code_t::INVALID_PARAM);
            }

            switch (interface_->method) {
            case EN_CIMT_INVALID:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            case EN_CIMT_XXTEA: {
                util::xxtea_decrypt(&xxtea_context_.key, reinterpret_cast<const void *>(input), ilen, reinterpret_cast<void *>(output),
                                    olen);
                return details::setup_errorno(*this, 0, error_code_t::OK);
            }
            case EN_CIMT_CIPHER: {
                if (NULL == cipher_context_.dec) {
                    return details::setup_errorno(*this, 0, error_code_t::CIPHER_DISABLED);
                }

                if (iv_.empty()) {
                    if (0 != get_iv_size()) {
                        iv_.resize(get_iv_size(), 0);
                    }
                }

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                int outl, finish_olen;

                if (!iv_.empty()) {
                    if (!EVP_CipherInit_ex(cipher_context_.dec, NULL, NULL, NULL, &iv_[0], -1)) {
                        return details::setup_errorno(*this, ERR_peek_error(), error_code_t::CIPHER_OPERATION);
                    }
                }

                if (0 != (interface_->flags & EN_CIFT_DECRYPT_NO_PADDING)) {
                    EVP_CIPHER_CTX_set_padding(cipher_context_.dec, 0);
                }

                if (!(EVP_CipherUpdate(cipher_context_.dec, output, &outl, input, static_cast<int>(ilen)))) {
                    return details::setup_errorno(*this, ERR_peek_error(), error_code_t::CIPHER_OPERATION);
                }

                if (!(EVP_CipherFinal_ex(cipher_context_.dec, output + outl, &finish_olen))) {
                    return details::setup_errorno(*this, ERR_peek_error(), error_code_t::CIPHER_OPERATION);
                }

                *olen = static_cast<size_t>(outl + finish_olen);

                return error_code_t::OK;
#elif defined(CRYPTO_USE_MBEDTLS)
                if (0 != (interface_->flags & EN_CIFT_DECRYPT_NO_PADDING) && MBEDTLS_MODE_CBC == cipher_context_.enc->cipher_info->mode) {
                    if ((last_errorno_ = mbedtls_cipher_set_padding_mode(cipher_context_.dec, MBEDTLS_PADDING_NONE)) != 0) {
                        return error_code_t::CIPHER_OPERATION;
                    }
                }

                unsigned char empty_iv[MBEDTLS_MAX_IV_LENGTH] = {0};
                if ((last_errorno_ = mbedtls_cipher_crypt(cipher_context_.dec, iv_.empty() ? empty_iv : &iv_[0], iv_.size(), input, ilen,
                                                          output, olen)) != 0) {
                    return error_code_t::CIPHER_OPERATION;
                }
                return error_code_t::OK;
#endif
            }
            default:
                return details::setup_errorno(*this, -1, error_code_t::NOT_INITED);
            }
        }

        const cipher::cipher_kt_t *cipher::get_cipher_by_name(const char *name) {
            const cipher_interface_info_t *interface = details::get_cipher_interface_by_name(name);

            if (NULL == interface) {
                return NULL;
            }

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            return EVP_get_cipherbyname(interface->name);
#elif defined(CRYPTO_USE_MBEDTLS)
            return mbedtls_cipher_info_from_string(interface->mbedtls_name);
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
                for (size_t i = 0; NULL != details::supported_ciphers[i].name; ++i) {
                    if (details::supported_ciphers[i].flags & EN_CIFT_AEAD) {
                        continue;
                    }

                    if (details::supported_ciphers[i].method < EN_CIMT_INNER) {
                        ret.push_back(details::supported_ciphers[i].name);
                        continue;
                    }

                    if (NULL != get_cipher_by_name(details::supported_ciphers[i].name)) {
                        ret.push_back(details::supported_ciphers[i].name);
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

        int cipher::cleanup_global_algorithm() {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
            EVP_cleanup();
#endif
            return 0;
        }
    } // namespace crypto
} // namespace util

#endif