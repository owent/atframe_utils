// Copyright 2026 atframework
//
// Created by owent on 2024-08-26.

#ifndef __STDC_WANT_LIB_EXT1__
#  define __STDC_WANT_LIB_EXT1__ 1
#endif

#if defined(__unix__) || defined(__unix) || defined(__APPLE__) || defined(__CYGWIN__) || defined(_AIX) || \
    defined(__sun) || defined(__hpux)
#  define ATFRAMEWORK_UTILS_HAS_PTHREAD 1
#  if !defined(_GNU_SOURCE) && !defined(_POSIX_C_SOURCE) && !defined(_XOPEN_SOURCE)
#    define _POSIX_C_SOURCE 200112L
#  endif
#  if defined(_GNU_SOURCE) || (defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200112L) || \
      (defined(_XOPEN_SOURCE) && _XOPEN_SOURCE >= 600)
#    define ATFRAMEWORK_UTILS_PLATFORM_HAS_STRERROR_R 1
#  endif
#endif

#include "common/platform_compat.h"

#if defined(ATFRAMEWORK_UTILS_HAS_PTHREAD) && ATFRAMEWORK_UTILS_HAS_PTHREAD
#  include <pthread.h>
#endif

#if defined(_WIN32)
#  if !defined(NOMINMAX)
#    define NOMINMAX
#  endif
#  if !defined(WIN32_LEAN_AND_MEAN)
#    define WIN32_LEAN_AND_MEAN
#  endif
#  include <Windows.h>
#elif defined(__APPLE__)
#  include <mach/mach_time.h>
#elif defined(__unix__) || defined(__unix) || defined(__ANDROID__)
#  include <time.h>
#endif

#if (defined(__linux__) || defined(__ANDROID__)) && defined(__x86_64__) && (defined(__clang__) || defined(__GNUC__))
#  define ATFRAMEWORK_UTILS_PLATFORM_USE_X86_TSC 1
#  include <cpuid.h>
#  include <x86intrin.h>
#endif

#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>  // IWYU pragma: keep
#include <limits>   // IWYU pragma: keep
#include <type_traits>

namespace {

// noexcept did not become part of the function type until C++17. Keep the stored function pointer compatible with the
// library's C++14 baseline; every reader implementation remains noexcept.
using cpu_time_reader = uint64_t (*)();

struct cpu_time_clock_state {
  cpu_time_clock_state(cpu_time_reader reader, uint64_t numerator, uint64_t denominator) noexcept
      : read(reader), nanoseconds_numerator(numerator), nanoseconds_denominator(denominator) {}

  cpu_time_reader read;
  uint64_t nanoseconds_numerator;
  uint64_t nanoseconds_denominator;
};

static uint64_t greatest_common_divisor(uint64_t left, uint64_t right) noexcept {
  while (right != 0) {
    const uint64_t remainder = left % right;
    left = right;
    right = remainder;
  }
  return left;
}

static cpu_time_clock_state make_cpu_time_clock_state(cpu_time_reader read, uint64_t nanoseconds_numerator,
                                                      uint64_t nanoseconds_denominator) noexcept {
  if (nanoseconds_numerator == 0 || nanoseconds_denominator == 0) {
    return cpu_time_clock_state{read, 1, 1};
  }

  const uint64_t divisor = greatest_common_divisor(nanoseconds_numerator, nanoseconds_denominator);
  return cpu_time_clock_state{read, nanoseconds_numerator / divisor, nanoseconds_denominator / divisor};
}

static uint64_t read_steady_clock() noexcept {
  const auto now = std::chrono::steady_clock::now().time_since_epoch();
  return static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(now).count());
}

#if defined(_WIN32)
static uint64_t read_performance_counter() noexcept {
  LARGE_INTEGER counter;
  if (::QueryPerformanceCounter(&counter) == 0) {
    return 0;
  }
  return static_cast<uint64_t>(counter.QuadPart);
}
#elif defined(__APPLE__)
static uint64_t read_mach_absolute_time() noexcept { return ::mach_absolute_time(); }
#elif defined(__unix__) || defined(__unix) || defined(__ANDROID__)
static uint64_t read_monotonic_clock() noexcept {
  struct timespec now;
  if (::clock_gettime(CLOCK_MONOTONIC, &now) != 0) {
    return read_steady_clock();
  }

  return static_cast<uint64_t>(now.tv_sec) * 1000000000ULL + static_cast<uint64_t>(now.tv_nsec);
}
#endif

#if defined(ATFRAMEWORK_UTILS_PLATFORM_USE_X86_TSC) && ATFRAMEWORK_UTILS_PLATFORM_USE_X86_TSC
struct cpuid_registers {
  uint32_t eax;
  uint32_t ebx;
  uint32_t ecx;
  uint32_t edx;
};

static cpuid_registers read_cpuid(uint32_t leaf, uint32_t subleaf = 0) noexcept {
  cpuid_registers result = {};
  __cpuid_count(leaf, subleaf, result.eax, result.ebx, result.ecx, result.edx);
  return result;
}

