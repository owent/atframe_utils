#include <cstdlib>
#include <cstring>
#include <fstream>
#include <std/chrono.h>
#include <std/smart_ptr.h>


#include "algorithm/crypto_cipher.h"
#include "common/file_system.h"
#include "common/string_oprs.h"

#include "frame/test_macros.h"

#ifdef CRYPTO_CIPHER_ENABLED

#include <sstream>

#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
struct openssl_test_init_wrapper {
    openssl_test_init_wrapper() { util::crypto::cipher::init_global_algorithm(); }

    ~openssl_test_init_wrapper() { util::crypto::cipher::cleanup_global_algorithm(); }
};

static std::shared_ptr<openssl_test_init_wrapper> openssl_test_inited;

#endif

CASE_TEST(crypto_cipher, get_all_cipher_names) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
    if (!openssl_test_inited) {
        openssl_test_inited = std::make_shared<openssl_test_init_wrapper>();
    }
#endif

    const std::vector<std::string> &all_ciphers = util::crypto::cipher::get_all_cipher_names();
    std::stringstream               ss;
    for (size_t i = 0; i < all_ciphers.size(); ++i) {
        if (i) {
            ss << ",";
        }

        ss << all_ciphers[i];
    }

    CASE_MSG_INFO() << "All ciphers: " << ss.str() << std::endl;
    CASE_EXPECT_NE(0, all_ciphers.size());
}

CASE_TEST(crypto_cipher, split_ciphers) {
    std::vector<std::string> all_ciphers;
    std::string              in =
        "xxtea,rc4,aes-128-cfb aes-192-cfb aes-256-cfb aes-128-ctr\raes-192-ctr\naes-256-ctr   bf-cfb:camellia-128-cfb:camellia-"
        "192-cfb;camellia-256-cfb;;;chacha20\tchacha20-poly1305";

    std::pair<const char *, const char *> res;
    res.first  = in.c_str();
    res.second = in.c_str();
    while (NULL != res.second) {
        res = util::crypto::cipher::ciphertok(res.second);

        if (NULL != res.second && NULL != res.first) {
            all_ciphers.push_back(std::string(res.first, res.second));
        }
    }
    CASE_EXPECT_EQ(14, all_ciphers.size());
}

static const unsigned char aes_test_cfb128_key[3][32] = {
    {0x2B, 0x7E, 0x15, 0x16, 0x28, 0xAE, 0xD2, 0xA6, 0xAB, 0xF7, 0x15, 0x88, 0x09, 0xCF, 0x4F, 0x3C},
    {0x8E, 0x73, 0xB0, 0xF7, 0xDA, 0x0E, 0x64, 0x52, 0xC8, 0x10, 0xF3, 0x2B,
     0x80, 0x90, 0x79, 0xE5, 0x62, 0xF8, 0xEA, 0xD2, 0x52, 0x2C, 0x6B, 0x7B},
    {0x60, 0x3D, 0xEB, 0x10, 0x15, 0xCA, 0x71, 0xBE, 0x2B, 0x73, 0xAE, 0xF0, 0x85, 0x7D, 0x77, 0x81,
     0x1F, 0x35, 0x2C, 0x07, 0x3B, 0x61, 0x08, 0xD7, 0x2D, 0x98, 0x10, 0xA3, 0x09, 0x14, 0xDF, 0xF4}};

static const unsigned char aes_test_cfb128_iv[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
                                                     0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};

static const unsigned char aes_test_cfb128_pt[64] = {
    0x6B, 0xC1, 0xBE, 0xE2, 0x2E, 0x40, 0x9F, 0x96, 0xE9, 0x3D, 0x7E, 0x11, 0x73, 0x93, 0x17, 0x2A, 0xAE, 0x2D, 0x8A, 0x57, 0x1E, 0x03,
    0xAC, 0x9C, 0x9E, 0xB7, 0x6F, 0xAC, 0x45, 0xAF, 0x8E, 0x51, 0x30, 0xC8, 0x1C, 0x46, 0xA3, 0x5C, 0xE4, 0x11, 0xE5, 0xFB, 0xC1, 0x19,
    0x1A, 0x0A, 0x52, 0xEF, 0xF6, 0x9F, 0x24, 0x45, 0xDF, 0x4F, 0x9B, 0x17, 0xAD, 0x2B, 0x41, 0x7B, 0xE6, 0x6C, 0x37, 0x10};

