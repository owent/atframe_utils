#include <cstdlib>
#include <cstring>
#include <string>

#include "frame/test_macros.h"

#include "algorithm/base64.h"

static const unsigned char base64_test_dec[64] = {
    0x24, 0x48, 0x6E, 0x56, 0x87, 0x62, 0x5A, 0xBD, 0xBF, 0x17, 0xD9, 0xA2, 0xC4, 0x17, 0x1A, 0x01,
    0x94, 0xED, 0x8F, 0x1E, 0x11, 0xB3, 0xD7, 0x09, 0x0C, 0xB6, 0xE9, 0x10, 0x6F, 0x22, 0xEE, 0x13,
    0xCA, 0xB3, 0x07, 0x05, 0x76, 0xC9, 0xFA, 0x31, 0x6C, 0x08, 0x34, 0xFF, 0x8D, 0xC2, 0x6C, 0x38,
    0x00, 0x43, 0xE9, 0x54, 0x97, 0xAF, 0x50, 0x4B, 0xD1, 0x41, 0xBA, 0x95, 0x31, 0x5A, 0x0B, 0x97};

static const unsigned char base64_test_enc_standard[] =
    "JEhuVodiWr2/F9mixBcaAZTtjx4Rs9cJDLbpEG8i7hPK"
    "swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw==";

static const unsigned char base64_test_enc_utf7[] =
    "JEhuVodiWr2/F9mixBcaAZTtjx4Rs9cJDLbpEG8i7hPK"
    "swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw";

static const unsigned char base64_test_enc_imap[] =
    "JEhuVodiWr2,F9mixBcaAZTtjx4Rs9cJDLbpEG8i7hPK"
    "swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw";

static const unsigned char base64_test_enc_url[] =
    "JEhuVodiWr2_F9mixBcaAZTtjx4Rs9cJDLbpEG8i7hPK"
    "swcFdsn6MWwINP-Nwmw4AEPpVJevUEvRQbqVMVoLlw==";

CASE_TEST(base64, encode_standard) {
  unsigned char buffer[128] = {0};
  size_t len = 0;
  CASE_EXPECT_EQ(-1, util::base64_encode(nullptr, 0, &len, base64_test_dec, 64));
  CASE_EXPECT_EQ(len, 89);  // \0 for tail

  CASE_EXPECT_EQ(0, util::base64_encode(buffer, sizeof(buffer), &len, base64_test_dec, 64));
  CASE_EXPECT_EQ(len, 88);
  CASE_EXPECT_EQ(0, memcmp(base64_test_enc_standard, buffer, 88));

  std::string std_str_in;
  std::string std_str_out;
  std_str_in.assign(reinterpret_cast<const char *>(&base64_test_dec[0]),
                    reinterpret_cast<const char *>(&base64_test_dec[0]) + 64);

  CASE_EXPECT_EQ(0, util::base64_encode(std_str_out, base64_test_dec, 64));
  CASE_EXPECT_EQ(std_str_out.size(), 88);
  CASE_EXPECT_EQ(0, memcmp(base64_test_enc_standard, std_str_out.c_str(), 88));

  std_str_out.clear();

  CASE_EXPECT_EQ(0, util::base64_encode(std_str_out, std_str_in));
  CASE_EXPECT_EQ(std_str_out.size(), 88);
  CASE_EXPECT_EQ(0, memcmp(base64_test_enc_standard, std_str_out.c_str(), 88));
}

CASE_TEST(base64, decode_standard) {
  unsigned char buffer[128] = {0};
  size_t len = 0;
  CASE_EXPECT_EQ(-1, util::base64_decode(nullptr, 0, &len, base64_test_enc_standard, 88));
  CASE_EXPECT_EQ(len, 64);

  CASE_EXPECT_EQ(0, util::base64_decode(buffer, sizeof(buffer), &len, base64_test_enc_standard, 88));
  CASE_EXPECT_EQ(len, 64);
  CASE_EXPECT_EQ(0, memcmp(base64_test_dec, buffer, 64));

  std::string std_str_in;
  std::string std_str_out;
  std_str_in.assign(reinterpret_cast<const char *>(&base64_test_enc_standard[0]),
                    reinterpret_cast<const char *>(&base64_test_enc_standard[0]) + 88);

  CASE_EXPECT_EQ(0, util::base64_decode(std_str_out, base64_test_enc_standard, 88));
  CASE_EXPECT_EQ(std_str_out.size(), 64);
  CASE_EXPECT_EQ(0, memcmp(base64_test_dec, std_str_out.c_str(), 64));

  std_str_out.clear();

  CASE_EXPECT_EQ(0, util::base64_decode(std_str_out, std_str_in));
  CASE_EXPECT_EQ(std_str_out.size(), 64);
  CASE_EXPECT_EQ(0, memcmp(base64_test_dec, std_str_out.c_str(), 64));
}

CASE_TEST(base64, decode_no_padding) {
  std::string std_str_out;

  util::base64_decode(std_str_out, "YW55IGNhcm5hbCBwbGVhcw");
  CASE_EXPECT_EQ(0, memcmp("any carnal pleas", std_str_out.c_str(), 16));
  CASE_EXPECT_EQ(16, std_str_out.size());

  util::base64_decode(std_str_out, "YW55IGNhcm5hbCBwbGVhc3U");
  CASE_EXPECT_EQ(0, memcmp("any carnal pleasu", std_str_out.c_str(), 17));
  CASE_EXPECT_EQ(17, std_str_out.size());

  util::base64_decode(std_str_out, "YW55IGNhcm5hbCBwbGVhc3Vy");
  CASE_EXPECT_EQ(0, memcmp("any carnal pleasur", std_str_out.c_str(), 18));
  CASE_EXPECT_EQ(18, std_str_out.size());
}

