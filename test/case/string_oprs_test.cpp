// Copyright 2026 atframework

#include <map>
#include <string>
#include <vector>

#include "common/string_oprs.h"
#include "string/tquerystring.h"

#if defined(ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW) && ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW
#  include <string_view>
#endif

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
  CASE_EXPECT_EQ(0x1234, atfw::util::string::to_int<int64_t>("0x1234"));
  CASE_EXPECT_EQ(0x1234, atfw::util::string::to_int<int64_t>("0X1234"));

  // dex
  CASE_EXPECT_EQ(1234, atfw::util::string::to_int<int64_t>("1234"));

  // oct
  CASE_EXPECT_EQ(668, atfw::util::string::to_int<int64_t>("\\1234"));
}

CASE_TEST(tquerystring, encode_uri_utf8) {
  CASE_EXPECT_EQ("%E4%BD%A0%E5%A5%BD", atfw::util::uri::encode_uri_component("\xe4\xbd\xa0\xe5\xa5\xbd"));

  CASE_EXPECT_EQ("\xe4\xbd\xa0\xe5\xa5\xbd", atfw::util::uri::decode_uri_component("%E4%BD%A0%E5%A5%BD"));
}

CASE_TEST(string_oprs, trim) {
  const char *test_origin = "  \t \n \rtrim done\t\n";
  const char *test_after_trim_left = "trim done\t\n";
  const char *test_after_trim_right = "  \t \n \rtrim done";
  const char *test_after_trim_all = "trim done";

  {
    std::pair<const char *, size_t> trim_left_res = atfw::util::string::trim(test_origin, 0, true, false);
    std::pair<const char *, size_t> trim_right_res = atfw::util::string::trim(test_origin, 0, false, true);
    std::pair<const char *, size_t> trim_all_res = atfw::util::string::trim(test_origin, 0);

    CASE_EXPECT_EQ(0, UTIL_STRFUNC_STRNCMP(trim_left_res.first, test_after_trim_left, trim_left_res.second));
    CASE_EXPECT_EQ(strlen(test_after_trim_left), trim_left_res.second);

    CASE_EXPECT_EQ(0, UTIL_STRFUNC_STRNCMP(trim_right_res.first, test_after_trim_right, trim_right_res.second));
    CASE_EXPECT_EQ(strlen(test_after_trim_right), trim_right_res.second);

    CASE_EXPECT_EQ(0, UTIL_STRFUNC_STRNCMP(trim_all_res.first, test_after_trim_all, trim_all_res.second));
    CASE_EXPECT_EQ(strlen(test_after_trim_all), trim_all_res.second);
  }

  {
    gsl::string_view trim_left_res = atfw::util::string::trim_string(test_origin, true, false);
    gsl::string_view trim_right_res = atfw::util::string::trim_string(test_origin, false, true);
    gsl::string_view trim_all_res = atfw::util::string::trim_string(test_origin);

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

  atfw::util::string::reverse(&t1[0], nullptr);
  atfw::util::string::reverse(t2, t2 + 7);

  CASE_EXPECT_EQ(t1, "gfedcba");
  CASE_EXPECT_EQ(t2, "gfedcba");
}

CASE_TEST(string_oprs, string_equal) {
  atfw::util::nostd::string_view same("hello world");
  atfw::util::nostd::string_view same_other_case("HELLO WORLD");
  atfw::util::nostd::string_view same_mixed_case("HeLLo worLD");
  atfw::util::nostd::string_view different("hello, world");
  atfw::util::nostd::string_view prefix("hello worl");

  // 默认区分大小写，按字节精确比较
  CASE_EXPECT_TRUE(atfw::util::string::string_equal(same, atfw::util::nostd::string_view("hello world")));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, same_other_case));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, different));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, prefix));

  // ignore_case=true 时 ASCII 字母不区分大小写，长度不同仍视为不相等
  CASE_EXPECT_TRUE(atfw::util::string::string_equal(same, same_other_case, true));
  CASE_EXPECT_TRUE(atfw::util::string::string_equal(same, same_mixed_case, true));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, different, true));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, prefix, true));

  // 数字和符号不参与大小写折叠
  CASE_EXPECT_TRUE(atfw::util::string::string_equal(atfw::util::nostd::string_view("user-42@AtFW"),
                                                    atfw::util::nostd::string_view("USER-42@atfw"), true));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(atfw::util::nostd::string_view("user-42@AtFW"),
                                                     atfw::util::nostd::string_view("USER_42@atfw"), true));
}