static const unsigned char aes_test_cfb128_ct[3][64] = {
    {0x3B, 0x3F, 0xD9, 0x2E, 0xB7, 0x2D, 0xAD, 0x20, 0x33, 0x34, 0x49, 0xF8, 0xE8, 0x3C, 0xFB, 0x4A, 0xC8, 0xA6, 0x45, 0x37, 0xA0, 0xB3,
     0xA9, 0x3F, 0xCD, 0xE3, 0xCD, 0xAD, 0x9F, 0x1C, 0xE5, 0x8B, 0x26, 0x75, 0x1F, 0x67, 0xA3, 0xCB, 0xB1, 0x40, 0xB1, 0x80, 0x8C, 0xF1,
     0x87, 0xA4, 0xF4, 0xDF, 0xC0, 0x4B, 0x05, 0x35, 0x7C, 0x5D, 0x1C, 0x0E, 0xEA, 0xC4, 0xC6, 0x6F, 0x9F, 0xF7, 0xF2, 0xE6},
    {0xCD, 0xC8, 0x0D, 0x6F, 0xDD, 0xF1, 0x8C, 0xAB, 0x34, 0xC2, 0x59, 0x09, 0xC9, 0x9A, 0x41, 0x74, 0x67, 0xCE, 0x7F, 0x7F, 0x81, 0x17,
     0x36, 0x21, 0x96, 0x1A, 0x2B, 0x70, 0x17, 0x1D, 0x3D, 0x7A, 0x2E, 0x1E, 0x8A, 0x1D, 0xD5, 0x9B, 0x88, 0xB1, 0xC8, 0xE6, 0x0F, 0xED,
     0x1E, 0xFA, 0xC4, 0xC9, 0xC0, 0x5F, 0x9F, 0x9C, 0xA9, 0x83, 0x4F, 0xA0, 0x42, 0xAE, 0x8F, 0xBA, 0x58, 0x4B, 0x09, 0xFF},
    {0xDC, 0x7E, 0x84, 0xBF, 0xDA, 0x79, 0x16, 0x4B, 0x7E, 0xCD, 0x84, 0x86, 0x98, 0x5D, 0x38, 0x60, 0x39, 0xFF, 0xED, 0x14, 0x3B, 0x28,
     0xB1, 0xC8, 0x32, 0x11, 0x3C, 0x63, 0x31, 0xE5, 0x40, 0x7B, 0xDF, 0x10, 0x13, 0x24, 0x15, 0xE5, 0x4B, 0x92, 0xA1, 0x3E, 0xD0, 0xA8,
     0x26, 0x7A, 0xE2, 0xF9, 0x75, 0xA3, 0x85, 0x74, 0x1A, 0xB9, 0xCE, 0xF8, 0x20, 0x31, 0x62, 0x3D, 0x55, 0xB1, 0xE4, 0x71}};

CASE_TEST(crypto_cipher, aes_cfb) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
    if (!openssl_test_inited) {
        openssl_test_inited = std::make_shared<openssl_test_init_wrapper>();
    }
