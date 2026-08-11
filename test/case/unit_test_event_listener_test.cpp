// Copyright 2026 atframework

#include "frame/test_macros.h"

#include <string>
#include <vector>

struct unit_test_event_listener_records_t {
  int program_start_count = 0;
  int program_end_count = 0;
  std::vector<std::string> suite_started;
  std::vector<std::string> suite_ended;
  std::vector<std::string> case_started;
  std::vector<std::string> case_ended;
  std::vector<std::string> case_passed;
};

static unit_test_event_listener_records_t &unit_test_event_listener_get_records() {
  static unit_test_event_listener_records_t ret;
  return ret;
}

static std::string unit_test_event_listener_full_name(gsl::string_view suite_name, gsl::string_view case_name) {
  std::string ret(suite_name.data(), suite_name.size());
  ret += ".";
  ret.append(case_name.data(), case_name.size());
  return ret;
}

class unit_test_event_listener_recorder : public ::atfw::util::testing::test_event_listener {
 public:
  void on_test_program_start() override { ++unit_test_event_listener_get_records().program_start_count; }

  void on_test_program_end(int) override { ++unit_test_event_listener_get_records().program_end_count; }

  void on_test_suite_start(const ::atfw::util::testing::test_event_suite_info &info) override {
    unit_test_event_listener_get_records().suite_started.emplace_back(info.name_.data(), info.name_.size());
  }

  void on_test_suite_end(const ::atfw::util::testing::test_event_suite_info &info) override {
    unit_test_event_listener_get_records().suite_ended.emplace_back(info.name_.data(), info.name_.size());
  }

  void on_test_case_start(const ::atfw::util::testing::test_event_case_info &info) override {
    unit_test_event_listener_get_records().case_started.push_back(
        unit_test_event_listener_full_name(info.suite_name_, info.case_name_));
  }

  void on_test_case_end(const ::atfw::util::testing::test_event_case_info &info) override {
    unit_test_event_listener_get_records().case_ended.push_back(
        unit_test_event_listener_full_name(info.suite_name_, info.case_name_));
    if (info.passed_) {
      unit_test_event_listener_get_records().case_passed.push_back(
          unit_test_event_listener_full_name(info.suite_name_, info.case_name_));
    }
  }
};

struct unit_test_event_listener_registrar_t {
  unit_test_event_listener_registrar_t() {
    ::atfw::util::testing::append_test_event_listener(new unit_test_event_listener_recorder);
  }
};

static unit_test_event_listener_registrar_t unit_test_event_listener_registrar;

static bool unit_test_event_listener_contains(const std::vector<std::string> &haystack, const std::string &needle) {
  for (std::vector<std::string>::const_iterator iter = haystack.begin(); iter != haystack.end(); ++iter) {
    if (*iter == needle) {
      return true;
    }
  }
  return false;
}

CASE_TEST(unit_test_event_listener, case_first) {
  unit_test_event_listener_records_t &records = unit_test_event_listener_get_records();

  CASE_EXPECT_EQ(1, records.program_start_count);
  CASE_EXPECT_EQ(0, records.program_end_count);
  CASE_EXPECT_TRUE(unit_test_event_listener_contains(records.suite_started, "unit_test_event_listener"));
  CASE_EXPECT_TRUE(unit_test_event_listener_contains(records.case_started, "unit_test_event_listener.case_first"));
  CASE_EXPECT_FALSE(unit_test_event_listener_contains(records.case_ended, "unit_test_event_listener.case_first"));
  CASE_EXPECT_FALSE(unit_test_event_listener_contains(records.suite_ended, "unit_test_event_listener"));
}

CASE_TEST(unit_test_event_listener, case_second) {
  unit_test_event_listener_records_t &records = unit_test_event_listener_get_records();

  CASE_EXPECT_TRUE(unit_test_event_listener_contains(records.case_started, "unit_test_event_listener.case_second"));
  CASE_EXPECT_FALSE(unit_test_event_listener_contains(records.case_ended, "unit_test_event_listener.case_second"));

  if (unit_test_event_listener_contains(records.case_started, "unit_test_event_listener.case_first")) {
    CASE_EXPECT_TRUE(unit_test_event_listener_contains(records.case_ended, "unit_test_event_listener.case_first"));
    CASE_EXPECT_TRUE(unit_test_event_listener_contains(records.case_passed, "unit_test_event_listener.case_first"));
  }
}

CASE_TEST(unit_test_event_listener, case_verify) {
  unit_test_event_listener_records_t &records = unit_test_event_listener_get_records();

  CASE_EXPECT_EQ(1, records.program_start_count);
  CASE_EXPECT_EQ(0, records.program_end_count);
  CASE_EXPECT_TRUE(unit_test_event_listener_contains(records.suite_started, "unit_test_event_listener"));
  CASE_EXPECT_FALSE(unit_test_event_listener_contains(records.suite_ended, "unit_test_event_listener"));
  CASE_EXPECT_TRUE(unit_test_event_listener_contains(records.case_started, "unit_test_event_listener.case_verify"));
  CASE_EXPECT_FALSE(unit_test_event_listener_contains(records.case_ended, "unit_test_event_listener.case_verify"));

  for (std::vector<std::string>::iterator iter = records.case_started.begin(); iter != records.case_started.end();
       ++iter) {
    if (unit_test_event_listener_contains(records.case_ended, *iter)) {
      CASE_EXPECT_TRUE(unit_test_event_listener_contains(records.case_passed, *iter));
    }
  }
}
