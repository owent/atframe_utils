// Copyright 2026 atframework

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <gsl/select-gsl.h>

#include <cstddef>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace testing {

struct test_event_suite_info {
  gsl::string_view name_;
  size_t total_case_count_ = 0;
  size_t run_case_count_ = 0;
  int success_count_ = 0;
  int failed_count_ = 0;
};

struct test_event_case_info {
  gsl::string_view suite_name_;
  gsl::string_view case_name_;
  int success_count_ = 0;
  int failed_count_ = 0;
  bool passed_ = false;
};

class test_event_listener {
 public:
  test_event_listener();
  virtual ~test_event_listener();

  virtual void on_test_program_start();
  virtual void on_test_program_end(int result);

  virtual void on_test_suite_start(const test_event_suite_info &info);
  virtual void on_test_suite_end(const test_event_suite_info &info);

  virtual void on_test_case_start(const test_event_case_info &info);
  virtual void on_test_case_end(const test_event_case_info &info);
};

void append_test_event_listener(test_event_listener *listener);

}  // namespace testing
ATFRAMEWORK_UTILS_NAMESPACE_END
