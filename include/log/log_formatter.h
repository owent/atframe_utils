// Copyright 2024 atframework
// Licensed under the MIT licenses.
// Created by owent on 2015-06-29

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <gsl/select-gsl.h>
#include <nostd/string_view.h>

#include <inttypes.h>
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <string>

#if defined(ATFRAMEWORK_UTILS_ENABLE_SOURCE_LOCATION) && ATFRAMEWORK_UTILS_ENABLE_SOURCE_LOCATION
#  include <source_location>
#endif

#include "string/string_format.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace log {
/**
 * @brief 日志格式化数据
 */
class log_formatter {
 public:
  struct ATFRAMEWORK_UTILS_API flag_t {
    enum type {
      INDEX = 0x01,  // 日志rotation序号
      DATE = 0x02,   // 日期
      TIME = 0x04,   // 时间
    };
  };

  struct ATFRAMEWORK_UTILS_API level_t {
    enum type {
      LOG_LW_DISABLED = 0,  // 关闭日志
      LOG_LW_FATAL,         // 强制输出
      LOG_LW_ERROR,         // 错误
      LOG_LW_WARNING,
      LOG_LW_INFO,
      LOG_LW_NOTICE,
      LOG_LW_DEBUG,
      LOG_LW_TRACE,
    };
  };

  struct UTIL_SYMBOL_VISIBLE caller_info_t {
    level_t::type level_id;
    gsl::string_view level_name;
    gsl::string_view file_path;
    uint32_t line_number;
    gsl::string_view func_name;
    uint32_t rotate_index;

    inline caller_info_t() noexcept
        : level_id(level_t::LOG_LW_DISABLED), level_name{}, file_path{}, line_number(0), func_name{}, rotate_index(0) {}

    inline ~caller_info_t() {}

    inline caller_info_t(level_t::type lid, gsl::string_view lname, gsl::string_view fpath, uint32_t lnum,
                         gsl::string_view fnname) noexcept
        : level_id(lid), level_name(lname), file_path(fpath), line_number(lnum), func_name(fnname), rotate_index(0) {}

    inline caller_info_t(level_t::type lid, gsl::string_view lname, gsl::string_view fpath, uint32_t lnum,
                         gsl::string_view fnname, uint32_t ridx) noexcept
        : level_id(lid),
          level_name(lname),
          file_path(fpath),
          line_number(lnum),
          func_name(fnname),
          rotate_index(ridx) {}

#if defined(ATFRAMEWORK_UTILS_ENABLE_SOURCE_LOCATION) && ATFRAMEWORK_UTILS_ENABLE_SOURCE_LOCATION
    inline caller_info_t(level_t::type lid, gsl::string_view lname, const std::source_location &sloc) noexcept
        : level_id(lid),
          level_name(lname),
          file_path(sloc.file_name()),
          line_number(static_cast<uint32_t>(sloc.line())),
          func_name(sloc.function_name()) {}

    inline caller_info_t(level_t::type lid, gsl::string_view lname, const std::source_location &sloc,
                         uint32_t ridx) noexcept
        : level_id(lid),
          level_name(lname),
          file_path(sloc.file_name()),
          line_number(static_cast<uint32_t>(sloc.line())),
          func_name(sloc.function_name()),
          rotate_index(ridx) {}
#endif
  };

 public:
  ATFRAMEWORK_UTILS_API static bool check_flag(int32_t flags, int32_t checked);

  /**
   * @brief 格式化到缓冲区，如果缓冲区不足忽略后面的数据
   * @note 如果返回值大于0，本函数保证输出的数据结尾有'\0'，且返回的长度不计这个'\0'
   * @return 返回消耗的缓存区长度
   * @see http://en.cppreference.com/w/c/chrono/strftime
   * @note 支持的格式规则
   *            %Y:  	writes year as a 4 digit decimal number
   *            %y:   writes last 2 digits of year as a decimal number (range [00,99])
   *            %m:  	writes month as a decimal number (range [01,12])
   *            %j:   writes day of the year as a decimal number (range [001,366])
   *            %d:  	writes day of the month as a decimal number (range [01,31])
   *            %w:  	writes weekday as a decimal number, where Sunday is 0 (range [0-6])
   *            %H:   writes hour as a decimal number, 24 hour clock (range [00-23])
   *            %I:  	writes hour as a decimal number, 12 hour clock (range [01,12])
   *            %M:  	writes minute as a decimal number (range [00,59])
   *            %S:  	writes second as a decimal number (range [00,60])
   *            %F:  	equivalent to "%Y-%m-%d" (the ISO 8601 date format)
   *            %T:  	equivalent to "%H:%M:%S" (the ISO 8601 time format)
   *            %R:  	equivalent to "%H:%M"
   *            %f:  	小于秒的时间标识
   *            %L:   日志级别名称
   *            %l:   日志级别ID
   *            %s:   调用文件名(尝试解析工程目录)
   *            %k:   调用文件名
   *            %n:   调用行号
   *            %C:   调用函数名
   *            %N:   轮询序号(仅在内部接口有效)
   */
  ATFRAMEWORK_UTILS_API static size_t format(char *buff, size_t bufz, const char *fmt, size_t fmtz,
                                             const caller_info_t &caller);

  ATFRAMEWORK_UTILS_API static bool check_rotation_var(const char *fmt, size_t fmtz);

  ATFRAMEWORK_UTILS_API static bool has_format(const char *fmt, size_t fmtz);

  /**
   * @brief 设置工程目录，会影响format时的%s参数，如果文件路径以工程目录开头，则会用~替换
   */
  ATFRAMEWORK_UTILS_API static void set_project_directory(const char *dirbuf, size_t dirsz);

  /**
   * @brief 设置工程目录，会影响format时的%s参数，如果文件路径以工程目录开头，则会用~替换
   * @param name 日志等级的名称（disable/disabled, fatal, error, warn/warning, info, notice, debug）
   * @return 读取到的等级id,默认会返回debug
   */
  ATFRAMEWORK_UTILS_API static level_t::type get_level_by_name(gsl::string_view name);

 private:
  ATFRAMEWORK_UTILS_API static struct tm *get_iso_tm();
  static std::string project_dir_;
};
}  // namespace log
ATFRAMEWORK_UTILS_NAMESPACE_END

#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI

ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_AS(typename ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_formatter::flag_t::type,
                                         int);
ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_AS(typename ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_formatter::level_t::type,
                                         int);

#endif
