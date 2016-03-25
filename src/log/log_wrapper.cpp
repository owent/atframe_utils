#include <cstdio>
#include <cstring>
#include <stdarg.h>
#include "log/log_wrapper.h"

namespace util {
    namespace log {

        bool log_wrapper::destroyed_ = false;
        time_t log_wrapper::log_time_cache_sec_ = 0;
        tm *log_wrapper::log_time_cache_sec_p_;

        log_wrapper::log_wrapper() : log_level_(level_t::LOG_LW_DISABLED) {
            auto_update_time_ = true;
            update();

            enable_print_file_location_ = true;
            enable_print_function_name_ = true;
            enable_print_log_type_ = true;
            enable_print_time_ = "[%Y-%m-%d %H:%M:%S]";
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

        void log_wrapper::addLogHandle(log_handler_t h, level_t::type level_min, level_t::type level_max) {
            if (h) {
                log_router_t router;
                router.handle = h;
                router.level_min = level_min;
                router.level_max = level_max;
                log_handlers_.push_back(router);
            };
        }

        void log_wrapper::update() {
            log_time_cache_sec_ = time(NULL);
            log_time_cache_sec_p_ = localtime(&log_time_cache_sec_);
        }

        void log_wrapper::log(level_t::type level_id, const char *level, const char *file_path, uint32_t line_number, const char *func_name,
                              const char *fmt, ...) {
            if (auto_update_time_ && !enable_print_time_.empty()) {
                update();
            }

            char log_buffer[LOG_WRAPPER_MAX_SIZE_PER_LINE];
            if (!log_handlers_.empty()) {
                // format => "[Log    DEBUG][2015-01-12 10:09:08.]
                int start_index = 0;

                if (enable_print_log_type_ && NULL != level) {
                    start_index = sprintf(log_buffer, "[Log %8s]", level);
                    if (start_index < 0) {
                        start_index = 14;
                    }
                }

                // 是否需要毫秒级？std::chrono
                if (!enable_print_time_.empty()) {
                    start_index += strftime(&log_buffer[start_index], sizeof(log_buffer) - start_index, enable_print_time_.c_str(),
                                            log_time_cache_sec_p_);
                }

                // 打印位置选项
                if (enable_print_file_location_ && enable_print_function_name_ && NULL != file_path && NULL != func_name) {
                    int res = sprintf(&log_buffer[start_index], "[%s:%u(%s)]: ", file_path, line_number, func_name);
                    start_index += res >= 0 ? res : 0;
                } else if (enable_print_file_location_ && NULL != file_path) {
                    int res = sprintf(&log_buffer[start_index], "[%s:%u]: ", file_path, line_number);
                    start_index += res >= 0 ? res : 0;
                } else if (enable_print_function_name_ && NULL != func_name) {
                    int res = sprintf(&log_buffer[start_index], "[(%s)]: ", func_name);
                    start_index += res >= 0 ? res : 0;
                }


                va_list va_args;
                va_start(va_args, fmt);

                start_index += vsnprintf(&log_buffer[start_index], sizeof(log_buffer) - start_index, fmt, va_args);

                va_end(va_args);

                log_buffer[sizeof(log_buffer) - 1] = 0;
                if (static_cast<size_t>(start_index) < sizeof(log_buffer)) {
                    log_buffer[start_index] = 0;
                }

                for (std::list<log_router_t>::iterator iter = log_handlers_.begin(); iter != log_handlers_.end(); ++iter) {
                    if (level_id >= iter->level_min && level_id <= iter->level_max) {
                        iter->handle(level_id, level, log_buffer);
                    }
                }
            }
        }

        log_wrapper *log_wrapper::getLogCat(uint32_t cats) {
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
