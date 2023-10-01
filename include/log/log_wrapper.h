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
#include <type_traits>

#include "config/compiler/template_prefix.h"

#include "cli/shell_font.h"

#include "lock/spin_rw_lock.h"

#include "log_formatter.h"

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace log {

#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
template <class TCONTEXT = LOG_WRAPPER_FWAPI_NAMESPACE format_context, class... TARGS>
LIBATFRAME_UTILS_API_HEAD_ONLY auto make_format_args(TARGS &&...args) {
#  if defined(LIBATFRAME_UTILS_ENABLE_FMTLIB) && LIBATFRAME_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
      FMT_VERSION >= 100000
  return LOG_WRAPPER_FWAPI_NAMESPACE make_format_args<TCONTEXT>(args...);
#  else
  return LOG_WRAPPER_FWAPI_NAMESPACE make_format_args<TCONTEXT>(std::forward<TARGS>(args)...);
#  endif
}

#  ifdef LOG_WRAPPER_FWAPI_USING_FORMAT_STRING
template <class... TARGS>
LIBATFRAME_UTILS_API_HEAD_ONLY std::string format(LOG_WRAPPER_FWAPI_USING_FORMAT_STRING(TARGS...) fmt_text,
                                                  TARGS &&...args) {
#  else
template <class TFMT, class... TARGS>
LIBATFRAME_UTILS_API_HEAD_ONLY std::string format(TFMT &&fmt_text, TARGS &&...args) {
#  endif
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  try {
#  endif
#  ifdef LOG_WRAPPER_FWAPI_USING_FORMAT_STRING
    return LOG_WRAPPER_FWAPI_NAMESPACE format(fmt_text, std::forward<TARGS>(args)...);
#  elif defined(LIBATFRAME_UTILS_ENABLE_FORWARD_FMTTEXT) && LIBATFRAME_UTILS_ENABLE_FORWARD_FMTTEXT
  return LOG_WRAPPER_FWAPI_NAMESPACE format(std::forward<TFMT>(fmt_text), std::forward<TARGS>(args)...);
#  else
return LOG_WRAPPER_FWAPI_NAMESPACE vformat(std::forward<TFMT>(fmt_text),
#    if defined(LIBATFRAME_UTILS_ENABLE_FMTLIB) && LIBATFRAME_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
        FMT_VERSION >= 100000
                                           LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(args...)
#    else
                                           LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(std::forward<TARGS>(args)...)
#    endif
);
#  endif
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  } catch (const LOG_WRAPPER_FWAPI_NAMESPACE format_error &e) {
    return e.what();
  } catch (const std::runtime_error &e) {
    return e.what();
  } catch (...) {
    return "format got unknown exception";
  }
#  endif
}

#  ifdef LOG_WRAPPER_FWAPI_USING_FORMAT_STRING
template <class OutputIt, class... TARGS>
LIBATFRAME_UTILS_API_HEAD_ONLY auto format_to(OutputIt out, LOG_WRAPPER_FWAPI_USING_FORMAT_STRING(TARGS...) fmt_text,
                                              TARGS &&...args) {
#  else
template <class OutputIt, class TFMT, class... TARGS>
LIBATFRAME_UTILS_API_HEAD_ONLY auto format_to(OutputIt out, TFMT &&fmt_text, TARGS &&...args) {
#  endif
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  try {
#  endif
#  ifdef LOG_WRAPPER_FWAPI_USING_FORMAT_STRING
    return LOG_WRAPPER_FWAPI_NAMESPACE format_to(out, fmt_text, std::forward<TARGS>(args)...);
#  elif defined(LIBATFRAME_UTILS_ENABLE_FORWARD_FMTTEXT) && LIBATFRAME_UTILS_ENABLE_FORWARD_FMTTEXT
  return LOG_WRAPPER_FWAPI_NAMESPACE format_to(out, std::forward<TFMT>(fmt_text), std::forward<TARGS>(args)...);
#  else
return LOG_WRAPPER_FWAPI_NAMESPACE vformat_to(out, std::forward<TFMT>(fmt_text),
#    if defined(LIBATFRAME_UTILS_ENABLE_FMTLIB) && LIBATFRAME_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
        FMT_VERSION >= 100000
                                              LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(args...)
#    else
                                               LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(
                                                   std::forward<TARGS>(args)...)
#    endif
);
#  endif
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  } catch (const LOG_WRAPPER_FWAPI_NAMESPACE format_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  } catch (const std::runtime_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  } catch (...) {
    const char *input_begin = "format got unknown exception";
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  }
#  endif
}

template <class OutputIt, class TFMT, class... TARGS>
LIBATFRAME_UTILS_API_HEAD_ONLY LOG_WRAPPER_FWAPI_NAMESPACE format_to_n_result<OutputIt> format_to_n(OutputIt out,
                                                                                                    size_t n,
                                                                                                    TFMT &&fmt_text,
                                                                                                    TARGS &&...args) {
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  try {
#  endif
#  ifdef LOG_WRAPPER_FWAPI_USING_FORMAT_STRING
    return LOG_WRAPPER_FWAPI_NAMESPACE vformat_to_n(
        out, static_cast<typename details::truncating_iterator<OutputIt>::size_type>(n), std::forward<TFMT>(fmt_text),
#    if defined(LIBATFRAME_UTILS_ENABLE_FMTLIB) && LIBATFRAME_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
        FMT_VERSION >= 100000
        LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(args...)
#    else
        LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(std::forward<TARGS>(args)...)
#    endif
    );
#  elif defined(LIBATFRAME_UTILS_ENABLE_FORWARD_FMTTEXT) && LIBATFRAME_UTILS_ENABLE_FORWARD_FMTTEXT
  return LOG_WRAPPER_FWAPI_NAMESPACE format_to_n(
      out, static_cast<typename details::truncating_iterator<OutputIt>::size_type>(n), std::forward<TFMT>(fmt_text),
      std::forward<TARGS>(args)...);
#  else
  typename details::truncating_iterator<OutputIt> buf(std::move(out), n);
  LOG_WRAPPER_FWAPI_NAMESPACE vformat_to(std::back_inserter(buf), std::forward<TFMT>(fmt_text),
#    if defined(LIBATFRAME_UTILS_ENABLE_FMTLIB) && LIBATFRAME_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
        FMT_VERSION >= 100000
                                         LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(args...)
#    else
                                         LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(std::forward<TARGS>(args)...)
#    endif
  );
  LOG_WRAPPER_FWAPI_NAMESPACE format_to_n_result<OutputIt> ret;
  ret.out = buf.base();
  ret.size = static_cast<decltype(ret.size)>(buf.count());
  return ret;
#  endif
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  } catch (const LOG_WRAPPER_FWAPI_NAMESPACE format_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    details::truncating_iterator<OutputIt> res =
        std::copy(input_begin, input_end, details::truncating_iterator<OutputIt>(out, n));
    return {res.base(), res.count()};
  } catch (const std::runtime_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    details::truncating_iterator<OutputIt> res =
        std::copy(input_begin, input_end, details::truncating_iterator<OutputIt>(out, n));
    return {res.base(), res.count()};
  } catch (...) {
    const char *input_begin = "format got unknown exception";
    const char *input_end = input_begin + strlen(input_begin);
    details::truncating_iterator<OutputIt> res =
        std::copy(input_begin, input_end, details::truncating_iterator<OutputIt>(out, n));
    return {res.base(), res.count()};
  }
#  endif
}

template <class TFMT, class TARGS>
LIBATFRAME_UTILS_API_HEAD_ONLY std::string vformat(TFMT &&fmt_text, TARGS &&args) {
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  try {
#  endif
    return LOG_WRAPPER_FWAPI_NAMESPACE vformat(std::forward<TFMT>(fmt_text), std::forward<TARGS>(args));
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  } catch (const LOG_WRAPPER_FWAPI_NAMESPACE format_error &e) {
    return e.what();
  } catch (const std::runtime_error &e) {
    return e.what();
  } catch (...) {
    return "format got unknown exception";
  }
#  endif
}

template <class OutputIt, class TFMT, class TARGS>
LIBATFRAME_UTILS_API_HEAD_ONLY OutputIt vformat_to(OutputIt out, TFMT &&fmt_text, TARGS &&args) {
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  try {
#  endif
    return LOG_WRAPPER_FWAPI_NAMESPACE vformat_to(out, std::forward<TFMT>(fmt_text), std::forward<TARGS>(args));
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  } catch (const LOG_WRAPPER_FWAPI_NAMESPACE format_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  } catch (const std::runtime_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  } catch (...) {
    const char *input_begin = "format got unknown exception";
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  }
#  endif
}
#endif

class log_wrapper {
 public:
  using ptr_t = std::shared_ptr<log_wrapper>;

  struct LIBATFRAME_UTILS_API categorize_t {
    enum type {
      DEFAULT = 0,  // 服务框架
      MAX = LOG_WRAPPER_CATEGORIZE_SIZE
    };
  };

  struct LIBATFRAME_UTILS_API options_t {
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
  struct LIBATFRAME_UTILS_API construct_helper_t {};
  struct LIBATFRAME_UTILS_API log_operation_t {
    char *buffer;
    size_t total_size;
    size_t writen_size;
  };
  LIBATFRAME_UTILS_API log_wrapper();

 public:
  LIBATFRAME_UTILS_API log_wrapper(construct_helper_t &h);
  LIBATFRAME_UTILS_API virtual ~log_wrapper();

 public:
  // 初始化
  LIBATFRAME_UTILS_API int32_t init(level_t::type level = level_t::LOG_LW_DEBUG);

  static LIBATFRAME_UTILS_API void update();

  LIBATFRAME_UTILS_API void log(const caller_info_t &caller,
#ifdef _MSC_VER
                                _In_z_ _Printf_format_string_ const char *fmt_text, ...);
#elif (defined(__clang__) && __clang_major__ >= 3)
                                const char *fmt_text, ...) __attribute__((__format__(__printf__, 3, 4)));
#elif (defined(__GNUC__) && __GNUC__ >= 4)
// 格式检查(成员函数有个隐含的this参数)
#  if defined(__MINGW32__) || defined(__MINGW64__)
                                const char *fmt_text, ...) __attribute__((format(__MINGW_PRINTF_FORMAT, 3, 4)));
#  else
                                const char *fmt_text, ...) __attribute__((format(printf, 3, 4)));
#  endif
#else
                                const char *fmt_text, ...);
#endif

#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
#  ifdef LOG_WRAPPER_FWAPI_USING_FORMAT_STRING
  template <class FMT, class... TARGS>
  LIBATFRAME_UTILS_API_HEAD_ONLY void format_log(const caller_info_t &caller, FMT &&fmt_text, TARGS &&...args) {
#  else
  template <class... TARGS>
  LIBATFRAME_UTILS_API_HEAD_ONLY void format_log(const caller_info_t &caller, TARGS &&...args) {
#  endif
    log_operation_t writer;
    start_log(caller, writer);
    if (!log_sinks_.empty()) {
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
      try {
#  endif
        LOG_WRAPPER_FWAPI_NAMESPACE format_to_n_result<char *> result =
#  ifdef LOG_WRAPPER_FWAPI_USING_FORMAT_STRING
            LOG_WRAPPER_FWAPI_NAMESPACE vformat_to_n<char *>(writer.buffer + writer.writen_size,
                                                             writer.total_size - writer.writen_size - 1,
                                                             std::forward<FMT>(fmt_text),
#    if defined(LIBATFRAME_UTILS_ENABLE_FMTLIB) && LIBATFRAME_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
        FMT_VERSION >= 100000
                                                             LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(args...)
#    else
                                                             LOG_WRAPPER_FWAPI_NAMESPACE make_format_args(
                                                                 std::forward<TARGS>(args)...)
#    endif
            );
#  else
          util::log::format_to_n<char *>(writer.buffer + writer.writen_size, writer.total_size - writer.writen_size - 1,
                                         std::forward<TARGS>(args)...);
#  endif
        if (result.size > 0) {
          writer.writen_size += static_cast<size_t>(result.size);
        }
        if (writer.writen_size < writer.total_size) {
          *(writer.buffer + writer.writen_size) = 0;
        } else {
          writer.writen_size = writer.total_size - 1;
          *(writer.buffer + writer.total_size - 1) = 0;
        }
#  if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
      } catch (const LOG_WRAPPER_FWAPI_NAMESPACE format_error &e) {
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
#endif

  // 一般日志级别检查
  UTIL_FORCEINLINE bool check_level(level_t::type level) const { return log_level_ >= level; }

  UTIL_FORCEINLINE static bool check_level(const log_wrapper *logger, level_t::type level) {
    if (nullptr == logger) {
      return false;
    }
    return logger->log_level_ >= level;
  }

  LIBATFRAME_UTILS_API size_t sink_size() const;

  /**
   * @brief 添加后端接口
   * @param h 输出函数
   * @param level_min 最低日志级别
   * @param level_min 最高日志级别
   */
  LIBATFRAME_UTILS_API void add_sink(log_handler_t h, level_t::type level_min = level_t::LOG_LW_FATAL,
                                     level_t::type level_max = level_t::LOG_LW_DEBUG);

  /**
   * @brief 移除最后一个后端接口
   */
  LIBATFRAME_UTILS_API void pop_sink();

  /**
   * @brief 设置后端接口的日志级别
   * @param idx 后端接口索引
   * @param level_min 最低日志级别
   * @param level_min 最高日志级别
   * @return 如果没找到则返回false，成功返回true
   */
  LIBATFRAME_UTILS_API bool set_sink(size_t idx, level_t::type level_min = level_t::LOG_LW_FATAL,
                                     level_t::type level_max = level_t::LOG_LW_DEBUG);

  /**
   * @brief 移除所有后端, std::function无法比较，所以只能全清
   */
  LIBATFRAME_UTILS_API void clear_sinks();

  UTIL_FORCEINLINE void set_level(level_t::type l) { log_level_ = l; }

  UTIL_FORCEINLINE level_t::type get_level() const { return log_level_; }

  UTIL_FORCEINLINE const std::string &get_prefix_format() const { return prefix_format_; }

  UTIL_FORCEINLINE void set_prefix_format(const std::string &prefix) { prefix_format_ = prefix; }

  UTIL_FORCEINLINE bool get_option(options_t::type t) const {
    if (t < options_t::OPT_MAX) {
      return false;
    }

    return options_.test(t);
  };

  UTIL_FORCEINLINE void set_option(options_t::type t, bool v) {
    if (t >= options_t::OPT_USER_MAX) {
      return;
    }

    options_.set(t, v);
  };

  LIBATFRAME_UTILS_API void set_stacktrace_level(level_t::type level_max = level_t::LOG_LW_DISABLED,
                                                 level_t::type level_min = level_t::LOG_LW_DISABLED);
  UTIL_FORCEINLINE const std::pair<level_t::type, level_t::type> &get_stacktrace_level() const {
    return stacktrace_level_;
  }

  /**
   * @brief 实际写出到落地接口
   */
  LIBATFRAME_UTILS_API void write_log(const caller_info_t &caller, const char *content, size_t content_size);

  // 白名单及用户指定日志输出可以针对哪个用户创建log_wrapper实例

  static LIBATFRAME_UTILS_API log_wrapper *mutable_log_cat(uint32_t cats = categorize_t::DEFAULT);
  static LIBATFRAME_UTILS_API ptr_t create_user_logger();

 private:
  LIBATFRAME_UTILS_API void start_log(const caller_info_t &caller, log_operation_t &);
  LIBATFRAME_UTILS_API void finish_log(const caller_info_t &caller, log_operation_t &);
  LIBATFRAME_UTILS_API void append_log(log_operation_t &, const char *str, size_t strsz);

 private:
  level_t::type log_level_;
  std::pair<level_t::type, level_t::type> stacktrace_level_;
  std::string prefix_format_;
  std::bitset<options_t::OPT_MAX> options_;
  std::list<log_router_t> log_sinks_;
  mutable LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_rw_lock log_sinks_lock_;
};
}  // namespace log
LIBATFRAME_UTILS_NAMESPACE_END

#define WLOG_LEVELID(lv) static_cast<LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::type>(lv)

#define WDTLOGGETCAT(cat) LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::mutable_log_cat(cat)
#define WDTLOGFILENF(lv, name) \
  LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::caller_info_t(lv, name, __FILE__, __LINE__, __FUNCTION__)

#define WLOG_INIT(cat, lv) nullptr != WDTLOGGETCAT(cat) ? WDTLOGGETCAT(cat)->init(lv) : -1

#define WLOG_GETCAT(cat) LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::mutable_log_cat(cat)

// 按分类日志输出工具
#ifdef _MSC_VER

/** 全局日志输出工具 - snprintf **/
#  define WCLOGDEFLV(lv, lv_name, cat, ...)                                                  \
    if (LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) \
      WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#  define WCLOGTRACE(cat, ...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, cat, __VA_ARGS__)
#  define WCLOGDEBUG(cat, ...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, cat, __VA_ARGS__)
#  define WCLOGNOTICE(cat, ...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, cat, __VA_ARGS__)
#  define WCLOGINFO(cat, ...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, cat, __VA_ARGS__)
#  define WCLOGWARNING(cat, ...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, cat, __VA_ARGS__)
#  define WCLOGERROR(cat, ...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, cat, __VA_ARGS__)
#  define WCLOGFATAL(cat, ...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, cat, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 - snprintf **/

#  define WINSTLOGDEFLV(lv, lv_name, __inst, ...) \
    if ((__inst).check_level(lv)) (__inst).log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#  define WINSTLOGTRACE(__inst, ...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __inst, __VA_ARGS__)
#  define WINSTLOGDEBUG(__inst, ...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __inst, __VA_ARGS__)
#  define WINSTLOGNOTICE(__inst, ...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __inst, __VA_ARGS__)
#  define WINSTLOGINFO(__inst, ...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __inst, __VA_ARGS__)
#  define WINSTLOGWARNING(__inst, ...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __inst, __VA_ARGS__)
#  define WINSTLOGERROR(__inst, ...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __inst, __VA_ARGS__)
#  define WINSTLOGFATAL(__inst, ...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __inst, __VA_ARGS__)

#  if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
/** 全局日志输出工具 - std::format **/
#    define FWCLOGDEFLV(lv, lv_name, cat, ...)                                                 \
      if (LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) \
        WDTLOGGETCAT(cat)->format_log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#    define FWCLOGTRACE(cat, ...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, cat, __VA_ARGS__)
#    define FWCLOGDEBUG(cat, ...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, cat, __VA_ARGS__)
#    define FWCLOGNOTICE(cat, ...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, cat, __VA_ARGS__)
#    define FWCLOGINFO(cat, ...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, cat, __VA_ARGS__)
#    define FWCLOGWARNING(cat, ...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, cat, __VA_ARGS__)
#    define FWCLOGERROR(cat, ...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, cat, __VA_ARGS__)
#    define FWCLOGFATAL(cat, ...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, cat, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 - std::format **/
#    define FWINSTLOGDEFLV(lv, lv_name, __inst, ...) \
      if ((__inst).check_level(lv)) (__inst).format_log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#    define FWINSTLOGTRACE(__inst, ...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGDEBUG(__inst, ...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGNOTICE(__inst, ...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGINFO(__inst, ...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGWARNING(__inst, ...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGERROR(__inst, ...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __inst, __VA_ARGS__)
#    define FWINSTLOGFATAL(__inst, ...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __inst, __VA_ARGS__)

#    define LOG_WRAPPER_FWAPI_FORMAT(...) LIBATFRAME_UTILS_NAMESPACE_ID::log::format(__VA_ARGS__)
#    define LOG_WRAPPER_FWAPI_FORMAT_TO(...) LIBATFRAME_UTILS_NAMESPACE_ID::log::format_to(__VA_ARGS__)
#    define LOG_WRAPPER_FWAPI_FORMAT_TO_N(...) LIBATFRAME_UTILS_NAMESPACE_ID::log::format_to_n(__VA_ARGS__)
#    define LOG_WRAPPER_FWAPI_VFORMAT(...) LIBATFRAME_UTILS_NAMESPACE_ID::log::vformat(__VA_ARGS__)
#    define LOG_WRAPPER_FWAPI_VFORMAT_TO(...) LIBATFRAME_UTILS_NAMESPACE_ID::log::vformat_to(__VA_ARGS__)
#    define LOG_WRAPPER_FWAPI_MAKE_FORMAT_ARGS(...) LIBATFRAME_UTILS_NAMESPACE_ID::log::make_format_args(__VA_ARGS__)
#  endif

#else

/** 全局日志输出工具 - snprintf **/
#  define WCLOGDEFLV(lv, lv_name, cat, args...)                                              \
    if (LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) \
      WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), ##args);

#  define WCLOGTRACE(...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __VA_ARGS__)
#  define WCLOGDEBUG(...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __VA_ARGS__)
#  define WCLOGNOTICE(...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __VA_ARGS__)
#  define WCLOGINFO(...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __VA_ARGS__)
#  define WCLOGWARNING(...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __VA_ARGS__)
#  define WCLOGERROR(...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __VA_ARGS__)
#  define WCLOGFATAL(...) \
    WCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 - snprintf **/
#  define WINSTLOGDEFLV(lv, lv_name, __inst, args...) \
    if ((__inst).check_level(lv)) (__inst).log(WDTLOGFILENF(lv, lv_name), ##args);

#  define WINSTLOGTRACE(...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __VA_ARGS__)
#  define WINSTLOGDEBUG(...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __VA_ARGS__)
#  define WINSTLOGNOTICE(...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __VA_ARGS__)
#  define WINSTLOGINFO(...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __VA_ARGS__)
#  define WINSTLOGWARNING(...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __VA_ARGS__)
#  define WINSTLOGERROR(...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __VA_ARGS__)
#  define WINSTLOGFATAL(...) \
    WINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __VA_ARGS__)

#  if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
/** 全局日志输出工具 - std::format **/
#    define FWCLOGDEFLV(lv, lv_name, cat, FMT, args...)                                        \
      if (LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) \
        WDTLOGGETCAT(cat)->format_log(WDTLOGFILENF(lv, lv_name), LOG_WRAPPER_FWAPI_FMT_STRING(FMT), ##args);

#    define FWCLOGTRACE(...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __VA_ARGS__)
#    define FWCLOGDEBUG(...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __VA_ARGS__)
#    define FWCLOGNOTICE(...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __VA_ARGS__)
#    define FWCLOGINFO(...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __VA_ARGS__)
#    define FWCLOGWARNING(...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __VA_ARGS__)
#    define FWCLOGERROR(...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __VA_ARGS__)
#    define FWCLOGFATAL(...) \
      FWCLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 - std::format **/
#    define FWINSTLOGDEFLV(lv, lv_name, __inst, FMT, args...) \
      if ((__inst).check_level(lv))                           \
        (__inst).format_log(WDTLOGFILENF(lv, lv_name), LOG_WRAPPER_FWAPI_FMT_STRING(FMT), ##args);

#    define FWINSTLOGTRACE(...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_TRACE, {}, __VA_ARGS__)
#    define FWINSTLOGDEBUG(...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_DEBUG, {}, __VA_ARGS__)
#    define FWINSTLOGNOTICE(...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_NOTICE, {}, __VA_ARGS__)
#    define FWINSTLOGINFO(...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_INFO, {}, __VA_ARGS__)
#    define FWINSTLOGWARNING(...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_WARNING, {}, __VA_ARGS__)
#    define FWINSTLOGERROR(...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_ERROR, {}, __VA_ARGS__)
#    define FWINSTLOGFATAL(...) \
      FWINSTLOGDEFLV(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::level_t::LOG_LW_FATAL, {}, __VA_ARGS__)

#    define LOG_WRAPPER_FWAPI_FORMAT(FMT, args...) \
      LIBATFRAME_UTILS_NAMESPACE_ID::log::format(LOG_WRAPPER_FWAPI_FMT_STRING(FMT), ##args)
#    define LOG_WRAPPER_FWAPI_FORMAT_TO(OUT, FMT, args...) \
      LIBATFRAME_UTILS_NAMESPACE_ID::log::format_to(OUT, LOG_WRAPPER_FWAPI_FMT_STRING(FMT), ##args)
#    define LOG_WRAPPER_FWAPI_FORMAT_TO_N(OUT, N, FMT, args...) \
      LIBATFRAME_UTILS_NAMESPACE_ID::log::format_to_n(OUT, N, LOG_WRAPPER_FWAPI_FMT_STRING(FMT), ##args)
#    define LOG_WRAPPER_FWAPI_VFORMAT(FMT, args) \
      LIBATFRAME_UTILS_NAMESPACE_ID::log::vformat(LOG_WRAPPER_FWAPI_FMT_STRING(FMT), ##args)
#    define LOG_WRAPPER_FWAPI_VFORMAT_TO(OUT, FMT, args...) \
      LIBATFRAME_UTILS_NAMESPACE_ID::log::vformat_to(OUT, LOG_WRAPPER_FWAPI_FMT_STRING(FMT), ##args)
#    define LOG_WRAPPER_FWAPI_MAKE_FORMAT_ARGS(...) LIBATFRAME_UTILS_NAMESPACE_ID::log::make_format_args(__VA_ARGS__)
#  endif

#endif

// 默认日志输出工具
#define WLOGTRACE(...) WCLOGTRACE(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGDEBUG(...) WCLOGDEBUG(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGNOTICE(...) WCLOGNOTICE(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGINFO(...) WCLOGINFO(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGWARNING(...) \
  WCLOGWARNING(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGERROR(...) WCLOGERROR(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGFATAL(...) WCLOGFATAL(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)

#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
#  define FWLOGTRACE(...) \
    FWCLOGTRACE(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGDEBUG(...) \
    FWCLOGDEBUG(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGNOTICE(...) \
    FWCLOGNOTICE(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGINFO(...) FWCLOGINFO(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGWARNING(...) \
    FWCLOGWARNING(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGERROR(...) \
    FWCLOGERROR(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#  define FWLOGFATAL(...) \
    FWCLOGFATAL(LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#endif

// 控制台输出工具
#ifdef _MSC_VER
#  define PSTDTERMCOLOR(os_ident, code, fmt_text, ...)                                                        \
                                                                                                              \
    {                                                                                                         \
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream::shell_stream_opr log_wrapper_pstd_ss(&std::os_ident); \
      log_wrapper_pstd_ss.open(code);                                                                         \
      log_wrapper_pstd_ss.close();                                                                            \
      printf(fmt_text, __VA_ARGS__);                                                                          \
    }

#else
#  define PSTDTERMCOLOR(os_ident, code, fmt_text, args...)                                                    \
                                                                                                              \
    {                                                                                                         \
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream::shell_stream_opr log_wrapper_pstd_ss(&std::os_ident); \
      log_wrapper_pstd_ss.open(code);                                                                         \
      log_wrapper_pstd_ss.close();                                                                            \
      printf(fmt_text, ##args);                                                                               \
    }

#endif

#define PSTDINFO(...) printf(__VA_ARGS__)
#define PSTDNOTICE(...) \
  PSTDTERMCOLOR(cout, LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW, __VA_ARGS__)
#define PSTDWARNING(...)                                                                                             \
  PSTDTERMCOLOR(cerr,                                                                                                \
                LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_SPEC_BOLD |                         \
                    static_cast<int>(LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW), \
                __VA_ARGS__)
#define PSTDERROR(...)                                                                                            \
  PSTDTERMCOLOR(cerr,                                                                                             \
                LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_SPEC_BOLD |                      \
                    static_cast<int>(LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED), \
                __VA_ARGS__)
#define PSTDFATAL(...) \
  PSTDTERMCOLOR(cerr, LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_MAGENTA, __VA_ARGS__)
#define PSTDOK(...) \
  PSTDTERMCOLOR(cout, LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_GREEN, __VA_ARGS__)
//
#ifndef NDEBUG
#  define PSTDTRACE(...) \
    PSTDTERMCOLOR(cout, LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_CYAN, __VA_ARGS__)
#  define PSTDDEBUG(...) \
    PSTDTERMCOLOR(cout, LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_CYAN, __VA_ARGS__)
#  define PSTDMARK                                                                                                  \
    PSTDTERMCOLOR(cout,                                                                                             \
                  LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_SPEC_BOLD |                      \
                      static_cast<int>(LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED), \
                  "Mark: %s:%s (function %s)\n", __FILE__, __LINE__, __FUNCTION__)
#else
#  define PSTDTRACE(...)
#  define PSTDDEBUG(...)
#  define PSTDMARK

#endif

#if defined(LOG_WRAPPER_ENABLE_FWAPI) && LOG_WRAPPER_ENABLE_FWAPI
LOG_WRAPPER_FWAPI_FORMAT_AS(typename LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::categorize_t::type, int);
LOG_WRAPPER_FWAPI_FORMAT_AS(typename LIBATFRAME_UTILS_NAMESPACE_ID::log::log_wrapper::options_t::type, int);
#endif

#include "config/compiler/template_suffix.h"
