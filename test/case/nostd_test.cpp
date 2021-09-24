// Copyright 2021 atframework

#include <cstdlib>
#include <cstring>
#include <map>
#include <string>

#include "frame/test_macros.h"

#include "nostd/string_view.h"

CASE_TEST(nostd_string_view, ctor) {
  {
    // Null.
    util::nostd::string_view s10;
    CASE_EXPECT_TRUE(s10.data() == nullptr);
    CASE_EXPECT_EQ(0, s10.length());
  }

  {
    // const char* without length.
    const char* hello = "hello";
    util::nostd::string_view s20(hello);
    CASE_EXPECT_TRUE(s20.data() == hello);
    CASE_EXPECT_EQ(5, s20.length());

    // const char* with length.
    util::nostd::string_view s21(hello, 4);
    CASE_EXPECT_TRUE(s21.data() == hello);
    CASE_EXPECT_EQ(4, s21.length());

    // Not recommended, but valid C++
    util::nostd::string_view s22(hello, 6);
    CASE_EXPECT_TRUE(s22.data() == hello);
    CASE_EXPECT_EQ(6, s22.length());
  }

  {
    // std::string.
    std::string hola = "hola";
    util::nostd::string_view s30(hola);
    CASE_EXPECT_TRUE(s30.data() == hola.data());
    CASE_EXPECT_EQ(4, s30.length());

    // std::string with embedded '\0'.
    hola.push_back('\0');
    hola.append("h2");
    hola.push_back('\0');
    util::nostd::string_view s31(hola);
    CASE_EXPECT_TRUE(s31.data() == hola.data());
    CASE_EXPECT_EQ(8, s31.length());
  }
}

CASE_TEST(nostd_string_view, swap) {
  util::nostd::string_view a("a");
  util::nostd::string_view b("bbb");
  CASE_EXPECT_TRUE(noexcept(a.swap(b)));
  a.swap(b);
  CASE_EXPECT_EQ(a, "bbb");
  CASE_EXPECT_EQ(b, "a");
  a.swap(b);
  CASE_EXPECT_EQ(a, "a");
  CASE_EXPECT_EQ(b, "bbb");
}

CASE_TEST(nostd_string_view, stl_comparator) {
  std::string s1("foo");
  std::string s2("bar");
  std::string s3("baz");

  util::nostd::string_view p1(s1);
  util::nostd::string_view p2(s2);
  util::nostd::string_view p3(s3);

  typedef std::map<util::nostd::string_view, int> TestMap;
  TestMap map;

  map.insert(std::make_pair(p1, 0));
  map.insert(std::make_pair(p2, 1));
  map.insert(std::make_pair(p3, 2));
  CASE_EXPECT_EQ(map.size(), 3);
  CASE_EXPECT_TRUE(p1 == s1);
  CASE_EXPECT_FALSE(p1 != s1);
  CASE_EXPECT_TRUE(p1 != nullptr);
  CASE_EXPECT_FALSE(p1 == nullptr);
  CASE_EXPECT_TRUE(nullptr != p1);
  CASE_EXPECT_FALSE(nullptr == p1);

  TestMap::const_iterator iter = map.begin();
  CASE_EXPECT_EQ(iter->second, 1);
  ++iter;
  CASE_EXPECT_EQ(iter->second, 2);
  ++iter;
  CASE_EXPECT_EQ(iter->second, 0);
  ++iter;
  CASE_EXPECT_TRUE(iter == map.end());

  TestMap::iterator new_iter = map.find("zot");
  CASE_EXPECT_TRUE(new_iter == map.end());

  new_iter = map.find("bar");
  CASE_EXPECT_TRUE(new_iter != map.end());

  map.erase(new_iter);
  CASE_EXPECT_EQ(map.size(), 2);

  iter = map.begin();
  CASE_EXPECT_EQ(iter->second, 2);
  ++iter;
  CASE_EXPECT_EQ(iter->second, 0);
  ++iter;
  CASE_EXPECT_TRUE(iter == map.end());
}