#endif


    for (int i = 0; i < 6; ++i) {
        int u = i >> 1;
        int v = i & 1;

        util::crypto::cipher ci;
        int mode = (0 == v) ? (::util::crypto::cipher::mode_t::EN_CMODE_DECRYPT) : (::util::crypto::cipher::mode_t::EN_CMODE_ENCRYPT);
        if (0 == u) {
            CASE_EXPECT_EQ(0, ci.init("AES-128-CFB", mode));
        } else if (1 == u) {
            CASE_EXPECT_EQ(0, ci.init("AES-192-CFB", mode));
        } else {
            CASE_EXPECT_EQ(0, ci.init("AES-256-CFB", mode));
        }

        CASE_EXPECT_EQ(16, ci.get_iv_size());
        CASE_EXPECT_EQ(0, ci.set_iv(aes_test_cfb128_iv, 16));
        CASE_EXPECT_EQ(128 + 64 * u, ci.get_key_bits());
        CASE_EXPECT_EQ(0, ci.set_key(aes_test_cfb128_key[u], 128 + 64 * u));

        unsigned char buf_in[64], buf_out[128];
        size_t        olen = sizeof(buf_out);
        if (::util::crypto::cipher::mode_t::EN_CMODE_DECRYPT == mode) {
            memcpy(buf_in, aes_test_cfb128_ct[u], 64);
            CASE_EXPECT_EQ(0, ci.decrypt(buf_in, 64, buf_out, &olen));

            CASE_EXPECT_EQ(0, memcmp(buf_out, aes_test_cfb128_pt, 64));
        } else {
            memcpy(buf_in, aes_test_cfb128_pt, 64);
            CASE_EXPECT_EQ(0, ci.encrypt(buf_in, 64, buf_out, &olen));

            CASE_EXPECT_EQ(0, memcmp(buf_out, aes_test_cfb128_ct[u], 64));
        }
    }
}

static const unsigned char aes_test_cfb128_nopadding_pt[3][30] = {"hello message 0x00000001,hi 1", "hello message 0x00000002,hi 2",
                                                                  "hello message 0x00000003,hi 3"};

static const unsigned char aes_test_cfb128_nopadding_ct[3][29] = {
    {0x8d, 0x0d, 0x9a, 0xed, 0xfb, 0xef, 0x1b, 0xb3, 0x64, 0x3e, 0x2d, 0xa7, 0x26, 0x30, 0x98,
     0x2c, 0xb0, 0xbe, 0xc4, 0xed, 0x3e, 0xeb, 0x74, 0xc1, 0x92, 0x68, 0x3c, 0x8e, 0x45},
    {0x8d, 0x0d, 0x9a, 0xed, 0xfb, 0xef, 0x1b, 0xb3, 0x64, 0x3e, 0x2d, 0xa7, 0x26, 0x30, 0x98,
     0x2c, 0xb0, 0xbe, 0xc4, 0xed, 0x3e, 0xeb, 0x74, 0xc2, 0x92, 0x68, 0x3c, 0x8e, 0x46},
    {0x8d, 0x0d, 0x9a, 0xed, 0xfb, 0xef, 0x1b, 0xb3, 0x64, 0x3e, 0x2d, 0xa7, 0x26, 0x30, 0x98,
     0x2c, 0xb0, 0xbe, 0xc4, 0xed, 0x3e, 0xeb, 0x74, 0xc3, 0x92, 0x68, 0x3c, 0x8e, 0x47}};

CASE_TEST(crypto_cipher, aes_cfb_nopadding_encrypt) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
    if (!openssl_test_inited) {
        openssl_test_inited = std::make_shared<openssl_test_init_wrapper>();
    }
