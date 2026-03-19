// Copyright 2026 atframework

#include <cstring>
#include <string>

#include "frame/test_macros.h"
#include "log/log_formatter.h"
#include "time/time_utility.h"

CASE_TEST(log_formatter, check_flag) {
  using flag_t = atfw::util::log::log_formatter::flag_t;
  CASE_EXPECT_TRUE(atfw::util::log::log_formatter::check_flag(flag_t::DATE | flag_t::TIME, flag_t::DATE));
  CASE_EXPECT_TRUE(atfw::util::log::log_formatter::check_flag(flag_t::DATE | flag_t::TIME, flag_t::TIME));
  CASE_EXPECT_TRUE(
      atfw::util::log::log_formatter::check_flag(flag_t::DATE | flag_t::TIME, flag_t::DATE | flag_t::TIME));
  CASE_EXPECT_FALSE(atfw::util::log::log_formatter::check_flag(flag_t::DATE, flag_t::TIME));
  CASE_EXPECT_FALSE(atfw::util::log::log_formatter::check_flag(flag_t::DATE, flag_t::DATE | flag_t::TIME));
}

CASE_TEST(log_formatter, format_date_Y) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%Y", 2, caller);
  CASE_EXPECT_EQ(4, static_cast<int>(len));
  // Year should be 4 digits
  CASE_EXPECT_TRUE(buffer[0] >= '0' && buffer[0] <= '9');
  CASE_EXPECT_TRUE(buffer[3] >= '0' && buffer[3] <= '9');
}

CASE_TEST(log_formatter, format_date_y) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%y", 2, caller);
  CASE_EXPECT_EQ(2, static_cast<int>(len));
}

CASE_TEST(log_formatter, format_date_month) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%m", 2, caller);
  CASE_EXPECT_EQ(2, static_cast<int>(len));
  // Month should be 01-12
  int month = (buffer[0] - '0') * 10 + (buffer[1] - '0');
  CASE_EXPECT_GE(month, 1);
  CASE_EXPECT_LE(month, 12);
}

CASE_TEST(log_formatter, format_date_day) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%d", 2, caller);
  CASE_EXPECT_EQ(2, static_cast<int>(len));
  int day = (buffer[0] - '0') * 10 + (buffer[1] - '0');
  CASE_EXPECT_GE(day, 1);
  CASE_EXPECT_LE(day, 31);
}

CASE_TEST(log_formatter, format_date_yday) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%j", 2, caller);
  CASE_EXPECT_EQ(3, static_cast<int>(len));
}

CASE_TEST(log_formatter, format_date_weekday) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%w", 2, caller);
  CASE_EXPECT_EQ(1, static_cast<int>(len));
  int wday = buffer[0] - '0';
  CASE_EXPECT_GE(wday, 0);
  CASE_EXPECT_LE(wday, 6);
}

CASE_TEST(log_formatter, format_time_HMS) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%H:%M:%S", 8, caller);
  CASE_EXPECT_EQ(8, static_cast<int>(len));
  CASE_EXPECT_EQ(':', buffer[2]);
  CASE_EXPECT_EQ(':', buffer[5]);
}

CASE_TEST(log_formatter, format_time_12hour) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%I", 2, caller);
  CASE_EXPECT_EQ(2, static_cast<int>(len));
  int hour = (buffer[0] - '0') * 10 + (buffer[1] - '0');
  CASE_EXPECT_GE(hour, 1);
  CASE_EXPECT_LE(hour, 12);
}

CASE_TEST(log_formatter, format_iso_date_F) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  // %F = YYYY-MM-DD
  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%F", 2, caller);
  CASE_EXPECT_EQ(10, static_cast<int>(len));
  CASE_EXPECT_EQ('-', buffer[4]);
  CASE_EXPECT_EQ('-', buffer[7]);
}

CASE_TEST(log_formatter, format_iso_time_T) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  // %T = HH:MM:SS
  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%T", 2, caller);
  CASE_EXPECT_EQ(8, static_cast<int>(len));
  CASE_EXPECT_EQ(':', buffer[2]);
  CASE_EXPECT_EQ(':', buffer[5]);
}

CASE_TEST(log_formatter, format_time_R) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  // %R = HH:MM
  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%R", 2, caller);
  CASE_EXPECT_EQ(5, static_cast<int>(len));
  CASE_EXPECT_EQ(':', buffer[2]);
}

CASE_TEST(log_formatter, format_microseconds_f) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  // %f = microseconds (5 digits)
  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%f", 2, caller);
  CASE_EXPECT_EQ(5, static_cast<int>(len));
  for (size_t i = 0; i < len; ++i) {
    CASE_EXPECT_TRUE(buffer[i] >= '0' && buffer[i] <= '9');
  }
}

CASE_TEST(log_formatter, format_level_name_L) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kError, "ERROR", __FILE__, __LINE__,
                                                       __FUNCTION__);

  // %L = level name padded to 8 chars
  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%L", 2, caller);
  CASE_EXPECT_EQ(8, static_cast<int>(len));
  CASE_EXPECT_EQ(0, strncmp(buffer, "ERROR   ", 8));
}

