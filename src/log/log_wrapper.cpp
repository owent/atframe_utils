#include "std/thread.h"
#include <cstdio>
#include <cstring>
#include <stdarg.h>


#include "common/string_oprs.h"
#include "lock/lock_holder.h"
#include "lock/spin_lock.h"

#include "time/time_utility.h"

#include "log/log_formatter.h"
#include "log/log_wrapper.h"

#if defined(THREAD_TLS_ENABLED) && 1 == THREAD_TLS_ENABLED
namespace util {
    namespace log {
        namespace detail {
            char *get_log_tls_buffer() {
                static THREAD_TLS char ret[LOG_WRAPPER_MAX_SIZE_PER_LINE];
                return ret;
            }
        }
    }
}
#else

#include <pthread.h>
namespace util {
    namespace log {
        namespace detail {
            static pthread_once_t gt_get_log_tls_once = PTHREAD_ONCE_INIT;
            static pthread_key_t gt_get_log_tls_key;

            static void dtor_pthread_get_log_tls(void *p) {
                char *buffer_block = reinterpret_cast<char *>(p);
                if (NULL != buffer_block) {
                    delete[] buffer_block;
                }
            }

            static void init_pthread_get_log_tls() {
                (void)pthread_key_create(&gt_get_log_tls_key, dtor_pthread_get_log_tls);
                char *buffer_block = new char[LOG_WRAPPER_MAX_SIZE_PER_LINE];
                pthread_setspecific(gt_get_log_tls_key, buffer_block);
            }

            char *get_log_tls_buffer() {
                (void)pthread_once(&gt_get_log_tls_once, init_pthread_get_log_tls);
                return reinterpret_cast<char *>(pthread_getspecific(gt_get_log_tls_key));
            }
        }
    }
}

#endif

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

            char *log_buffer = detail::get_log_tls_buffer();
            size_t log_size = 0;
            {
                if (!log_sinks_.empty()) {
                    // format => "[Log    DEBUG][2015-01-12 10:09:08.]
                    size_t start_index = log_formatter::format(log_buffer, LOG_WRAPPER_MAX_SIZE_PER_LINE, prefix_format_.c_str(),
                                                               prefix_format_.size(), caller);

                    va_list va_args;
                    va_start(va_args, fmt);
                    int prt_res =
                        UTIL_STRFUNC_VSNPRINTF(&log_buffer[start_index], LOG_WRAPPER_MAX_SIZE_PER_LINE - start_index, fmt, va_args);
                    va_end(va_args);
                    if (prt_res >= 0) {
                        start_index += static_cast<size_t>(prt_res);
                    }

                    if (start_index < LOG_WRAPPER_MAX_SIZE_PER_LINE) {
                        log_buffer[start_index] = 0;
                        log_size = start_index;
                    } else {
                        log_size = LOG_WRAPPER_MAX_SIZE_PER_LINE;
                        log_buffer[LOG_WRAPPER_MAX_SIZE_PER_LINE - 1] = 0;
                    }
                }
            }

            write_log(caller, log_buffer, log_size);
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