#endif

    {
        util::crypto::cipher ci;
        CASE_EXPECT_EQ(0, ci.init("AES-256-CFB", ::util::crypto::cipher::mode_t::EN_CMODE_ENCRYPT));

        // CASE_EXPECT_EQ(16, ci.get_iv_size());
        // CASE_EXPECT_EQ(0, ci.set_iv(aes_test_cfb128_iv, 16));
        CASE_EXPECT_EQ(256, ci.get_key_bits());
        CASE_EXPECT_EQ(0, ci.set_key(aes_test_cfb128_key[2], 256));

        const size_t buffer_len = 29;

        for (int i = 0; i < 3; ++i) {
            unsigned char buf_in[64] = {0}, buf_out[128] = {0};
            size_t        olen = sizeof(buf_out);
            memcpy(buf_in, aes_test_cfb128_nopadding_pt[i], buffer_len);
            CASE_EXPECT_EQ(0, ci.encrypt(buf_in, buffer_len, buf_out, &olen));
            CASE_EXPECT_EQ(0, memcmp(buf_out, aes_test_cfb128_nopadding_ct[i], buffer_len));

            CASE_MSG_INFO() << "AES-256-CFB => txt: " << aes_test_cfb128_nopadding_pt[i] << std::endl;
            CASE_MSG_INFO() << "AES-256-CFB => enc: ";
            util::string::dumphex(buf_out, olen, std::cout);
            std::cout << std::endl;
        }
    }

    {
        util::crypto::cipher ci;
        CASE_EXPECT_EQ(0, ci.init("AES-256-CFB", ::util::crypto::cipher::mode_t::EN_CMODE_DECRYPT));

        // CASE_EXPECT_EQ(16, ci.get_iv_size());
        // CASE_EXPECT_EQ(0, ci.set_iv(aes_test_cfb128_iv, 16));
        CASE_EXPECT_EQ(256, ci.get_key_bits());
        CASE_EXPECT_EQ(0, ci.set_key(aes_test_cfb128_key[2], 256));

        const size_t buffer_len = 29;

        for (int i = 0; i < 3; ++i) {
            unsigned char buf_in[64] = {0}, buf_out[128] = {0};
            size_t        olen = sizeof(buf_out);
            memcpy(buf_in, aes_test_cfb128_nopadding_ct[i], buffer_len);
            CASE_EXPECT_EQ(0, ci.decrypt(buf_in, buffer_len, buf_out, &olen));
            CASE_EXPECT_EQ(0, memcmp(buf_out, aes_test_cfb128_nopadding_pt[i], buffer_len));

            CASE_MSG_INFO() << "AES-256-CFB => dec: ";
            util::string::dumphex(buf_in, buffer_len, std::cout);
            std::cout << std::endl;
            CASE_MSG_INFO() << "AES-256-CFB => txt: " << ((unsigned char *)buf_out) << std::endl;
        }
    }
}


/*
 * XTEA tests vectors (non-official)
 */

static const unsigned char xtea_test_key[6][16] = {
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
    {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}};

static const unsigned char xtea_test_pt[6][8] = {
    {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48}, {0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41},
    {0x5a, 0x5b, 0x6e, 0x27, 0x89, 0x48, 0xd7, 0x7f}, {0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48},
    {0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41, 0x41}, {0x70, 0xe1, 0x22, 0x5d, 0x6e, 0x4e, 0x76, 0x55}};

static const unsigned char xtea_test_ct[6][8] = {
    {0x70, 0x6c, 0xd7, 0x32, 0x3e, 0xd8, 0x60, 0xe8}, {0x0d, 0x5a, 0x2d, 0x8b, 0x6a, 0x43, 0x18, 0x30},
    {0x0d, 0xa4, 0xba, 0xd3, 0xb4, 0x2a, 0x78, 0x85}, {0x62, 0xeb, 0x33, 0x08, 0x10, 0x86, 0x0a, 0x17},
    {0xd1, 0xbe, 0xdf, 0x50, 0xdc, 0xf2, 0x90, 0x43}, {0x47, 0xcc, 0x5f, 0xb9, 0x91, 0x90, 0x66, 0x6b}};

CASE_TEST(crypto_cipher, xxtea) {
    for (int i = 0; i < 6; ++i) {
        util::crypto::cipher ci;
        CASE_EXPECT_EQ(0, ci.init("XXTEA"));
        CASE_EXPECT_EQ(0, ci.set_key(xtea_test_key[i], ci.get_key_bits()));

        unsigned char buf_in[8], buf_out[16];

        size_t olen = sizeof(buf_out);
        memcpy(buf_in, xtea_test_pt[i], 8);
        CASE_EXPECT_EQ(0, ci.encrypt(buf_in, 8, buf_out, &olen));
        CASE_EXPECT_EQ(0, memcmp(buf_out, xtea_test_ct[i], 8));

        olen = sizeof(buf_out);
        memcpy(buf_in, xtea_test_ct[i], 8);
        CASE_EXPECT_EQ(0, ci.decrypt(buf_in, 8, buf_out, &olen));
        CASE_EXPECT_EQ(0, memcmp(buf_out, xtea_test_pt[i], 8));

        CASE_EXPECT_EQ(8, olen);
    }
}

enum evp_test_operation_type {
    EN_ETOT_BOTH    = 0,
    EN_ETOT_ENCRYPT = 1,
    EN_ETOT_DECRYPT = 2,
};

