// Copyright 2026 atframework
//
// Licensed under the MIT licenses.
// Created by owent on 2015-06-29

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>

#include "common/string_oprs.h"
#include "nostd/type_traits.h"
#include "std/thread.h"

#include "time/time_utility.h"

#include "log/log_formatter.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace log {
namespace {
ATFW_UTIL_SANITIZER_NO_THREAD static nostd::string_view log_formatter_get_level_name(log_level l) {
  if (l <= log_level::kTrace) {
    return "TRACE";
  }
  if (l <= log_level::kDebug) {
    return "DEBUG";
  }
  if (l <= log_level::kNotice) {
    return "NOTICE";
  }
  if (l <= log_level::kInfo) {
    return "INFO";
  }
  if (l <= log_level::kWarning) {
    return "WARN";
  }
  if (l <= log_level::kError) {
    return "ERROR";
  }

  if (l >= log_level::kDisabled) {
    return "DISABLED";
  }

  return "FATAL";
}

inline static std::pair<struct tm, time::time_utility::raw_time_t> &log_formatter_get_timepoint() {
  static THREAD_TLS time_t tm_update_tp = 0;
  static THREAD_TLS std::pair<struct tm, time::time_utility::raw_time_t> tm_obj{};

  tm_obj.second = time::time_utility::sys_now_realtime();
  time_t tm_realtime_tp = std::chrono::system_clock::to_time_t(tm_obj.second);

  if (tm_update_tp != tm_realtime_tp) {
    tm_update_tp = tm_realtime_tp;
    UTIL_STRFUNC_LOCALTIME_S(&tm_realtime_tp, &tm_obj.first);  // lgtm [cpp/potentially-dangerous-function]
  }

  return tm_obj;
}

inline static void log_formatter_write_six_digits(char *output, uint32_t value) noexcept {
  const uint32_t first_pair = value / 10000;
  value %= 10000;
  const uint32_t second_pair = value / 100;
  const uint32_t third_pair = value % 100;

  output[0] = static_cast<char>((first_pair / 10) + '0');
  output[1] = static_cast<char>((first_pair % 10) + '0');
  output[2] = static_cast<char>((second_pair / 10) + '0');
  output[3] = static_cast<char>((second_pair % 10) + '0');
  output[4] = static_cast<char>((third_pair / 10) + '0');
  output[5] = static_cast<char>((third_pair % 10) + '0');
}
}  // namespace

std::string log_formatter::project_dir_;

ATFRAMEWORK_UTILS_API bool log_formatter::check_flag(int32_t flags, int32_t checked) {
  return (flags & checked) == checked;
}

