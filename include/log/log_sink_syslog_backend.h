// Copyright 2021 atframework
// Licensed under the MIT licenses.
// Created by owent on 2021-09-28

#ifndef UTIL_LOG_LOG_SINK_SYSLOG_BACKEND_H
#define UTIL_LOG_LOG_SINK_SYSLOG_BACKEND_H

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <memory>
#include <string>

#include "log_formatter.h"

#if defined(LOG_SINK_ENABLE_SYSLOG_SUPPORT) && LOG_SINK_ENABLE_SYSLOG_SUPPORT

namespace util {
namespace log {

class log_sink_syslog_backend_handle;

/**
 * @brief syslog sink
 * @note Use journalctl -e -t <ident> to see the latest log writted by this sink
 */
class log_sink_syslog_backend {
 public:
  // @see https://www.man7.org/linux/man-pages/man3/syslog.3.html for details
  LIBATFRAME_UTILS_API log_sink_syslog_backend(const char *ident);
  LIBATFRAME_UTILS_API log_sink_syslog_backend(const char *ident, int option);
  LIBATFRAME_UTILS_API log_sink_syslog_backend(const char *ident, int option, int facility);
  LIBATFRAME_UTILS_API ~log_sink_syslog_backend();

 public:
  LIBATFRAME_UTILS_API void operator()(const log_formatter::caller_info_t &caller, const char *content,
                                       size_t content_size);

 private:
  std::shared_ptr<log_sink_syslog_backend_handle> handle_;
  std::string ident_;
  int option_;
  int facility_;
};
}  // namespace log
}  // namespace util

#endif

#endif