static bool linux_clocksource_is_tsc() noexcept {
  std::FILE *clocksource_file = std::fopen("/sys/devices/system/clocksource/clocksource0/current_clocksource", "r");
  if (clocksource_file == nullptr) {
    return false;
  }

  char clocksource[32] = {};
  const bool read_succeeded = std::fgets(clocksource, sizeof(clocksource), clocksource_file) != nullptr;
  std::fclose(clocksource_file);
  if (!read_succeeded) {
    return false;
  }

  clocksource[std::strcspn(clocksource, " \t\r\n")] = '\0';
  return std::strcmp(clocksource, "tsc") == 0;
}

static bool get_tsc_frequency(uint64_t &frequency, bool &has_rdtscp) noexcept {
  frequency = 0;
  has_rdtscp = false;

  const uint32_t maximum_basic_leaf = __get_cpuid_max(0, nullptr);
  if (maximum_basic_leaf < 1) {
    return false;
  }

  const cpuid_registers basic_features = read_cpuid(1);
  constexpr uint32_t CPUID_TSC_BIT = 1U << 4;
  if ((basic_features.edx & CPUID_TSC_BIT) == 0) {
    return false;
  }

  const uint32_t maximum_extended_leaf = __get_cpuid_max(0x80000000U, nullptr);
  if (maximum_extended_leaf < 0x80000007U) {
    return false;
  }

  const cpuid_registers advanced_power_management = read_cpuid(0x80000007U);
  constexpr uint32_t CPUID_INVARIANT_TSC_BIT = 1U << 8;
  if ((advanced_power_management.edx & CPUID_INVARIANT_TSC_BIT) == 0) {
    return false;
  }

  if (maximum_extended_leaf >= 0x80000001U) {
    const cpuid_registers extended_features = read_cpuid(0x80000001U);
    constexpr uint32_t CPUID_RDTSCP_BIT = 1U << 27;
    has_rdtscp = (extended_features.edx & CPUID_RDTSCP_BIT) != 0;
  }

  if (maximum_basic_leaf < 0x15U) {
    return false;
  }

  const cpuid_registers tsc_ratio = read_cpuid(0x15U);
  if (tsc_ratio.eax == 0 || tsc_ratio.ebx == 0 || tsc_ratio.ecx == 0) {
    return false;
  }

  frequency = static_cast<uint64_t>(tsc_ratio.ecx) * static_cast<uint64_t>(tsc_ratio.ebx) /
              static_cast<uint64_t>(tsc_ratio.eax);
  return frequency != 0;
}

static uint64_t read_rdtscp() noexcept {
  unsigned int processor_id = 0;
  _mm_lfence();
  const uint64_t result = __rdtscp(&processor_id);
  _mm_lfence();
  return result;
}

static uint64_t read_rdtsc() noexcept {
  _mm_lfence();
  const uint64_t result = __rdtsc();
  _mm_lfence();
  return result;
}
#endif

static cpu_time_clock_state initialize_cpu_time_clock_state() noexcept {
#if defined(_WIN32)
  LARGE_INTEGER frequency;
  if (::QueryPerformanceFrequency(&frequency) != 0 && frequency.QuadPart > 0) {
    return make_cpu_time_clock_state(&read_performance_counter, 1000000000ULL,
                                     static_cast<uint64_t>(frequency.QuadPart));
  }
#elif defined(__APPLE__)
  mach_timebase_info_data_t timebase = {};
  if (::mach_timebase_info(&timebase) == KERN_SUCCESS && timebase.numer != 0 && timebase.denom != 0) {
    return make_cpu_time_clock_state(&read_mach_absolute_time, static_cast<uint64_t>(timebase.numer),
                                     static_cast<uint64_t>(timebase.denom));
  }
#elif defined(ATFRAMEWORK_UTILS_PLATFORM_USE_X86_TSC) && ATFRAMEWORK_UTILS_PLATFORM_USE_X86_TSC
  uint64_t tsc_frequency = 0;
  bool has_rdtscp = false;
  if (linux_clocksource_is_tsc() && get_tsc_frequency(tsc_frequency, has_rdtscp)) {
    return make_cpu_time_clock_state(has_rdtscp ? &read_rdtscp : &read_rdtsc, 1000000000ULL, tsc_frequency);
  }
  return make_cpu_time_clock_state(&read_monotonic_clock, 1, 1);
#elif defined(__unix__) || defined(__unix) || defined(__ANDROID__)
  return make_cpu_time_clock_state(&read_monotonic_clock, 1, 1);
#endif

  return make_cpu_time_clock_state(&read_steady_clock, 1, 1);
}

static const cpu_time_clock_state &get_cpu_time_clock_state() noexcept {
  // All OS and CPU capability detection is cached here. After this thread-safe one-time initialization, the hot path
  // only checks the static guard, loads the cached reader, and invokes it.
  static const cpu_time_clock_state state = initialize_cpu_time_clock_state();
  return state;
}