ATFRAMEWORK_UTILS_API struct tm *log_formatter::get_iso_tm() { return &log_formatter_get_timepoint().first; }

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
ATFRAMEWORK_UTILS_API size_t log_formatter::format(char *buff, size_t bufz, const char *fmt, size_t fmtz,
                                                   const caller_info_t &caller) {
  if (nullptr == buff || 0 == bufz) {
    return 0;
  }

  if (nullptr == fmt || 0 == fmtz) {
    buff[0] = '\0';
    return 0;
  }

  // Level id to level name
  nostd::string_view level_name = caller.level_name;
  if (level_name.empty()) {
    level_name = log_formatter_get_level_name(caller.level_id);
  }

  bool need_parse = false, running = true;
  size_t ret = 0;
  struct std::pair<struct tm, time::time_utility::raw_time_t> *tm_obj_ptr = nullptr;

// 时间加缓存，以防使用过程中时间变化
// NOLINTBEGIN(bugprone-macro-parentheses)
#define LOG_FMT_FN_TM_MEM(VAR, EXPRESS)          \
                                                 \
  int VAR;                                       \
                                                 \
  if (nullptr == tm_obj_ptr) {                   \
    tm_obj_ptr = &log_formatter_get_timepoint(); \
  }                                              \
                                                 \
  VAR = tm_obj_ptr->first.EXPRESS;
  // NOLINTEND(bugprone-macro-parentheses)

  for (size_t i = 0; i < fmtz && ret < bufz && running; ++i) {
    if (!need_parse) {
      if ('%' == fmt[i]) {
        need_parse = true;
      } else {
        buff[ret++] = fmt[i];
      }
      continue;
    }

    need_parse = false;
    // 简化版本的 strftime 格式支持
    // @see https://en.cppreference.com/w/cpp/chrono/c/strftime
    // 额外支持毫秒，rotate index, log level 名称等
    switch (fmt[i]) {
      // =================== datetime ===================
      case 'Y': {
        if (bufz - ret < 4) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(year, tm_year + 1900);
          buff[ret++] = static_cast<char>((year / 1000) + '0');
          buff[ret++] = static_cast<char>(((year / 100) % 10) + '0');
          buff[ret++] = static_cast<char>(((year / 10) % 10) + '0');
          buff[ret++] = static_cast<char>((year % 10) + '0');
        }
        break;
      }
      case 'y': {
        if (bufz - ret < 2) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(year, tm_year + 1900);
          buff[ret++] = static_cast<char>(((year / 10) % 10) + '0');
          buff[ret++] = static_cast<char>((year % 10) + '0');
        }
        break;
      }
      case 'm': {
        if (bufz - ret < 2) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(mon, tm_mon + 1);
          buff[ret++] = static_cast<char>((mon / 10) + '0');
          buff[ret++] = static_cast<char>((mon % 10) + '0');
        }
        break;
      }
      case 'j': {
        if (bufz - ret < 3) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(yday, tm_yday);
          buff[ret++] = static_cast<char>((yday / 100) + '0');
          buff[ret++] = static_cast<char>(((yday / 10) % 10) + '0');
          buff[ret++] = static_cast<char>((yday % 10) + '0');
        }
        break;
      }
      case 'd': {
        if (bufz - ret < 2) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(mday, tm_mday);
          buff[ret++] = static_cast<char>((mday / 10) + '0');
          buff[ret++] = static_cast<char>((mday % 10) + '0');
        }
        break;
      }
      case 'w': {
        LOG_FMT_FN_TM_MEM(wday, tm_wday);
        buff[ret++] = static_cast<char>(wday + '0');
        break;
      }
      case 'H': {
        if (bufz - ret < 2) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(hour, tm_hour);
          buff[ret++] = static_cast<char>((hour / 10) + '0');
          buff[ret++] = static_cast<char>((hour % 10) + '0');
        }
        break;
      }
      case 'I': {
        if (bufz - ret < 2) {
          running = false;
        } else {
          // NOLINTNEXTLINE(readability-math-missing-parentheses)
          LOG_FMT_FN_TM_MEM(hour, tm_hour % 12 + 1);
          buff[ret++] = static_cast<char>((hour / 10) + '0');
          buff[ret++] = static_cast<char>((hour % 10) + '0');
        }
        break;
      }
      case 'M': {
        if (bufz - ret < 2) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(minite, tm_min);
          buff[ret++] = static_cast<char>((minite / 10) + '0');
          buff[ret++] = static_cast<char>((minite % 10) + '0');
        }
        break;
      }
      case 'S': {
        if (bufz - ret < 2) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(sec, tm_sec);
          buff[ret++] = static_cast<char>((sec / 10) + '0');
          buff[ret++] = static_cast<char>((sec % 10) + '0');
        }
        break;
      }
      case 'F': {
        if (bufz - ret < 10) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(year, tm_year + 1900);
          LOG_FMT_FN_TM_MEM(mon, tm_mon + 1);
          LOG_FMT_FN_TM_MEM(mday, tm_mday);
          buff[ret++] = static_cast<char>((year / 1000) + '0');
          buff[ret++] = static_cast<char>(((year / 100) % 10) + '0');
          buff[ret++] = static_cast<char>(((year / 10) % 10) + '0');
          buff[ret++] = static_cast<char>((year % 10) + '0');
          buff[ret++] = '-';
          buff[ret++] = static_cast<char>((mon / 10) + '0');
          buff[ret++] = static_cast<char>((mon % 10) + '0');
          buff[ret++] = '-';
          buff[ret++] = static_cast<char>((mday / 10) + '0');
          buff[ret++] = static_cast<char>((mday % 10) + '0');
        }
        break;
      }
      case 'T': {
        if (bufz - ret < 8) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(hour, tm_hour);
          LOG_FMT_FN_TM_MEM(minite, tm_min);
          LOG_FMT_FN_TM_MEM(sec, tm_sec);
          buff[ret++] = static_cast<char>((hour / 10) + '0');
          buff[ret++] = static_cast<char>((hour % 10) + '0');
          buff[ret++] = ':';
          buff[ret++] = static_cast<char>((minite / 10) + '0');
          buff[ret++] = static_cast<char>((minite % 10) + '0');
          buff[ret++] = ':';
          buff[ret++] = static_cast<char>((sec / 10) + '0');
          buff[ret++] = static_cast<char>((sec % 10) + '0');
        }
        break;
      }
      case 'R': {
        if (bufz - ret < 5) {
          running = false;
        } else {
          LOG_FMT_FN_TM_MEM(hour, tm_hour);
          LOG_FMT_FN_TM_MEM(minite, tm_min);
          buff[ret++] = static_cast<char>((hour / 10) + '0');
          buff[ret++] = static_cast<char>((hour % 10) + '0');
          buff[ret++] = ':';
          buff[ret++] = static_cast<char>((minite / 10) + '0');
          buff[ret++] = static_cast<char>((minite % 10) + '0');
        }
        break;
      }

      case 'f': {
        if (bufz - ret < 1) {
          running = false;
        } else {
          if (nullptr == tm_obj_ptr) {
            tm_obj_ptr = &log_formatter_get_timepoint();
          }

          auto realtime_now_usec =
              std::chrono::duration_cast<std::chrono::microseconds>(tm_obj_ptr->second.time_since_epoch()).count() %
              1000000;
          if (realtime_now_usec < 0) {
            realtime_now_usec += 1000000;
          }

          constexpr size_t MICROSECOND_TEXT_SIZE = 6;
          if (bufz - ret >= MICROSECOND_TEXT_SIZE) {
            log_formatter_write_six_digits(&buff[ret], static_cast<uint32_t>(realtime_now_usec));
            ret += MICROSECOND_TEXT_SIZE;
          } else {
            char microsecond_text[MICROSECOND_TEXT_SIZE];
            log_formatter_write_six_digits(microsecond_text, static_cast<uint32_t>(realtime_now_usec));
            const size_t write_size = bufz - ret;
            memcpy(&buff[ret], microsecond_text, write_size);
            ret += write_size;
          }
        }

        break;
      }

      // =================== caller data ===================
      case 'L': {
        if (!level_name.empty()) {
          size_t write_size = 8;
          if (level_name.size() < write_size) {
            write_size = level_name.size();
          }
          if (bufz - ret <= 8) {
            running = false;
          } else {
            memcpy(&buff[ret], level_name.data(), write_size);
            for (size_t j = write_size; j < 8; ++j) {
              buff[ret + j] = ' ';
            }
            ret += 8;
          }
        }
        break;
      }
      case 'l': {
        int res = UTIL_STRFUNC_SNPRINTF(&buff[ret], bufz - ret, "%d", static_cast<int>(caller.level_id));
        if (res < 0) {
          running = false;
        } else {
          ret += static_cast<size_t>(res);
        }
        break;
      }
      case 's': {
        if (!caller.file_path.empty()) {
          nostd::string_view file_path = caller.file_path;
          size_t strip_position = 0;
          if (!project_dir_.empty()) {
            for (size_t j = 0; j < project_dir_.size() && j < file_path.size(); ++j) {
              if (project_dir_[j] == file_path[j]) {
                strip_position = j + 1;
              } else {
                break;
              }
            }

            if (strip_position > 0) {
              file_path = file_path.substr(strip_position);
              buff[ret++] = '~';
            }
          }

          if (bufz - ret <= file_path.size()) {
            running = false;
          } else {
            memcpy(&buff[ret], file_path.data(), file_path.size());
            ret += file_path.size();
          }
        }
        break;
      }
      case 'k': {
        if (!caller.file_path.empty()) {
          nostd::string_view file_name = caller.file_path;
          for (size_t j = 0; j < caller.file_path.size(); ++j) {
            if ('/' == caller.file_path[j] || '\\' == caller.file_path[j]) {
              file_name = caller.file_path.substr(j + 1);
            }
          }
          if (bufz - ret <= file_name.size()) {
            running = false;
          } else {
            memcpy(&buff[ret], file_name.data(), file_name.size());
            ret += file_name.size();
          }
        }
        break;
      }
      case 'n': {
        int res = UTIL_STRFUNC_SNPRINTF(&buff[ret], bufz - ret, "%u", caller.line_number);
        if (res < 0) {
          running = false;
        } else {
          ret += static_cast<size_t>(res);
        }
        break;
      }
      case 'C': {
        if (!caller.func_name.empty()) {
          if (bufz - ret <= caller.func_name.size()) {
            running = false;
          } else {
            memcpy(&buff[ret], caller.func_name.data(), caller.func_name.size());
            ret += caller.func_name.size();
          }
        }
        break;
      }
      // =================== rotate index ===================
      case 'N': {
        int res = UTIL_STRFUNC_SNPRINTF(&buff[ret], bufz - ret, "%u", caller.rotate_index);
        if (res < 0) {
          running = false;
        } else {
          ret += static_cast<size_t>(res);
        }
        break;
      }

      // =================== unknown ===================
      default: {
        buff[ret++] = fmt[i];
        break;
      }
    }
  }

