// Copyright 2021 atframework

#include <cstdlib>
#include <cstring>
#include <string>

#include "frame/test_macros.h"

#include "string/utf8_char_t.h"

#if defined(_MSC_VER) && _MSC_VER >= 1900
#  define U8_LITERALS(x) (const char *)(u8##x)
#elif defined(__clang__)
// apple clang
#  if defined(__apple_build_version__)
#    if ((__clang_major__ * 100) + __clang_minor__) >= 600
#      define U8_LITERALS(x) (const char *)(u8##x)
#    endif
#  else
// clang
#    if ((__clang_major__ * 100) + __clang_minor__) >= 306
#      define U8_LITERALS(x) (const char *)(u8##x)
#    endif
#  endif
#elif defined(__GNUC__) && (__GNUC__ * 100 + __GNUC_MINOR__) >= 600
#  define U8_LITERALS(x) (const char *)(u8##x)
#endif

#ifndef U8_LITERALS
#  define U8_LITERALS(x) x
#endif

CASE_TEST(utf8, length) {
  CASE_EXPECT_EQ(0, atfw::util::string::utf8_char_t::length(nullptr));
  CASE_EXPECT_EQ(1, atfw::util::string::utf8_char_t::length("o"));
  CASE_EXPECT_EQ(3, atfw::util::string::utf8_char_t::length(U8_LITERALS("欧")));
}

CASE_TEST(utf8, equal) {
  atfw::util::string::utf8_char_t a(U8_LITERALS("欧"));
  atfw::util::string::utf8_char_t b(U8_LITERALS("o"));
  atfw::util::string::utf8_char_t c(U8_LITERALS("o"));
  atfw::util::string::utf8_char_t d(nullptr);
  CASE_EXPECT_EQ(1, c.length());
  CASE_EXPECT_EQ(1, d.length());
  CASE_EXPECT_EQ(3, a.length());

  CASE_EXPECT_TRUE(a == a);
  CASE_EXPECT_TRUE(b == c);
  CASE_EXPECT_TRUE(d == d);
  CASE_EXPECT_FALSE(a == b);
  CASE_EXPECT_FALSE(c == d);
}

CASE_TEST(utf8, string_length) {
  CASE_EXPECT_EQ(2, atfw::util::string::utf8_char_t::utf8_string_length(U8_LITERALS("欧o")));
  CASE_EXPECT_EQ(0, atfw::util::string::utf8_char_t::utf8_string_length(""));
}