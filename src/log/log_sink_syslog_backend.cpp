// Copyright 2021 atframework
// Licensed under the MIT licenses.
// Created by owent on 2021-09-28

#include "log/log_sink_syslog_backend.h"

#if defined(LOG_SINK_ENABLE_SYSLOG_SUPPORT) && LOG_SINK_ENABLE_SYSLOG_SUPPORT

#  include <syslog.h>
#  include <cstring>
#  include <memory>

#  include "design_pattern/nomovable.h"
#  include "design_pattern/noncopyable.h"

#  include "lock/lock_holder.h"
#  include "lock/spin_rw_lock.h"

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace log {

class log_sink_syslog_backend_handle {
  UTIL_DESIGN_PATTERN_NOCOPYABLE(log_sink_syslog_backend_handle)
  UTIL_DESIGN_PATTERN_NOMOVABLE(log_sink_syslog_backend_handle)

 public:
  log_sink_syslog_backend_handle(const char *ident, int option, int facility) : option_(option), facility_(facility) {
    if (nullptr != ident) {
      ident_ = ident;
    }

    openlog(ident_.empty() ? nullptr : ident_.c_str(), option_, facility_);
  }
  ~log_sink_syslog_backend_handle() { closelog(); }

  static std::shared_ptr<log_sink_syslog_backend_handle> mutable_instance(const char *ident, int option, int facility) {
    static std::shared_ptr<log_sink_syslog_backend_handle> syslog_handle;
    static LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_rw_lock syslog_lock;

    {
      LIBATFRAME_UTILS_NAMESPACE_ID::lock::read_lock_holder<LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_rw_lock> guard{
          syslog_lock};
      if (syslog_handle) {
        return syslog_handle;
      }
    }

    LIBATFRAME_UTILS_NAMESPACE_ID::lock::write_lock_holder<LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_rw_lock> guard{
        syslog_lock};
    syslog_handle = std::make_shared<log_sink_syslog_backend_handle>(ident, option, facility);
    return syslog_handle;
  };

 private:
  std::string ident_;
  int option_;
  int facility_;
};

LIBATFRAME_UTILS_API log_sink_syslog_backend::log_sink_syslog_backend(const char *ident)
    : option_(LOG_ODELAY | LOG_PID), facility_(LOG_USER) {
  if (nullptr != ident) {
    ident_ = ident;
  }
}

LIBATFRAME_UTILS_API log_sink_syslog_backend::log_sink_syslog_backend(const char *ident, int option)
    : option_(option), facility_(LOG_USER) {
  if (nullptr != ident) {
    ident_ = ident;
  }
}

LIBATFRAME_UTILS_API log_sink_syslog_backend::log_sink_syslog_backend(const char *ident, int option, int facility)
    : option_(option), facility_(facility) {
  if (nullptr != ident) {
    ident_ = ident;
  }
}

LIBATFRAME_UTILS_API log_sink_syslog_backend::~log_sink_syslog_backend() {}

LIBATFRAME_UTILS_API void log_sink_syslog_backend::operator()(const log_formatter::caller_info_t &caller,
                                                              const char *content, size_t /*content_size*/) {
  if (!handle_) {
    handle_ =
        log_sink_syslog_backend_handle::mutable_instance(ident_.empty() ? nullptr : ident_.c_str(), option_, facility_);
  }

  switch (caller.level_id) {
    // case log_formatter::level_t::LOG_LW_DISABLED:
    //   break;
    case log_formatter::level_t::LOG_LW_FATAL:
      syslog(facility_ | LOG_ALERT, "%s", content);
      break;
    case log_formatter::level_t::LOG_LW_ERROR:
      syslog(facility_ | LOG_ERR, "%s", content);
      break;
    case log_formatter::level_t::LOG_LW_WARNING:
      syslog(facility_ | LOG_WARNING, "%s", content);
      break;
    case log_formatter::level_t::LOG_LW_INFO:
      syslog(facility_ | LOG_INFO, "%s", content);
      break;
    case log_formatter::level_t::LOG_LW_NOTICE:
      syslog(facility_ | LOG_NOTICE, "%s", content);
      break;
    case log_formatter::level_t::LOG_LW_DEBUG:
      syslog(facility_ | LOG_DEBUG, "%s", content);
      break;
    case log_formatter::level_t::LOG_LW_TRACE:
      syslog(facility_ | LOG_DEBUG, "%s", content);
      break;
    default:
      break;
  }
}

}  // namespace log
LIBATFRAME_UTILS_NAMESPACE_END

#endif
