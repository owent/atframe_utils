// Copyright 2026 atframework
//
// Created by owent on 2024-08-26.

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <gsl/select-gsl.h>
#include <nostd/type_traits.h>

#include <chrono>
#include <cstdint>

#ifdef __cpp_impl_three_way_comparison
#  include <compare>
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace platform {

/**
 * @brief Strong type for a snapshot returned by now().
 *
 * This type cannot be created from or implicitly converted to a raw integer, preventing counters from different APIs
 * or units from being mixed accidentally.
 */
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY cpu_time_counter {
 public:
  using value_type = uint64_t;
  using duration_type = nostd::common_type_t<std::chrono::nanoseconds, std::chrono::system_clock::duration>;

  /**
   * @brief Strong type for an offset in the selected clock source's native counter ticks.
   *
   * Arithmetic follows uint64_t modular semantics so a counter interval can cross one wrap. Use from_duration() and
   * to_duration() when converting between native ticks and physical time.
   */
  struct ATFRAMEWORK_UTILS_API_HEAD_ONLY offset_type {
   public:
    using value_type = cpu_time_counter::value_type;
    using duration_type = cpu_time_counter::duration_type;

    /**
     * @brief Explicitly create an offset from a native counter tick count.
     */
    ATFW_UTIL_FORCEINLINE static offset_type from_raw_value(value_type value) noexcept { return offset_type{value}; }

    /**
     * @brief Convert physical time to a native counter offset.
     *
     * Non-positive durations become zero. The result is rounded down to a representable duration and saturates at the
     * largest unambiguous offset, which is less than half of the uint64_t counter cycle.
     */
    static ATFRAMEWORK_UTILS_API offset_type from_duration(duration_type duration) noexcept;

    /**
     * @brief Convert this native counter offset to the preferred std::chrono duration type.
     *
     * @return Zero when the offset exceeds the unambiguous half-cycle range.
     */
    ATFRAMEWORK_UTILS_API duration_type to_duration() const noexcept;

    template <class TDuration>
    ATFW_UTIL_FORCEINLINE TDuration to_duration() const noexcept {
      return std::chrono::duration_cast<TDuration>(to_duration());
    }

    ATFW_UTIL_FORCEINLINE value_type get_raw_value() const noexcept { return value_; }

    ATFW_UTIL_FORCEINLINE offset_type &operator+=(offset_type other) noexcept {
      value_ += other.value_;
      return *this;
    }

    ATFW_UTIL_FORCEINLINE offset_type &operator-=(offset_type other) noexcept {
      value_ -= other.value_;
      return *this;
    }

    ATFW_UTIL_FORCEINLINE offset_type operator+(offset_type other) const noexcept {
      offset_type result = *this;
      result += other;
      return result;
    }

    ATFW_UTIL_FORCEINLINE offset_type operator-(offset_type other) const noexcept {
      offset_type result = *this;
      result -= other;
      return result;
    }

    ATFW_UTIL_FORCEINLINE bool operator==(const offset_type &other) const noexcept { return value_ == other.value_; }

#ifdef __cpp_impl_three_way_comparison
    ATFW_UTIL_FORCEINLINE std::strong_ordering operator<=>(const offset_type &other) const noexcept {
      return value_ <=> other.value_;
    }
#else
    ATFW_UTIL_FORCEINLINE bool operator!=(const offset_type &other) const noexcept { return !(*this == other); }
    ATFW_UTIL_FORCEINLINE bool operator<(const offset_type &other) const noexcept { return value_ < other.value_; }
    ATFW_UTIL_FORCEINLINE bool operator<=(const offset_type &other) const noexcept { return value_ <= other.value_; }
    ATFW_UTIL_FORCEINLINE bool operator>(const offset_type &other) const noexcept { return value_ > other.value_; }
    ATFW_UTIL_FORCEINLINE bool operator>=(const offset_type &other) const noexcept { return value_ >= other.value_; }
#endif

   private:
    ATFW_UTIL_FORCEINLINE explicit offset_type(value_type value) noexcept : value_(value) {}

    value_type value_;
  };

  /**
   * @brief Get an opaque, high-performance monotonic CPU time counter.
   *
   * The underlying clock source is selected once for the current platform. The counter representation and frequency
   * are intentionally hidden; use to_duration() to measure elapsed time.
   */
  static ATFRAMEWORK_UTILS_API cpu_time_counter now() noexcept;

  /**
   * @brief Convert the interval between two CPU time counters to the preferred std::chrono duration type.
   *
   * duration_type uses the finest period that can represent both nanoseconds and system_clock::duration.
   *
   * A single counter wrap is supported when the elapsed interval is less than half of the uint64_t counter cycle. The
   * result is zero when end is interpreted as preceding begin, and saturates at duration_type::max() when needed.
   */
  static ATFRAMEWORK_UTILS_API duration_type to_duration(cpu_time_counter begin, cpu_time_counter end) noexcept;

  /**
   * @brief Convert the interval between two CPU time counters to the requested std::chrono::duration type.
   */
  template <class TDuration>
  static ATFRAMEWORK_UTILS_API_HEAD_ONLY TDuration to_duration(cpu_time_counter begin, cpu_time_counter end) noexcept {
    return std::chrono::duration_cast<TDuration>(to_duration(begin, end));
  }

  /**
   * @brief Get the original counter value without losing precision.
   *
   * The unit is specific to the clock source selected by the current process. Prefer the duration conversion helpers
   * for elapsed time, and only compare raw values produced by this API in the same process.
   */
  ATFW_UTIL_FORCEINLINE value_type get_raw_value() const noexcept { return value_; }

  ATFW_UTIL_FORCEINLINE cpu_time_counter &operator+=(offset_type offset) noexcept {
    value_ += offset.get_raw_value();
    return *this;
  }

  ATFW_UTIL_FORCEINLINE cpu_time_counter &operator-=(offset_type offset) noexcept {
    value_ -= offset.get_raw_value();
    return *this;
  }

  ATFW_UTIL_FORCEINLINE cpu_time_counter operator+(offset_type offset) const noexcept {
    cpu_time_counter result = *this;
    result += offset;
    return result;
  }

  ATFW_UTIL_FORCEINLINE cpu_time_counter operator-(offset_type offset) const noexcept {
    cpu_time_counter result = *this;
    result -= offset;
    return result;
  }

  ATFW_UTIL_FORCEINLINE offset_type operator-(cpu_time_counter other) const noexcept {
    return offset_type::from_raw_value(value_ - other.value_);
  }

  ATFW_UTIL_FORCEINLINE friend cpu_time_counter operator+(offset_type offset, cpu_time_counter counter) noexcept {
    counter += offset;
    return counter;
  }

  ATFW_UTIL_FORCEINLINE bool operator==(const cpu_time_counter &other) const noexcept { return value_ == other.value_; }

#ifdef __cpp_impl_three_way_comparison
  ATFW_UTIL_FORCEINLINE std::strong_ordering operator<=>(const cpu_time_counter &other) const noexcept {
    return value_ <=> other.value_;
  }
#else
  ATFW_UTIL_FORCEINLINE bool operator!=(const cpu_time_counter &other) const noexcept { return !(*this == other); }
  ATFW_UTIL_FORCEINLINE bool operator<(const cpu_time_counter &other) const noexcept { return value_ < other.value_; }
  ATFW_UTIL_FORCEINLINE bool operator<=(const cpu_time_counter &other) const noexcept { return value_ <= other.value_; }
  ATFW_UTIL_FORCEINLINE bool operator>(const cpu_time_counter &other) const noexcept { return value_ > other.value_; }
  ATFW_UTIL_FORCEINLINE bool operator>=(const cpu_time_counter &other) const noexcept { return value_ >= other.value_; }
#endif

 private:
  ATFW_UTIL_FORCEINLINE explicit cpu_time_counter(uint64_t value) noexcept : value_(value) {}

  value_type value_;
};

ATFRAMEWORK_UTILS_API int32_t get_errno() noexcept;

ATFRAMEWORK_UTILS_API gsl::string_view get_strerrno(int32_t result_from_get_errno, gsl::span<char> buffer) noexcept;

ATFRAMEWORK_UTILS_API int32_t atfork(void (*prepare)(), void (*parent)(), void (*child)()) noexcept;

}  // namespace platform
ATFRAMEWORK_UTILS_NAMESPACE_END
