// Copyright 2026 atframework

#include <cerrno>
#include <cstring>

#include "common/platform_compat.h"
#include "frame/test_macros.h"

CASE_TEST(platform_compat, get_errno_basic) {
  errno = 0;
  int32_t err = atfw::util::platform::get_errno();
  CASE_EXPECT_EQ(0, err);

  errno = EINVAL;
  err = atfw::util::platform::get_errno();
  CASE_EXPECT_EQ(EINVAL, err);

  errno = 0;
}

CASE_TEST(platform_compat, get_strerrno_basic) {
  char buffer[256] = {0};
  auto result = atfw::util::platform::get_strerrno(EINVAL, gsl::make_span(buffer));
  CASE_EXPECT_FALSE(result.empty());
}

CASE_TEST(platform_compat, get_strerrno_zero) {
  char buffer[256] = {0};
  auto result = atfw::util::platform::get_strerrno(0, gsl::make_span(buffer));
  // errno 0 should return a valid (possibly empty) message
  (void)result;
}

CASE_TEST(platform_compat, get_strerrno_small_buffer) {
  // Buffer too small (size <= 1)
  char buffer[1] = {0};
  auto result = atfw::util::platform::get_strerrno(EINVAL, gsl::make_span(buffer, 1));
  CASE_EXPECT_TRUE(result.empty());
}

CASE_TEST(platform_compat, get_strerrno_empty_buffer) {
  auto result = atfw::util::platform::get_strerrno(EINVAL, gsl::span<char>());
  CASE_EXPECT_TRUE(result.empty());
}

CASE_TEST(platform_compat, get_strerrno_enoent) {
  char buffer[256] = {0};
  auto result = atfw::util::platform::get_strerrno(ENOENT, gsl::make_span(buffer));
  CASE_EXPECT_FALSE(result.empty());
}

CASE_TEST(platform_compat, get_strerrno_eacces) {
  char buffer[256] = {0};
  auto result = atfw::util::platform::get_strerrno(EACCES, gsl::make_span(buffer));
  CASE_EXPECT_FALSE(result.empty());
}
