// Copyright 2021 atframework
// Licensed under the MIT licenses.
// Created by owent on 2015-06-29

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <algorithm>
#include <bitset>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <utility>

#include "config/compiler/template_prefix.h"

#include "cli/shell_font.h"

#include "lock/spin_rw_lock.h"

#include "log/log_formatter.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace log {
using ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format;
using ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format_to;
using ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format_to_n;
using ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format_to_n_result;
using ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::vformat;
using ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::vformat_to;

class log_wrapper {
 public:
  using ptr_t = std::shared_ptr<log_wrapper>;

  struct ATFRAMEWORK_UTILS_API categorize_t {
    enum type {
      DEFAULT = 0,  // 服务框架
      MAX = ATFRAMEWORK_UTILS_LOG_CATEGORIZE_SIZE
    };
  };

  struct ATFRAMEWORK_UTILS_API options_t {
    enum type {
      OPT_AUTO_UPDATE_TIME = 0,  // 是否自动更新时间（会降低性能）
      OPT_USER_MAX,              // 允许外部接口修改的flag范围
      OPT_IS_GLOBAL,             // 是否是全局log的tag
      OPT_MAX
    };
  };

 public:
  using level_t = log_formatter::level_t;
  using caller_info_t = log_formatter::caller_info_t;
  using log_handler_t = std::function<void(const caller_info_t &caller, const char *content, size_t content_size)>;

  struct log_router_t {
    level_t::type level_min;
    level_t::type level_max;
    log_handler_t handle;
  };

 private:
  struct ATFRAMEWORK_UTILS_API construct_helper_t {};
  struct ATFRAMEWORK_UTILS_API log_operation_t {
    char *buffer;
    size_t total_size;
    size_t writen_size;
  };
  ATFRAMEWORK_UTILS_API log_wrapper();

 public:
  ATFRAMEWORK_UTILS_API log_wrapper(construct_helper_t &h);
  ATFRAMEWORK_UTILS_API virtual ~log_wrapper();

 public:
  // 初始化
  ATFRAMEWORK_UTILS_API int32_t init(level_t::type level = level_t::LOG_LW_DEBUG);

  static ATFRAMEWORK_UTILS_API void update();

#ifdef _MSC_VER
  ATFRAMEWORK_UTILS_API void log(const caller_info_t &caller, _In_z_ _Printf_format_string_ const char *fmt_text, ...);
#elif (defined(__clang__) && __clang_major__ >= 3)
  ATFRAMEWORK_UTILS_API void log(const caller_info_t &caller, const char *fmt_text, ...)
      __attribute__((__format__(__printf__, 3, 4)));
#elif (defined(__GNUC__) && __GNUC__ >= 4)
// 格式检查(成员函数有个隐含的this参数)
#  if defined(__MINGW32__) || defined(__MINGW64__)
  ATFRAMEWORK_UTILS_API void log(const caller_info_t &caller, const char *fmt_text, ...)
      __attribute__((format(__MINGW_PRINTF_FORMAT, 3, 4)));
#  else
  ATFRAMEWORK_UTILS_API void log(const caller_info_t &caller, const char *fmt_text, ...)
      __attribute__((format(printf, 3, 4)));
#  endif
#else
  ATFRAMEWORK_UTILS_API void log(const caller_info_t &caller, const char *fmt_text, ...);
#endif

#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
  template <class CharT, class... TARGS>
  ATFW_UTIL_NOINLINE_NOCLONE ATFRAMEWORK_UTILS_API_HEAD_ONLY void __format_log(
      const caller_info_t &caller,
      const ATFRAMEWORK_UTILS_NAMESPACE_ID::string::details::fmtapi_format_string_t<CharT, TARGS...> &fmt_text,
      TARGS &&...args) {
    log_operation_t writer;
    start_log(caller, writer);
    if (!log_sinks_.empty()) {
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
      try {
#  endif
        auto result = ATFRAMEWORK_UTILS_NAMESPACE_ID::string::__internal_format_to_n<CharT *, CharT>(
            reinterpret_cast<CharT *>(writer.buffer + writer.writen_size),
            (writer.total_size - writer.writen_size - 1) / sizeof(CharT), fmt_text, std::forward<TARGS>(args)...);

        if (result.size > 0) {
          writer.writen_size += static_cast<size_t>(result.size);
        }
        if (writer.writen_size < writer.total_size) {
          *(writer.buffer + writer.writen_size) = 0;
        } else {
          writer.writen_size = writer.total_size - 1;
          *(writer.buffer + writer.total_size - 1) = 0;
        }
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
      } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::format_error &e) {
        append_log(writer, "\r\nGot format error:\r\n", 0);
        append_log(writer, e.what(), 0);
      } catch (const std::runtime_error &e) {
        append_log(writer, "\r\nGot runtime error:\r\n", 0);
        append_log(writer, e.what(), 0);
      } catch (...) {
        append_log(writer, "\r\nGot unknown exception", 0);
      }
#  endif
    }
    finish_log(caller, writer);
  }

  template <class... TARGS>
  inline ATFRAMEWORK_UTILS_API_HEAD_ONLY void format_log(
      const caller_info_t &caller,
      ATFRAMEWORK_UTILS_NAMESPACE_ID::string::details::fmtapi_format_string_t<char, TARGS...> fmt_text,
      TARGS &&...args) {
    __format_log<char>(caller, fmt_text, std::forward<TARGS>(args)...);
  }

  template <class... TARGS>
  inline ATFRAMEWORK_UTILS_API_HEAD_ONLY void format_log(
      const caller_info_t &caller,
      ATFRAMEWORK_UTILS_NAMESPACE_ID::string::details::fmtapi_format_string_t<wchar_t, TARGS...> fmt_text,
      TARGS &&...args) {
    __format_log<wchar_t>(caller, fmt_text, std::forward<TARGS>(args)...);
  }

