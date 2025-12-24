// Copyright 2021 atframework
// Licensed under the MIT licenses.
// Created by owent on 2015-06-29

/**
 * @version 1.0
 * @author owent
 * @date 2018-01-09
 * @note MSVC 需要使用/Zi导出pdb文件才能正确读取符号
 * @note GCC/Clang 在类Unix环境下需要使用-rdynamic选项来支持符号提取，或者使用addr2line来解析符号
 * @note addr2line命令实例: addr2line -Cfpe <二进制名称> [函数地址（可多个）...]
 * @note execinfo模式的符号解析不支持Watcom C++编译器
 * @history
 */

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>
#include <config/compiler_features.h>

#include <gsl/select-gsl.h>
#include <memory/rc_ptr.h>
#include <nostd/string_view.h>

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <functional>
#include <memory>
#include <utility>  // IWYU pragma: keep
#include <vector>

#if defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_LIBUNWIND
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 0
#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_EXECINFO
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_UNWIND
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGHELP
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#elif defined(ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG) && ATFRAMEWORK_UTILS_LOG_STACKTRACE_USING_DBGENG
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#else
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace log {
struct ATFRAMEWORK_UTILS_API stacktrace_options {
  uint16_t skip_start_frames;
  uint16_t skip_end_frames;
  uint16_t max_frames;
};

class stacktrace_handle {
 public:
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  ATFRAMEWORK_UTILS_API explicit stacktrace_handle(uintptr_t) noexcept;  // NOLINT: readability/casting
#else
  struct impl_type;

  ATFRAMEWORK_UTILS_API explicit stacktrace_handle(
      const ATFRAMEWORK_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<impl_type> &impl) noexcept;
  ATFRAMEWORK_UTILS_API explicit stacktrace_handle(
      ATFRAMEWORK_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<impl_type> &&impl) noexcept;
#endif

  ATFRAMEWORK_UTILS_API ~stacktrace_handle();

  ATFRAMEWORK_UTILS_API stacktrace_handle(const stacktrace_handle &other) noexcept;
  ATFRAMEWORK_UTILS_API stacktrace_handle(stacktrace_handle &&other) noexcept;

  ATFRAMEWORK_UTILS_API stacktrace_handle &operator=(const stacktrace_handle &other) noexcept;
  ATFRAMEWORK_UTILS_API stacktrace_handle &operator=(stacktrace_handle &&other) noexcept;

  ATFRAMEWORK_UTILS_API size_t hash_code() const noexcept;

  ATFRAMEWORK_UTILS_API void swap(stacktrace_handle &other) noexcept;

  ATFRAMEWORK_UTILS_API operator bool() const noexcept;

  ATFRAMEWORK_UTILS_API bool operator==(const stacktrace_handle &other) const noexcept;
  ATFRAMEWORK_UTILS_API bool operator<(const stacktrace_handle &other) const noexcept;

#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  inline uintptr_t get_address() const noexcept { return address_; }
#else
  inline const ATFRAMEWORK_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<impl_type> &get_internal_impl() const noexcept {
    return impl_;
  }
#endif

 private:
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  uintptr_t address_;
#else
  ATFRAMEWORK_UTILS_NAMESPACE_ID::memory::strong_rc_ptr<impl_type> impl_;
#endif
};

class ATFW_UTIL_SYMBOL_VISIBLE stacktrace_symbol {
 private:
  stacktrace_symbol(const stacktrace_symbol &) = delete;
  stacktrace_symbol(stacktrace_symbol &&) = delete;

  stacktrace_symbol &operator=(const stacktrace_symbol &) = delete;
  stacktrace_symbol &operator=(stacktrace_symbol &&) = delete;

 protected:
  ATFRAMEWORK_UTILS_API stacktrace_symbol();

 public:
  ATFRAMEWORK_UTILS_API virtual ~stacktrace_symbol();

  virtual nostd::string_view get_demangle_name() const noexcept = 0;

  virtual nostd::string_view get_raw_name() const noexcept = 0;

  virtual nostd::string_view get_offset_hint() const noexcept = 0;

  virtual uintptr_t get_address() const noexcept = 0;

  virtual std::chrono::system_clock::time_point get_timeout() const noexcept = 0;
};

ATFRAMEWORK_UTILS_API bool is_stacktrace_enabled() noexcept;

ATFRAMEWORK_UTILS_API void set_stacktrace_lru_cache_size(size_t sz) noexcept;

ATFRAMEWORK_UTILS_API size_t get_stacktrace_lru_cache_size() noexcept;

ATFRAMEWORK_UTILS_API void set_stacktrace_lru_cache_timeout(std::chrono::microseconds timeout) noexcept;

ATFRAMEWORK_UTILS_API std::chrono::microseconds get_stacktrace_lru_cache_timeout() noexcept;

ATFRAMEWORK_UTILS_API void clear_stacktrace_lru_cache() noexcept;

ATFRAMEWORK_UTILS_API void stacktrace_get_context(std::vector<stacktrace_handle> &stack_handles,
                                                  const stacktrace_options *options = nullptr) noexcept;

ATFRAMEWORK_UTILS_API void stacktrace_parse_symbols(gsl::span<stacktrace_handle> stack_handles,
                                                    std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) noexcept;

ATFRAMEWORK_UTILS_API size_t stacktrace_write(char *buf, size_t bufsz, const stacktrace_options *options = nullptr);

ATFRAMEWORK_UTILS_API std::shared_ptr<stacktrace_symbol> stacktrace_find_symbol(
    stacktrace_handle stack_handle) noexcept;

}  // namespace log
ATFRAMEWORK_UTILS_NAMESPACE_END

namespace std {
template <>
struct hash<ATFRAMEWORK_UTILS_NAMESPACE_ID::log::stacktrace_handle> {
  size_t operator()(const ATFRAMEWORK_UTILS_NAMESPACE_ID::log::stacktrace_handle &handle) const noexcept {
    return handle.hash_code();
  }
};

}  // namespace std