#undef LOG_FMT_FN_TM_MEM

  if (ret < bufz) {
    buff[ret] = '\0';
  } else {
    buff[bufz - 1] = '\0';
  }
  return ret;
}

ATFRAMEWORK_UTILS_API bool log_formatter::check_rotation_var(const char *fmt, size_t fmtz) {
  for (size_t i = 0; fmt != nullptr && i < fmtz - 1; ++i) {
    if ('%' == fmt[i] && 'N' == fmt[i + 1]) {
      return true;
    }
  }

  return false;
}

ATFRAMEWORK_UTILS_API bool log_formatter::has_format(const char *fmt, size_t fmtz) {
  for (size_t i = 0; fmt != nullptr && i < fmtz - 1; ++i) {
    if ('%' == fmt[i]) {
      return true;
    }
  }

  return false;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void log_formatter::set_project_directory(const char *dirbuf,
                                                                                              size_t dirsz) {
  if (nullptr == dirbuf) {
    project_dir_.clear();
  } else if (dirsz <= 0) {
    project_dir_ = dirbuf;
  } else {
    project_dir_.assign(dirbuf, dirsz);
  }
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void log_formatter::set_project_directory(nostd::string_view dir) {
  set_project_directory(dir.data(), dir.size());
}

ATFRAMEWORK_UTILS_API log_level log_formatter::get_level_by_name(nostd::string_view name) {
  if (name.empty()) {
    return log_level::kDebug;
  }

  // number, directly convert it
  if (name[0] == '\\' || (name[0] >= '0' && name[0] <= '9')) {
    nostd::underlying_type_t<log_level> l =
        ATFRAMEWORK_UTILS_NAMESPACE_ID::string::to_int<nostd::underlying_type_t<log_level>>(name);
    if (l >= 0 && l <= static_cast<nostd::underlying_type_t<log_level>>(log_level::kFatal)) {
      return static_cast<log_level>(l);
    }

    return log_level::kDebug;
  }

  // name and convert into level

  if (0 == UTIL_STRFUNC_STRNCASE_CMP("disable", name.data(), name.size())) {
    return log_level::kDisabled;
  }

  if (0 == UTIL_STRFUNC_STRNCASE_CMP("fatal", name.data(), name.size())) {
    return log_level::kFatal;
  }

  if (0 == UTIL_STRFUNC_STRNCASE_CMP("error", name.data(), name.size())) {
    return log_level::kError;
  }

  if (0 == UTIL_STRFUNC_STRNCASE_CMP("warn", name.data(), name.size())) {
    return log_level::kWarning;
  }

  if (0 == UTIL_STRFUNC_STRNCASE_CMP("warning", name.data(), name.size())) {
    return log_level::kWarning;
  }

  if (0 == UTIL_STRFUNC_STRNCASE_CMP("info", name.data(), name.size())) {
    return log_level::kInfo;
  }

  if (0 == UTIL_STRFUNC_STRNCASE_CMP("notice", name.data(), name.size())) {
    return log_level::kNotice;
  }

  if (0 == UTIL_STRFUNC_STRNCASE_CMP("debug", name.data(), name.size())) {
    return log_level::kDebug;
  }

  if (0 == UTIL_STRFUNC_STRNCASE_CMP("trace", name.data(), name.size())) {
    return log_level::kTrace;
  }

  return log_level::kDebug;
}

}  // namespace log
ATFRAMEWORK_UTILS_NAMESPACE_END
