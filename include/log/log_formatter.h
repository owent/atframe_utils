/**
 * @file log_wrapper.h
 * @brief 日志包装器
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2015-06-29
 * @history
 */

#ifndef UTIL_LOG_LOG_FORMATTER_H
#define UTIL_LOG_LOG_FORMATTER_H

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <nostd/string_view.h>

#include <inttypes.h>
#include <stdint.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <iterator>
#include <string>
#include <type_traits>

#if defined(LIBATFRAME_UTILS_ENABLE_STD_FORMAT) && LIBATFRAME_UTILS_ENABLE_STD_FORMAT
#  include <format>
#  ifndef LOG_WRAPPER_FWAPI_FMT_STRING
#    define LOG_WRAPPER_FWAPI_FMT_STRING(S) (S)
#  endif
#elif defined(LIBATFRAME_UTILS_ENABLE_FMTLIB) && LIBATFRAME_UTILS_ENABLE_FMTLIB
#  include <fmt/format.h>
#  ifndef LOG_WRAPPER_FWAPI_FMT_STRING
#    define LOG_WRAPPER_FWAPI_FMT_STRING(S) FMT_STRING(S)
#  endif
#  if FMT_VERSION >= 80000
#    define LOG_WRAPPER_FWAPI_USING_FORMAT_STRING(...) fmt::format_string<__VA_ARGS__>
#  endif
#endif

namespace util {
namespace log {
#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
namespace details {
template <class OutputIt>
class LIBATFRAME_UTILS_API_HEAD_ONLY truncating_iterator_base {
 public:
#  if defined(LIBATFRAME_UTILS_ENABLE_STD_FORMAT) && LIBATFRAME_UTILS_ENABLE_STD_FORMAT
  using size_type = typename std::iter_difference_t<OutputIt>;
#  else
  using size_type = size_t;
#  endif
  using iterator_category = std::output_iterator_tag;
  using value_type = typename std::iterator_traits<OutputIt>::value_type;
  using difference_type = void;
  using pointer = void;
  using reference = void;

  truncating_iterator_base(OutputIt out, size_type limit) : out_(out), limit_(limit), count_(0) {}

  OutputIt base() const { return out_; }
  size_type count() const { return count_; }

 protected:
  OutputIt out_;
  size_type limit_;
  size_type count_;
};

// An output iterator that truncates the output and counts the number of objects
// written to it.
template <class OutputIt,
          class Enable = typename std::is_void<typename std::iterator_traits<OutputIt>::value_type>::type>
class LIBATFRAME_UTILS_API_HEAD_ONLY truncating_iterator;

template <class OutputIt>
class LIBATFRAME_UTILS_API_HEAD_ONLY truncating_iterator<OutputIt, std::false_type>
    : public truncating_iterator_base<OutputIt> {
  mutable typename truncating_iterator_base<OutputIt>::value_type blackhole_;

 public:
  using value_type = typename truncating_iterator_base<OutputIt>::value_type;
  using size_type = typename truncating_iterator_base<OutputIt>::size_type;

  truncating_iterator(OutputIt out, size_type limit) : truncating_iterator_base<OutputIt>(out, limit) {}

  truncating_iterator &operator++() {
    if (this->count_++ < this->limit_) ++this->out_;
    return *this;
  }

  truncating_iterator operator++(int) {
    auto it = *this;
    ++*this;
    return it;
  }

  value_type &operator*() const { return this->count_ < this->limit_ ? *this->out_ : blackhole_; }
};

template <class OutputIt>
class LIBATFRAME_UTILS_API_HEAD_ONLY truncating_iterator<OutputIt, std::true_type>
    : public truncating_iterator_base<OutputIt> {
 public:
  using value_type = typename truncating_iterator_base<OutputIt>::value_type;
  using size_type = typename truncating_iterator_base<OutputIt>::size_type;

  truncating_iterator(OutputIt out, size_type limit) : truncating_iterator_base<OutputIt>(out, limit) {}

  template <typename T>
  truncating_iterator &operator=(T val) {
    if (this->count_++ < this->limit_) *this->out_++ = val;
    return *this;
  }

  truncating_iterator &operator++() { return *this; }
  truncating_iterator &operator++(int) { return *this; }
  truncating_iterator &operator*() { return *this; }
};

}  // namespace details
#endif

/**
 * @brief 日志格式化数据
 */
class log_formatter {
 public:
  struct LIBATFRAME_UTILS_API flag_t {
    enum type {
      INDEX = 0x01,  // 日志rotation序号
      DATE = 0x02,   // 日期
      TIME = 0x04,   // 时间
    };
  };