CASE_TEST(string_oprs, string_equal_empty) {
  atfw::util::nostd::string_view empty;  // data() 为空指针
  atfw::util::nostd::string_view empty_literal("");
  atfw::util::nostd::string_view non_empty("x");

  CASE_EXPECT_TRUE(atfw::util::string::string_equal(empty, empty));
  CASE_EXPECT_TRUE(atfw::util::string::string_equal(empty, empty_literal));
  // 空视图不能把空指针传给 strncasecmp
  CASE_EXPECT_TRUE(atfw::util::string::string_equal(empty, empty, true));
  CASE_EXPECT_TRUE(atfw::util::string::string_equal(empty, empty_literal, true));

  CASE_EXPECT_FALSE(atfw::util::string::string_equal(empty, non_empty));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(non_empty, empty_literal));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(empty, non_empty, true));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(non_empty, empty, true));
}

CASE_TEST(string_oprs, string_equal_sources) {
  // 至少一侧使用显式的 nostd::string_view，避免与 std::string_view 重载产生调用歧义
  atfw::util::nostd::string_view literal_source("Hello");
  std::string string_source("hello");

  CASE_EXPECT_TRUE(atfw::util::string::string_equal(literal_source, "Hello"));
  CASE_EXPECT_TRUE(atfw::util::string::string_equal(literal_source, string_source, true));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(literal_source, "hello"));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(literal_source, string_source));

  // 比较使用视图长度，内嵌的 '\0' 也是内容的一部分
  std::string with_nul_same("a\0b", 3);
  std::string with_nul_diff("a\0c", 3);
  atfw::util::nostd::string_view nul_lhs(with_nul_same);

  CASE_EXPECT_TRUE(atfw::util::string::string_equal(nul_lhs, with_nul_same));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(nul_lhs, with_nul_diff));
}

#if defined(ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW) && ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW
CASE_TEST(string_oprs, string_equal_stl_string_view) {
  std::string_view same("hello world");
  std::string_view upper("HELLO WORLD");
  std::string_view different("hello, world");
  std::string_view prefix("hello worl");

  CASE_EXPECT_TRUE(atfw::util::string::string_equal(same, std::string_view("hello world")));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, upper));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, different));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, prefix));

  CASE_EXPECT_TRUE(atfw::util::string::string_equal(same, upper, true));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, different, true));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(same, prefix, true));

  std::string_view empty;  // data() 可能为空指针
  CASE_EXPECT_TRUE(atfw::util::string::string_equal(empty, std::string_view(), true));
  CASE_EXPECT_FALSE(atfw::util::string::string_equal(empty, same, true));
}
#endif

CASE_TEST(string_oprs, int2str) {
  char buffer[32];

  CASE_EXPECT_EQ(9, atfw::util::string::int2str(&buffer[0], 9, 123456789U));
  buffer[9] = 0;
  CASE_EXPECT_EQ("123456789", buffer);

  CASE_EXPECT_EQ(10, atfw::util::string::int2str(&buffer[0], sizeof(buffer), -123456789));
  buffer[10] = 0;
  CASE_EXPECT_EQ("-123456789", buffer);

  CASE_EXPECT_EQ(18, atfw::util::string::int2str(&buffer[0], sizeof(buffer), 123456789123456789ULL));
  buffer[18] = 0;
  CASE_EXPECT_EQ("123456789123456789", buffer);

  CASE_EXPECT_EQ(19, atfw::util::string::int2str(buffer, sizeof(buffer), -123456789123456789LL));
  buffer[19] = 0;
  CASE_EXPECT_EQ("-123456789123456789", buffer);

  CASE_EXPECT_EQ(0, atfw::util::string::int2str(buffer, 0, 123456789U));
  CASE_EXPECT_EQ(0, atfw::util::string::int2str(buffer, 8, 123456789U));
}
