#include <cstdio>
#include <cstring>
#include <stdarg.h>
#include "std/thread.h"

#include "common/string_oprs.h"
#include "lock/spin_lock.h"
#include "lock/lock_holder.h"

#include "time/time_utility.h"

#include "log/log_formatter.h"
#include "log/log_wrapper.h"

namespace util {
    namespace log {
        bool log_wrapper::destroyed_ = false;

        log_wrapper::log_wrapper() : log_level_(level_t::LOG_LW_DISABLED) {
            update();

            set_option(options_t::OPT_AUTO_UPDATE_TIME, true);
            prefix_format_ = "[Log %L][%F %T.%f][%s:%n(%C)]: ";
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
            if (get_option(options_t::OPT_AUTO_UPDATE_TIME) && !prefix_format_.empty()) {
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
                    size_t start_index =
                        log_formatter::format(log_buffer, sizeof(log_buffer), prefix_format_.c_str(), prefix_format_.size(), caller);

                    va_list va_args;
                    va_start(va_args, fmt);
                    int prt_res = UTIL_STRFUNC_VSNPRINTF(&log_buffer[start_index], sizeof(log_buffer) - start_index, fmt, va_args);
                    va_end(va_args);
                    if (prt_res >= 0) {
                        start_index += static_cast<size_t>(prt_res);
                    }

                    if (start_index < sizeof(log_buffer)) {
                        log_buffer[start_index] = 0;
                        log_size = start_index;
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
                if (log_level_ >= iter->level_min && log_level_ <= iter->level_max) {
                    iter->handle(caller, content, content_size);
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