  struct LIBATFRAME_UTILS_API level_t {
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

  struct LIBATFRAME_UTILS_API caller_info_t {
    level_t::type level_id;
    const char *level_name;
    const char *file_path;
    uint32_t line_number;
    const char *func_name;
    uint32_t rotate_index;

    caller_info_t();
    caller_info_t(level_t::type lid, const char *lname, const char *fpath, uint32_t lnum, const char *fnname);
    caller_info_t(level_t::type lid, const char *lname, const char *fpath, uint32_t lnum, const char *fnname,
                  uint32_t ridx);
  };

 public:
  LIBATFRAME_UTILS_API static bool check_flag(int32_t flags, int32_t checked);

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
   *            %f:  	小于秒的时间标识 TODO:
   * 目前为了速度是clock中的单位为毫秒的部分，但不是真正的毫秒数，以后改成真正的毫秒数 %L:  	日志级别名称 %l:
   * 日志级别ID %s:  	调用处源码文件路径 %k:  	调用处源码文件名 %n:  	调用处源码行号 %C:  	调用处函数名称
   *            %N:   轮询序号(仅在内部接口有效)
   */
  LIBATFRAME_UTILS_API static size_t format(char *buff, size_t bufz, const char *fmt, size_t fmtz,
                                            const caller_info_t &caller);

  LIBATFRAME_UTILS_API static bool check_rotation_var(const char *fmt, size_t fmtz);

  LIBATFRAME_UTILS_API static bool has_format(const char *fmt, size_t fmtz);

  /**
   * @brief 设置工程目录，会影响format时的%s参数，如果文件路径以工程目录开头，则会用~替换
   */
  LIBATFRAME_UTILS_API static void set_project_directory(const char *dirbuf, size_t dirsz);

  /**
   * @brief 设置工程目录，会影响format时的%s参数，如果文件路径以工程目录开头，则会用~替换
   * @param name 日志等级的名称（disable/disabled, fatal, error, warn/warning, info, notice, debug）
   * @return 读取到的等级id,默认会返回debug
   */
  LIBATFRAME_UTILS_API static level_t::type get_level_by_name(const char *name);

 private:
  LIBATFRAME_UTILS_API static struct tm *get_iso_tm();
  static std::string project_dir_;
};
}  // namespace log
}  // namespace util

#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
#  if defined(LIBATFRAME_UTILS_ENABLE_STD_FORMAT) && LIBATFRAME_UTILS_ENABLE_STD_FORMAT
#    define LOG_WRAPPER_FWAPI_DECL_NAMESPACE() namespace std
#    define LOG_WRAPPER_FWAPI_FORMAT_AS(Type, Base)                                 \
      LOG_WRAPPER_FWAPI_DECL_NAMESPACE() {                                          \
        template <class CharT>                                                      \
        struct formatter<Type, CharT> : formatter<Base, CharT> {                    \
          template <class FormatContext>                                            \
          auto format(Type const &val, FormatContext &ctx) -> decltype(ctx.out()) { \
            return formatter<Base, CharT>::format(val, ctx);                        \
          }                                                                         \
        };                                                                          \
      }
#  else
#    define LOG_WRAPPER_FWAPI_DECL_NAMESPACE() namespace fmt
#    define LOG_WRAPPER_FWAPI_FORMAT_AS(Type, Base) \
      LOG_WRAPPER_FWAPI_DECL_NAMESPACE() { FMT_FORMAT_AS(Type, Base); }
#  endif

LOG_WRAPPER_FWAPI_FORMAT_AS(typename ::util::log::log_formatter::flag_t::type, int);
LOG_WRAPPER_FWAPI_FORMAT_AS(typename ::util::log::log_formatter::level_t::type, int);

LOG_WRAPPER_FWAPI_DECL_NAMESPACE() {
  template <class CharT, class Traits>
  struct formatter<::util::nostd::basic_string_view<CharT, Traits>, CharT>
      : formatter<LOG_WRAPPER_FWAPI_NAMESPACE basic_string_view<CharT>, CharT> {
    template <class FormatContext>
    auto format(::util::nostd::basic_string_view<CharT, Traits> const &val, FormatContext &ctx) -> decltype(ctx.out()) {
      return formatter<LOG_WRAPPER_FWAPI_NAMESPACE basic_string_view<CharT>, CharT>::format(
          LOG_WRAPPER_FWAPI_NAMESPACE basic_string_view<CharT>{val.data(), val.size()}, ctx);
    }
  };
}

#endif

#endif