#  if defined(ATFRAMEWORK_UTILS_ENABLE_FMTLIB) && ATFRAMEWORK_UTILS_ENABLE_FMTLIB
  template <class... TARGS>
  inline ATFRAMEWORK_UTILS_API_HEAD_ONLY void format_log(
      const caller_info_t &caller,
      ATFRAMEWORK_UTILS_NAMESPACE_ID::string::details::fmtapi_format_string_t<char16_t, TARGS...> fmt_text,
      TARGS &&...args) {
    __format_log<char16_t>(caller, fmt_text, std::forward<TARGS>(args)...);
  }

  template <class... TARGS>
  inline ATFRAMEWORK_UTILS_API_HEAD_ONLY void format_log(
      const caller_info_t &caller,
      ATFRAMEWORK_UTILS_NAMESPACE_ID::string::details::fmtapi_format_string_t<char32_t, TARGS...> fmt_text,
      TARGS &&...args) {
    __format_log<char32_t>(caller, fmt_text, std::forward<TARGS>(args)...);
  }

#    ifdef __cpp_char8_t
  template <class... TARGS>
  inline ATFRAMEWORK_UTILS_API_HEAD_ONLY void format_log(
      const caller_info_t &caller,
      ATFRAMEWORK_UTILS_NAMESPACE_ID::string::details::fmtapi_format_string_t<char8_t, TARGS...> fmt_text,
      TARGS &&...args) {
    __format_log<char8_t>(caller, fmt_text, std::forward<TARGS>(args)...);
  }
