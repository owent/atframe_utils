#pragma once

#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <string>
#include <inttypes.h>
#include <ctime>
#include <list>
#include "std/functional.h"
#include "std/smart_ptr.h"

#include "DesignPattern/Singleton.h"

#ifndef LOG_WRAPPER_MAX_SIZE_PER_LINE
#define LOG_WRAPPER_MAX_SIZE_PER_LINE 65536
#endif

#ifndef LOG_WRAPPER_CATEGORIZE_SIZE
#define LOG_WRAPPER_CATEGORIZE_SIZE 4
#endif

namespace util {
    namespace log {
        class LogWrapper : public Singleton<LogWrapper>
        {
        public:
            struct categorize_t {
                enum type {
                    DEFAULT = 0,   // 服务框架
                    MAX = LOG_WRAPPER_CATEGORIZE_SIZE
                };
            };

            struct level_t {
                enum type {
                    LOG_LW_DISABLED = 0,     // 关闭日志
                    LOG_LW_FATAL,            // 强制输出
                    LOG_LW_ERROR,            // 错误
                    LOG_LW_WARNING,
                    LOG_LW_INFO,
                    LOG_LW_NOTICE,
                    LOG_LW_DEBUG,
                };
            };

            typedef std::function<void(level_t::type level_id, const char* level, const char* content)> log_handler_t;
            typedef struct {
                level_t::type level_min;
                level_t::type level_max;
                log_handler_t handle;
            } log_router_t;

        protected:
            LogWrapper();
            virtual ~LogWrapper();

        public:
            // 初始化
            int32_t init(level_t::type level = level_t::LOG_LW_DEBUG);

            static void update();

            static inline time_t getLogTime() { return log_time_cache_sec_; }
            static inline const tm* getLogTm() { return log_time_cache_sec_p_; }

            void log(level_t::type level_id, const char* level, const char* file_path, uint32_t line_number, const char* func_name, 
#ifdef _MSC_VER
                _In_z_ _Printf_format_string_ const char* fmt, ...);
#elif (defined(__clang__) && __clang_major__ >= 3) || (defined(__GNUC__) && __GNUC__ >= 4)
                // 格式检查(成员函数有个隐含的this参数)
                const char* fmt, ...) __attribute__((format(printf, 7, 8)));
#else
                const char* fmt, ...);
#endif

            // 一般日志级别检查
            inline bool check(level_t::type level) {
                return !IsInstanceDestroyed() && log_level_ >= level;
            }

            inline const std::list<log_router_t>& getLogHandles() const { return log_handlers_; }

            void addLogHandle(log_handler_t h, level_t::type level_min = level_t::LOG_LW_FATAL, level_t::type level_max = level_t::LOG_LW_DEBUG);

            inline void setLevel(level_t::type l) { log_level_ = l; }

            inline level_t::type getLevel() const { return log_level_; }

            inline void setAutoUpdate(bool u) { auto_update_time_ = u; }

            inline bool getAutoUpdate() const { return auto_update_time_; }

            inline bool getEnablePrintFileLocation() const {
                return enable_print_file_location_;
            }

            inline void setEnablePrintFileLocation(bool enable_print_file_location) {
                enable_print_file_location_ = enable_print_file_location;
            }

            inline bool getEnablePrintFunctionName() const {
                return enable_print_function_name_;
            }

            inline void setEnablePrintFunctionName(bool enable_print_function_name) {
                enable_print_function_name_ = enable_print_function_name;
            }

            inline bool getEnablePrintLogType() const {
                return enable_print_log_type_;
            }

            inline void setEnablePrintLogType(bool enable_print_log_type) {
                enable_print_log_type_ = enable_print_log_type;
            }

            inline const std::string& getEnablePrintTime() const {
                return enable_print_time_;
            }

            inline void setEnablePrintTime(const std::string& enable_print_time) {
                enable_print_time_ = enable_print_time;
            }

            // TODO 白名单及用户指定日志输出以后有需要再说

            static LogWrapper* getLogCat(uint32_t cats = categorize_t::DEFAULT);
        private:
            level_t::type log_level_;
            bool auto_update_time_;
            static time_t log_time_cache_sec_;
            static tm* log_time_cache_sec_p_;
            std::list<log_router_t> log_handlers_;

            bool enable_print_file_location_;

        private:
            bool enable_print_function_name_;
            bool enable_print_log_type_;
            std::string enable_print_time_;

            static bool destroyed_;
        };
    }
}

#define WLOG_LEVELID(lv) static_cast<util::log::LogWrapper::level_t::type>(lv)

#define WDTLOGGETCAT(cat) util::log::LogWrapper::getLogCat(cat)
#define WDTLOGFILENF(lv, name)  lv, name, __FILE__, __LINE__, __FUNCTION__

#define WLOG_INIT(cat, lv) NULL != WDTLOGGETCAT(cat)? WDTLOGGETCAT(cat)->init(lv): -1

