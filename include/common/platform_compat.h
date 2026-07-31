// Copyright 2026 atframework
//
// Created by owent on 2024-08-26.

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <gsl/select-gsl.h>

#include <chrono>
#include <cstdint>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace platform {

struct cpu_time_counter;

/**
 * @brief Get an opaque, high-performance monotonic CPU time counter.
 *
 * The underlying clock source is selected once for the current platform. The counter representation and frequency are
 * intentionally hidden; use cpu_time_counter_to_duration() to measure elapsed time.
 */
ATFRAMEWORK_UTILS_API cpu_time_counter get_cpu_time_counter() noexcept;

/**
 * @brief Convert the interval between two CPU time counters to nanoseconds.
 *
 * @return Zero when end was sampled before begin. The result saturates at std::chrono::nanoseconds::max() when needed.
 */
ATFRAMEWORK_UTILS_API std::chrono::nanoseconds cpu_time_counter_to_nanoseconds(cpu_time_counter begin,
                                                                               cpu_time_counter end) noexcept;

/**
 * @brief Strong type for a snapshot returned by get_cpu_time_counter().
 *
 * This type cannot be created from or implicitly converted to a raw integer, preventing counters from different APIs
 * or units from being mixed accidentally.
 */
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY cpu_time_counter {
 public:
  using value_type = uint64_t;

  /**
   * @brief Get the original counter value without losing precision.
   *
   * The unit is specific to the clock source selected by the current process. Prefer the duration conversion helpers
   * for elapsed time, and only compare raw values produced by this API in the same process.
   */
  ATFW_UTIL_FORCEINLINE value_type get_raw_value() const noexcept { return value_; }

 private:
  ATFW_UTIL_FORCEINLINE explicit cpu_time_counter(uint64_t value) noexcept : value_(value) {}

  value_type value_;

  friend ATFRAMEWORK_UTILS_API cpu_time_counter get_cpu_time_counter() noexcept;
  friend ATFRAMEWORK_UTILS_API std::chrono::nanoseconds cpu_time_counter_to_nanoseconds(cpu_time_counter begin,
                                                                                        cpu_time_counter end) noexcept;
};

/**
 * @brief Convert the interval between two CPU time counters to the requested std::chrono::duration type.
 */
template <class TDuration>
ATFRAMEWORK_UTILS_API_HEAD_ONLY TDuration cpu_time_counter_to_duration(cpu_time_counter begin,
                                                                       cpu_time_counter end) noexcept {
  return std::chrono::duration_cast<TDuration>(cpu_time_counter_to_nanoseconds(begin, end));
}

ATFRAMEWORK_UTILS_API int32_t get_errno() noexcept;

ATFRAMEWORK_UTILS_API gsl::string_view get_strerrno(int32_t result_from_get_errno, gsl::span<char> buffer) noexcept;

ATFRAMEWORK_UTILS_API int32_t atfork(void (*prepare)(), void (*parent)(), void (*child)()) noexcept;

}  // namespace platform
ATFRAMEWORK_UTILS_NAMESPACE_END
