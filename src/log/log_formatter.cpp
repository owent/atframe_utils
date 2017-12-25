#include <cstdio>
#include <cstring>

#include "common/string_oprs.h"

#include "time/time_utility.h"

#include "log/log_formatter.h"

namespace util {
    namespace log {
        namespace detail {
            static const char *log_formatter_get_level_name(int l) {
                static const char *all_level_name[log_formatter::level_t::LOG_LW_DEBUG + 1] = {NULL};

                if (l > log_formatter::level_t::LOG_LW_DEBUG || l < 0) {
                    return "Unknown";
                }

                if (NULL == all_level_name[log_formatter::level_t::LOG_LW_DEBUG]) {
                    all_level_name[log_formatter::level_t::LOG_LW_DISABLED] = "Disabled";
                    all_level_name[log_formatter::level_t::LOG_LW_FATAL] = "Fatal";
                    all_level_name[log_formatter::level_t::LOG_LW_ERROR] = "Error";
                    all_level_name[log_formatter::level_t::LOG_LW_WARNING] = "Warn";
                    all_level_name[log_formatter::level_t::LOG_LW_INFO] = "Info";
                    all_level_name[log_formatter::level_t::LOG_LW_NOTICE] = "Notice";
                    all_level_name[log_formatter::level_t::LOG_LW_DEBUG] = "Debug";
                }
                return all_level_name[l];
            }
        } // namespace detail

        log_formatter::caller_info_t::caller_info_t()
            : level_id(level_t::LOG_LW_DISABLED), level_name(NULL), file_path(NULL), line_number(0), func_name(NULL), rotate_index(0) {
            if (NULL == level_name) {
                level_name = detail::log_formatter_get_level_name(level_id);
            }
        }
        log_formatter::caller_info_t::caller_info_t(level_t::type lid, const char *lname, const char *fpath, uint32_t lnum,
                                                    const char *fnname)
            : level_id(lid), level_name(lname), file_path(fpath), line_number(lnum), func_name(fnname), rotate_index(0) {
            if (NULL == level_name) {
                level_name = detail::log_formatter_get_level_name(level_id);
            }
        }

        log_formatter::caller_info_t::caller_info_t(level_t::type lid, const char *lname, const char *fpath, uint32_t lnum,
                                                    const char *fnname, uint32_t ridx)
            : level_id(lid), level_name(lname), file_path(fpath), line_number(lnum), func_name(fnname), rotate_index(ridx) {
            if (NULL == level_name) {
                level_name = detail::log_formatter_get_level_name(level_id);
            }
        }

        std::string log_formatter::project_dir_;

        bool log_formatter::check(int32_t flags, int32_t checked) { return (flags & checked) == checked; }

        struct tm *log_formatter::get_iso_tm() {
            static time_t tm_tp = 0;
            static struct tm tm_obj;
            if (tm_tp != util::time::time_utility::get_now()) {
                tm_tp = util::time::time_utility::get_now();
                UTIL_STRFUNC_LOCALTIME_S(&tm_tp, &tm_obj);
            }

            return &tm_obj;
        }

