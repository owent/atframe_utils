// Copyright 2026 atframework

#include <cerrno>
#include <chrono>
#include <cstring>
#include <thread>
#include <type_traits>

#include "common/platform_compat.h"
#include "frame/test_macros.h"

static_assert(!std::is_integral<atfw::util::platform::cpu_time_counter>::value,
              "CPU time counters must not be raw integers");
static_assert(!std::is_constructible<atfw::util::platform::cpu_time_counter, uint64_t>::value,
              "CPU time counters must not be constructible from raw integers");
static_assert(!std::is_convertible<atfw::util::platform::cpu_time_counter, uint64_t>::value,
              "CPU time counters must not be implicitly convertible to raw integers");

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
  CASE_EXPECT_EQ(buffer, result.data());
  CASE_EXPECT_EQ(std::strlen(buffer), result.size());
}

CASE_TEST(platform_compat, get_strerrno_zero) {
  char buffer[256] = {0};
  auto result = atfw::util::platform::get_strerrno(0, gsl::make_span(buffer));
  // errno 0 should return a valid (possibly empty) message
  (void)result;
}

CASE_TEST(platform_compat, get_strerrno_small_buffer) {
  // Buffer too small (size <= 1)
  char buffer[1] = {'x'};
  auto result = atfw::util::platform::get_strerrno(EINVAL, gsl::make_span(buffer, 1));
  CASE_EXPECT_TRUE(result.empty());
  CASE_EXPECT_EQ('\0', buffer[0]);
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

CASE_TEST(platform_compat, cpu_time_counter_duration) {
  auto begin = atfw::util::platform::get_cpu_time_counter();
  uint64_t begin_raw_value = begin.get_raw_value();
  CASE_EXPECT_EQ(begin_raw_value, begin.get_raw_value());

  auto zero_elapsed = atfw::util::platform::cpu_time_counter_to_nanoseconds(begin, begin);
  CASE_EXPECT_EQ(0, zero_elapsed.count());

  std::this_thread::sleep_for(std::chrono::milliseconds{5});

  auto end = atfw::util::platform::get_cpu_time_counter();
  CASE_EXPECT_TRUE(end.get_raw_value() >= begin_raw_value);

  auto elapsed_ns = atfw::util::platform::cpu_time_counter_to_nanoseconds(begin, end);
  auto elapsed_us = atfw::util::platform::cpu_time_counter_to_duration<std::chrono::microseconds>(begin, end);

  CASE_EXPECT_TRUE(elapsed_ns >= std::chrono::milliseconds{1});
  CASE_EXPECT_TRUE(elapsed_ns < std::chrono::seconds{30});
  CASE_EXPECT_EQ(std::chrono::duration_cast<std::chrono::microseconds>(elapsed_ns).count(), elapsed_us.count());
}
