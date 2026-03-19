// Copyright 2026 atframework

#include <cstdio>
#include <cstring>
#include <string>

#include "common/file_system.h"
#include "log/log_sink_file_backend.h"
#include "time/time_utility.h"

#include "frame/test_macros.h"

CASE_TEST(log_sink_file_backend, default_constructor) {
  atfw::util::log::log_sink_file_backend backend;

  // Default values - max_file_size defaults to 262144, rotate_size defaults to 10
  CASE_EXPECT_EQ(262144, static_cast<int>(backend.get_max_file_size()));
  CASE_EXPECT_EQ(10, static_cast<int>(backend.get_rotate_size()));
}

CASE_TEST(log_sink_file_backend, set_get_check_interval) {
  atfw::util::log::log_sink_file_backend backend;
  backend.set_check_interval(60);
  CASE_EXPECT_EQ(60, backend.get_check_interval());
}

CASE_TEST(log_sink_file_backend, set_get_flush_interval) {
  atfw::util::log::log_sink_file_backend backend;
  backend.set_flush_interval(30);
  CASE_EXPECT_EQ(30, backend.get_flush_interval());
}

CASE_TEST(log_sink_file_backend, set_get_auto_flush) {
  atfw::util::log::log_sink_file_backend backend;
  backend.set_auto_flush(atfw::util::log::log_level::kWarning);
  CASE_EXPECT_EQ(atfw::util::log::log_level::kWarning, backend.get_auto_flush());

  backend.set_auto_flush(atfw::util::log::log_level::kError);
  CASE_EXPECT_EQ(atfw::util::log::log_level::kError, backend.get_auto_flush());
}

CASE_TEST(log_sink_file_backend, set_get_max_file_size) {
  atfw::util::log::log_sink_file_backend backend;
  backend.set_max_file_size(1024 * 1024);
  CASE_EXPECT_EQ(1024 * 1024, static_cast<int>(backend.get_max_file_size()));
}

CASE_TEST(log_sink_file_backend, set_get_rotate_size) {
  atfw::util::log::log_sink_file_backend backend;
  backend.set_rotate_size(5);
  CASE_EXPECT_EQ(5, static_cast<int>(backend.get_rotate_size()));
}

CASE_TEST(log_sink_file_backend, pattern_constructor) {
  atfw::util::log::log_sink_file_backend backend("test_%Y%m%d.log");
  // Should be constructed without crash
  CASE_EXPECT_GE(backend.get_check_interval(), 0);
}

CASE_TEST(log_sink_file_backend, copy_constructor) {
  atfw::util::log::log_sink_file_backend backend1;
  backend1.set_max_file_size(2048);
  backend1.set_rotate_size(3);
  backend1.set_flush_interval(10);
  backend1.set_auto_flush(atfw::util::log::log_level::kError);

  atfw::util::log::log_sink_file_backend backend2(backend1);
  CASE_EXPECT_EQ(2048, static_cast<int>(backend2.get_max_file_size()));
  CASE_EXPECT_EQ(3, static_cast<int>(backend2.get_rotate_size()));
  CASE_EXPECT_EQ(10, backend2.get_flush_interval());
  CASE_EXPECT_EQ(atfw::util::log::log_level::kError, backend2.get_auto_flush());
}

CASE_TEST(log_sink_file_backend, write_log) {
  atfw::util::time::time_utility::update();

  // Create a temp directory for log files
  std::string log_dir = "test_log_sink_output";
  atfw::util::file_system::mkdir(log_dir.c_str(), true);

  std::string pattern = log_dir + "/test_log.log";
  atfw::util::log::log_sink_file_backend backend;
  backend.set_file_pattern(pattern);

  // Write a log entry
  atfw::util::log::log_formatter::caller_info_t caller;
  caller.level_id = atfw::util::log::log_level::kInfo;
  caller.level_name = "Info";
  caller.file_path = __FILE__;
  caller.line_number = __LINE__;
  caller.func_name = __FUNCTION__;
  caller.rotate_index = 0;

  std::string content = "test log content\n";
  backend(caller, content);

  // Verify the file exists
  CASE_EXPECT_TRUE(atfw::util::file_system::is_exist(pattern.c_str()));

  // Cleanup
  atfw::util::file_system::remove(pattern.c_str());
  atfw::util::file_system::remove(log_dir.c_str());
}