enum evp_test_key_type {
    EN_ETKT_NONE = 0,
    EN_ETKT_CIPHER,
    EN_ETKT_KEY,
    EN_ETKT_IV,
    EN_ETKT_OPERATOR,
    EN_ETKT_PLAINTEXT,
    EN_ETKT_CIPHERTEXT,
    EN_ETKT_AAD,
    EN_ETKT_TAG,
    EN_ETKT_RESULT,
};

struct evp_test_info {
    std::string             cipher;
    std::string             key;
    std::string             iv;
    evp_test_operation_type operation;
    std::string             plaintext;
    std::string             ciphertext;

    // ========== for aead ==========
    std::string aad;
    std::string tag;
    bool        is_final_error;
};

static void evp_test_reset_info(evp_test_info &info) {
    info.cipher.clear();
    info.key.clear();
    info.iv.clear();
    info.operation = EN_ETOT_BOTH;
    info.plaintext.clear();
    info.ciphertext.clear();

    // ====== aead ======
    info.aad.clear();
    info.tag.clear();
    info.is_final_error = false;
}

static bool evp_test_is_aead(const evp_test_info &info) { return !info.aad.empty() || !info.tag.empty(); }

static std::pair<evp_test_key_type, std::string> evp_test_parse_line(std::istream &in) {
    evp_test_key_type ret_key = EN_ETKT_NONE;
    std::string       ret_val;

    std::string line;
    do {
        if (!std::getline(in, line)) {
            break;
        }

        if (line.empty()) {
            break;
        }

        size_t s, e;
        for (s = 0; s < line.size(); ++s) {
            if (line[s] != ' ' && line[s] != '\t' && line[s] != '\r' && line[s] != '\n') {
                break;
            }
        }

        // comment or empty line
        if (s >= line.size() || line[s] == '#') {
            break;
        }

        for (e = s; e < line.size(); ++e) {
            if (line[e] == ' ' || line[e] == '\t' || line[e] == '\r' || line[e] == '\n' || line[e] == '=') {
                break;
            }
        }

        std::string key = line.substr(s, e - s);

        for (s = e; s < line.size(); ++s) {
            if (line[s] != ' ' && line[s] != '\t' && line[s] != '\r' && line[s] != '\n' && line[s] != '=') {
                break;
            }
        }

        for (e = s; e < line.size(); ++e) {
            if (line[e] == ' ' || line[e] == '\t' || line[e] == '\r' || line[e] == '\n' || line[e] == '=') {
                break;
            }
        }

        std::string value = line.substr(s, e - s);

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("Cipher", key.c_str(), key.size())) {
            ret_key = EN_ETKT_CIPHER;
            ret_val.swap(value);
            break;
        }

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("Key", key.c_str(), key.size())) {
            ret_key = EN_ETKT_KEY;
            ret_val.swap(value);
            break;
        }

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("IV", key.c_str(), key.size())) {
            ret_key = EN_ETKT_IV;
            ret_val.swap(value);
            break;
        }

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("Operation", key.c_str(), key.size())) {
            ret_key = EN_ETKT_OPERATOR;
            ret_val.swap(value);
            break;
        }

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("Plaintext", key.c_str(), key.size())) {
            ret_key = EN_ETKT_PLAINTEXT;
            ret_val.swap(value);
            break;
        }

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("Ciphertext", key.c_str(), key.size())) {
            ret_key = EN_ETKT_CIPHERTEXT;
            ret_val.swap(value);
            break;
        }

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("AAD", key.c_str(), key.size())) {
            ret_key = EN_ETKT_AAD;
            ret_val.swap(value);
            break;
        }

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("Tag", key.c_str(), key.size())) {
            ret_key = EN_ETKT_TAG;
            ret_val.swap(value);
            break;
        }

        if (0 == UTIL_STRFUNC_STRNCASE_CMP("Result", key.c_str(), key.size())) {
            ret_key = EN_ETKT_RESULT;
            ret_val.swap(value);
            break;
        }

    } while (false);

    return std::make_pair(ret_key, ret_val);
}

