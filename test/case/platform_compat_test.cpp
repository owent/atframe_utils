// Copyright 2026 atframework

#include <cerrno>
#include <chrono>
#include <cstring>
#include <limits>
#include <thread>
#include <type_traits>

#include "common/platform_compat.h"
#include "frame/test_macros.h"

using cpu_time_counter = atfw::util::platform::cpu_time_counter;
using cpu_time_offset = cpu_time_counter::offset_type;
using expected_cpu_time_duration =
    typename std::common_type<std::chrono::nanoseconds, std::chrono::system_clock::duration>::type;

static_assert(!std::is_integral<cpu_time_counter>::value, "CPU time counters must not be raw integers");
static_assert(!std::is_constructible<cpu_time_counter, uint64_t>::value,
              "CPU time counters must not be constructible from raw integers");
static_assert(!std::is_convertible<cpu_time_counter, uint64_t>::value,
              "CPU time counters must not be implicitly convertible to raw integers");
static_assert(!std::is_integral<cpu_time_offset>::value, "CPU time offsets must not be raw integers");
static_assert(!std::is_constructible<cpu_time_offset, uint64_t>::value,
              "CPU time offsets must not be directly constructible from raw integers");
static_assert(!std::is_convertible<cpu_time_offset, uint64_t>::value,
              "CPU time offsets must not be implicitly convertible to raw integers");
static_assert(!std::is_convertible<cpu_time_counter::duration_type, cpu_time_offset>::value,
              "Durations must be converted to CPU time offsets explicitly");
static_assert(std::is_same<cpu_time_offset::value_type, cpu_time_counter::value_type>::value,
              "CPU time offsets must expose their native counter value type");
static_assert(std::is_same<cpu_time_offset::duration_type, cpu_time_counter::duration_type>::value,
              "CPU time offsets must use the counter's preferred duration type");

static_assert(std::is_same<cpu_time_counter::duration_type, expected_cpu_time_duration>::value,
              "CPU time duration must preserve the finest system clock or nanosecond precision");
static_assert(std::ratio_less_equal<cpu_time_counter::duration_type::period, std::chrono::nanoseconds::period>::value,
              "CPU time duration must not be less precise than nanoseconds");
static_assert(std::ratio_less_equal<cpu_time_counter::duration_type::period, std::chrono::system_clock::period>::value,
              "CPU time duration must not be less precise than system_clock");

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
  // Bracketing each counter read absorbs arbitrary scheduler stalls between the two clock APIs into the permitted
  // system-clock interval instead of treating them as a counter conversion error.
  const auto system_begin_before = std::chrono::system_clock::now();
  auto begin = cpu_time_counter::now();
  const auto system_begin_after = std::chrono::system_clock::now();
  uint64_t begin_raw_value = begin.get_raw_value();
  CASE_EXPECT_EQ(begin_raw_value, begin.get_raw_value());

  auto zero_elapsed = cpu_time_counter::to_duration(begin, begin);
  CASE_EXPECT_EQ(0, zero_elapsed.count());

  std::this_thread::sleep_for(std::chrono::seconds{1});

  const auto system_end_before = std::chrono::system_clock::now();
  auto end = cpu_time_counter::now();
  const auto system_end_after = std::chrono::system_clock::now();

  auto elapsed = cpu_time_counter::to_duration(begin, end);
  auto elapsed_us = cpu_time_counter::to_duration<std::chrono::microseconds>(begin, end);
  const auto system_lower_bound =
      std::chrono::duration_cast<cpu_time_counter::duration_type>(system_end_before - system_begin_after);
  const auto system_upper_bound =
      std::chrono::duration_cast<cpu_time_counter::duration_type>(system_end_after - system_begin_before);
  // Scheduling jitter is already covered by the bracket. Keep an additional margin for heavily loaded CI runners,
  // clock granularity, conversion rounding, and short-term clock-rate differences.
  const auto clock_tolerance =
      std::chrono::duration_cast<cpu_time_counter::duration_type>(std::chrono::milliseconds{250});

  CASE_EXPECT_TRUE(elapsed >= std::chrono::milliseconds{1});
  CASE_EXPECT_TRUE(elapsed < std::chrono::seconds{30});
  CASE_EXPECT_EQ(std::chrono::duration_cast<std::chrono::microseconds>(elapsed).count(), elapsed_us.count());
  CASE_EXPECT_TRUE(elapsed + clock_tolerance >= system_lower_bound);
  CASE_EXPECT_TRUE(elapsed <= system_upper_bound + clock_tolerance);
}

