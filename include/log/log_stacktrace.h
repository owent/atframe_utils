/**
 * @file log_stacktrace.h
 * @brief 日志的导出执行栈封装
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2018-01-09
 * @note MSVC 需要使用/Zi导出pdb文件才能正确读取符号
 * @note GCC/Clang 在类Unix环境下需要使用-rdynamic选项来支持符号提取，或者使用addr2line来解析符号
 * @note addr2line命令实例: addr2line -Cfpe <二进制名称> [函数地址（可多个）...]
 * @note execinfo模式的符号解析不支持Watcom C++编译器
 * @history
 */

#ifndef UTIL_LOG_LOG_STACKTRACE_H
#define UTIL_LOG_LOG_STACKTRACE_H

#pragma once

#include <cstddef>
#include <stdint.h>

#include "config/compiler_features.h"

namespace util {
    namespace log {
        bool is_stacktrace_enabled() UTIL_CONFIG_NOEXCEPT;

        size_t stacktrace_write(char *buf, size_t bufsz);
    } // namespace log
} // namespace util

#endif
