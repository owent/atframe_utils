// Copyright 2026 atframework

#include <stdint.h>

#include "frame/test_macros.h"

#include "config/compiler_features.h"

#include "common/file_system.h"

#include "config/ini_loader.h"

CASE_TEST(ini_loader, number) {
  std::string conf_path;
  atfw::util::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!atfw::util::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.vector" << std::endl;
    return;
  }

  atfw::util::config::ini_loader cfg_loader;
  cfg_loader.load_file(conf_path);

  int t1 = 9999, t2 = 9999;
  cfg_loader.dump_to("a.b.c1", t1);
  CASE_EXPECT_EQ(123, t1);
  cfg_loader.dump_to("a.b.c4", t2, true);
  CASE_EXPECT_EQ(0, t2);

  float t3 = 0.0;
  cfg_loader.dump_to("a.b.c2", t3);
  CASE_EXPECT_LE(std::abs(1.23f - t3), std::numeric_limits<float>::epsilon());
}

CASE_TEST(ini_loader, string) {
  std::string conf_path;
  atfw::util::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!atfw::util::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.vector" << std::endl;
    return;
  }

  atfw::util::config::ini_loader cfg_loader;
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
  atfw::util::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!atfw::util::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.vector" << std::endl;
    return;
  }

  atfw::util::config::ini_loader cfg_loader;
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
  atfw::util::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!atfw::util::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.duration_value"
                    << std::endl;
    return;
  }

  atfw::util::config::ini_loader cfg_loader;
  cfg_loader.load_file(conf_path);

  atfw::util::config::duration_value dur;
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
  atfw::util::file_system::dirname(__FILE__, 0, conf_path, 3);
  conf_path += "/sample/test.ini";

  if (!atfw::util::file_system::is_exist(conf_path.c_str())) {
    CASE_MSG_INFO() << CASE_MSG_FCOLOR(YELLOW) << conf_path << " not found, skip ini_loader.vector" << std::endl;
    return;
  }

  atfw::util::config::ini_loader cfg_loader;
  cfg_loader.load_file(conf_path);

  atfw::util::config::ini_value& map_value = cfg_loader.get_node("system.test");
  auto iter = map_value.get_children().find("map.string");
  CASE_EXPECT_TRUE(iter != map_value.get_children().end());
  if (iter != map_value.get_children().end()) {
    CASE_EXPECT_EQ("map.string=value", iter->second->as_cpp_string());
  }
}

CASE_TEST(ini_loader, ini_value_basic) {
  atfw::util::config::ini_value val;
  CASE_EXPECT_TRUE(val.empty());
  CASE_EXPECT_FALSE(val.has_data());
  CASE_EXPECT_EQ(static_cast<size_t>(0), val.size());

  val.add("hello");
  CASE_EXPECT_FALSE(val.empty());
  CASE_EXPECT_TRUE(val.has_data());
  CASE_EXPECT_EQ(static_cast<size_t>(1), val.size());
  CASE_EXPECT_EQ("hello", val.as_cpp_string());

  val.add("world");
  CASE_EXPECT_EQ(static_cast<size_t>(2), val.size());
  CASE_EXPECT_EQ("world", val.as_cpp_string(1));

  val.clear();
  CASE_EXPECT_TRUE(val.empty());
}

CASE_TEST(ini_loader, ini_value_children) {
  atfw::util::config::ini_value val;
  val["child1"].add("value1");
  val["child2"].add("value2");

  CASE_EXPECT_FALSE(val.empty());
  CASE_EXPECT_EQ("value1", val["child1"].as_cpp_string());
  CASE_EXPECT_EQ("value2", val["child2"].as_cpp_string());
}

CASE_TEST(ini_loader, ini_value_get_child_by_path) {
  atfw::util::config::ini_value val;
  val["a"]["b"]["c"].add("deep");

  auto child = val.get_child_by_path("a.b.c");
  CASE_EXPECT_TRUE(nullptr != child);
  if (child) {
    CASE_EXPECT_EQ("deep", child->as_cpp_string());
  }

  // Non-existent path
  auto missing = val.get_child_by_path("x.y.z");
  CASE_EXPECT_TRUE(nullptr == missing);
}