CASE_TEST(base64, encode_no_padding) {
  std::string std_str_out;

  util::base64_encode(std_str_out, "any carnal pleas", util::base64_mode_t::EN_BMT_UTF7);
  CASE_EXPECT_EQ(0, memcmp("YW55IGNhcm5hbCBwbGVhcw", std_str_out.c_str(), 22));
  CASE_EXPECT_EQ(22, std_str_out.size());

  util::base64_encode(std_str_out, "any carnal pleasu", util::base64_mode_t::EN_BMT_UTF7);
  CASE_EXPECT_EQ(0, memcmp("YW55IGNhcm5hbCBwbGVhc3U", std_str_out.c_str(), 23));
  CASE_EXPECT_EQ(23, std_str_out.size());

  util::base64_encode(std_str_out, "any carnal pleasur", util::base64_mode_t::EN_BMT_UTF7);
  CASE_EXPECT_EQ(0, memcmp("YW55IGNhcm5hbCBwbGVhc3Vy", std_str_out.c_str(), 24));
  CASE_EXPECT_EQ(24, std_str_out.size());
}

CASE_TEST(base64, encode_utf7) {
  unsigned char buffer[128] = {0};
  size_t len = 0;
  CASE_EXPECT_EQ(-1, util::base64_encode(nullptr, 0, &len, base64_test_dec, 64, util::base64_mode_t::EN_BMT_UTF7));
  CASE_EXPECT_EQ(len, 87);  // \0 for tail

  CASE_EXPECT_EQ(
      0, util::base64_encode(buffer, sizeof(buffer), &len, base64_test_dec, 64, util::base64_mode_t::EN_BMT_UTF7));
  CASE_EXPECT_EQ(len, 86);
  CASE_EXPECT_EQ(0, memcmp(base64_test_enc_utf7, buffer, 86));
}

CASE_TEST(base64, decode_utf7) {
  unsigned char buffer[128] = {0};
  size_t len = 0;
  CASE_EXPECT_EQ(-1, util::base64_decode(nullptr, 0, &len, base64_test_enc_utf7, 86, util::base64_mode_t::EN_BMT_UTF7));
  CASE_EXPECT_EQ(len, 64);

  CASE_EXPECT_EQ(
      0, util::base64_decode(buffer, sizeof(buffer), &len, base64_test_enc_utf7, 86, util::base64_mode_t::EN_BMT_UTF7));
  CASE_EXPECT_EQ(len, 64);
  CASE_EXPECT_EQ(0, memcmp(base64_test_dec, buffer, 64));
}

CASE_TEST(base64, encode_imap) {
  unsigned char buffer[128] = {0};
  size_t len = 0;
  CASE_EXPECT_EQ(
      -1, util::base64_encode(nullptr, 0, &len, base64_test_dec, 64, util::base64_mode_t::EN_BMT_IMAP_MAILBOX_NAME));
  CASE_EXPECT_EQ(len, 87);  // \0 for tail

  CASE_EXPECT_EQ(0, util::base64_encode(buffer, sizeof(buffer), &len, base64_test_dec, 64,
                                        util::base64_mode_t::EN_BMT_IMAP_MAILBOX_NAME));
  CASE_EXPECT_EQ(len, 86);
  CASE_EXPECT_EQ(0, memcmp(base64_test_enc_imap, buffer, 86));
}

CASE_TEST(base64, decode_imap) {
  unsigned char buffer[128] = {0};
  size_t len = 0;
  CASE_EXPECT_EQ(-1, util::base64_decode(nullptr, 0, &len, base64_test_enc_imap, 86,
                                         util::base64_mode_t::EN_BMT_IMAP_MAILBOX_NAME));
  CASE_EXPECT_EQ(len, 64);

  CASE_EXPECT_EQ(0, util::base64_decode(buffer, sizeof(buffer), &len, base64_test_enc_imap, 86,
                                        util::base64_mode_t::EN_BMT_IMAP_MAILBOX_NAME));
  CASE_EXPECT_EQ(len, 64);
  CASE_EXPECT_EQ(0, memcmp(base64_test_dec, buffer, 64));
}

CASE_TEST(base64, encode_url) {
  unsigned char buffer[128] = {0};
  size_t len = 0;
  CASE_EXPECT_EQ(
      -1, util::base64_encode(nullptr, 0, &len, base64_test_dec, 64, util::base64_mode_t::EN_BMT_URL_FILENAME_SAFE));
  CASE_EXPECT_EQ(len, 89);  // \0 for tail

  CASE_EXPECT_EQ(0, util::base64_encode(buffer, sizeof(buffer), &len, base64_test_dec, 64,
                                        util::base64_mode_t::EN_BMT_URL_FILENAME_SAFE));
  CASE_EXPECT_EQ(len, 88);
  CASE_EXPECT_EQ(0, memcmp(base64_test_enc_url, buffer, 88));
}

CASE_TEST(base64, decode_url) {
  unsigned char buffer[128] = {0};
  size_t len = 0;
  CASE_EXPECT_EQ(-1, util::base64_decode(nullptr, 0, &len, base64_test_enc_url, 88,
                                         util::base64_mode_t::EN_BMT_URL_FILENAME_SAFE));
  CASE_EXPECT_EQ(len, 64);

  CASE_EXPECT_EQ(0, util::base64_decode(buffer, sizeof(buffer), &len, base64_test_enc_url, 88,
                                        util::base64_mode_t::EN_BMT_URL_FILENAME_SAFE));
  CASE_EXPECT_EQ(len, 64);
  CASE_EXPECT_EQ(0, memcmp(base64_test_dec, buffer, 64));
}