        size_t log_formatter::format(char *buff, size_t bufz, const char *fmt, size_t fmtz, const caller_info_t &caller) {
            if (NULL == buff || 0 == bufz) {
                return 0;
            }

            if (NULL == fmt || 0 == fmtz) {
                buff[0] = '\0';
                return 0;
            }

            bool need_parse = false, running = true;
            size_t ret = 0;
            struct tm tm_obj_cache;
            struct tm *tm_obj_ptr = NULL;

// 时间加缓存，以防使用过程中时间变化
#define LOG_FMT_FN_TM_MEM(VAR, EXPRESS) \
    \
int VAR;                                \
    \
if(NULL == tm_obj_ptr) {                \
        tm_obj_cache = *get_iso_tm();   \
        tm_obj_ptr = &tm_obj_cache;     \
        VAR = tm_obj_ptr->EXPRESS;      \
    \
}                                  \
    else {                              \
        VAR = tm_obj_ptr->EXPRESS;      \
    \
}

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
                // 以后再优化
                switch (fmt[i]) {
                // =================== datetime ===================
                case 'Y': {
                    if (bufz - ret < 4) {
                        running = false;
                    } else {
                        LOG_FMT_FN_TM_MEM(year, tm_year + 1900);
                        buff[ret++] = static_cast<char>(year / 1000 + '0');
                        buff[ret++] = static_cast<char>((year / 100) % 10 + '0');
                        buff[ret++] = static_cast<char>((year / 10) % 10 + '0');
                        buff[ret++] = static_cast<char>(year % 10 + '0');
                    }
                    break;
                }
                case 'y': {
                    if (bufz - ret < 2) {
                        running = false;
                    } else {
                        LOG_FMT_FN_TM_MEM(year, tm_year + 1900);
                        buff[ret++] = static_cast<char>((year / 10) % 10 + '0');
                        buff[ret++] = static_cast<char>(year % 10 + '0');
                    }
                    break;
                }
                case 'm': {
                    if (bufz - ret < 2) {
                        running = false;
                    } else {
                        LOG_FMT_FN_TM_MEM(mon, tm_mon + 1);
                        buff[ret++] = static_cast<char>(mon / 10 + '0');
                        buff[ret++] = static_cast<char>(mon % 10 + '0');
                    }
                    break;
                }
                case 'j': {
                    if (bufz - ret < 3) {
                        running = false;
                    } else {
                        LOG_FMT_FN_TM_MEM(yday, tm_yday);
                        buff[ret++] = static_cast<char>(yday / 100 + '0');
                        buff[ret++] = static_cast<char>((yday / 10) % 10 + '0');
                        buff[ret++] = static_cast<char>(yday % 10 + '0');
                    }
                    break;
                }
                case 'd': {
                    if (bufz - ret < 2) {
                        running = false;
                    } else {
                        LOG_FMT_FN_TM_MEM(mday, tm_mday);
                        buff[ret++] = static_cast<char>(mday / 10 + '0');
                        buff[ret++] = static_cast<char>(mday % 10 + '0');
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
                        buff[ret++] = static_cast<char>(hour / 10 + '0');
                        buff[ret++] = static_cast<char>(hour % 10 + '0');
                    }
                    break;
                }
                case 'I': {
                    if (bufz - ret < 2) {
                        running = false;
                    } else {
                        LOG_FMT_FN_TM_MEM(hour, tm_hour % 12 + 1);
                        buff[ret++] = static_cast<char>(hour / 10 + '0');
                        buff[ret++] = static_cast<char>(hour % 10 + '0');
                    }
                    break;
                }
                case 'M': {
                    if (bufz - ret < 2) {
                        running = false;
                    } else {
                        LOG_FMT_FN_TM_MEM(minite, tm_min);
                        buff[ret++] = static_cast<char>(minite / 10 + '0');
                        buff[ret++] = static_cast<char>(minite % 10 + '0');
                    }
                    break;
                }
                case 'S': {
                    if (bufz - ret < 2) {
                        running = false;
                    } else {
                        LOG_FMT_FN_TM_MEM(sec, tm_sec);
                        buff[ret++] = static_cast<char>(sec / 10 + '0');
                        buff[ret++] = static_cast<char>(sec % 10 + '0');
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
                        buff[ret++] = static_cast<char>(year / 1000 + '0');
                        buff[ret++] = static_cast<char>((year / 100) % 10 + '0');
                        buff[ret++] = static_cast<char>((year / 10) % 10 + '0');
                        buff[ret++] = static_cast<char>(year % 10 + '0');
                        buff[ret++] = '-';
                        buff[ret++] = static_cast<char>(mon / 10 + '0');
                        buff[ret++] = static_cast<char>(mon % 10 + '0');
                        buff[ret++] = '-';
                        buff[ret++] = static_cast<char>(mday / 10 + '0');
                        buff[ret++] = static_cast<char>(mday % 10 + '0');
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
                        buff[ret++] = static_cast<char>(hour / 10 + '0');
                        buff[ret++] = static_cast<char>(hour % 10 + '0');
                        buff[ret++] = ':';
                        buff[ret++] = static_cast<char>(minite / 10 + '0');
                        buff[ret++] = static_cast<char>(minite % 10 + '0');
                        buff[ret++] = ':';
                        buff[ret++] = static_cast<char>(sec / 10 + '0');
                        buff[ret++] = static_cast<char>(sec % 10 + '0');
                    }
                    break;
                }
                case 'R': {
                    if (bufz - ret < 5) {
                        running = false;
                    } else {
                        LOG_FMT_FN_TM_MEM(hour, tm_hour);
                        LOG_FMT_FN_TM_MEM(minite, tm_min);
                        buff[ret++] = static_cast<char>(hour / 10 + '0');
                        buff[ret++] = static_cast<char>(hour % 10 + '0');
                        buff[ret++] = ':';
                        buff[ret++] = static_cast<char>(minite / 10 + '0');
                        buff[ret++] = static_cast<char>(minite % 10 + '0');
                    }
                    break;
                }

                case 'f': {
                    if (bufz - ret < 3) {
                        running = false;
                    } else {
                        time_t ms = ::util::time::time_utility::get_now_usec() / 1000;
                        buff[ret++] = static_cast<char>(ms / 100 + '0');
                        buff[ret++] = static_cast<char>((ms / 10) % 10 + '0');
                        buff[ret++] = static_cast<char>(ms % 10 + '0');
                        // old version use clock() to get the data
                        // clock_t clk = (clock() / (CLOCKS_PER_SEC / 1000)) % 1000;
                        // buff[ret++] = static_cast<char>(clk / 100 + '0');
                        // buff[ret++] = static_cast<char>((clk / 10) % 10 + '0');
                        // buff[ret++] = static_cast<char>(clk % 10 + '0');
                    }

                    break;
                }

                // =================== caller data ===================
                case 'L': {
                    if (NULL != caller.level_name) {
                        int res = UTIL_STRFUNC_SNPRINTF(&buff[ret], bufz - ret, "%8s", caller.level_name);
                        if (res < 0) {
                            running = false;
                        } else {
                            ret += static_cast<size_t>(res);
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
                    if (NULL != caller.file_path) {
                        const char *file_path = caller.file_path;
                        if (!project_dir_.empty()) {
                            for (size_t j = 0; j < project_dir_.size(); ++j) {
                                if (file_path && *file_path && project_dir_[j] == *file_path) {
                                    ++file_path;
                                } else {
                                    file_path = caller.file_path;
                                    break;
                                }
                            }

                            if (file_path != caller.file_path && ret < bufz - 1) {
                                buff[ret++] = '~';
                            }
                        }

                        int res = UTIL_STRFUNC_SNPRINTF(&buff[ret], bufz - ret, "%s", file_path);
                        if (res < 0) {
                            running = false;
                        } else {
                            ret += static_cast<size_t>(res);
                        }
                    }
                    break;
                }
                case 'k': {
                    if (NULL != caller.file_path) {
                        const char *file_name = caller.file_path;
                        for (const char *dir_split = caller.file_path; *dir_split; ++dir_split) {
                            if ('/' == *dir_split || '\\' == *dir_split) {
                                file_name = dir_split + 1;
                            }
                        }
                        int res = UTIL_STRFUNC_SNPRINTF(&buff[ret], bufz - ret, "%s", file_name);
                        if (res < 0) {
                            running = false;
                        } else {
                            ret += static_cast<size_t>(res);
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
                    if (NULL != caller.func_name) {
                        int res = UTIL_STRFUNC_SNPRINTF(&buff[ret], bufz - ret, "%s", caller.func_name);
                        if (res < 0) {
                            running = false;
                        } else {
                            ret += static_cast<size_t>(res);
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

        bool log_formatter::check_rotation_var(const char *fmt, size_t fmtz) {
            for (size_t i = 0; fmt && i < fmtz - 1; ++i) {
                if ('%' == fmt[i] && 'N' == fmt[i + 1]) {
                    return true;
                }
            }

            return false;
        }

        bool log_formatter::has_format(const char *fmt, size_t fmtz) {
            for (size_t i = 0; fmt && i < fmtz - 1; ++i) {
                if ('%' == fmt[i]) {
                    return true;
                }
            }

            return false;
        }

        void log_formatter::set_project_directory(const char *dirbuf, size_t dirsz) {
            if (NULL == dirbuf) {
                project_dir_.clear();
            } else if (dirsz <= 0) {
                project_dir_ = dirbuf;
            } else {
                project_dir_.assign(dirbuf, dirsz);
            }
        }

        log_formatter::level_t::type log_formatter::get_level_by_name(const char *name) {
            if (NULL == name) {
                return level_t::LOG_LW_DEBUG;
            }

            // number, directly convert it
            if (name && (name[0] == '\\' || (name[0] >= '0' && name[0] <= '9'))) {
                int l = util::string::to_int<int>(name);
                if (l >= 0 && l <= level_t::LOG_LW_DEBUG) {
                    return static_cast<level_t::type>(l);
                }

                return level_t::LOG_LW_DEBUG;
            }

            // name and convert into level

            if (0 == UTIL_STRFUNC_STRNCASE_CMP("disable", name, 7)) {
                return level_t::LOG_LW_DISABLED;
            }

            if (0 == UTIL_STRFUNC_STRNCASE_CMP("fatal", name, 5)) {
                return level_t::LOG_LW_FATAL;
            }

            if (0 == UTIL_STRFUNC_STRNCASE_CMP("error", name, 5)) {
                return level_t::LOG_LW_ERROR;
            }

            if (0 == UTIL_STRFUNC_STRNCASE_CMP("warn", name, 4)) {
                return level_t::LOG_LW_WARNING;
            }

            if (0 == UTIL_STRFUNC_STRNCASE_CMP("info", name, 4)) {
                return level_t::LOG_LW_INFO;
            }

            if (0 == UTIL_STRFUNC_STRNCASE_CMP("notice", name, 6)) {
                return level_t::LOG_LW_NOTICE;
            }

            return level_t::LOG_LW_DEBUG;
        }

    } // namespace log
} // namespace util