#    endif
#  endif
#endif

  // 一般日志级别检查
  UTIL_FORCEINLINE bool check_level(level_t::type level) const { return log_level_ >= level; }

  UTIL_FORCEINLINE static bool check_level(const log_wrapper *logger, level_t::type level) {
    if (nullptr == logger) {
      return false;
    }
    return logger->log_level_ >= level;
  }

  ATFRAMEWORK_UTILS_API size_t sink_size() const;

  /**
   * @brief 添加后端接口
   * @param h 输出函数
   * @param level_min 最低日志级别
   * @param level_min 最高日志级别
   */
  ATFRAMEWORK_UTILS_API void add_sink(log_handler_t h, level_t::type level_min = level_t::LOG_LW_FATAL,
                                      level_t::type level_max = level_t::LOG_LW_DEBUG);

  /**
   * @brief 移除最后一个后端接口
   */
  ATFRAMEWORK_UTILS_API void pop_sink();

  /**
   * @brief 设置后端接口的日志级别
   * @param idx 后端接口索引
   * @param level_min 最低日志级别
   * @param level_min 最高日志级别
   * @return 如果没找到则返回false，成功返回true
   */
  ATFRAMEWORK_UTILS_API bool set_sink(size_t idx, level_t::type level_min = level_t::LOG_LW_FATAL,
                                      level_t::type level_max = level_t::LOG_LW_DEBUG);

  /**
   * @brief 移除所有后端, std::function无法比较，所以只能全清
   */
  ATFRAMEWORK_UTILS_API void clear_sinks();

  UTIL_FORCEINLINE void set_level(level_t::type l) { log_level_ = l; }

  UTIL_FORCEINLINE level_t::type get_level() const { return log_level_; }

  UTIL_FORCEINLINE const std::string &get_prefix_format() const { return prefix_format_; }

  UTIL_FORCEINLINE void set_prefix_format(const std::string &prefix) { prefix_format_ = prefix; }

  UTIL_FORCEINLINE bool get_option(options_t::type t) const {
    if (t < 0 || t >= options_t::OPT_MAX) {
      return false;
    }

    return options_.test(t);
  }

  UTIL_FORCEINLINE void set_option(options_t::type t, bool v) {
    if (t >= options_t::OPT_USER_MAX) {
      return;
    }

    options_.set(t, v);
  }

  ATFRAMEWORK_UTILS_API void set_stacktrace_level(level_t::type level_max = level_t::LOG_LW_DISABLED,
                                                  level_t::type level_min = level_t::LOG_LW_DISABLED);
  UTIL_FORCEINLINE const std::pair<level_t::type, level_t::type> &get_stacktrace_level() const {
    return stacktrace_level_;
  }

  /**
   * @brief 实际写出到落地接口
   */
  ATFRAMEWORK_UTILS_API void write_log(const caller_info_t &caller, const char *content, size_t content_size);

  // 白名单及用户指定日志输出可以针对哪个用户创建log_wrapper实例

  static ATFRAMEWORK_UTILS_API log_wrapper *mutable_log_cat(uint32_t cats = categorize_t::DEFAULT);
  static ATFRAMEWORK_UTILS_API ptr_t create_user_logger();

 private:
  ATFRAMEWORK_UTILS_API void start_log(const caller_info_t &caller, log_operation_t &);
  ATFRAMEWORK_UTILS_API void finish_log(const caller_info_t &caller, log_operation_t &);
  ATFRAMEWORK_UTILS_API void append_log(log_operation_t &, const char *str, size_t strsz);

 private:
  level_t::type log_level_;
  std::pair<level_t::type, level_t::type> stacktrace_level_;
  std::string prefix_format_;
  std::bitset<options_t::OPT_MAX> options_;
  std::list<log_router_t> log_sinks_;
  mutable ATFRAMEWORK_UTILS_NAMESPACE_ID::lock::spin_rw_lock log_sinks_lock_;
};  // NOLINT: readability/braces
}  // namespace log
ATFRAMEWORK_UTILS_NAMESPACE_END

#define WLOG_LEVELID(lv) static_cast<ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::type>(lv)

#define WDTLOGGETCAT(cat) ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::mutable_log_cat(cat)

#if defined(ATFRAMEWORK_UTILS_ENABLE_SOURCE_LOCATION) && ATFRAMEWORK_UTILS_ENABLE_SOURCE_LOCATION
#  define WDTLOGFILENF(lv, name) \
    ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::caller_info_t(lv, name, ::std::source_location::current())
#else
#  define WDTLOGFILENF(lv, name) \
    ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::caller_info_t(lv, name, __FILE__, __LINE__, __FUNCTION__)
#endif

#define WLOG_INIT(cat, lv) nullptr != WDTLOGGETCAT(cat) ? WDTLOGGETCAT(cat)->init(lv) : -1

#define WLOG_GETCAT(cat) ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::mutable_log_cat(cat)

// 按分类日志输出工具
#ifdef _MSC_VER

/** 全局日志输出工具 - snprintf **/
#  define WCLOGDEFLV(lv, lv_name, cat, ...)                                                   \
    if (ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) \
      WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#  define WCLOGTRACE(cat, ...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, cat, __VA_ARGS__)
#  define WCLOGDEBUG(cat, ...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, cat, __VA_ARGS__)
#  define WCLOGNOTICE(cat, ...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, cat, __VA_ARGS__)
#  define WCLOGINFO(cat, ...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, cat, __VA_ARGS__)
#  define WCLOGWARNING(cat, ...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, cat, __VA_ARGS__)
#  define WCLOGERROR(cat, ...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, cat, __VA_ARGS__)
#  define WCLOGFATAL(cat, ...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, cat, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 - snprintf **/

#  define WINSTLOGDEFLV(lv, lv_name, __inst, ...) \
    if ((__inst).check_level(lv)) (__inst).log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#  define WINSTLOGTRACE(__inst, ...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __inst, __VA_ARGS__)
