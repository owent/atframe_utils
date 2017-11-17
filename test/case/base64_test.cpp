#include <cstdlib>
#include <cstring>

#include "frame/test_macros.h"

#include "algorithm/base64.h"


static const unsigned char base64_test_dec[64] = {
    0x24, 0x48, 0x6E, 0x56, 0x87, 0x62, 0x5A, 0xBD, 0xBF, 0x17, 0xD9, 0xA2, 0xC4, 0x17, 0x1A, 0x01, 0x94, 0xED, 0x8F, 0x1E, 0x11, 0xB3,
    0xD7, 0x09, 0x0C, 0xB6, 0xE9, 0x10, 0x6F, 0x22, 0xEE, 0x13, 0xCA, 0xB3, 0x07, 0x05, 0x76, 0xC9, 0xFA, 0x31, 0x6C, 0x08, 0x34, 0xFF,
    0x8D, 0xC2, 0x6C, 0x38, 0x00, 0x43, 0xE9, 0x54, 0x97, 0xAF, 0x50, 0x4B, 0xD1, 0x41, 0xBA, 0x95, 0x31, 0x5A, 0x0B, 0x97};

static const unsigned char base64_test_enc[] = "JEhuVodiWr2/F9mixBcaAZTtjx4Rs9cJDLbpEG8i7hPK"
                                               "swcFdsn6MWwINP+Nwmw4AEPpVJevUEvRQbqVMVoLlw==";

CASE_TEST(base64, encode) {
    unsigned char buffer[128] = {0};
    size_t len = 0;
    CASE_EXPECT_EQ(-1, util::base64_encode(NULL, 0, &len, base64_test_dec, 64));
    CASE_EXPECT_EQ(len, 89); // \0 for tail

    CASE_EXPECT_EQ(0, util::base64_encode(buffer, sizeof(buffer), &len, base64_test_dec, 64));
    CASE_EXPECT_EQ(len, 88);
    CASE_EXPECT_EQ(0, memcmp(base64_test_enc, buffer, 88));
}

CASE_TEST(base64, decode) {
    unsigned char buffer[128] = {0};
    size_t len = 0;
    CASE_EXPECT_EQ(-1, util::base64_decode(NULL, 0, &len, base64_test_enc, 88));
    CASE_EXPECT_EQ(len, 64);

    CASE_EXPECT_EQ(0, util::base64_decode(buffer, sizeof(buffer), &len, base64_test_enc, 88));
    CASE_EXPECT_EQ(len, 64);
    CASE_EXPECT_EQ(0, memcmp(base64_test_dec, buffer, 64));
}