CASE_TEST(log_formatter, format_level_name_auto) {
  atfw::util::time::time_utility::update();

  // Test with empty level_name to trigger auto-detection
  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller;
  caller.level_id = atfw::util::log::log_level::kWarning;
  caller.line_number = __LINE__;

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%L", 2, caller);
  CASE_EXPECT_EQ(8, static_cast<int>(len));
  CASE_EXPECT_EQ(0, strncmp(buffer, "WARN    ", 8));
}

CASE_TEST(log_formatter, format_level_id_l) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kError, "ERROR", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%l", 2, caller);
  CASE_EXPECT_GT(static_cast<int>(len), 0);
  // Error level is 500
  CASE_EXPECT_EQ(0, strcmp(buffer, "500"));
}

CASE_TEST(log_formatter, format_file_path_s) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO",
                                                       "src/test/my_test.cpp", __LINE__, __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%s", 2, caller);
  CASE_EXPECT_GT(static_cast<int>(len), 0);
  CASE_EXPECT_TRUE(nullptr != strstr(buffer, "my_test.cpp"));
}

CASE_TEST(log_formatter, format_file_path_s_with_project_dir) {
  atfw::util::time::time_utility::update();

  atfw::util::log::log_formatter::set_project_directory("/home/user/project/");

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO",
                                                       "/home/user/project/src/main.cpp", __LINE__, __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%s", 2, caller);
  CASE_EXPECT_GT(static_cast<int>(len), 0);
  // Should start with ~ after stripping project dir
  CASE_EXPECT_EQ('~', buffer[0]);
  CASE_EXPECT_TRUE(nullptr != strstr(buffer, "src/main.cpp"));

  // Reset project directory
  atfw::util::log::log_formatter::set_project_directory(nullptr, 0);
}

CASE_TEST(log_formatter, format_filename_k) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO",
                                                       "src/test/my_test.cpp", __LINE__, __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%k", 2, caller);
  CASE_EXPECT_GT(static_cast<int>(len), 0);
  CASE_EXPECT_EQ(0, strcmp(buffer, "my_test.cpp"));
}

CASE_TEST(log_formatter, format_line_number_n) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, 42,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%n", 2, caller);
  CASE_EXPECT_GT(static_cast<int>(len), 0);
  CASE_EXPECT_EQ(0, strcmp(buffer, "42"));
}

CASE_TEST(log_formatter, format_function_name_C) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       "my_function");

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%C", 2, caller);
  CASE_EXPECT_GT(static_cast<int>(len), 0);
  CASE_EXPECT_EQ(0, strcmp(buffer, "my_function"));
}

CASE_TEST(log_formatter, format_rotate_index_N) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__, 7);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%N", 2, caller);
  CASE_EXPECT_GT(static_cast<int>(len), 0);
  CASE_EXPECT_EQ(0, strcmp(buffer, "7"));
}

CASE_TEST(log_formatter, format_combined) {
  atfw::util::time::time_utility::update();

  char buffer[1024] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO",
                                                       "src/test/my_test.cpp", 100, "test_func", 0);

  const char *fmt = "[%F %T.%f][%L][%k:%n %C] ";
  size_t fmtz = strlen(fmt);
  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), fmt, fmtz, caller);
  CASE_EXPECT_GT(static_cast<int>(len), 0);

  // Should contain the file name
  CASE_EXPECT_TRUE(nullptr != strstr(buffer, "my_test.cpp"));
  // Should contain the function name
  CASE_EXPECT_TRUE(nullptr != strstr(buffer, "test_func"));
  // Should contain the line number
  CASE_EXPECT_TRUE(nullptr != strstr(buffer, "100"));
}

CASE_TEST(log_formatter, format_empty_input) {
  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller;

  // Null buffer
  size_t len = atfw::util::log::log_formatter::format(nullptr, 0, "%Y", 2, caller);
  CASE_EXPECT_EQ(0, static_cast<int>(len));

  // Null format
  len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), nullptr, 0, caller);
  CASE_EXPECT_EQ(0, static_cast<int>(len));
  CASE_EXPECT_EQ('\0', buffer[0]);
}

CASE_TEST(log_formatter, format_small_buffer) {
  atfw::util::time::time_utility::update();

  // Buffer too small for %Y (needs 4 bytes)
  char buffer[4] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%Y", 2, caller);
  // Should handle gracefully
  CASE_EXPECT_LE(static_cast<int>(len), 4);
}

CASE_TEST(log_formatter, check_rotation_var) {
  CASE_EXPECT_TRUE(atfw::util::log::log_formatter::check_rotation_var("test%N.log", 10));
  CASE_EXPECT_FALSE(atfw::util::log::log_formatter::check_rotation_var("test.log", 8));
  CASE_EXPECT_TRUE(atfw::util::log::log_formatter::check_rotation_var("%N", 2));
  CASE_EXPECT_FALSE(atfw::util::log::log_formatter::check_rotation_var("N", 1));
}

