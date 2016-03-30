#include <cstdio>
#include <cstring>
#include <stdarg.h>
#include "std/thread.h"
#include "lock/spin_lock.h"
#include "lock/lock_holder.h"

#include "time/time_utility.h"

#include "log/log_wrapper.h"

namespace util {
    namespace log {

        bool log_wrapper::destroyed_ = false;

        log_wrapper::caller_info_t::caller_info_t(level_t::type lid, const char *lname, const char *fpath, uint32_t lnum,
                                                  const char *fnname)
            : level_id(lid), level_name(lname), file_path(fpath), line_number(lnum), func_name(fnname) {}

        log_wrapper::log_wrapper() : log_level_(level_t::LOG_LW_DISABLED) {
            update();

            set_option(options_t::OPT_AUTO_UPDATE_TIME, true);
            set_option(options_t::OPT_PRINT_FILE_NAME, true);
            set_option(options_t::OPT_PRINT_FUNCTION_NAME, true);
            set_option(options_t::OPT_PRINT_LEVEL, true);
            time_format_ = "[%Y-%m-%d %H:%M:%S]";
        }

        log_wrapper::~log_wrapper() {
            log_wrapper::destroyed_ = true;

            // 重置level，只要内存没释放，就还可以内存访问，但是不能写出日志
            log_level_ = level_t::LOG_LW_DISABLED;
        }

        int32_t log_wrapper::init(level_t::type level) {
            log_level_ = level;

            return 0;
        }

        void log_wrapper::add_sink(log_handler_t h, level_t::type level_min, level_t::type level_max) {
            if (h) {
                log_router_t router;
                router.handle = h;
                router.level_min = level_min;
                router.level_max = level_max;
                log_sinks_.push_back(router);
            };
        }

        void log_wrapper::update() {}

        void log_wrapper::log(const caller_info_t &caller, const char *fmt, ...) {
            if (get_option(options_t::OPT_AUTO_UPDATE_TIME) && !time_format_.empty()) {
                update();
            }

#if defined(THREAD_TLS_ENABLED) && 1 == THREAD_TLS_ENABLED
            static THREAD_TLS char log_buffer[LOG_WRAPPER_MAX_SIZE_PER_LINE];
#else
#define LOG_WRAPPER_WITH_LOCK 1
            static util::lock::spin_lock log_fmt_lock;
            static char log_buffer[LOG_WRAPPER_MAX_SIZE_PER_LINE];

            std::string this_log;
#endif
            size_t log_size = 0;
            {
#ifdef LOG_WRAPPER_WITH_LOCK
                util::lock::lock_holder<util::lock::spin_lock> fmt_lock(log_fmt_lock);
#endif

                if (!log_sinks_.empty()) {
                    // format => "[Log    DEBUG][2015-01-12 10:09:08.]
                    int start_index = 0;

                    if (get_option(options_t::OPT_PRINT_LEVEL) && NULL != caller.level_name) {
                        start_index = UTIL_STRFUNC_VSNPRINTF(log_buffer, sizeof(log_buffer), "[Log %8s]", caller.level_name);
                        if (start_index < 0) {
                            start_index = 14;
                        }
                    }

                    // TODO 以后自己写时间format函数,自己记update
                    if (!time_format_.empty()) {
                        static time_t tm_tp = 0;
                        static struct tm tm_obj;
                        if (tm_tp != util::time::time_utility::get_now()) {
                            tm_tp = util::time::time_utility::get_now();
                            UTIL_STRFUNC_LOCALTIME_S(&tm_tp, &tm_obj);
                        }

                        start_index += strftime(&log_buffer[start_index], sizeof(log_buffer) - start_index, time_format_.c_str(), tm_obj);
                    }

                    // 打印位置选项
                    if (get_option(options_t::OPT_PRINT_FILE_NAME) && get_option(options_t::OPT_PRINT_FUNCTION_NAME) && NULL != file_path &&
                        NULL != func_name) {
                        int res = UTIL_STRFUNC_VSNPRINTF(&log_buffer[start_index], sizeof(log_buffer) - start_index, "[%s:%u(%s)]: ",
                                                         file_path, line_number, func_name);
                        start_index += res >= 0 ? res : 0;
                    } else if (get_option(options_t::OPT_PRINT_FILE_NAME) && NULL != file_path) {
                        int res = UTIL_STRFUNC_VSNPRINTF(&log_buffer[start_index], sizeof(log_buffer) - start_index, "[%s:%u]: ", file_path,
                                                         line_number);
                        start_index += res >= 0 ? res : 0;
                    } else if (get_option(options_t::OPT_PRINT_FUNCTION_NAME) && NULL != func_name) {
                        int res = UTIL_STRFUNC_VSNPRINTF(&log_buffer[start_index], sizeof(log_buffer) - start_index, "[(%s)]: ", func_name);
                        start_index += res >= 0 ? res : 0;
                    }

                    va_list va_args;
                    va_start(va_args, fmt);
                    int prt_res = UTIL_STRFUNC_VSNPRINTF(&log_buffer[start_index], sizeof(log_buffer) - start_index, fmt, va_args);
                    va_end(va_args);
                    if (prt_res >= 0) {
                        start_index += prt_res;
                    }

                    if (static_cast<size_t>(start_index) < sizeof(log_buffer)) {
                        log_buffer[start_index] = 0;
                        log_size = static_cast<size_t>(start_index);
                    } else {
                        log_size = sizeof(log_buffer);
                        log_buffer[sizeof(log_buffer) - 1] = 0;
                    }
                }

#ifdef LOG_WRAPPER_WITH_LOCK
                this_log.assign(log_buffer, start_index < sizeof(log_buffer) ? (start_index + 1) : sizeof(log_buffer));
#endif
            }

#ifdef LOG_WRAPPER_WITH_LOCK
            write_log(caller, this_log.c_str(), this_log.size());
#else
            write_log(caller, log_buffer, log_size);
#endif
        }

        void log_wrapper::write_log(const caller_info_t &caller, const char *content, size_t content_size) {
            for (std::list<log_router_t>::iterator iter = log_sinks_.begin(); iter != log_sinks_.end(); ++iter) {
                if (level_id >= iter->level_min && level_id <= iter->level_max) {
                    iter->handle(caller, log_buffer, log_size);
                }
            }
        }

        log_wrapper *log_wrapper::mutable_log_cat(uint32_t cats) {
            if (log_wrapper::destroyed_) {
                return NULL;
            }

            static log_wrapper all_logger[categorize_t::MAX];

            if (cats >= categorize_t::MAX) {
                return NULL;
            }

            return &all_logger[cats];
        }
    }
}