#define COMPARE(result, op, x, y)                                                         \
  CASE_EXPECT_EQ(result, util::nostd::string_view((x)) op util::nostd::string_view((y))); \
  CASE_EXPECT_EQ(result, util::nostd::string_view((x)).compare(util::nostd::string_view((y))) op 0)

CASE_TEST(nostd_string_view, comparison_operators) {
  COMPARE(true, ==, "", "");
  COMPARE(true, ==, "", util::nostd::string_view());
  COMPARE(true, ==, util::nostd::string_view(), "");
  COMPARE(true, ==, "a", "a");
  COMPARE(true, ==, "aa", "aa");
  COMPARE(false, ==, "a", "");
  COMPARE(false, ==, "", "a");
  COMPARE(false, ==, "a", "b");
  COMPARE(false, ==, "a", "aa");
  COMPARE(false, ==, "aa", "a");

  COMPARE(false, !=, "", "");
  COMPARE(false, !=, "a", "a");
  COMPARE(false, !=, "aa", "aa");
  COMPARE(true, !=, "a", "");
  COMPARE(true, !=, "", "a");
  COMPARE(true, !=, "a", "b");
  COMPARE(true, !=, "a", "aa");
  COMPARE(true, !=, "aa", "a");

  COMPARE(true, <, "a", "b");
  COMPARE(true, <, "a", "aa");
  COMPARE(true, <, "aa", "b");
  COMPARE(true, <, "aa", "bb");
  COMPARE(false, <, "a", "a");
  COMPARE(false, <, "b", "a");
  COMPARE(false, <, "aa", "a");
  COMPARE(false, <, "b", "aa");
  COMPARE(false, <, "bb", "aa");

  COMPARE(true, <=, "a", "a");
  COMPARE(true, <=, "a", "b");
  COMPARE(true, <=, "a", "aa");
  COMPARE(true, <=, "aa", "b");
  COMPARE(true, <=, "aa", "bb");
  COMPARE(false, <=, "b", "a");
  COMPARE(false, <=, "aa", "a");
  COMPARE(false, <=, "b", "aa");
  COMPARE(false, <=, "bb", "aa");

  COMPARE(false, >=, "a", "b");
  COMPARE(false, >=, "a", "aa");
  COMPARE(false, >=, "aa", "b");
  COMPARE(false, >=, "aa", "bb");
  COMPARE(true, >=, "a", "a");
  COMPARE(true, >=, "b", "a");
  COMPARE(true, >=, "aa", "a");
  COMPARE(true, >=, "b", "aa");
  COMPARE(true, >=, "bb", "aa");

  COMPARE(false, >, "a", "a");
  COMPARE(false, >, "a", "b");
  COMPARE(false, >, "a", "aa");
  COMPARE(false, >, "aa", "b");
  COMPARE(false, >, "aa", "bb");
  COMPARE(true, >, "b", "a");
  COMPARE(true, >, "aa", "a");
  COMPARE(true, >, "b", "aa");
  COMPARE(true, >, "bb", "aa");
}

CASE_TEST(nostd_string_view, comparison_operators_by_character_position) {
  std::string x;
  for (int i = 0; i < 256; i++) {
    x += 'a';
    std::string y = x;
    COMPARE(true, ==, x, y);
    for (int j = 0; j < i; j++) {
      std::string z = x;
      z[j] = 'b';  // Differs in position 'j'
      COMPARE(false, ==, x, z);
      COMPARE(true, <, x, z);
      COMPARE(true, >, z, x);
      if (j + 1 < i) {
        z[j + 1] = 'A';  // Differs in position 'j+1' as well
        COMPARE(false, ==, x, z);
        COMPARE(true, <, x, z);
        COMPARE(true, >, z, x);
        z[j + 1] = 'z';  // Differs in position 'j+1' as well
        COMPARE(false, ==, x, z);
        COMPARE(true, <, x, z);
        COMPARE(true, >, z, x);
      }
    }
  }
}
#undef COMPARE

// Sadly, our users often confuse std::string::npos with
// util::nostd::string_view::npos; So much so that we test here that they are the same.
// They need to both be unsigned, and both be the maximum-valued integer of
// their type.