static std::string evp_test_read_hex_bin(const std::string &hex) {
    std::string ret;
    ret.resize((hex.size() + 1) / 2);
    for (size_t i = 0; i < hex.size(); ++i) {
        char c = hex[i] - '0';
        if (hex[i] >= 'A' && hex[i] <= 'F') {
            c = 10 + hex[i] - 'A';
        } else if (hex[i] >= 'a' && hex[i] <= 'f') {
            c = 10 + hex[i] - 'a';
        }

        if (i & 0x01) {
            ret[i >> 1] = (ret[i >> 1] << 4) | c;
        } else {
            ret[i >> 1] = c;
        }
    }

    return ret;
}

static bool evp_test_parse_info(std::istream &in, evp_test_info &info) {
    evp_test_reset_info(info);

    bool has_begin = false;
    while (true) {
        if (in.eof()) {
            break;
        }

        std::pair<evp_test_key_type, std::string> res = evp_test_parse_line(in);
        if (res.first == EN_ETKT_NONE) {
            if (has_begin) {
                break;
            } else {
                continue;
            }
        }

        switch (res.first) {
        case EN_ETKT_CIPHER:
            info.cipher = res.second;
            has_begin   = true;
            break;
        case EN_ETKT_KEY:
            info.key  = evp_test_read_hex_bin(res.second);
            has_begin = true;
            break;
        case EN_ETKT_IV:
            info.iv   = evp_test_read_hex_bin(res.second);
            has_begin = true;
            break;
        case EN_ETKT_OPERATOR:
            if (0 == UTIL_STRFUNC_STRNCASE_CMP("ENCRYPT", res.second.c_str(), res.second.size())) {
                info.operation = EN_ETOT_ENCRYPT;
            } else if (0 == UTIL_STRFUNC_STRNCASE_CMP("DECRYPT", res.second.c_str(), res.second.size())) {
                info.operation = EN_ETOT_DECRYPT;
            }
            has_begin = true;
            break;
        case EN_ETKT_PLAINTEXT:
            info.plaintext = evp_test_read_hex_bin(res.second);
            has_begin      = true;
            break;
        case EN_ETKT_CIPHERTEXT:
            info.ciphertext = evp_test_read_hex_bin(res.second);
            has_begin       = true;
            break;
        case EN_ETKT_AAD:
            info.aad  = evp_test_read_hex_bin(res.second);
            has_begin = true;
            break;
        case EN_ETKT_TAG:
            info.tag  = evp_test_read_hex_bin(res.second);
            has_begin = true;
            break;
        case EN_ETKT_RESULT:
            if (0 == UTIL_STRFUNC_STRNCASE_CMP("CIPHERFINAL_ERROR", res.second.c_str(), res.second.size()) ||
                0 == UTIL_STRFUNC_STRNCASE_CMP("CIPHERUPDATE_ERROR", res.second.c_str(), res.second.size())) {
                info.is_final_error = true;
            }
            has_begin = true;
            break;
        default:
            break;
        }
    }

    return has_begin;
}

