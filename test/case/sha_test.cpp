// Copyright 2021 atframework

#include <time.h>
#include <string>

#include <config/compiler_features.h>

#include "frame/test_macros.h"

#include <algorithm/sha.h>
#include <common/string_oprs.h>

CASE_TEST(sha, sha1) {
  const unsigned char sha1_test_buf[3][57] = {
      {"abc"}, {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"}, {""}};

  const size_t sha1_test_buflen[3] = {3, 56, 0};

  const unsigned char sha1_test_sum[3][20] = {{0xA9, 0x99, 0x3E, 0x36, 0x47, 0x06, 0x81, 0x6A, 0xBA, 0x3E,
                                               0x25, 0x71, 0x78, 0x50, 0xC2, 0x6C, 0x9C, 0xD0, 0xD8, 0x9D},
                                              {0x84, 0x98, 0x3E, 0x44, 0x1C, 0x3B, 0xD2, 0x6E, 0xBA, 0xAE,
                                               0x4A, 0xA1, 0xF9, 0x51, 0x29, 0xE5, 0xE5, 0x46, 0x70, 0xF1},
                                              {0xda, 0x39, 0xa3, 0xee, 0x5e, 0x6b, 0x4b, 0x0d, 0x32, 0x55,
                                               0xbf, 0xef, 0x95, 0x60, 0x18, 0x90, 0xaf, 0xd8, 0x07, 0x09}};

  const size_t output_length = atfw::util::hash::sha::get_output_length(atfw::util::hash::sha::EN_ALGORITHM_SHA1);
  for (int i = 0; i < 3; ++i) {
    std::string hash = atfw::util::hash::sha::hash_to_binary(atfw::util::hash::sha::EN_ALGORITHM_SHA1, sha1_test_buf[i],
                                                             sha1_test_buflen[i]);
    CASE_EXPECT_EQ(hash, std::string(reinterpret_cast<const char*>(sha1_test_sum[i]), output_length));

    hash = atfw::util::hash::sha::hash_to_hex(atfw::util::hash::sha::EN_ALGORITHM_SHA1, sha1_test_buf[i],
                                              sha1_test_buflen[i]);
    std::string serialize_str;
    serialize_str.resize(output_length * 2, 0);
    atfw::util::string::dumphex(sha1_test_sum[i], output_length, &serialize_str[0]);
    CASE_EXPECT_EQ(hash, serialize_str);

    hash = atfw::util::hash::sha::hash_to_base64(atfw::util::hash::sha::EN_ALGORITHM_SHA1, sha1_test_buf[i],
                                                 sha1_test_buflen[i]);
    serialize_str.clear();
    atfw::util::base64_encode(serialize_str, sha1_test_sum[i], output_length);
    CASE_EXPECT_EQ(hash, serialize_str);
  }
}

CASE_TEST(sha, sha224) {
  const unsigned char sha1_test_buf[3][57] = {
      {"abc"}, {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"}, {""}};

  const size_t sha1_test_buflen[3] = {3, 56, 0};

  const unsigned char sha1_test_sum[3][32] = {
      {0x23, 0x09, 0x7D, 0x22, 0x34, 0x05, 0xD8, 0x22, 0x86, 0x42, 0xA4, 0x77, 0xBD, 0xA2,
       0x55, 0xB3, 0x2A, 0xAD, 0xBC, 0xE4, 0xBD, 0xA0, 0xB3, 0xF7, 0xE3, 0x6C, 0x9D, 0xA7},
      {0x75, 0x38, 0x8B, 0x16, 0x51, 0x27, 0x76, 0xCC, 0x5D, 0xBA, 0x5D, 0xA1, 0xFD, 0x89,
       0x01, 0x50, 0xB0, 0xC6, 0x45, 0x5C, 0xB4, 0xF5, 0x8B, 0x19, 0x52, 0x52, 0x25, 0x25},
      {0xd1, 0x4a, 0x02, 0x8c, 0x2a, 0x3a, 0x2b, 0xc9, 0x47, 0x61, 0x02, 0xbb, 0x28, 0x82,
       0x34, 0xc4, 0x15, 0xa2, 0xb0, 0x1f, 0x82, 0x8e, 0xa6, 0x2a, 0xc5, 0xb3, 0xe4, 0x2f}};

  const size_t output_length = atfw::util::hash::sha::get_output_length(atfw::util::hash::sha::EN_ALGORITHM_SHA224);
  for (int i = 0; i < 3; ++i) {
    std::string hash = atfw::util::hash::sha::hash_to_binary(atfw::util::hash::sha::EN_ALGORITHM_SHA224,
                                                             sha1_test_buf[i], sha1_test_buflen[i]);
    CASE_EXPECT_EQ(hash, std::string(reinterpret_cast<const char*>(sha1_test_sum[i]), output_length));

    hash = atfw::util::hash::sha::hash_to_hex(atfw::util::hash::sha::EN_ALGORITHM_SHA224, sha1_test_buf[i],
                                              sha1_test_buflen[i]);
    std::string serialize_str;
    serialize_str.resize(output_length * 2, 0);
    atfw::util::string::dumphex(sha1_test_sum[i], output_length, &serialize_str[0]);
    CASE_EXPECT_EQ(hash, serialize_str);

    hash = atfw::util::hash::sha::hash_to_base64(atfw::util::hash::sha::EN_ALGORITHM_SHA224, sha1_test_buf[i],
                                                 sha1_test_buflen[i]);
    serialize_str.clear();
    atfw::util::base64_encode(serialize_str, sha1_test_sum[i], output_length);
    CASE_EXPECT_EQ(hash, serialize_str);
  }
}

CASE_TEST(sha, sha256) {
  const unsigned char sha1_test_buf[3][57] = {
      {"abc"}, {"abcdbcdecdefdefgefghfghighijhijkijkljklmklmnlmnomnopnopq"}, {""}};

  const size_t sha1_test_buflen[3] = {3, 56, 0};

  const unsigned char sha1_test_sum[3][32] = {
      {0xBA, 0x78, 0x16, 0xBF, 0x8F, 0x01, 0xCF, 0xEA, 0x41, 0x41, 0x40, 0xDE, 0x5D, 0xAE, 0x22, 0x23,
       0xB0, 0x03, 0x61, 0xA3, 0x96, 0x17, 0x7A, 0x9C, 0xB4, 0x10, 0xFF, 0x61, 0xF2, 0x00, 0x15, 0xAD},
      {0x24, 0x8D, 0x6A, 0x61, 0xD2, 0x06, 0x38, 0xB8, 0xE5, 0xC0, 0x26, 0x93, 0x0C, 0x3E, 0x60, 0x39,
       0xA3, 0x3C, 0xE4, 0x59, 0x64, 0xFF, 0x21, 0x67, 0xF6, 0xEC, 0xED, 0xD4, 0x19, 0xDB, 0x06, 0xC1},
      {0xe3, 0xb0, 0xc4, 0x42, 0x98, 0xfc, 0x1c, 0x14, 0x9a, 0xfb, 0xf4, 0xc8, 0x99, 0x6f, 0xb9, 0x24,
       0x27, 0xae, 0x41, 0xe4, 0x64, 0x9b, 0x93, 0x4c, 0xa4, 0x95, 0x99, 0x1b, 0x78, 0x52, 0xb8, 0x55}};

  const size_t output_length = atfw::util::hash::sha::get_output_length(atfw::util::hash::sha::EN_ALGORITHM_SHA256);
  for (int i = 0; i < 3; ++i) {
    std::string hash = atfw::util::hash::sha::hash_to_binary(atfw::util::hash::sha::EN_ALGORITHM_SHA256,
                                                             sha1_test_buf[i], sha1_test_buflen[i]);
    CASE_EXPECT_EQ(hash, std::string(reinterpret_cast<const char*>(sha1_test_sum[i]), output_length));

    hash = atfw::util::hash::sha::hash_to_hex(atfw::util::hash::sha::EN_ALGORITHM_SHA256, sha1_test_buf[i],
                                              sha1_test_buflen[i]);
    std::string serialize_str;
    serialize_str.resize(output_length * 2, 0);
    atfw::util::string::dumphex(sha1_test_sum[i], output_length, &serialize_str[0]);
    CASE_EXPECT_EQ(hash, serialize_str);

    hash = atfw::util::hash::sha::hash_to_base64(atfw::util::hash::sha::EN_ALGORITHM_SHA256, sha1_test_buf[i],
                                                 sha1_test_buflen[i]);
    serialize_str.clear();
    atfw::util::base64_encode(serialize_str, sha1_test_sum[i], output_length);
    CASE_EXPECT_EQ(hash, serialize_str);
  }
}

CASE_TEST(sha, sha384) {
  const unsigned char sha1_test_buf[3][113] = {{"abc"},
                                               {("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
                                                 "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu")},
                                               {""}};

  const size_t sha1_test_buflen[3] = {3, 112, 0};

  const unsigned char sha1_test_sum[3][64] = {
      {0xCB, 0x00, 0x75, 0x3F, 0x45, 0xA3, 0x5E, 0x8B, 0xB5, 0xA0, 0x3D, 0x69, 0x9A, 0xC6, 0x50, 0x07,
       0x27, 0x2C, 0x32, 0xAB, 0x0E, 0xDE, 0xD1, 0x63, 0x1A, 0x8B, 0x60, 0x5A, 0x43, 0xFF, 0x5B, 0xED,
       0x80, 0x86, 0x07, 0x2B, 0xA1, 0xE7, 0xCC, 0x23, 0x58, 0xBA, 0xEC, 0xA1, 0x34, 0xC8, 0x25, 0xA7},
      {0x09, 0x33, 0x0C, 0x33, 0xF7, 0x11, 0x47, 0xE8, 0x3D, 0x19, 0x2F, 0xC7, 0x82, 0xCD, 0x1B, 0x47,
       0x53, 0x11, 0x1B, 0x17, 0x3B, 0x3B, 0x05, 0xD2, 0x2F, 0xA0, 0x80, 0x86, 0xE3, 0xB0, 0xF7, 0x12,
       0xFC, 0xC7, 0xC7, 0x1A, 0x55, 0x7E, 0x2D, 0xB9, 0x66, 0xC3, 0xE9, 0xFA, 0x91, 0x74, 0x60, 0x39},
      {0x38, 0xb0, 0x60, 0xa7, 0x51, 0xac, 0x96, 0x38, 0x4c, 0xd9, 0x32, 0x7e, 0xb1, 0xb1, 0xe3, 0x6a,
       0x21, 0xfd, 0xb7, 0x11, 0x14, 0xbe, 0x07, 0x43, 0x4c, 0x0c, 0xc7, 0xbf, 0x63, 0xf6, 0xe1, 0xda,
       0x27, 0x4e, 0xde, 0xbf, 0xe7, 0x6f, 0x65, 0xfb, 0xd5, 0x1a, 0xd2, 0xf1, 0x48, 0x98, 0xb9, 0x5b}};

  const size_t output_length = atfw::util::hash::sha::get_output_length(atfw::util::hash::sha::EN_ALGORITHM_SHA384);
  for (int i = 0; i < 3; ++i) {
    std::string hash = atfw::util::hash::sha::hash_to_binary(atfw::util::hash::sha::EN_ALGORITHM_SHA384,
                                                             sha1_test_buf[i], sha1_test_buflen[i]);
    CASE_EXPECT_EQ(hash, std::string(reinterpret_cast<const char*>(sha1_test_sum[i]), output_length));

    hash = atfw::util::hash::sha::hash_to_hex(atfw::util::hash::sha::EN_ALGORITHM_SHA384, sha1_test_buf[i],
                                              sha1_test_buflen[i]);
    std::string serialize_str;
    serialize_str.resize(output_length * 2, 0);
    atfw::util::string::dumphex(sha1_test_sum[i], output_length, &serialize_str[0]);
    CASE_EXPECT_EQ(hash, serialize_str);

    hash = atfw::util::hash::sha::hash_to_base64(atfw::util::hash::sha::EN_ALGORITHM_SHA384, sha1_test_buf[i],
                                                 sha1_test_buflen[i]);
    serialize_str.clear();
    atfw::util::base64_encode(serialize_str, sha1_test_sum[i], output_length);
    CASE_EXPECT_EQ(hash, serialize_str);
  }
}

CASE_TEST(sha, sha512) {
  const unsigned char sha1_test_buf[3][113] = {{"abc"},
                                               {("abcdefghbcdefghicdefghijdefghijkefghijklfghijklmghijklmn"
                                                 "hijklmnoijklmnopjklmnopqklmnopqrlmnopqrsmnopqrstnopqrstu")},
                                               {""}};

  const size_t sha1_test_buflen[3] = {3, 112, 0};

  const unsigned char sha1_test_sum[3][64] = {
      {0xDD, 0xAF, 0x35, 0xA1, 0x93, 0x61, 0x7A, 0xBA, 0xCC, 0x41, 0x73, 0x49, 0xAE, 0x20, 0x41, 0x31,
       0x12, 0xE6, 0xFA, 0x4E, 0x89, 0xA9, 0x7E, 0xA2, 0x0A, 0x9E, 0xEE, 0xE6, 0x4B, 0x55, 0xD3, 0x9A,
       0x21, 0x92, 0x99, 0x2A, 0x27, 0x4F, 0xC1, 0xA8, 0x36, 0xBA, 0x3C, 0x23, 0xA3, 0xFE, 0xEB, 0xBD,
       0x45, 0x4D, 0x44, 0x23, 0x64, 0x3C, 0xE8, 0x0E, 0x2A, 0x9A, 0xC9, 0x4F, 0xA5, 0x4C, 0xA4, 0x9F},
      {0x8E, 0x95, 0x9B, 0x75, 0xDA, 0xE3, 0x13, 0xDA, 0x8C, 0xF4, 0xF7, 0x28, 0x14, 0xFC, 0x14, 0x3F,
       0x8F, 0x77, 0x79, 0xC6, 0xEB, 0x9F, 0x7F, 0xA1, 0x72, 0x99, 0xAE, 0xAD, 0xB6, 0x88, 0x90, 0x18,
       0x50, 0x1D, 0x28, 0x9E, 0x49, 0x00, 0xF7, 0xE4, 0x33, 0x1B, 0x99, 0xDE, 0xC4, 0xB5, 0x43, 0x3A,
       0xC7, 0xD3, 0x29, 0xEE, 0xB6, 0xDD, 0x26, 0x54, 0x5E, 0x96, 0xE5, 0x5B, 0x87, 0x4B, 0xE9, 0x09},
      {0xcf, 0x83, 0xe1, 0x35, 0x7e, 0xef, 0xb8, 0xbd, 0xf1, 0x54, 0x28, 0x50, 0xd6, 0x6d, 0x80, 0x07,
       0xd6, 0x20, 0xe4, 0x05, 0x0b, 0x57, 0x15, 0xdc, 0x83, 0xf4, 0xa9, 0x21, 0xd3, 0x6c, 0xe9, 0xce,
       0x47, 0xd0, 0xd1, 0x3c, 0x5d, 0x85, 0xf2, 0xb0, 0xff, 0x83, 0x18, 0xd2, 0x87, 0x7e, 0xec, 0x2f,
       0x63, 0xb9, 0x31, 0xbd, 0x47, 0x41, 0x7a, 0x81, 0xa5, 0x38, 0x32, 0x7a, 0xf9, 0x27, 0xda, 0x3e}};

  const size_t output_length = atfw::util::hash::sha::get_output_length(atfw::util::hash::sha::EN_ALGORITHM_SHA512);
  for (int i = 0; i < 3; ++i) {
    std::string hash = atfw::util::hash::sha::hash_to_binary(atfw::util::hash::sha::EN_ALGORITHM_SHA512,
                                                             sha1_test_buf[i], sha1_test_buflen[i]);
    CASE_EXPECT_EQ(hash, std::string(reinterpret_cast<const char*>(sha1_test_sum[i]), output_length));

    hash = atfw::util::hash::sha::hash_to_hex(atfw::util::hash::sha::EN_ALGORITHM_SHA512, sha1_test_buf[i],
                                              sha1_test_buflen[i]);
    std::string serialize_str;
    serialize_str.resize(output_length * 2, 0);
    atfw::util::string::dumphex(sha1_test_sum[i], output_length, &serialize_str[0]);
    CASE_EXPECT_EQ(hash, serialize_str);

    hash = atfw::util::hash::sha::hash_to_base64(atfw::util::hash::sha::EN_ALGORITHM_SHA512, sha1_test_buf[i],
                                                 sha1_test_buflen[i]);
    serialize_str.clear();
    atfw::util::base64_encode(serialize_str, sha1_test_sum[i], output_length);
    CASE_EXPECT_EQ(hash, serialize_str);
  }
}