template <typename T>
struct is_type {
  template <typename U>
  static bool same(U) {
    return false;
  }
  static bool same(T) { return true; }
};

CASE_TEST(nostd_string_view, npos_matches_std_string_view) {
  CASE_EXPECT_EQ(util::nostd::string_view::npos, std::string::npos);

  CASE_EXPECT_TRUE(is_type<size_t>::same(util::nostd::string_view::npos));
  CASE_EXPECT_FALSE(is_type<size_t>::same(""));

  // Make sure util::nostd::string_view::npos continues to be a header constant.
  char test[util::nostd::string_view::npos & 1] = {0};
  CASE_EXPECT_EQ(0, test[0]);
}

CASE_TEST(nostd_string_view, stl1) {
  const util::nostd::string_view a("abcdefghijklmnopqrstuvwxyz");
  const util::nostd::string_view b("abc");
  const util::nostd::string_view c("xyz");
  const util::nostd::string_view d("foobar");
  const util::nostd::string_view e;
  std::string temp("123");
  temp += '\0';
  temp += "456";
  const util::nostd::string_view f(temp);

  CASE_EXPECT_EQ(a[6], 'g');
  CASE_EXPECT_EQ(b[0], 'a');
  CASE_EXPECT_EQ(c[2], 'z');
  CASE_EXPECT_EQ(f[3], '\0');
  CASE_EXPECT_EQ(f[5], '5');

  CASE_EXPECT_EQ(*d.data(), 'f');
  CASE_EXPECT_EQ(d.data()[5], 'r');
  CASE_EXPECT_TRUE(e.data() == nullptr);

  CASE_EXPECT_EQ(*a.begin(), 'a');
  CASE_EXPECT_EQ(*(b.begin() + 2), 'c');
  CASE_EXPECT_EQ(*(c.end() - 1), 'z');

  CASE_EXPECT_EQ(*a.rbegin(), 'z');
  CASE_EXPECT_EQ(*(b.rbegin() + 2), 'a');
  CASE_EXPECT_EQ(*(c.rend() - 1), 'x');
  CASE_EXPECT_TRUE(a.rbegin() + 26 == a.rend());

  CASE_EXPECT_EQ(a.size(), 26);
  CASE_EXPECT_EQ(b.size(), 3);
  CASE_EXPECT_EQ(c.size(), 3);
  CASE_EXPECT_EQ(d.size(), 6);
  CASE_EXPECT_EQ(e.size(), 0);
  CASE_EXPECT_EQ(f.size(), 7);

  CASE_EXPECT_TRUE(!d.empty());
  CASE_EXPECT_TRUE(d.begin() != d.end());
  CASE_EXPECT_TRUE(d.begin() + 6 == d.end());

  CASE_EXPECT_TRUE(e.empty());
  CASE_EXPECT_TRUE(e.begin() == e.end());

  char buf[4] = {'%', '%', '%', '%'};
  CASE_EXPECT_EQ(a.copy(buf, 4), 4);
  CASE_EXPECT_EQ(buf[0], a[0]);
  CASE_EXPECT_EQ(buf[1], a[1]);
  CASE_EXPECT_EQ(buf[2], a[2]);
  CASE_EXPECT_EQ(buf[3], a[3]);
  CASE_EXPECT_EQ(a.copy(buf, 3, 7), 3);
  CASE_EXPECT_EQ(buf[0], a[7]);
  CASE_EXPECT_EQ(buf[1], a[8]);
  CASE_EXPECT_EQ(buf[2], a[9]);
  CASE_EXPECT_EQ(buf[3], a[3]);
  CASE_EXPECT_EQ(c.copy(buf, 99), 3);
  CASE_EXPECT_EQ(buf[0], c[0]);
  CASE_EXPECT_EQ(buf[1], c[1]);
  CASE_EXPECT_EQ(buf[2], c[2]);
  CASE_EXPECT_EQ(buf[3], a[3]);
}