CASE_TEST(crypto_cipher, evp_test) {
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
    if (!openssl_test_inited) {
        openssl_test_inited = std::make_shared<openssl_test_init_wrapper>();
    }
#endif

    std::string evptest_file_path;
    util::file_system::dirname(__FILE__, 0, evptest_file_path);
    evptest_file_path += util::file_system::DIRECTORY_SEPARATOR;
    evptest_file_path += "evptests.txt";

    CASE_MSG_INFO() << "Load " << evptest_file_path << " for additional cipher tests." << std::endl;
    std::fstream fin;
    fin.open(evptest_file_path.c_str(), std::ios::in);
    evp_test_info info;

    while (evp_test_parse_info(fin, info)) {
        int mode = util::crypto::cipher::mode_t::EN_CMODE_ENCRYPT | util::crypto::cipher::mode_t::EN_CMODE_DECRYPT;
        if (info.operation == EN_ETOT_ENCRYPT) {
            mode = util::crypto::cipher::mode_t::EN_CMODE_ENCRYPT;
        } else if (info.operation == EN_ETOT_DECRYPT) {
            mode = util::crypto::cipher::mode_t::EN_CMODE_DECRYPT;
        }

#if defined(CRYPTO_USE_MBEDTLS)
        if (info.iv.size() > MBEDTLS_MAX_IV_LENGTH) {
            continue;
        }
#endif

        util::crypto::cipher ci;
        if (0 != ci.init(info.cipher.c_str(), mode)) {
            CASE_MSG_INFO() << "\tCipher: " << info.cipher << " => not available for current crypto libraries, skipped." << std::endl;
            continue;
        }

        CASE_EXPECT_EQ(evp_test_is_aead(info), ci.is_aead());

        if (!info.key.empty()) {
            CASE_EXPECT_EQ(
                0, ci.set_key(reinterpret_cast<const unsigned char *>(info.key.c_str()), static_cast<uint32_t>(info.key.size() * 8)));
        }

        std::string buffer;
        buffer.resize((info.plaintext.size() > info.ciphertext.size() ? info.plaintext.size() : info.ciphertext.size()) +
                      ci.get_block_size());

        if (mode & util::crypto::cipher::mode_t::EN_CMODE_ENCRYPT) {
            std::chrono::system_clock::time_point begin       = std::chrono::system_clock::now();
            int                                   enc_res     = 0;
            const char *                          failed_step = "memory check";
            memset(&buffer[0], 0, buffer.size());

            do {
                if (!info.iv.empty()) {
                    CASE_EXPECT_EQ(
                        0, ci.set_iv(reinterpret_cast<const unsigned char *>(info.iv.c_str()), static_cast<uint32_t>(info.iv.size())));
                } else {
                    ci.clear_iv();
                }

                size_t olen = buffer.size();

                if (evp_test_is_aead(info)) {
                    unsigned char aead_tag[16];
                    size_t        aead_tag_len = sizeof(aead_tag);
                    if (info.tag.size() < aead_tag_len) {
                        aead_tag_len = info.tag.size();
                    }
                    CASE_EXPECT_LE(info.tag.size(), aead_tag_len);

                    enc_res =
                        ci.encrypt_aead(reinterpret_cast<const unsigned char *>(info.plaintext.c_str()), info.plaintext.size(),
                                        reinterpret_cast<unsigned char *>(&buffer[0]), &olen,
                                        reinterpret_cast<const unsigned char *>(info.aad.c_str()), info.aad.size(), aead_tag, aead_tag_len);

                    if (info.is_final_error) {
                        CASE_EXPECT_NE(0, enc_res);
                    } else {
                        CASE_EXPECT_EQ(0, enc_res);
                    }
                    if (0 != enc_res) {
                        if (!info.is_final_error) {
                            failed_step = "encrypt(AEAD)";
                        } else {
                            enc_res = 0;
                        }
                        break;
                    }

                    int check_tag = memcmp(aead_tag, info.tag.c_str(), aead_tag_len);
                    CASE_EXPECT_EQ(0, check_tag);
                    if (0 != check_tag) {
                        std::cout << "Expect Tag: ";
                        util::string::dumphex(info.tag.c_str(), info.tag.size(), std::cout);
                        std::cout << std::endl << "Real   Tag: ";
                        util::string::dumphex(&aead_tag[0], aead_tag_len, std::cout);
                        std::cout << std::endl;
                    }
                } else {
                    enc_res = ci.encrypt(reinterpret_cast<const unsigned char *>(info.plaintext.c_str()), info.plaintext.size(),
                                         reinterpret_cast<unsigned char *>(&buffer[0]), &olen);

                    CASE_EXPECT_EQ(0, enc_res);
                    if (0 != enc_res) {
                        failed_step = "encrypt";
                        break;
                    }
                }

                if (!info.is_final_error) {
                    enc_res = memcmp(info.ciphertext.c_str(), buffer.c_str(), info.ciphertext.size());
                    CASE_EXPECT_EQ(0, enc_res);
                    if (0 != enc_res) {
                        std::cout << "Expect CipherText: ";
                        util::string::dumphex(info.ciphertext.c_str(), info.ciphertext.size(), std::cout);
                        std::cout << std::endl << "Real   CipherText: ";
                        util::string::dumphex(&buffer[0], olen, std::cout);
                        std::cout << std::endl;
                    }
                }
            } while (false);

            if (0 == enc_res) {
                std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
                double ns_count = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
                CASE_MSG_INFO() << "\tCipher: " << info.cipher << " => encrypt " << info.plaintext.size() << " bytes in " << ns_count
                                << "ns." << std::endl;
            } else {
                CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << "\tCipher: " << info.cipher << " => encrypt " << info.plaintext.size()
                                << " bytes failed(" << failed_step << ":" << ci.get_last_errno() << ")." << std::endl;
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                char err_msg[8192] = {0};
                ERR_error_string_n((unsigned long)ci.get_last_errno(), err_msg, sizeof(err_msg));
                CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << "\t" << err_msg << std::endl;
#endif
            }
        }

        if (mode & util::crypto::cipher::mode_t::EN_CMODE_DECRYPT) {
            std::chrono::system_clock::time_point begin       = std::chrono::system_clock::now();
            int                                   dec_res     = 0;
            const char *                          failed_step = "memory check";
            memset(&buffer[0], 0, buffer.size());

            do {
                if (!info.iv.empty()) {
                    CASE_EXPECT_EQ(
                        0, ci.set_iv(reinterpret_cast<const unsigned char *>(info.iv.c_str()), static_cast<uint32_t>(info.iv.size())));
                } else {
                    ci.clear_iv();
                }

                size_t olen = buffer.size();

                if (evp_test_is_aead(info)) {
                    // if (info.cipher == "chacha20-poly1305") {
                    //     puts("debug");
                    // }
                    dec_res = ci.decrypt_aead(reinterpret_cast<const unsigned char *>(info.ciphertext.c_str()), info.ciphertext.size(),
                                              reinterpret_cast<unsigned char *>(&buffer[0]), &olen,
                                              reinterpret_cast<const unsigned char *>(info.aad.c_str()), info.aad.size(),
                                              reinterpret_cast<const unsigned char *>(info.tag.c_str()), info.tag.size());

                    if (info.is_final_error) {
                        CASE_EXPECT_NE(0, dec_res);
                    } else {
                        CASE_EXPECT_EQ(0, dec_res);
                    }

                    if (0 != dec_res) {
                        if (!info.is_final_error) {
                            failed_step = "decrypt(AEAD)";
                        } else {
                            dec_res = 0;
                        }
                        break;
                    }
                } else {
                    dec_res = ci.decrypt(reinterpret_cast<const unsigned char *>(info.ciphertext.c_str()), info.ciphertext.size(),
                                         reinterpret_cast<unsigned char *>(&buffer[0]), &olen);
                    CASE_EXPECT_EQ(0, dec_res);

                    if (0 != dec_res) {
                        failed_step = "decrypt";
                        break;
                    }
                }

                if (!info.is_final_error) {
                    dec_res = memcmp(info.plaintext.c_str(), buffer.c_str(), info.plaintext.size());
                    CASE_EXPECT_EQ(0, dec_res);
                    if (0 != dec_res) {
                        std::cout << "Expect PlainText: ";
                        util::string::dumphex(info.plaintext.c_str(), info.plaintext.size(), std::cout);
                        std::cout << std::endl << "Real   PlainText: ";
                        util::string::dumphex(&buffer[0], olen, std::cout);
                        std::cout << std::endl;
                    }
                }
            } while (false);

            if (0 == dec_res) {
                std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
                double ns_count = (double)std::chrono::duration_cast<std::chrono::nanoseconds>(end - begin).count();
                CASE_MSG_INFO() << "\tCipher: " << info.cipher << " => decrypt " << info.plaintext.size() << " bytes in " << ns_count
                                << "ns." << std::endl;
            } else {
                CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << "\tCipher: " << info.cipher << " => decrypt " << info.plaintext.size()
                                << " bytes failed(" << failed_step << ":" << ci.get_last_errno() << ")." << std::endl;
#if defined(CRYPTO_USE_OPENSSL) || defined(CRYPTO_USE_LIBRESSL) || defined(CRYPTO_USE_BORINGSSL)
                char err_msg[8192] = {0};
                ERR_error_string_n((unsigned long)ci.get_last_errno(), err_msg, sizeof(err_msg));
                CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << "\t" << err_msg << std::endl;
#endif
            }
        }
    }
}


#endif