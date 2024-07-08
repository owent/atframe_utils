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

#include <stdint.h>
#include <chrono>
#include <cstddef>
#include <cstring>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace log {
struct LIBATFRAME_UTILS_API stacktrace_options {
  uint16_t skip_start_frames;
  uint16_t skip_end_frames;
  uint16_t max_frames;
};

LIBATFRAME_UTILS_API bool is_stacktrace_enabled() noexcept;

LIBATFRAME_UTILS_API void set_stacktrace_lru_cache_size(size_t sz) noexcept;

LIBATFRAME_UTILS_API size_t get_stacktrace_lru_cache_size() noexcept;

LIBATFRAME_UTILS_API void set_stacktrace_lru_cache_timeout(std::chrono::microseconds timeout) noexcept;

LIBATFRAME_UTILS_API std::chrono::microseconds get_stacktrace_lru_cache_timeout() noexcept;

LIBATFRAME_UTILS_API void clear_stacktrace_lru_cache() noexcept;

LIBATFRAME_UTILS_API size_t stacktrace_write(char *buf, size_t bufsz, const stacktrace_options *options = nullptr);
}  // namespace log
LIBATFRAME_UTILS_NAMESPACE_END