static std::chrono::nanoseconds convert_cpu_time_counter_to_nanoseconds(const cpu_time_clock_state &clock_state,
                                                                        uint64_t counter_delta) noexcept {
  using nanoseconds_rep = std::chrono::nanoseconds::rep;
  const uint64_t maximum_nanoseconds = static_cast<uint64_t>(std::chrono::nanoseconds::max().count());

  const uint64_t whole_units = counter_delta / clock_state.nanoseconds_denominator;
  const uint64_t remainder = counter_delta % clock_state.nanoseconds_denominator;
  if (whole_units > maximum_nanoseconds / clock_state.nanoseconds_numerator) {
    return std::chrono::nanoseconds::max();
  }

  const uint64_t whole_nanoseconds = whole_units * clock_state.nanoseconds_numerator;
  const uint64_t fractional_nanoseconds = static_cast<uint64_t>(
      static_cast<long double>(remainder) * static_cast<long double>(clock_state.nanoseconds_numerator) /
      static_cast<long double>(clock_state.nanoseconds_denominator));
  if (fractional_nanoseconds > maximum_nanoseconds - whole_nanoseconds) {
    return std::chrono::nanoseconds::max();
  }

  return std::chrono::nanoseconds{static_cast<nanoseconds_rep>(whole_nanoseconds + fractional_nanoseconds)};
}

template <class TMessage>
static gsl::string_view copy_strerror_message(TMessage message, gsl::span<char> buffer) noexcept {
  if (message == nullptr) {
    return {};
  }

  const std::size_t buffer_size = static_cast<std::size_t>(buffer.size());
  std::size_t message_size = 0;
  while (message_size + 1 < buffer_size && message[message_size] != '\0') {
    ++message_size;
  }

  if (message != buffer.data()) {
    std::memmove(buffer.data(), message, message_size);
  }
  buffer.data()[message_size] = '\0';
  return gsl::string_view{buffer.data(), message_size};
}

template <class TResult>
static gsl::string_view handle_strerror_r_result(TResult result, gsl::span<char> buffer, std::true_type) noexcept {
  (void)result;
  return copy_strerror_message(buffer.data(), buffer);
}

template <class TResult>
static gsl::string_view handle_strerror_r_result(TResult result, gsl::span<char> buffer, std::false_type) noexcept {
  return copy_strerror_message(result, buffer);
}

template <class TResult>
static gsl::string_view handle_strerror_r_result(TResult result, gsl::span<char> buffer) noexcept {
  return handle_strerror_r_result(result, buffer, typename std::is_integral<TResult>::type{});
}

}  // namespace

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace platform {

ATFRAMEWORK_UTILS_API cpu_time_counter get_cpu_time_counter() noexcept {
  const cpu_time_clock_state &clock_state = get_cpu_time_clock_state();
  return cpu_time_counter{clock_state.read()};
}

ATFRAMEWORK_UTILS_API std::chrono::nanoseconds cpu_time_counter_to_nanoseconds(cpu_time_counter begin,
                                                                               cpu_time_counter end) noexcept {
  if (end.value_ <= begin.value_) {
    return std::chrono::nanoseconds{0};
  }
  return convert_cpu_time_counter_to_nanoseconds(get_cpu_time_clock_state(), end.value_ - begin.value_);
}

ATFRAMEWORK_UTILS_API int32_t get_errno() noexcept { return errno; }

ATFRAMEWORK_UTILS_API gsl::string_view get_strerrno(int32_t result_from_get_errno, gsl::span<char> buffer) noexcept {
  if (buffer.empty()) {
    return {};
  }
  *buffer.data() = '\0';
  if (buffer.size() == 1) {
    return {};
  }
  buffer.data()[buffer.size() - 1] = '\0';

#if defined(_MSC_VER) || defined(__MINGW32__) || defined(__MINGW64__) || defined(__STDC_LIB_EXT1__)
  const int strerror_result =
      ::strerror_s(buffer.data(), static_cast<std::size_t>(buffer.size()), result_from_get_errno);
  if (strerror_result != 0 && *buffer.data() == '\0') {
    return {};
  }
  return copy_strerror_message(buffer.data(), buffer);
#elif defined(ATFRAMEWORK_UTILS_PLATFORM_HAS_STRERROR_R)
  return handle_strerror_r_result(
      ::strerror_r(result_from_get_errno, buffer.data(), static_cast<std::size_t>(buffer.size())), buffer);
#else
  return copy_strerror_message(std::strerror(result_from_get_errno), buffer);
#endif
}

ATFRAMEWORK_UTILS_API int32_t atfork(void (*prepare)(), void (*parent)(), void (*child)()) noexcept {
#if defined(ATFRAMEWORK_UTILS_HAS_PTHREAD) && ATFRAMEWORK_UTILS_HAS_PTHREAD
  return ::pthread_atfork(prepare, parent, child);
#else
  (void)prepare;
  (void)parent;
  (void)child;
  return 0;
#endif
}

}  // namespace platform
ATFRAMEWORK_UTILS_NAMESPACE_END