CASE_TEST(log_sink_file_backend, write_log_auto_mkdir) {
  atfw::util::time::time_utility::update();

  std::string log_dir = "test_log_sink_mkdir/sub/dir";
  std::string pattern = log_dir + "/test_auto_mkdir.log";

  atfw::util::log::log_sink_file_backend backend;
  backend.set_file_pattern(pattern);

  atfw::util::log::log_formatter::caller_info_t caller;
  caller.level_id = atfw::util::log::log_level::kInfo;
  caller.level_name = "Info";
  caller.file_path = __FILE__;
  caller.line_number = __LINE__;
  caller.func_name = __FUNCTION__;
  caller.rotate_index = 0;

  std::string content = "auto mkdir test\n";
  backend(caller, content);

  CASE_EXPECT_TRUE(atfw::util::file_system::is_exist(pattern.c_str()));

  // Cleanup
  atfw::util::file_system::remove(pattern.c_str());
  atfw::util::file_system::remove("test_log_sink_mkdir/sub/dir");
  atfw::util::file_system::remove("test_log_sink_mkdir/sub");
  atfw::util::file_system::remove("test_log_sink_mkdir");
}

CASE_TEST(log_sink_file_backend, chain_setters) {
  atfw::util::log::log_sink_file_backend backend;

  // Test method chaining
  backend.set_check_interval(120)
      .set_flush_interval(60)
      .set_auto_flush(atfw::util::log::log_level::kWarning)
      .set_max_file_size(1048576)
      .set_rotate_size(10);

  CASE_EXPECT_EQ(120, backend.get_check_interval());
  CASE_EXPECT_EQ(60, backend.get_flush_interval());
  CASE_EXPECT_EQ(atfw::util::log::log_level::kWarning, backend.get_auto_flush());
  CASE_EXPECT_EQ(1048576, static_cast<int>(backend.get_max_file_size()));
  CASE_EXPECT_EQ(10, static_cast<int>(backend.get_rotate_size()));
}

CASE_TEST(log_sink_file_backend, set_file_pattern) {
  atfw::util::log::log_sink_file_backend backend;
  backend.set_file_pattern("logs/%Y/%m/%d/app.log");

  // check_interval should be auto-calculated based on the pattern
  CASE_EXPECT_GT(backend.get_check_interval(), 0);
}

CASE_TEST(log_sink_file_backend, set_file_pattern_no_time) {
  atfw::util::log::log_sink_file_backend backend;
  backend.set_file_pattern("logs/static.log");

  // For a static filename with no time tokens, check_interval should still be reasonable
  CASE_EXPECT_GE(backend.get_check_interval(), 0);
}

CASE_TEST(log_sink_file_backend, multiple_writes) {
  atfw::util::time::time_utility::update();

  std::string log_dir = "test_log_multi";
  atfw::util::file_system::mkdir(log_dir.c_str(), true);

  std::string pattern = log_dir + "/multi_write.log";
  atfw::util::log::log_sink_file_backend backend;
  backend.set_file_pattern(pattern);

  atfw::util::log::log_formatter::caller_info_t caller;
  caller.level_id = atfw::util::log::log_level::kInfo;
  caller.level_name = "Info";
  caller.file_path = __FILE__;
  caller.line_number = __LINE__;
  caller.func_name = __FUNCTION__;
  caller.rotate_index = 0;

  // Write multiple entries
  for (int i = 0; i < 10; ++i) {
    std::string content = "log entry " + std::to_string(i) + "\n";
    backend(caller, content);
  }

  // File should exist
  CASE_EXPECT_TRUE(atfw::util::file_system::is_exist(pattern.c_str()));

  // Cleanup
  atfw::util::file_system::remove(pattern.c_str());
  atfw::util::file_system::remove(log_dir.c_str());
}

CASE_TEST(log_sink_file_backend, auto_flush_on_high_level) {
  atfw::util::time::time_utility::update();

  std::string log_dir = "test_log_flush";
  atfw::util::file_system::mkdir(log_dir.c_str(), true);

  std::string pattern = log_dir + "/flush_test.log";
  atfw::util::log::log_sink_file_backend backend;
  backend.set_file_pattern(pattern);
  backend.set_auto_flush(atfw::util::log::log_level::kWarning);

  atfw::util::log::log_formatter::caller_info_t caller;
  caller.level_id = atfw::util::log::log_level::kError;  // Higher than warning
  caller.level_name = "Error";
  caller.file_path = __FILE__;
  caller.line_number = __LINE__;
  caller.func_name = __FUNCTION__;
  caller.rotate_index = 0;

  std::string content = "error level message\n";
  backend(caller, content);

  // Should be flushed immediately
  CASE_EXPECT_TRUE(atfw::util::file_system::is_exist(pattern.c_str()));

  // Cleanup
  atfw::util::file_system::remove(pattern.c_str());
  atfw::util::file_system::remove(log_dir.c_str());
}
