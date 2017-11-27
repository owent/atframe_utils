#include <map>
#include <vector>

#include "common/string_oprs.h"
#include "frame/test_macros.h"

CASE_TEST(string_oprs, version_compare) {

    CASE_EXPECT_EQ(-1, util::string::version_compare("1.0.0.0", "1.0.0.1"));
    CASE_EXPECT_EQ(1, util::string::version_compare("1.0.0.1", "1.0.0.0"));
    CASE_EXPECT_EQ(0, util::string::version_compare("1.0.0.1", "1.0.0.1"));


    CASE_EXPECT_EQ(0, util::string::version_compare("2.3.4.0", "2.3.4"));
    CASE_EXPECT_EQ(0, util::string::version_compare("0.2.3.4", ".2.3.4"));

    CASE_EXPECT_EQ(1, util::string::version_compare("1.2.3.4", "0.2.3.4"));
    CASE_EXPECT_EQ(-1, util::string::version_compare("1.2.3.4", "2.3.4"));
    CASE_EXPECT_EQ(1, util::string::version_compare("1.2.3.4", ".2.3.4"));

    CASE_EXPECT_EQ(-1, util::string::version_compare("0.2.3.4", "1.2.3.4"));
    CASE_EXPECT_EQ(1, util::string::version_compare("2.3.4", "1.2.3.4"));
    CASE_EXPECT_EQ(-1, util::string::version_compare(".2.3.4", "1.2.3.4"));


    CASE_EXPECT_EQ(0, util::string::version_compare("3.4.0.0.0.0", "3.4"));


    CASE_EXPECT_EQ(0, util::string::version_compare("1.2.3.4", "1.2.3.4"));
    CASE_EXPECT_EQ(0, util::string::version_compare("   1.2.3.4", "1.2.3.4"));
    CASE_EXPECT_EQ(0, util::string::version_compare("1  .2.3.4", "1.2.3.4"));
    CASE_EXPECT_EQ(0, util::string::version_compare("1.  2.3.4", "1.2.3.4"));
    CASE_EXPECT_EQ(0, util::string::version_compare("1.2  .3.4", "1.2.3.4"));
    CASE_EXPECT_EQ(0, util::string::version_compare("1.2  .  3.4", "1.2.3.4"));
    CASE_EXPECT_EQ(0, util::string::version_compare("1.2  .  3  .4", "1.2.3.4"));
    CASE_EXPECT_EQ(0, util::string::version_compare("1.2  .  3  .4  ", "1.2.3.4"));
}

CASE_TEST(string_oprs, to_int) {
    // hex
    CASE_EXPECT_EQ(0x1234, util::string::to_int<int64_t>("0x1234"));
    CASE_EXPECT_EQ(0x1234, util::string::to_int<int64_t>("0X1234"));

    // dex
    CASE_EXPECT_EQ(1234, util::string::to_int<int64_t>("1234"));

    // oct
    CASE_EXPECT_EQ(668, util::string::to_int<int64_t>("\\1234"));
}