CASE_TEST(ini_loader, ini_value_as_integer_types) {
  atfw::util::config::ini_value val;
  val.add("42");

  CASE_EXPECT_EQ(42, val.as_int());
  CASE_EXPECT_EQ(42, val.as_int32());
  CASE_EXPECT_EQ(42, val.as_int64());
  CASE_EXPECT_EQ(42, val.as_short());
  CASE_EXPECT_EQ(42, val.as_long());
  CASE_EXPECT_EQ(42, val.as_longlong());
  CASE_EXPECT_EQ(42U, val.as_uint32());
  CASE_EXPECT_EQ(42U, val.as_uint64());
  CASE_EXPECT_EQ(static_cast<char>(42), val.as_char());
  CASE_EXPECT_EQ(static_cast<unsigned char>(42), val.as_uchar());
}

CASE_TEST(ini_loader, ini_value_as_float_types) {
  atfw::util::config::ini_value val;
  val.add("3.14");

  CASE_EXPECT_LE(std::abs(3.14f - val.as_float()), 0.01f);
  CASE_EXPECT_LE(std::abs(3.14 - val.as_double()), 0.01);
}

CASE_TEST(ini_loader, ini_value_out_of_range_index) {
  atfw::util::config::ini_value val;
  val.add("hello");

  // Index out of range should return empty/default
  CASE_EXPECT_TRUE(val.as_cpp_string(99).empty());
  atfw::util::config::duration_value dur = val.as_duration(99);
  CASE_EXPECT_EQ(0, dur.sec);
  CASE_EXPECT_EQ(0, dur.nsec);
}

CASE_TEST(ini_loader, ini_value_copy) {
  atfw::util::config::ini_value val;
  val.add("data");
  val["child"].add("child_data");

  atfw::util::config::ini_value copy(val);
  CASE_EXPECT_EQ("data", copy.as_cpp_string());
  CASE_EXPECT_EQ("child_data", copy["child"].as_cpp_string());

  atfw::util::config::ini_value assigned;
  assigned = val;
  CASE_EXPECT_EQ("data", assigned.as_cpp_string());
}

CASE_TEST(ini_loader, ini_value_duration_direct) {
  // Test as_duration with various unit strings without loading from file
  atfw::util::config::ini_value val;

  val.add("500ms");
  atfw::util::config::duration_value dur = val.as_duration(0);
  CASE_EXPECT_EQ(0, dur.sec);
  CASE_EXPECT_EQ(500000000, dur.nsec);

  val.clear();
  val.add("2500us");
  dur = val.as_duration(0);
  CASE_EXPECT_EQ(0, dur.sec);
  CASE_EXPECT_EQ(2500000, dur.nsec);

  val.clear();
  val.add("5 seconds");
  dur = val.as_duration(0);
  CASE_EXPECT_EQ(5, dur.sec);

  val.clear();
  val.add("2 minutes");
  dur = val.as_duration(0);
  CASE_EXPECT_EQ(120, dur.sec);

  val.clear();
  val.add("1 hour");
  dur = val.as_duration(0);
  CASE_EXPECT_EQ(3600, dur.sec);

  val.clear();
  val.add("1 day");
  dur = val.as_duration(0);
  CASE_EXPECT_EQ(86400, dur.sec);

  val.clear();
  val.add("1 week");
  dur = val.as_duration(0);
  CASE_EXPECT_EQ(604800, dur.sec);

  val.clear();
  val.add("1000000000 nanoseconds");
  dur = val.as_duration(0);
  CASE_EXPECT_EQ(1, dur.sec);
  CASE_EXPECT_EQ(0, dur.nsec);

  val.clear();
  val.add("2000 milliseconds");
  dur = val.as_duration(0);
  CASE_EXPECT_EQ(2, dur.sec);
  CASE_EXPECT_EQ(0, dur.nsec);

  val.clear();
  val.add("1000000 microseconds");
  dur = val.as_duration(0);
  CASE_EXPECT_EQ(1, dur.sec);
  CASE_EXPECT_EQ(0, dur.nsec);
}