CASE_TEST(log_formatter, has_format) {
  CASE_EXPECT_TRUE(atfw::util::log::log_formatter::has_format("test%Y.log", 10));
  CASE_EXPECT_FALSE(atfw::util::log::log_formatter::has_format("test.log", 8));
  CASE_EXPECT_TRUE(atfw::util::log::log_formatter::has_format("%F", 2));
}

CASE_TEST(log_formatter, get_level_by_name) {
  CASE_EXPECT_EQ(atfw::util::log::log_level::kTrace, atfw::util::log::log_formatter::get_level_by_name("trace"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kDebug, atfw::util::log::log_formatter::get_level_by_name("debug"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kNotice, atfw::util::log::log_formatter::get_level_by_name("notice"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kInfo, atfw::util::log::log_formatter::get_level_by_name("info"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kWarning, atfw::util::log::log_formatter::get_level_by_name("warn"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kWarning, atfw::util::log::log_formatter::get_level_by_name("warning"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kError, atfw::util::log::log_formatter::get_level_by_name("error"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kFatal, atfw::util::log::log_formatter::get_level_by_name("fatal"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kDisabled, atfw::util::log::log_formatter::get_level_by_name("disable"));

  // Empty name
  CASE_EXPECT_EQ(atfw::util::log::log_level::kDebug, atfw::util::log::log_formatter::get_level_by_name(""));

  // Unknown name falls back to debug
  CASE_EXPECT_EQ(atfw::util::log::log_level::kDebug, atfw::util::log::log_formatter::get_level_by_name("unknown"));
}

CASE_TEST(log_formatter, get_level_by_number) {
  // Numeric log level
  CASE_EXPECT_EQ(atfw::util::log::log_level::kTrace, atfw::util::log::log_formatter::get_level_by_name("0"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kInfo, atfw::util::log::log_formatter::get_level_by_name("300"));
  CASE_EXPECT_EQ(atfw::util::log::log_level::kFatal, atfw::util::log::log_formatter::get_level_by_name("600"));

  // Out of range numeric
  CASE_EXPECT_EQ(atfw::util::log::log_level::kDebug, atfw::util::log::log_formatter::get_level_by_name("9999"));
}

CASE_TEST(log_formatter, set_project_directory) {
  // Set via string_view
  atfw::util::log::log_formatter::set_project_directory("/project/dir/");

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO",
                                                       "/project/dir/src/file.cpp", __LINE__, __FUNCTION__);
  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%s", 2, caller);
  CASE_EXPECT_GT(static_cast<int>(len), 0);
  CASE_EXPECT_EQ('~', buffer[0]);

  // Set with zero length (null-terminated)
  atfw::util::log::log_formatter::set_project_directory("/new/dir/", 0);

  // Reset
  atfw::util::log::log_formatter::set_project_directory(nullptr, 0);
}

CASE_TEST(log_formatter, format_unknown_specifier) {
  atfw::util::time::time_utility::update();

  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller(atfw::util::log::log_level::kInfo, "INFO", __FILE__, __LINE__,
                                                       __FUNCTION__);

  // Unknown specifier %Z should just output 'Z'
  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%Z", 2, caller);
  CASE_EXPECT_EQ(1, static_cast<int>(len));
  CASE_EXPECT_EQ('Z', buffer[0]);
}

CASE_TEST(log_formatter, format_literal_text) {
  char buffer[256] = {0};
  atfw::util::log::log_formatter::caller_info_t caller;

  const char *fmt = "hello world";
  size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), fmt, strlen(fmt), caller);
  CASE_EXPECT_EQ(11, static_cast<int>(len));
  CASE_EXPECT_EQ(0, strcmp(buffer, "hello world"));
}

CASE_TEST(log_formatter, format_level_auto_names) {
  atfw::util::time::time_utility::update();

  // Test all auto-detected level names
  struct {
    atfw::util::log::log_level level;
    const char *expected_prefix;
  } cases[] = {
      {atfw::util::log::log_level::kTrace, "TRACE"},   {atfw::util::log::log_level::kDebug, "DEBUG"},
      {atfw::util::log::log_level::kNotice, "NOTICE"}, {atfw::util::log::log_level::kInfo, "INFO"},
      {atfw::util::log::log_level::kWarning, "WARN"},  {atfw::util::log::log_level::kError, "ERROR"},
      {atfw::util::log::log_level::kFatal, "FATAL"},   {atfw::util::log::log_level::kDisabled, "DISABLED"},
  };

  for (auto &c : cases) {
    char buffer[256] = {0};
    atfw::util::log::log_formatter::caller_info_t caller;
    caller.level_id = c.level;
    caller.line_number = 0;

    size_t len = atfw::util::log::log_formatter::format(buffer, sizeof(buffer), "%L", 2, caller);
    CASE_EXPECT_EQ(8, static_cast<int>(len));
    CASE_EXPECT_EQ(0, strncmp(buffer, c.expected_prefix, strlen(c.expected_prefix)));
  }
}