CASE_TEST(platform_compat, cpu_time_counter_operators_and_wrap) {
  auto sampled = cpu_time_counter::now();
  auto sampled_offset = cpu_time_offset::from_raw_value(sampled.get_raw_value());
  auto zero = sampled - sampled_offset;
  auto zero_offset = zero - zero;
  auto one_offset = cpu_time_offset::from_raw_value(1);
  auto one = zero + one_offset;

  CASE_EXPECT_EQ(0, zero.get_raw_value());
  CASE_EXPECT_EQ(1, one.get_raw_value());
  CASE_EXPECT_EQ(1, (one - zero).get_raw_value());
  CASE_EXPECT_TRUE(zero == one - one_offset);
  CASE_EXPECT_TRUE(one == one_offset + zero);
  auto compound = zero;
  compound += one_offset;
  CASE_EXPECT_TRUE(compound == one);
  compound -= one_offset;
  CASE_EXPECT_TRUE(compound == zero);

  CASE_EXPECT_TRUE(zero == zero);
  CASE_EXPECT_TRUE(zero != one);
  CASE_EXPECT_TRUE(zero < one);
  CASE_EXPECT_TRUE(zero <= one);
  CASE_EXPECT_TRUE(one > zero);
  CASE_EXPECT_TRUE(one >= zero);

  auto two_offset = one_offset + one_offset;
  CASE_EXPECT_EQ(0, zero_offset.get_raw_value());
  CASE_EXPECT_EQ(2, two_offset.get_raw_value());
  CASE_EXPECT_TRUE(two_offset - one_offset == one_offset);
  auto compound_offset = zero_offset;
  compound_offset += one_offset;
  CASE_EXPECT_TRUE(compound_offset == one_offset);
  compound_offset -= one_offset;
  CASE_EXPECT_TRUE(compound_offset == zero_offset);

  CASE_EXPECT_TRUE(zero_offset == zero_offset);
  CASE_EXPECT_TRUE(zero_offset != one_offset);
  CASE_EXPECT_TRUE(zero_offset < one_offset);
  CASE_EXPECT_TRUE(zero_offset <= one_offset);
  CASE_EXPECT_TRUE(one_offset > zero_offset);
  CASE_EXPECT_TRUE(one_offset >= zero_offset);

  const auto one_second = std::chrono::duration_cast<cpu_time_counter::duration_type>(std::chrono::seconds{1});
  auto duration_offset = cpu_time_offset::from_duration(one_second);
  CASE_EXPECT_EQ(0, cpu_time_offset::from_duration(cpu_time_counter::duration_type{-1}).get_raw_value());
  auto round_trip_duration = duration_offset.to_duration();
  auto round_trip_microseconds = duration_offset.to_duration<std::chrono::microseconds>();
  CASE_EXPECT_TRUE(round_trip_duration <= one_second);
  CASE_EXPECT_TRUE(round_trip_duration + std::chrono::milliseconds{1} >= one_second);
  CASE_EXPECT_EQ(std::chrono::duration_cast<std::chrono::microseconds>(round_trip_duration).count(),
                 round_trip_microseconds.count());
  CASE_EXPECT_EQ(round_trip_duration.count(), cpu_time_counter::to_duration(zero, zero + duration_offset).count());

  const auto maximum_value = (std::numeric_limits<cpu_time_counter::value_type>::max)();
  CASE_EXPECT_EQ(maximum_value / 2,
                 cpu_time_offset::from_duration((cpu_time_counter::duration_type::max)()).get_raw_value());
  auto ambiguous_offset = cpu_time_offset::from_raw_value(maximum_value / 2 + 1);
  CASE_EXPECT_EQ(0, ambiguous_offset.to_duration().count());
  auto near_wrap = zero + cpu_time_offset::from_raw_value(maximum_value - 4);
  auto ten_offset = cpu_time_offset::from_raw_value(10);
  auto wrapped = near_wrap + ten_offset;
  auto straight = zero + ten_offset;

  CASE_EXPECT_EQ(5, wrapped.get_raw_value());
  CASE_EXPECT_EQ(10, (wrapped - near_wrap).get_raw_value());
  CASE_EXPECT_EQ(cpu_time_counter::to_duration(zero, straight).count(),
                 cpu_time_counter::to_duration(near_wrap, wrapped).count());
  CASE_EXPECT_EQ(0, cpu_time_counter::to_duration(straight, zero).count());
}