#  define WINSTLOGDEBUG(__inst, ...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __inst, __VA_ARGS__)
#  define WINSTLOGNOTICE(__inst, ...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __inst, __VA_ARGS__)
#  define WINSTLOGINFO(__inst, ...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __inst, __VA_ARGS__)
#  define WINSTLOGWARNING(__inst, ...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __inst, __VA_ARGS__)
#  define WINSTLOGERROR(__inst, ...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __inst, __VA_ARGS__)
#  define WINSTLOGFATAL(__inst, ...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __inst, __VA_ARGS__)

#  if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
/** 全局日志输出工具 - std::format **/
#    define FWCLOGDEFLV(lv, lv_name, cat, ...)                                                  \
      if (ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) \
        WDTLOGGETCAT(cat)->format_log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#    define FWCLOGTRACE(cat, ...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, cat, __VA_ARGS__)
#    define FWCLOGDEBUG(cat, ...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, cat, __VA_ARGS__)
#    define FWCLOGNOTICE(cat, ...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, cat, __VA_ARGS__)
#    define FWCLOGINFO(cat, ...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, cat, __VA_ARGS__)
#    define FWCLOGWARNING(cat, ...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, cat, __VA_ARGS__)
#    define FWCLOGERROR(cat, ...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, cat, __VA_ARGS__)
#    define FWCLOGFATAL(cat, ...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, cat, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 - std::format **/
#    define FWINSTLOGDEFLV(lv, lv_name, __inst, ...) \
      if ((__inst).check_level(lv)) (__inst).format_log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#    define FWINSTLOGTRACE(__inst, ...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGDEBUG(__inst, ...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGNOTICE(__inst, ...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGINFO(__inst, ...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGWARNING(__inst, ...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGERROR(__inst, ...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGFATAL(__inst, ...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __inst, __VA_ARGS__)

#  endif

#else

/** 全局日志输出工具 - snprintf **/
#  define WCLOGDEFLV(lv, lv_name, cat, args...)                                               \
    if (ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) \
      WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), ##args);

#  define WCLOGTRACE(...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __VA_ARGS__)
#  define WCLOGDEBUG(...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __VA_ARGS__)
#  define WCLOGNOTICE(...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __VA_ARGS__)
#  define WCLOGINFO(...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __VA_ARGS__)
#  define WCLOGWARNING(...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __VA_ARGS__)
#  define WCLOGERROR(...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __VA_ARGS__)
#  define WCLOGFATAL(...) \
    WCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 - snprintf **/
#  define WINSTLOGDEFLV(lv, lv_name, __inst, args...) \
    if ((__inst).check_level(lv)) (__inst).log(WDTLOGFILENF(lv, lv_name), ##args);

#  define WINSTLOGTRACE(...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __VA_ARGS__)
#  define WINSTLOGDEBUG(...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __VA_ARGS__)
#  define WINSTLOGNOTICE(...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __VA_ARGS__)
#  define WINSTLOGINFO(...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __VA_ARGS__)
#  define WINSTLOGWARNING(...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __VA_ARGS__)
#  define WINSTLOGERROR(...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __VA_ARGS__)
#  define WINSTLOGFATAL(...) \
    WINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __VA_ARGS__)

