// Copyright 2021 atframework

#include <map>
#include <vector>

#include "common/string_oprs.h"
#include "string/tquerystring.h"

#include "frame/test_macros.h"

CASE_TEST(string_oprs, version_compare) {
  CASE_EXPECT_EQ(-1, util_string_version_compare("1.0.0.0", "1.0.0.1"));
  CASE_EXPECT_EQ(1, util_string_version_compare("1.0.0.1", "1.0.0.0"));
  CASE_EXPECT_EQ(0, util_string_version_compare("1.0.0.1", "1.0.0.1"));

  CASE_EXPECT_EQ(0, util_string_version_compare("2.3.4.0", "2.3.4"));
  CASE_EXPECT_EQ(0, util_string_version_compare("0.2.3.4", ".2.3.4"));

  CASE_EXPECT_EQ(1, util_string_version_compare("1.2.3.4", "0.2.3.4"));
  CASE_EXPECT_EQ(-1, util_string_version_compare("1.2.3.4", "2.3.4"));
  CASE_EXPECT_EQ(1, util_string_version_compare("1.2.3.4", ".2.3.4"));

  CASE_EXPECT_EQ(-1, util_string_version_compare("0.2.3.4", "1.2.3.4"));
  CASE_EXPECT_EQ(1, util_string_version_compare("2.3.4", "1.2.3.4"));
  CASE_EXPECT_EQ(-1, util_string_version_compare(".2.3.4", "1.2.3.4"));

  CASE_EXPECT_EQ(0, util_string_version_compare("3.4.0.0.0.0", "3.4"));
  CASE_EXPECT_EQ(0, util_string_version_compare("", "0.0.0.0"));

  CASE_EXPECT_EQ(0, util_string_version_compare("1.2.3.4", "1.2.3.4"));
  CASE_EXPECT_EQ(0, util_string_version_compare("   1.2.3.4", "1.2.3.4"));
  CASE_EXPECT_EQ(0, util_string_version_compare("1  .2.3.4", "1.2.3.4"));
  CASE_EXPECT_EQ(0, util_string_version_compare("1.  2.3.4", "1.2.3.4"));
  CASE_EXPECT_EQ(0, util_string_version_compare("1.2  .3.4", "1.2.3.4"));
  CASE_EXPECT_EQ(0, util_string_version_compare("1.2  .  3.4", "1.2.3.4"));
  CASE_EXPECT_EQ(0, util_string_version_compare("1.2  .  3  .4", "1.2.3.4"));
  CASE_EXPECT_EQ(0, util_string_version_compare("1.2  .  3  .4  ", "1.2.3.4"));
}

CASE_TEST(string_oprs, version_normalize) {
  std::string t1 = util_string_version_normalize("1.2.3.4");
  std::string t2 = util_string_version_normalize("   \t\r  \n1. 2.   3  . 4 \t");
  std::string t3 = util_string_version_normalize("..3.4");
  std::string t4 = util_string_version_normalize("1.2..");
  std::string t5 = util_string_version_normalize("1...4");
  std::string t6 = util_string_version_normalize("1.2.0.0...");
  std::string t7 = util_string_version_normalize("0.0.0...");
  std::string t8 = util_string_version_normalize("....");
  std::string t9 = util_string_version_normalize("0");
  std::string t10 = util_string_version_normalize("");

  CASE_EXPECT_EQ("1.2.3.4", t1.c_str());
  CASE_EXPECT_EQ("1.2.3.4", t2.c_str());
  CASE_EXPECT_EQ("0.0.3.4", t3.c_str());
  CASE_EXPECT_EQ("1.2", t4.c_str());
  CASE_EXPECT_EQ("1.0.0.4", t5.c_str());
  CASE_EXPECT_EQ("1.2", t6.c_str());
  CASE_EXPECT_EQ("0", t7.c_str());
  CASE_EXPECT_EQ("0", t8.c_str());
  CASE_EXPECT_EQ("0", t9.c_str());
  CASE_EXPECT_EQ("0", t10.c_str());
}

CASE_TEST(string_oprs, to_int) {
  // hex
  CASE_EXPECT_EQ(0x1234, LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<int64_t>("0x1234"));
  CASE_EXPECT_EQ(0x1234, LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<int64_t>("0X1234"));

  // dex
  CASE_EXPECT_EQ(1234, LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<int64_t>("1234"));

  // oct
  CASE_EXPECT_EQ(668, LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<int64_t>("\\1234"));
}

CASE_TEST(tquerystring, encode_uri_utf8) {
  CASE_EXPECT_EQ("%E4%BD%A0%E5%A5%BD",
                 LIBATFRAME_UTILS_NAMESPACE_ID::uri::encode_uri_component("\xe4\xbd\xa0\xe5\xa5\xbd"));

  CASE_EXPECT_EQ("\xe4\xbd\xa0\xe5\xa5\xbd",
                 LIBATFRAME_UTILS_NAMESPACE_ID::uri::decode_uri_component("%E4%BD%A0%E5%A5%BD"));
}

