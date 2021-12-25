// Copyright 2021 atframework

#include <stdint.h>

#include "frame/test_macros.h"

#include "config/compiler_features.h"

#include "common/file_system.h"

#include "config/ini_loader.h"

CASE_TEST(ini_loader, number) {
  std::string conf_path;
  LIBATFRAME_UTILS_NAMESPACE_ID::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!LIBATFRAME_UTILS_NAMESPACE_ID::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.vector" << std::endl;
    return;
  }

  LIBATFRAME_UTILS_NAMESPACE_ID::config::ini_loader cfg_loader;
  cfg_loader.load_file(conf_path);

  int t1 = 9999, t2 = 9999;
  cfg_loader.dump_to("a.b.c1", t1);
  CASE_EXPECT_EQ(123, t1);
  cfg_loader.dump_to("a.b.c4", t2, true);
  CASE_EXPECT_EQ(0, t2);

  float t3 = 0.0;
  cfg_loader.dump_to("a.b.c2", t3);
  CASE_EXPECT_EQ(1.23f, t3);
}

CASE_TEST(ini_loader, string) {
  std::string conf_path;
  LIBATFRAME_UTILS_NAMESPACE_ID::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!LIBATFRAME_UTILS_NAMESPACE_ID::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.vector" << std::endl;
    return;
  }

  LIBATFRAME_UTILS_NAMESPACE_ID::config::ini_loader cfg_loader;
  cfg_loader.load_file(conf_path);

  char t1[32] = {0};
  std::string t2, t3 = "0000000000000000";
  std::string t4, t5;

  cfg_loader.dump_to("d.c.b.a.e.f1", t2);
  cfg_loader.dump_to("d.c.b.a.e.f1", t3.begin(), t3.end());
  cfg_loader.dump_to("d.c.b.a.e.f2", t1);
  cfg_loader.dump_to("d.c.b.a.e.f2", t1 + 16, t1 + 32);
  cfg_loader.dump_to("a.b.c3", t4);
  cfg_loader.dump_to("d.c.b.a.e.f3", t5);
  CASE_EXPECT_EQ("123456789", t5);
}

CASE_TEST(ini_loader, vector) {
  std::string conf_path;
  LIBATFRAME_UTILS_NAMESPACE_ID::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!LIBATFRAME_UTILS_NAMESPACE_ID::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.vector" << std::endl;
    return;
  }

  LIBATFRAME_UTILS_NAMESPACE_ID::config::ini_loader cfg_loader;
  cfg_loader.load_file(conf_path);

  std::vector<std::string> t1;
  std::list<bool> t2;
  cfg_loader.dump_to("a.arr", t1);
  cfg_loader.dump_to("a.bool", t2);

  CASE_EXPECT_EQ(5, t1.size());
  CASE_EXPECT_EQ(8, t2.size());

  CASE_EXPECT_EQ("1", t1[0]);
  CASE_EXPECT_EQ("2", t1[1]);
  CASE_EXPECT_EQ("3", t1[2]);
  CASE_EXPECT_EQ("/usr/local/gcc-4.8.2", t1[4]);

  auto t2_iter = t2.begin();
  while (t2_iter != t2.end()) {
    CASE_EXPECT_TRUE(*t2_iter);
    ++t2_iter;
    CASE_EXPECT_FALSE(*t2_iter);
    ++t2_iter;
  }
}

CASE_TEST(ini_loader, duration_value) {
  std::string conf_path;
  LIBATFRAME_UTILS_NAMESPACE_ID::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!LIBATFRAME_UTILS_NAMESPACE_ID::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.duration_value"
                    << std::endl;
    return;
  }

  LIBATFRAME_UTILS_NAMESPACE_ID::config::ini_loader cfg_loader;
  cfg_loader.load_file(conf_path);

  LIBATFRAME_UTILS_NAMESPACE_ID::config::duration_value dur;
  cfg_loader.dump_to("system.interval_ns", dur, true);
  CASE_EXPECT_EQ(0, dur.sec);
  CASE_EXPECT_EQ(123, dur.nsec);

  cfg_loader.dump_to("system.interval_us", dur, true);
  CASE_EXPECT_EQ(0, dur.sec);
  CASE_EXPECT_EQ(123000, dur.nsec);

  cfg_loader.dump_to("system.interval_ms", dur, true);
  CASE_EXPECT_EQ(0, dur.sec);
  CASE_EXPECT_EQ(123000000, dur.nsec);

  cfg_loader.dump_to("system.interval_s", dur, true);
  CASE_EXPECT_EQ(123, dur.sec);
  CASE_EXPECT_EQ(0, dur.nsec);

  cfg_loader.dump_to("system.interval_m", dur, true);
  CASE_EXPECT_EQ(123 * 60, dur.sec);
  CASE_EXPECT_EQ(0, dur.nsec);

  cfg_loader.dump_to("system.interval_h", dur, true);
  CASE_EXPECT_EQ(123 * 3600, dur.sec);
  CASE_EXPECT_EQ(0, dur.nsec);

  cfg_loader.dump_to("system.interval_d", dur, true);
  CASE_EXPECT_EQ(123 * 86400, dur.sec);
  CASE_EXPECT_EQ(0, dur.nsec);

  cfg_loader.dump_to("system.interval_w", dur, true);
  CASE_EXPECT_EQ(123 * 86400 * 7, dur.sec);
  CASE_EXPECT_EQ(0, dur.nsec);
}

CASE_TEST(ini_loader, map) {
  std::string conf_path;
  LIBATFRAME_UTILS_NAMESPACE_ID::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!LIBATFRAME_UTILS_NAMESPACE_ID::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.vector" << std::endl;
    return;
  }

  LIBATFRAME_UTILS_NAMESPACE_ID::config::ini_loader cfg_loader;
  cfg_loader.load_file(conf_path);

  LIBATFRAME_UTILS_NAMESPACE_ID::config::ini_value& map_value = cfg_loader.get_node("system.test");
  auto iter = map_value.get_children().find("map.string");
  CASE_EXPECT_TRUE(iter != map_value.get_children().end());
  if (iter != map_value.get_children().end()) {
    CASE_EXPECT_EQ("map.string=value", iter->second->as_cpp_string());
  }
}