#define WLOG_GETCAT(cat) util::log::LogWrapper::getLogCat(cat)

// 按分类日志输出工具
#ifdef _MSC_VER

#define WCLOGDEFLV(lv, lv_name, cat, ...) \
            if (NULL != WDTLOGGETCAT(cat) && WDTLOGGETCAT(cat)->check(lv))\
                WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#define WCLOGDEBUG(cat, ...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_DEBUG, "Debug", cat, __VA_ARGS__)
#define WCLOGNOTICE(cat, ...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_NOTICE, "Notice", cat, __VA_ARGS__)
#define WCLOGINFO(cat, ...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_INFO, "Info", cat, __VA_ARGS__)
#define WCLOGWARNING(cat, ...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_WARNING, "Warning", cat, __VA_ARGS__)
#define WCLOGERROR(cat, ...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_ERROR, "Error", cat, __VA_ARGS__)
#define WCLOGFATAL(cat, ...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_FATAL, "Fatal", cat, __VA_ARGS__)

#else

#define WCLOGDEFLV(lv, lv_name, cat, args...) \
            if (NULL != WDTLOGGETCAT(cat) && WDTLOGGETCAT(cat)->check(lv))\
                WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), ##args);

#define WCLOGDEBUG(...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_DEBUG, "Debug", __VA_ARGS__)
#define WCLOGNOTICE(...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_NOTICE, "Notice", __VA_ARGS__)
#define WCLOGINFO(...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_INFO, "Info", __VA_ARGS__)
#define WCLOGWARNING(...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_WARNING, "Warning", __VA_ARGS__)
#define WCLOGERROR(...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_ERROR, "Error", __VA_ARGS__)
#define WCLOGFATAL(...) WCLOGDEFLV(util::log::LogWrapper::level_t::LOG_LW_FATAL, "Fatal", __VA_ARGS__)

#endif

// 默认日志输出工具
#define WLOGDEBUG(...) WCLOGDEBUG(util::log::LogWrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGNOTICE(...) WCLOGNOTICE(util::log::LogWrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGINFO(...)  WCLOGINFO(util::log::LogWrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGWARNING(...) WCLOGWARNING(util::log::LogWrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGERROR(...) WCLOGERROR(util::log::LogWrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGFATAL(...) WCLOGFATAL(util::log::LogWrapper::categorize_t::DEFAULT, __VA_ARGS__)


// 控制台输出工具
#ifdef WIN32
#define PSTDTERMCOLOR(code, fmt) fmt
#else
#define PSTDTERMCOLOR(code, fmt) "\033[" #code ";1m" fmt "\033[0m"
#endif

#ifdef _MSC_VER

#define PSTDINFO(fmt, ...)       printf("Info: " fmt, __VA_ARGS__)
#define PSTDNOTICE(fmt, ...)     printf(PSTDTERMCOLOR(36, "Notice: " fmt), __VA_ARGS__)
#define PSTDWARNING(fmt, ...)    printf(PSTDTERMCOLOR(33, "Warning: " fmt), __VA_ARGS__)
#define PSTDERROR(fmt, ...)      printf(PSTDTERMCOLOR(31, "Error: " fmt), __VA_ARGS__)
#define PSTDOK(fmt, ...)         printf(PSTDTERMCOLOR(32, "OK: " fmt), __VA_ARGS__)
//
#ifndef NDEBUG
#define PSTDDEBUG(fmt, ...)     printf(PSTDTERMCOLOR(35, "Debug: " fmt), __VA_ARGS__)
#define PSTDMARK                printf(PSTDTERMCOLOR(35, "Mark: %s:%s (function %s)"), __FILE__, __LINE__, __FUNCTION__)
#else
#define PSTDDEBUG(fmt, ...)
#define PSTDMARK
#endif

#else
#define PSTDINFO(fmt, args...)       printf("Info: " fmt, ##args)
#define PSTDNOTICE(fmt, args...)     printf(PSTDTERMCOLOR(36, "Notice: " fmt), ##args)
#define PSTDWARNING(fmt, args...)    printf(PSTDTERMCOLOR(33, "Warning: " fmt), ##args)
#define PSTDERROR(fmt, args...)      printf(PSTDTERMCOLOR(31, "Error: " fmt), ##args)
#define PSTDOK(fmt, args...)         printf(PSTDTERMCOLOR(32, "OK: " fmt), ##args)
//
#ifndef NDEBUG
#define PSTDDEBUG(fmt, args...)     printf(PSTDTERMCOLOR(35, "Debug: " fmt), ##args)
#define PSTDMARK                    printf(PSTDTERMCOLOR(35, "Mark: %s:%s (function %s)"), __FILE__, __LINE__, __FUNCTION__)
#else
#define PSTDDEBUG(fmt, args...)
#define PSTDMARK
#endif

#endif
