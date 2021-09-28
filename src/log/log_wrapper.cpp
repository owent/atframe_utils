// Copyright 2021 atframework
// Licensed under the MIT licenses.
// Created by owent on 2015-06-29

#include <stdarg.h>
#include <cstdio>
#include <cstring>
#include "std/thread.h"

#include "common/string_oprs.h"
#include "config/atframe_utils_build_feature.h"
#include "lock/lock_holder.h"
#include "lock/spin_lock.h"

#include "time/time_utility.h"

#include "log/log_formatter.h"
#include "log/log_stacktrace.h"
#include "log/log_wrapper.h"

#if !(defined(THREAD_TLS_USE_PTHREAD) && THREAD_TLS_USE_PTHREAD) && defined(THREAD_TLS_ENABLED) && \
    1 == THREAD_TLS_ENABLED
namespace util {
namespace log {
namespace detail {
char *get_log_tls_buffer() {
  static THREAD_TLS char ret[LOG_WRAPPER_MAX_SIZE_PER_LINE];
  return ret;
}
}  // namespace detail
}  // namespace log
}  // namespace util
#else

#  include <pthread.h>
namespace util {
namespace log {
namespace detail {
static pthread_once_t gt_get_log_tls_once = PTHREAD_ONCE_INIT;
static pthread_key_t gt_get_log_tls_key;

static void dtor_pthread_get_log_tls(void *p) {
  char *buffer_block = reinterpret_cast<char *>(p);
  if (nullptr != buffer_block) {
    delete[] buffer_block;
  }
}

static void init_pthread_get_log_tls() { (void)pthread_key_create(&gt_get_log_tls_key, dtor_pthread_get_log_tls); }

char *get_log_tls_buffer() {
  (void)pthread_once(&gt_get_log_tls_once, init_pthread_get_log_tls);
  char *buffer_block = reinterpret_cast<char *>(pthread_getspecific(gt_get_log_tls_key));
  if (nullptr == buffer_block) {
    buffer_block = new char[LOG_WRAPPER_MAX_SIZE_PER_LINE];
    pthread_setspecific(gt_get_log_tls_key, buffer_block);
  }
  return buffer_block;
}

struct gt_get_log_tls_buffer_main_thread_dtor_t {
  char *buffer_ptr;
  gt_get_log_tls_buffer_main_thread_dtor_t() { buffer_ptr = get_log_tls_buffer(); }

  ~gt_get_log_tls_buffer_main_thread_dtor_t() {
    pthread_setspecific(gt_get_log_tls_key, nullptr);
    dtor_pthread_get_log_tls(buffer_ptr);
  }
};
static gt_get_log_tls_buffer_main_thread_dtor_t gt_get_log_tls_buffer_main_thread_dtor;
}  // namespace detail
}  // namespace log
}  // namespace util

#endif