#  if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
/** 全局日志输出工具 - std::format **/
#    define FWCLOGDEFLV(lv, lv_name, cat, FMT, args...)                                         \
      if (ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) \
        WDTLOGGETCAT(cat)->format_log(WDTLOGFILENF(lv, lv_name), FMT, ##args);

#    define FWCLOGTRACE(...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __VA_ARGS__)
#    define FWCLOGDEBUG(...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __VA_ARGS__)
#    define FWCLOGNOTICE(...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __VA_ARGS__)
#    define FWCLOGINFO(...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __VA_ARGS__)
#    define FWCLOGWARNING(...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __VA_ARGS__)
#    define FWCLOGERROR(...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __VA_ARGS__)
#    define FWCLOGFATAL(...) \
      FWCLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 - std::format **/
#    define FWINSTLOGDEFLV(lv, lv_name, __inst, FMT, args...) \
      if ((__inst).check_level(lv)) (__inst).format_log(WDTLOGFILENF(lv, lv_name), FMT, ##args);

#    define FWINSTLOGTRACE(...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __VA_ARGS__)
#    define FWINSTLOGDEBUG(...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __VA_ARGS__)
#    define FWINSTLOGNOTICE(...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __VA_ARGS__)
#    define FWINSTLOGINFO(...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __VA_ARGS__)
#    define FWINSTLOGWARNING(...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __VA_ARGS__)
#    define FWINSTLOGERROR(...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __VA_ARGS__)
#    define FWINSTLOGFATAL(...) \
      FWINSTLOGDEFLV(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __VA_ARGS__)

#  endif
#endif
#define LOG_WRAPPER_FWAPI_FORMAT(...) ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT(__VA_ARGS__)
#define LOG_WRAPPER_FWAPI_FORMAT_TO(...) ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO(__VA_ARGS__)
#define LOG_WRAPPER_FWAPI_FORMAT_TO_N(...) ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO_N(__VA_ARGS__)
#define LOG_WRAPPER_FWAPI_VFORMAT(...) ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT(__VA_ARGS__)
#define LOG_WRAPPER_FWAPI_VFORMAT_TO(...) ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT_TO(__VA_ARGS__)
#define LOG_WRAPPER_FWAPI_MAKE_FORMAT_ARGS(...) ATFRAMEWORK_UTILS_STRING_FWAPI_MAKE_FORMAT_ARGS(__VA_ARGS__)

// 默认日志输出工具
#define WLOGTRACE(...) WCLOGTRACE(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGDEBUG(...) WCLOGDEBUG(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGNOTICE(...) \
  WCLOGNOTICE(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGINFO(...) WCLOGINFO(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGWARNING(...) \
  WCLOGWARNING(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGERROR(...) WCLOGERROR(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGFATAL(...) WCLOGFATAL(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)

#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
#  define FWLOGTRACE(...) \
    FWCLOGTRACE(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGDEBUG(...) \
    FWCLOGDEBUG(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGNOTICE(...) \
    FWCLOGNOTICE(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGINFO(...) \
    FWCLOGINFO(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGWARNING(...) \
    FWCLOGWARNING(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGERROR(...) \
    FWCLOGERROR(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGFATAL(...) \
    FWCLOGFATAL(ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#endif

// 控制台输出工具
#ifdef _MSC_VER
#  define PSTDTERMCOLOR(os_ident, code, fmt_text, ...)                                                         \
                                                                                                               \
    {                                                                                                          \
      ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_stream::shell_stream_opr log_wrapper_pstd_ss(&std::os_ident); \
      log_wrapper_pstd_ss.open(code);                                                                          \
      log_wrapper_pstd_ss.close();                                                                             \
      printf(fmt_text, __VA_ARGS__);                                                                           \
    }

#else
#  define PSTDTERMCOLOR(os_ident, code, fmt_text, args...)                                                     \
                                                                                                               \
    {                                                                                                          \
      ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_stream::shell_stream_opr log_wrapper_pstd_ss(&std::os_ident); \
      log_wrapper_pstd_ss.open(code);                                                                          \
      log_wrapper_pstd_ss.close();                                                                             \
      printf(fmt_text, ##args);                                                                                \
    }

#endif

#define PSTDINFO(...) printf(__VA_ARGS__)
#define PSTDNOTICE(...) \
  PSTDTERMCOLOR(cout, ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW, __VA_ARGS__)
#define PSTDWARNING(...)                                                                                              \
  PSTDTERMCOLOR(cerr,                                                                                                 \
                ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_SPEC_BOLD |                         \
                    static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW), \
                __VA_ARGS__)
#define PSTDERROR(...)                                                                                             \
  PSTDTERMCOLOR(cerr,                                                                                              \
                ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_SPEC_BOLD |                      \
                    static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED), \
                __VA_ARGS__)
#define PSTDFATAL(...) \
  PSTDTERMCOLOR(cerr, ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_MAGENTA, __VA_ARGS__)
#define PSTDOK(...) \
  PSTDTERMCOLOR(cout, ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_GREEN, __VA_ARGS__)
//
#ifndef NDEBUG
#  define PSTDTRACE(...) \
    PSTDTERMCOLOR(cout, ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_CYAN, __VA_ARGS__)
#  define PSTDDEBUG(...) \
    PSTDTERMCOLOR(cout, ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_CYAN, __VA_ARGS__)

#  define PSTDMARK                                                                                                   \
    PSTDTERMCOLOR(cout,                                                                                              \
                  ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_SPEC_BOLD |                      \
                      static_cast<int>(ATFRAMEWORK_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED), \
                  "Mark: %s:%s (function %s)\n", __FILE__, __LINE__, __FUNCTION__)
#else
#  define PSTDTRACE(...)
#  define PSTDDEBUG(...)
#  define PSTDMARK

#endif

#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_AS(typename ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::type,
                                         int);
ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_AS(typename ATFRAMEWORK_UTILS_NAMESPACE_ID::log::log_wrapper::options_t::type,
                                         int);
#endif

#include "config/compiler/template_suffix.h"