// Separated from STL1() because some compilers produce an overly
// large stack frame for the combined function.
CASE_TEST(nostd_string_view, stl2) {
  const util::nostd::string_view a("abcdefghijklmnopqrstuvwxyz");
  const util::nostd::string_view b("abc");
  const util::nostd::string_view c("xyz");
  util::nostd::string_view d("foobar");
  const util::nostd::string_view e;
  const util::nostd::string_view f(
      "123"
      "\0"
      "456",
      7);

  d = util::nostd::string_view();
  CASE_EXPECT_EQ(d.size(), 0);
  CASE_EXPECT_TRUE(d.empty());
  CASE_EXPECT_TRUE(d.data() == nullptr);
  CASE_EXPECT_TRUE(d.begin() == d.end());

  CASE_EXPECT_EQ(a.find(b), 0);
  CASE_EXPECT_EQ(a.find(b, 1), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(a.find(c), 23);
  CASE_EXPECT_EQ(a.find(c, 9), 23);
  CASE_EXPECT_EQ(a.find(c, util::nostd::string_view::npos), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(b.find(c), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(b.find(c, util::nostd::string_view::npos), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(a.find(d), 0);
  CASE_EXPECT_EQ(a.find(e), 0);
  CASE_EXPECT_EQ(a.find(d, 12), 12);
  CASE_EXPECT_EQ(a.find(e, 17), 17);
  util::nostd::string_view g("xx not found bb");
  CASE_EXPECT_EQ(a.find(g), util::nostd::string_view::npos);
  // empty string nonsense
  CASE_EXPECT_EQ(d.find(b), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.find(b), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(d.find(b, 4), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.find(b, 7), util::nostd::string_view::npos);

  size_t empty_search_pos = std::string().find(std::string());
  CASE_EXPECT_EQ(d.find(d), empty_search_pos);
  CASE_EXPECT_EQ(d.find(e), empty_search_pos);
  CASE_EXPECT_EQ(e.find(d), empty_search_pos);
  CASE_EXPECT_EQ(e.find(e), empty_search_pos);
  CASE_EXPECT_EQ(d.find(d, 4), std::string().find(std::string(), 4));
  CASE_EXPECT_EQ(d.find(e, 4), std::string().find(std::string(), 4));
  CASE_EXPECT_EQ(e.find(d, 4), std::string().find(std::string(), 4));
  CASE_EXPECT_EQ(e.find(e, 4), std::string().find(std::string(), 4));

  CASE_EXPECT_EQ(a.find('a'), 0);
  CASE_EXPECT_EQ(a.find('c'), 2);
  CASE_EXPECT_EQ(a.find('z'), 25);
  CASE_EXPECT_EQ(a.find('$'), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(a.find('\0'), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(f.find('\0'), 3);
  CASE_EXPECT_EQ(f.find('3'), 2);
  CASE_EXPECT_EQ(f.find('5'), 5);
  CASE_EXPECT_EQ(g.find('o'), 4);
  CASE_EXPECT_EQ(g.find('o', 4), 4);
  CASE_EXPECT_EQ(g.find('o', 5), 8);
  CASE_EXPECT_EQ(a.find('b', 5), util::nostd::string_view::npos);
  // empty string nonsense
  CASE_EXPECT_EQ(d.find('\0'), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.find('\0'), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(d.find('\0', 4), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.find('\0', 7), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(d.find('x'), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.find('x'), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(d.find('x', 4), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.find('x', 7), util::nostd::string_view::npos);

  CASE_EXPECT_EQ(a.find(b.data(), 1, 0), 1);
  CASE_EXPECT_EQ(a.find(c.data(), 9, 0), 9);
  CASE_EXPECT_EQ(a.find(c.data(), util::nostd::string_view::npos, 0), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(b.find(c.data(), util::nostd::string_view::npos, 0), util::nostd::string_view::npos);
  // empty string nonsense
  CASE_EXPECT_EQ(d.find(b.data(), 4, 0), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.find(b.data(), 7, 0), util::nostd::string_view::npos);

  CASE_EXPECT_EQ(a.find(b.data(), 1), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(a.find(c.data(), 9), 23);
  CASE_EXPECT_EQ(a.find(c.data(), util::nostd::string_view::npos), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(b.find(c.data(), util::nostd::string_view::npos), util::nostd::string_view::npos);
  // empty string nonsense
  CASE_EXPECT_EQ(d.find(b.data(), 4), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.find(b.data(), 7), util::nostd::string_view::npos);

  CASE_EXPECT_EQ(a.rfind(b), 0);
  CASE_EXPECT_EQ(a.rfind(b, 1), 0);
  CASE_EXPECT_EQ(a.rfind(c), 23);
  CASE_EXPECT_EQ(a.rfind(c, 22), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(a.rfind(c, 1), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(a.rfind(c, 0), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(b.rfind(c), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(b.rfind(c, 0), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(a.rfind(d), std::string(a).rfind(std::string()));
  CASE_EXPECT_EQ(a.rfind(e), std::string(a).rfind(std::string()));
  CASE_EXPECT_EQ(a.rfind(d, 12), 12);
  CASE_EXPECT_EQ(a.rfind(e, 17), 17);
  CASE_EXPECT_EQ(a.rfind(g), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(d.rfind(b), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.rfind(b), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(d.rfind(b, 4), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.rfind(b, 7), util::nostd::string_view::npos);
  // empty string nonsense
  CASE_EXPECT_EQ(d.rfind(d, 4), std::string().rfind(std::string()));
  CASE_EXPECT_EQ(e.rfind(d, 7), std::string().rfind(std::string()));
  CASE_EXPECT_EQ(d.rfind(e, 4), std::string().rfind(std::string()));
  CASE_EXPECT_EQ(e.rfind(e, 7), std::string().rfind(std::string()));
  CASE_EXPECT_EQ(d.rfind(d), std::string().rfind(std::string()));
  CASE_EXPECT_EQ(e.rfind(d), std::string().rfind(std::string()));
  CASE_EXPECT_EQ(d.rfind(e), std::string().rfind(std::string()));
  CASE_EXPECT_EQ(e.rfind(e), std::string().rfind(std::string()));

  CASE_EXPECT_EQ(g.rfind('o'), 8);
  CASE_EXPECT_EQ(g.rfind('q'), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(g.rfind('o', 8), 8);
  CASE_EXPECT_EQ(g.rfind('o', 7), 4);
  CASE_EXPECT_EQ(g.rfind('o', 3), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(f.rfind('\0'), 3);
  CASE_EXPECT_EQ(f.rfind('\0', 12), 3);
  CASE_EXPECT_EQ(f.rfind('3'), 2);
  CASE_EXPECT_EQ(f.rfind('5'), 5);
  // empty string nonsense
  CASE_EXPECT_EQ(d.rfind('o'), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.rfind('o'), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(d.rfind('o', 4), util::nostd::string_view::npos);
  CASE_EXPECT_EQ(e.rfind('o', 7), util::nostd::string_view::npos);

  CASE_EXPECT_EQ(a.rfind(b.data(), 1, 0), 1);
  CASE_EXPECT_EQ(a.rfind(c.data(), 22, 0), 22);
  CASE_EXPECT_EQ(a.rfind(c.data(), 1, 0), 1);
  CASE_EXPECT_EQ(a.rfind(c.data(), 0, 0), 0);
  CASE_EXPECT_EQ(b.rfind(c.data(), 0, 0), 0);
  CASE_EXPECT_EQ(d.rfind(b.data(), 4, 0), 0);
  CASE_EXPECT_EQ(e.rfind(b.data(), 7, 0), 0);
}

// Continued from STL2
CASE_TEST(nostd_string_view, stl2_substr) {
  const util::nostd::string_view a("abcdefghijklmnopqrstuvwxyz");
  const util::nostd::string_view b("abc");
  const util::nostd::string_view c("xyz");
  util::nostd::string_view d("foobar");
  const util::nostd::string_view e;

  d = util::nostd::string_view();
  CASE_EXPECT_EQ(a.substr(0, 3), b);
  CASE_EXPECT_EQ(a.substr(23), c);
  CASE_EXPECT_EQ(a.substr(23, 3), c);
  CASE_EXPECT_EQ(a.substr(23, 99), c);
  CASE_EXPECT_EQ(a.substr(0), a);
  CASE_EXPECT_EQ(a.substr(), a);
  CASE_EXPECT_EQ(a.substr(3, 2), "de");
  // empty string nonsense
  CASE_EXPECT_EQ(d.substr(0, 99), e);
  // use of npos
  CASE_EXPECT_EQ(a.substr(0, util::nostd::string_view::npos), a);
  CASE_EXPECT_EQ(a.substr(23, util::nostd::string_view::npos), c);
}

CASE_TEST(nostd_string_view, find_conformance) {
  struct {
    std::string haystack;
    std::string needle;
  } specs[] = {
      {"", ""},      {"", "a"},      {"a", ""},      {"a", "a"},     {"a", "b"},       {"aa", ""},
      {"aa", "a"},   {"aa", "b"},    {"ab", "a"},    {"ab", "b"},    {"abcd", ""},     {"abcd", "a"},
      {"abcd", "d"}, {"abcd", "ab"}, {"abcd", "bc"}, {"abcd", "cd"}, {"abcd", "abcd"},
  };
  for (const auto& s : specs) {
    std::string st = s.haystack;
    util::nostd::string_view sp = s.haystack;
    for (size_t i = 0; i <= sp.size(); ++i) {
      size_t pos = (i == sp.size()) ? util::nostd::string_view::npos : i;
      CASE_EXPECT_EQ(sp.find(s.needle, pos), st.find(s.needle, pos));
      CASE_EXPECT_EQ(sp.rfind(s.needle, pos), st.rfind(s.needle, pos));
    }
  }
}

CASE_TEST(nostd_string_view, remove) {
  util::nostd::string_view a("foobar");
  std::string s1("123");
  s1 += '\0';
  s1 += "456";
  util::nostd::string_view e;
  std::string s2;

  // remove_prefix
  util::nostd::string_view c(a);
  c.remove_prefix(3);
  CASE_EXPECT_EQ(c, "bar");
  c = a;
  c.remove_prefix(0);
  CASE_EXPECT_EQ(c, a);
  c.remove_prefix(c.size());
  CASE_EXPECT_EQ(c, e);

  // remove_suffix
  c = a;
  c.remove_suffix(3);
  CASE_EXPECT_EQ(c, "foo");
  c = a;
  c.remove_suffix(0);
  CASE_EXPECT_EQ(c, a);
  c.remove_suffix(c.size());
  CASE_EXPECT_EQ(c, e);
}

CASE_TEST(nostd_string_view, set) {
  util::nostd::string_view a("foobar");
  util::nostd::string_view empty;
  util::nostd::string_view b;

  // set
  b = util::nostd::string_view("foobar", 6);
  CASE_EXPECT_EQ(b, a);
  b = util::nostd::string_view("foobar", 0);
  CASE_EXPECT_EQ(b, empty);
  b = util::nostd::string_view("foobar", 7);
  CASE_EXPECT_NE(b, a);

  b = util::nostd::string_view("foobar");
  CASE_EXPECT_EQ(b, a);
}

CASE_TEST(nostd_string_view, front_back) {
  static const char arr[] = "abcd";
  const util::nostd::string_view csp(arr, 4);
  CASE_EXPECT_EQ(&arr[0], &csp.front());
  CASE_EXPECT_EQ(&arr[3], &csp.back());
}

CASE_TEST(nostd_string_view, front_back_single_char) {
  static const char c = 'a';
  const util::nostd::string_view csp(&c, 1);
  CASE_EXPECT_EQ(&c, &csp.front());
  CASE_EXPECT_EQ(&c, &csp.back());
}

CASE_TEST(nostd_string_view, null_input) {
  util::nostd::string_view s;
  CASE_EXPECT_EQ(s.data(), nullptr);
  CASE_EXPECT_EQ(s.size(), 0);

  CASE_EXPECT_EQ("", std::string(s));
}

CASE_TEST(nostd_string_view, comparisons2) {
  // The `compare` member has 6 overloads (v: string_view, s: const char*):
  //  (1) compare(v)
  //  (2) compare(pos1, count1, v)
  //  (3) compare(pos1, count1, v, pos2, count2)
  //  (4) compare(s)
  //  (5) compare(pos1, count1, s)
  //  (6) compare(pos1, count1, s, count2)

  util::nostd::string_view abc("abcdefghijklmnopqrstuvwxyz");

  // check comparison operations on strings longer than 4 bytes.
  CASE_EXPECT_EQ(abc, util::nostd::string_view("abcdefghijklmnopqrstuvwxyz"));
  CASE_EXPECT_EQ(abc.compare(util::nostd::string_view("abcdefghijklmnopqrstuvwxyz")), 0);

  CASE_EXPECT_LT(abc, util::nostd::string_view("abcdefghijklmnopqrstuvwxzz"));
  CASE_EXPECT_LT(abc.compare(util::nostd::string_view("abcdefghijklmnopqrstuvwxzz")), 0);

  CASE_EXPECT_GT(abc, util::nostd::string_view("abcdefghijklmnopqrstuvwxyy"));
  CASE_EXPECT_GT(abc.compare(util::nostd::string_view("abcdefghijklmnopqrstuvwxyy")), 0);

  // The "substr" variants of `compare`.
  util::nostd::string_view digits("0123456789");
  auto npos = util::nostd::string_view::npos;

  // Taking string_view
  CASE_EXPECT_EQ(digits.compare(3, npos, util::nostd::string_view("3456789")), 0);  // 2
  CASE_EXPECT_EQ(digits.compare(3, 4, util::nostd::string_view("3456")), 0);        // 2
  CASE_EXPECT_EQ(digits.compare(10, 0, util::nostd::string_view()), 0);             // 2
  CASE_EXPECT_EQ(digits.compare(3, 4, util::nostd::string_view("0123456789"), 3, 4),
                 0);  // 3
  CASE_EXPECT_LT(digits.compare(3, 4, util::nostd::string_view("0123456789"), 3, 5),
                 0);  // 3
  CASE_EXPECT_LT(digits.compare(0, npos, util::nostd::string_view("0123456789"), 3, 5),
                 0);  // 3
  // Taking const char*
  CASE_EXPECT_EQ(digits.compare(3, 4, "3456"), 0);                 // 5
  CASE_EXPECT_EQ(digits.compare(3, npos, "3456789"), 0);           // 5
  CASE_EXPECT_EQ(digits.compare(10, 0, ""), 0);                    // 5
  CASE_EXPECT_EQ(digits.compare(3, 4, "0123456789", 3, 4), 0);     // 6
  CASE_EXPECT_LT(digits.compare(3, 4, "0123456789", 3, 5), 0);     // 6
  CASE_EXPECT_LT(digits.compare(0, npos, "0123456789", 3, 5), 0);  // 6
}

CASE_TEST(nostd_string_view, at) {
  util::nostd::string_view abc = "abc";
  CASE_EXPECT_EQ(abc.at(0), 'a');
  CASE_EXPECT_EQ(abc.at(1), 'b');
  CASE_EXPECT_EQ(abc.at(2), 'c');
}

CASE_TEST(nostd_string_view, explicit_conversion_operator) {
  util::nostd::string_view sp = "hi";
  CASE_EXPECT_EQ(sp, std::string(sp));
}

#if defined(__cplusplus) && __cplusplus >= 201402L
static constexpr char ConstexprMethodsHelper() {
  util::nostd::string_view str("123", 3);
  str.remove_prefix(1);
  str.remove_suffix(1);
  util::nostd::string_view bar;
  str.swap(bar);
  return bar.front();
}

CASE_TEST(nostd_string_view, constexpr_methods) {
  // remove_prefix, remove_suffix, swap
  static_assert(ConstexprMethodsHelper() == '2', "");

  // substr
  constexpr util::nostd::string_view foobar("foobar", 6);
  constexpr util::nostd::string_view foo = foobar.substr(0, 3);
  constexpr util::nostd::string_view bar = foobar.substr(3);
  CASE_EXPECT_EQ(foo, "foo");
  CASE_EXPECT_EQ(bar, "bar");
}
#endif

CASE_TEST(nostd_string_view, with_noexcept) {
  CASE_EXPECT_TRUE((std::is_nothrow_constructible<util::nostd::string_view, const std::string&>::value));
  CASE_EXPECT_TRUE((std::is_nothrow_constructible<util::nostd::string_view, const std::string&>::value));
  CASE_EXPECT_TRUE(std::is_nothrow_constructible<util::nostd::string_view>::value);
  constexpr util::nostd::string_view sp;
  CASE_EXPECT_TRUE(noexcept(sp.begin()));
  CASE_EXPECT_TRUE(noexcept(sp.end()));
  CASE_EXPECT_TRUE(noexcept(sp.cbegin()));
  CASE_EXPECT_TRUE(noexcept(sp.cend()));
  CASE_EXPECT_TRUE(noexcept(sp.rbegin()));
  CASE_EXPECT_TRUE(noexcept(sp.rend()));
  CASE_EXPECT_TRUE(noexcept(sp.crbegin()));
  CASE_EXPECT_TRUE(noexcept(sp.crend()));
  CASE_EXPECT_TRUE(noexcept(sp.size()));
  CASE_EXPECT_TRUE(noexcept(sp.length()));
  CASE_EXPECT_TRUE(noexcept(sp.empty()));
  CASE_EXPECT_TRUE(noexcept(sp.data()));
  CASE_EXPECT_TRUE(noexcept(sp.compare(sp)));
  CASE_EXPECT_TRUE(noexcept(sp.find(sp)));
  CASE_EXPECT_TRUE(noexcept(sp.find('f')));
  CASE_EXPECT_TRUE(noexcept(sp.rfind(sp)));
  CASE_EXPECT_TRUE(noexcept(sp.rfind('f')));
}

CASE_TEST(nostd_string_view, string_compare_not_ambiguous) {
  CASE_EXPECT_EQ("hello", std::string("hello"));
  CASE_EXPECT_LT("hello", std::string("world"));
}

CASE_TEST(nostd_string_view, heterogenous_string_view_equals) {
  CASE_EXPECT_EQ(util::nostd::string_view("hello"), std::string("hello"));
  CASE_EXPECT_EQ("hello", util::nostd::string_view("hello"));
}

CASE_TEST(nostd_string_view, edge_cases) {
  util::nostd::string_view a("xxyyyxx");

  // Set a = "xyyyx".
  a.remove_prefix(1);
  a.remove_suffix(1);

  CASE_EXPECT_EQ(0, a.find('x'));
  CASE_EXPECT_EQ(0, a.find('x', 0));
  CASE_EXPECT_EQ(4, a.find('x', 1));
  CASE_EXPECT_EQ(4, a.find('x', 4));
  CASE_EXPECT_EQ(util::nostd::string_view::npos, a.find('x', 5));

  CASE_EXPECT_EQ(4, a.rfind('x'));
  CASE_EXPECT_EQ(4, a.rfind('x', 5));
  CASE_EXPECT_EQ(4, a.rfind('x', 4));
  CASE_EXPECT_EQ(0, a.rfind('x', 3));
  CASE_EXPECT_EQ(0, a.rfind('x', 0));

  // Set a = "yyy".
  a.remove_prefix(1);
  a.remove_suffix(1);

  CASE_EXPECT_EQ(util::nostd::string_view::npos, a.find('x'));
  CASE_EXPECT_EQ(util::nostd::string_view::npos, a.rfind('x'));
}

CASE_TEST(nostd_string_view, ostream) {
  // Width should reset after one formatted write.
  // If we weren't resetting width after formatting the string_view,
  // we'd have width=5 carrying over to the printing of the "]",
  // creating "[###hi####]".
  std::string s = "hi";
  std::ostringstream oss;
  oss << s << " ostream";

  CASE_EXPECT_EQ(std::string("hi ostream"), oss.str());
}