namespace util {
namespace log {
namespace detail {
static bool log_wrapper_global_destroyed_ = false;
}

LIBATFRAME_UTILS_API log_wrapper::log_wrapper()
    : log_level_(level_t::LOG_LW_DISABLED), stacktrace_level_(level_t::LOG_LW_DISABLED, level_t::LOG_LW_DISABLED) {
  // 默认设为全局logger，如果是用户logger，则create_user_logger里重新设为false
  options_.set(options_t::OPT_IS_GLOBAL, true);

  update();
  prefix_format_ = "[Log %L][%F %T.%f][%s:%n(%C)]: ";
}

LIBATFRAME_UTILS_API log_wrapper::log_wrapper(construct_helper_t &)
    : log_level_(level_t::LOG_LW_DISABLED), stacktrace_level_(level_t::LOG_LW_DISABLED, level_t::LOG_LW_DISABLED) {
  // 这个接口由create_user_logger调用，不设置OPT_IS_GLOBAL
  prefix_format_ = "[Log %L][%F %T.%f][%s:%n(%C)]: ";
}

LIBATFRAME_UTILS_API log_wrapper::~log_wrapper() {
  if (get_option(options_t::OPT_IS_GLOBAL)) {
    detail::log_wrapper_global_destroyed_ = true;
  }

  // 重置level，只要内存没释放，就还可以内存访问，但是不能写出日志
  log_level_ = level_t::LOG_LW_DISABLED;
}

LIBATFRAME_UTILS_API int32_t log_wrapper::init(level_t::type level) {
  log_level_ = level;

  return 0;
}

LIBATFRAME_UTILS_API size_t log_wrapper::sink_size() const {
  util::lock::read_lock_holder<util::lock::spin_rw_lock> holder(log_sinks_lock_);

  return log_sinks_.size();
}

LIBATFRAME_UTILS_API void log_wrapper::add_sink(log_handler_t h, level_t::type level_min, level_t::type level_max) {
  if (h) {
    log_router_t router;
    router.handle = h;
    router.level_min = level_min;
    router.level_max = level_max;

    util::lock::write_lock_holder<util::lock::spin_rw_lock> holder(log_sinks_lock_);
    log_sinks_.push_back(router);
  };
}

LIBATFRAME_UTILS_API void log_wrapper::pop_sink() {
  util::lock::write_lock_holder<util::lock::spin_rw_lock> holder(log_sinks_lock_);

  if (log_sinks_.empty()) {
    return;
  }

  log_sinks_.pop_front();
}

LIBATFRAME_UTILS_API bool log_wrapper::set_sink(size_t idx, level_t::type level_min, level_t::type level_max) {
  util::lock::write_lock_holder<util::lock::spin_rw_lock> holder(log_sinks_lock_);

  if (log_sinks_.size() <= idx) {
    return false;
  }

  std::list<log_router_t>::iterator beg = log_sinks_.begin();
  if (idx > 0) {
    std::advance(beg, idx);
  }

  (*beg).level_min = level_min;
  (*beg).level_max = level_max;
  return true;
}

LIBATFRAME_UTILS_API void log_wrapper::clear_sinks() {
  util::lock::write_lock_holder<util::lock::spin_rw_lock> holder(log_sinks_lock_);
  log_sinks_.clear();
}

LIBATFRAME_UTILS_API void log_wrapper::set_stacktrace_level(level_t::type level_max, level_t::type level_min) {
  stacktrace_level_.first = level_min;
  stacktrace_level_.second = level_max;

  // make sure first <= second
  if (stacktrace_level_.second < stacktrace_level_.first) {
    level_t::type tmp = stacktrace_level_.second;
    stacktrace_level_.second = stacktrace_level_.first;
    stacktrace_level_.first = tmp;
  }
}

LIBATFRAME_UTILS_API void log_wrapper::update() { util::time::time_utility::update(); }

LIBATFRAME_UTILS_API void log_wrapper::log(const caller_info_t &caller,
#ifdef _MSC_VER
                                           _In_z_ _Printf_format_string_ const char *fmt, ...
#else
                                           const char *fmt, ...
#endif
) {
  log_operation_t writer;
  writer.buffer = nullptr;
  writer.total_size = 0;
  writer.writen_size = 0;

  start_log(caller, writer);

  if (!log_sinks_.empty() && nullptr != writer.buffer) {
    va_list va_args;
    va_start(va_args, fmt);
    int prt_res = UTIL_STRFUNC_VSNPRINTF(writer.buffer + writer.writen_size, writer.total_size - writer.writen_size - 1,
                                         fmt, va_args);
    va_end(va_args);
    if (prt_res >= 0) {
      writer.writen_size += static_cast<size_t>(prt_res);
    }

    if (writer.writen_size < writer.total_size) {
      *(writer.buffer + writer.writen_size) = 0;
    } else {
      writer.writen_size = writer.total_size - 1;
      *(writer.buffer + writer.total_size - 1) = 0;
    }
  }
  finish_log(caller, writer);
}

LIBATFRAME_UTILS_API void log_wrapper::write_log(const caller_info_t &caller, const char *content,
                                                 size_t content_size) {
  util::lock::read_lock_holder<util::lock::spin_rw_lock> holder(log_sinks_lock_);

  for (std::list<log_router_t>::iterator iter = log_sinks_.begin(); iter != log_sinks_.end(); ++iter) {
    if (caller.level_id >= iter->level_min && caller.level_id <= iter->level_max) {
      iter->handle(caller, content, content_size);
    }
  }
}

LIBATFRAME_UTILS_API log_wrapper *log_wrapper::mutable_log_cat(uint32_t cats) {
  if (detail::log_wrapper_global_destroyed_) {
    return nullptr;
  }

  static log_wrapper all_logger[categorize_t::MAX];

  if (cats >= categorize_t::MAX) {
    return nullptr;
  }

  return &all_logger[cats];
}

LIBATFRAME_UTILS_API log_wrapper::ptr_t log_wrapper::create_user_logger() {
  construct_helper_t h;
  ptr_t ret = std::make_shared<log_wrapper>(h);
  if (ret) {
    ret->options_.set(options_t::OPT_IS_GLOBAL, false);
  }

  return ret;
}

LIBATFRAME_UTILS_API void log_wrapper::start_log(const caller_info_t &caller, log_operation_t &writer) {
  if (get_option(options_t::OPT_AUTO_UPDATE_TIME) && !prefix_format_.empty()) {
    update();
  }
  if (log_sinks_.empty()) {
    return;
  }

  writer.buffer = detail::get_log_tls_buffer();
  writer.total_size = LOG_WRAPPER_MAX_SIZE_PER_LINE;
  writer.writen_size = 0;

  // format => "[Log    DEBUG][2015-01-12 10:09:08.]
  writer.writen_size =
      log_formatter::format(writer.buffer, writer.total_size, prefix_format_.c_str(), prefix_format_.size(), caller);
}

LIBATFRAME_UTILS_API void log_wrapper::finish_log(const caller_info_t &caller, log_operation_t &writer) {
  if (log_sinks_.empty() || nullptr == writer.buffer) {
    return;
  }

  if (is_stacktrace_enabled() && caller.level_id >= stacktrace_level_.first &&
      caller.level_id <= stacktrace_level_.second) {
    append_log(writer, "\r\nCall stacks:\r\n", 0);
    if (writer.writen_size + 1 < writer.total_size) {
      stacktrace_options options;
      options.skip_start_frames = 1;
      options.skip_end_frames = 0;
      options.max_frames = 0;
      size_t stacktrace_len =
          stacktrace_write(writer.buffer + writer.writen_size, writer.total_size - writer.writen_size - 1, &options);
      if (stacktrace_len > 0) {
        writer.writen_size += stacktrace_len;
      }
    }

    if (writer.writen_size < writer.total_size) {
      *(writer.buffer + writer.writen_size) = 0;
    } else {
      writer.writen_size = writer.total_size - 1;
      *(writer.buffer + writer.total_size - 1) = 0;
    }
  }

  write_log(caller, writer.buffer, writer.writen_size);
}

LIBATFRAME_UTILS_API void log_wrapper::append_log(log_operation_t &writer, const char *str, size_t strsz) {
  if (!writer.buffer || writer.writen_size + 1 >= writer.total_size) {
    return;
  }

  if (!str) {
    return;
  }

  if (0 == strsz) {
    strsz = strlen(str);
  }

  if (strsz + writer.writen_size + 1 >= writer.total_size) {
    memcpy(writer.buffer + writer.writen_size, str, writer.total_size - writer.writen_size - 1);
    writer.writen_size = writer.total_size - 1;
    *(writer.buffer + writer.writen_size) = 0;
  } else {
    memcpy(writer.buffer + writer.writen_size, str, strsz);
    writer.writen_size += strsz;
    *(writer.buffer + writer.writen_size) = 0;
  }
}

}  // namespace log
}  // namespace util
