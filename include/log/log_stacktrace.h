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

#include <stdint.h>
#include <chrono>
#include <cstddef>
#include <cstring>
#include <functional>
#include <memory>
#include <utility>
#include <vector>

#if defined(LOG_STACKTRACE_USING_LIBUNWIND) && LOG_STACKTRACE_USING_LIBUNWIND
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 0
#elif defined(LOG_STACKTRACE_USING_EXECINFO) && LOG_STACKTRACE_USING_EXECINFO
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#elif defined(LOG_STACKTRACE_USING_UNWIND) && LOG_STACKTRACE_USING_UNWIND
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#elif defined(LOG_STACKTRACE_USING_DBGHELP) && LOG_STACKTRACE_USING_DBGHELP
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#elif defined(LOG_STACKTRACE_USING_DBGENG) && LOG_STACKTRACE_USING_DBGENG
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#else
#  define LOG_STACKTRACE_USING_TRIVALLY_HANDLE 1
#endif

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace log {
struct LIBATFRAME_UTILS_API stacktrace_options {
  uint16_t skip_start_frames;
  uint16_t skip_end_frames;
  uint16_t max_frames;
};

class stacktrace_handle {
 public:
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  LIBATFRAME_UTILS_API explicit stacktrace_handle(uintptr_t) noexcept;  // NOLINT: readability/casting
#else
  struct impl_type;

  LIBATFRAME_UTILS_API explicit stacktrace_handle(const util::memory::strong_rc_ptr<impl_type> &impl) noexcept;
  LIBATFRAME_UTILS_API explicit stacktrace_handle(util::memory::strong_rc_ptr<impl_type> &&impl) noexcept;
#endif

  LIBATFRAME_UTILS_API ~stacktrace_handle();

  LIBATFRAME_UTILS_API stacktrace_handle(const stacktrace_handle &other) noexcept;
  LIBATFRAME_UTILS_API stacktrace_handle(stacktrace_handle &&other) noexcept;

  LIBATFRAME_UTILS_API stacktrace_handle &operator=(const stacktrace_handle &other) noexcept;
  LIBATFRAME_UTILS_API stacktrace_handle &operator=(stacktrace_handle &&other) noexcept;

  LIBATFRAME_UTILS_API size_t hash_code() const noexcept;

  LIBATFRAME_UTILS_API void swap(stacktrace_handle &other) noexcept;

  LIBATFRAME_UTILS_API operator bool() const noexcept;

  LIBATFRAME_UTILS_API bool operator==(const stacktrace_handle &other) const noexcept;
  LIBATFRAME_UTILS_API bool operator<(const stacktrace_handle &other) const noexcept;

#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  inline uintptr_t get_address() const noexcept { return address_; }
#else
  inline const util::memory::strong_rc_ptr<impl_type> &get_internal_impl() const noexcept { return impl_; }
#endif

 private:
#if LOG_STACKTRACE_USING_TRIVALLY_HANDLE
  uintptr_t address_;
#else
  util::memory::strong_rc_ptr<impl_type> impl_;
#endif
};

class UTIL_SYMBOL_VISIBLE stacktrace_symbol {
 private:
  stacktrace_symbol(const stacktrace_symbol &) = delete;
  stacktrace_symbol(stacktrace_symbol &&) = delete;

  stacktrace_symbol &operator=(const stacktrace_symbol &) = delete;
  stacktrace_symbol &operator=(stacktrace_symbol &&) = delete;

 protected:
  LIBATFRAME_UTILS_API stacktrace_symbol();

 public:
  LIBATFRAME_UTILS_API virtual ~stacktrace_symbol();

  virtual nostd::string_view get_demangle_name() const noexcept = 0;

  virtual nostd::string_view get_raw_name() const noexcept = 0;

  virtual nostd::string_view get_offset_hint() const noexcept = 0;

  virtual uintptr_t get_address() const noexcept = 0;

  virtual std::chrono::system_clock::time_point get_timeout() const noexcept = 0;
};

LIBATFRAME_UTILS_API bool is_stacktrace_enabled() noexcept;

LIBATFRAME_UTILS_API void set_stacktrace_lru_cache_size(size_t sz) noexcept;

LIBATFRAME_UTILS_API size_t get_stacktrace_lru_cache_size() noexcept;

LIBATFRAME_UTILS_API void set_stacktrace_lru_cache_timeout(std::chrono::microseconds timeout) noexcept;

LIBATFRAME_UTILS_API std::chrono::microseconds get_stacktrace_lru_cache_timeout() noexcept;

LIBATFRAME_UTILS_API void clear_stacktrace_lru_cache() noexcept;

LIBATFRAME_UTILS_API void stacktrace_get_context(std::vector<stacktrace_handle> &stack_handles,
                                                 const stacktrace_options *options = nullptr) noexcept;

LIBATFRAME_UTILS_API void stacktrace_parse_symbols(gsl::span<stacktrace_handle> stack_handles,
                                                   std::vector<std::shared_ptr<stacktrace_symbol>> &symbols) noexcept;

LIBATFRAME_UTILS_API size_t stacktrace_write(char *buf, size_t bufsz, const stacktrace_options *options = nullptr);

LIBATFRAME_UTILS_API std::shared_ptr<stacktrace_symbol> stacktrace_find_symbol(stacktrace_handle stack_handle) noexcept;

}  // namespace log
LIBATFRAME_UTILS_NAMESPACE_END

namespace std {
template <>
struct hash<LIBATFRAME_UTILS_NAMESPACE_ID::log::stacktrace_handle> {
  size_t operator()(const LIBATFRAME_UTILS_NAMESPACE_ID::log::stacktrace_handle &handle) const noexcept {
    return handle.hash_code();
  }
};

}  // namespace std
