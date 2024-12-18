// Copyright 2021 atframework

#include <algorithm>
#include <cstring>
#include <ctime>
#include <fstream>
#include <sstream>

#include "common/file_system.h"
#include "frame/test_macros.h"
#include "string/ac_automation.h"

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

CASE_TEST(ac_automation, basic) {
  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<> actree;

  actree.insert_keyword("acd");
  actree.insert_keyword("aceb");
  actree.insert_keyword("bef");
  actree.insert_keyword("cef");
  actree.insert_keyword("ef");

  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<>::value_type res = actree.match("acefcabefefefcevfefbc");

  CASE_EXPECT_EQ(5, res.size());
  if (!res.empty()) {
    CASE_EXPECT_EQ(1, res[0].start);
    CASE_EXPECT_EQ(3, res[0].length);

    CASE_EXPECT_EQ(6, res[1].start);
    CASE_EXPECT_EQ(3, res[1].length);
    CASE_EXPECT_EQ('b', res[1].keyword->at(0));
  }

  // dump/load
  std::stringstream dumped_data;
  actree.dump(dumped_data);
}

CASE_TEST(ac_automation, failed) {
  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<> actree;

  actree.insert_keyword("acd");
  actree.insert_keyword("aceb");
  actree.insert_keyword("bef");
  actree.insert_keyword("cef");
  actree.insert_keyword("ef");

  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<>::value_type res = actree.match("lolololnmmnmuiyt");

  CASE_EXPECT_EQ(0, res.size());
}

CASE_TEST(ac_automation, skip_space) {
  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<> actree;

  actree.insert_keyword("acd");
  actree.insert_keyword("aceb");
  actree.insert_keyword("bef");
  actree.insert_keyword("cef");
  actree.insert_keyword("ef");
  actree.set_skip(' ');

  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<>::value_type res =
      actree.match("ac  efca   b   e f efefcevfefbc");

  CASE_EXPECT_EQ(5, res.size());
  if (!res.empty()) {
    CASE_EXPECT_EQ(1, res[0].start);
    CASE_EXPECT_EQ(5, res[0].length);

    CASE_EXPECT_EQ(11, res[1].start);
    CASE_EXPECT_EQ(7, res[1].length);
    CASE_EXPECT_EQ('b', res[1].keyword->at(0));
  }
}

CASE_TEST(ac_automation, match_char) {
  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<> actree;

  actree.insert_keyword("a");

  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<>::value_type res = actree.match(" aaa");

  CASE_EXPECT_EQ(3, res.size());

  res = actree.match("a");
  CASE_EXPECT_EQ(1, res.size());

  res = actree.match("b");
  CASE_EXPECT_EQ(0, res.size());

  res = actree.match("");
  CASE_EXPECT_EQ(0, res.size());
}

CASE_TEST(ac_automation, prefix) {
  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<> actree;

  actree.insert_keyword("cb");
  actree.insert_keyword("abc");
  actree.insert_keyword("bc");
  actree.set_skip(' ');
  actree.set_skip('\n');

  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<>::value_type res = actree.match("ca b\ncd");

  CASE_EXPECT_EQ(1, res.size());
  if (!res.empty()) {
    CASE_EXPECT_EQ(1, res[0].start);
    CASE_EXPECT_EQ(5, res[0].length);
  }
}

CASE_TEST(ac_automation, skip) {
  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<> actree;

  actree.insert_keyword(U8_LITERALS("艹"));
  actree.insert_keyword(U8_LITERALS("测试脏字"));
  actree.insert_keyword(U8_LITERALS("试脏字"));
  actree.insert_keyword(U8_LITERALS("艹试脏"));
  actree.set_skip(' ');
  actree.set_skip('\t');
  actree.set_skip('\r');
  actree.set_skip('\n');

  std::string input = "小册老艹，我干死试测  试脏测  试脏\r\n字艹 试脏";
  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<>::value_type res = actree.match(input);

// CI may not support this encoding
#ifndef _MSC_VER
  CASE_EXPECT_EQ(3, res.size());
#endif

  std::stringstream ss;
  size_t in_idx = 0;
  for (size_t i = 0; i < res.size(); ++i) {
    ss.write(&input[in_idx], res[i].start - in_idx);
    ss << "**";
    in_idx = res[i].start + res[i].length;
  }

  if (in_idx < input.size()) {
    ss.write(&input[in_idx], input.size() - in_idx);
  }

  CASE_MSG_INFO() << "filter resault: " << ss.str() << std::endl;
}

CASE_TEST(ac_automation, dump_dot) {
  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<LIBATFRAME_UTILS_NAMESPACE_ID::string::utf8_char_t> actree;

  actree.insert_keyword(U8_LITERALS("艹"));
  actree.insert_keyword(U8_LITERALS("测试脏字"));
  actree.insert_keyword(U8_LITERALS("试脏字"));
  actree.insert_keyword(U8_LITERALS("艹试脏"));
  actree.set_skip(' ');
  actree.set_skip('\t');
  actree.set_skip('\r');
  actree.set_skip('\n');

  const char *node_options[] = {"shape=box",     "fontname = \"SimHei\"", "labelfontname = \"SimHei\"",
                                "fontsize = 14", "labelfontsize = 14",    nullptr};

  const char *edge_options[] = {"fontname = \"SimHei\"", "labelfontname = \"SimHei\"", "fontsize = 14",
                                "labelfontsize = 14", nullptr};
  actree.dump_dot(std::cout, nullptr, node_options, edge_options);

  std::fstream fos;
  fos.open("ac_automation.dump_dot.txt", std::ios::out);
  actree.dump_dot(fos, nullptr, node_options, edge_options);
}

CASE_TEST(ac_automation, load_and_dump) {
  LIBATFRAME_UTILS_NAMESPACE_ID::string::ac_automation<LIBATFRAME_UTILS_NAMESPACE_ID::string::utf8_char_t> actree;

  if (false == LIBATFRAME_UTILS_NAMESPACE_ID::file_system::is_exist("ac_automation.in.txt")) {
    return;
  }

  std::fstream fos, fis, fdot;
  fos.open("ac_automation.out.txt", std::ios::out);
  fis.open("ac_automation.in.txt");
  fdot.open("ac_automation.out.dot", std::ios::out);
  std::string line;
  while (std::getline(fis, line)) {
    if (!line.empty()) {
      actree.insert_keyword(line);
    }
  }

  const char *node_options[] = {"shape=box",     "fontname = \"SimHei\"", "labelfontname = \"SimHei\"",
                                "fontsize = 14", "labelfontsize = 14",    nullptr};
  const char *edge_options[] = {"fontname = \"SimHei\"", "labelfontname = \"SimHei\"", "fontsize = 14",
                                "labelfontsize = 14", nullptr};

  actree.dump(fos);
  actree.dump_dot(fdot, nullptr, node_options, edge_options);
}