CASE_TEST(string_oprs, trim) {
  const char *test_origin = "  \t \n \rtrim done\t\n";
  const char *test_after_trim_left = "trim done\t\n";
  const char *test_after_trim_right = "  \t \n \rtrim done";
  const char *test_after_trim_all = "trim done";

  {
    std::pair<const char *, size_t> trim_left_res =
        LIBATFRAME_UTILS_NAMESPACE_ID::string::trim(test_origin, 0, true, false);
    std::pair<const char *, size_t> trim_right_res =
        LIBATFRAME_UTILS_NAMESPACE_ID::string::trim(test_origin, 0, false, true);
    std::pair<const char *, size_t> trim_all_res = LIBATFRAME_UTILS_NAMESPACE_ID::string::trim(test_origin, 0);

    CASE_EXPECT_EQ(0, UTIL_STRFUNC_STRNCMP(trim_left_res.first, test_after_trim_left, trim_left_res.second));
    CASE_EXPECT_EQ(strlen(test_after_trim_left), trim_left_res.second);

    CASE_EXPECT_EQ(0, UTIL_STRFUNC_STRNCMP(trim_right_res.first, test_after_trim_right, trim_right_res.second));
    CASE_EXPECT_EQ(strlen(test_after_trim_right), trim_right_res.second);

    CASE_EXPECT_EQ(0, UTIL_STRFUNC_STRNCMP(trim_all_res.first, test_after_trim_all, trim_all_res.second));
    CASE_EXPECT_EQ(strlen(test_after_trim_all), trim_all_res.second);
  }

  {
    gsl::string_view trim_left_res = LIBATFRAME_UTILS_NAMESPACE_ID::string::trim_string(test_origin, true, false);
    gsl::string_view trim_right_res = LIBATFRAME_UTILS_NAMESPACE_ID::string::trim_string(test_origin, false, true);
    gsl::string_view trim_all_res = LIBATFRAME_UTILS_NAMESPACE_ID::string::trim_string(test_origin);

    CASE_EXPECT_EQ(0, UTIL_STRFUNC_STRNCMP(trim_left_res.data(), test_after_trim_left, trim_left_res.size()));
    CASE_EXPECT_EQ(strlen(test_after_trim_left), trim_left_res.size());

    CASE_EXPECT_EQ(0, UTIL_STRFUNC_STRNCMP(trim_right_res.data(), test_after_trim_right, trim_right_res.size()));
    CASE_EXPECT_EQ(strlen(test_after_trim_right), trim_right_res.size());

    CASE_EXPECT_EQ(0, UTIL_STRFUNC_STRNCMP(trim_all_res.data(), test_after_trim_all, trim_all_res.size()));
    CASE_EXPECT_EQ(strlen(test_after_trim_all), trim_all_res.size());
  }
}

CASE_TEST(string_oprs, reverse) {
  char t1[] = "abcdefg";
  char t2[] = "abcdefg";

  LIBATFRAME_UTILS_NAMESPACE_ID::string::reverse(&t1[0], nullptr);
  LIBATFRAME_UTILS_NAMESPACE_ID::string::reverse(t2, t2 + 7);

  CASE_EXPECT_EQ(t1, "gfedcba");
  CASE_EXPECT_EQ(t2, "gfedcba");
}

CASE_TEST(string_oprs, int2str) {
  char buffer[32];

  CASE_EXPECT_EQ(9, LIBATFRAME_UTILS_NAMESPACE_ID::string::int2str(&buffer[0], 9, 123456789U));
  buffer[9] = 0;
  CASE_EXPECT_EQ("123456789", buffer);

  CASE_EXPECT_EQ(10, LIBATFRAME_UTILS_NAMESPACE_ID::string::int2str(&buffer[0], sizeof(buffer), -123456789));
  buffer[10] = 0;
  CASE_EXPECT_EQ("-123456789", buffer);

  CASE_EXPECT_EQ(18, LIBATFRAME_UTILS_NAMESPACE_ID::string::int2str(&buffer[0], sizeof(buffer), 123456789123456789ULL));
  buffer[18] = 0;
  CASE_EXPECT_EQ("123456789123456789", buffer);

  CASE_EXPECT_EQ(19, LIBATFRAME_UTILS_NAMESPACE_ID::string::int2str(buffer, sizeof(buffer), -123456789123456789LL));
  buffer[19] = 0;
  CASE_EXPECT_EQ("-123456789123456789", buffer);

  CASE_EXPECT_EQ(0, LIBATFRAME_UTILS_NAMESPACE_ID::string::int2str(buffer, 0, 123456789U));
  CASE_EXPECT_EQ(0, LIBATFRAME_UTILS_NAMESPACE_ID::string::int2str(buffer, 8, 123456789U));
}
