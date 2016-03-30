/**
 * @file log_wrapper.h
 * @brief 日志包装器
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2015-06-29
 * @history
 */

#ifndef _UTIL_LOG_LOG_WRAPPER_H_
#define _UTIL_LOG_LOG_WRAPPER_H_

#pragma once

#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <string>
#include <inttypes.h>
#include <ctime>
#include <list>
#include <bitset>
#include "std/functional.h"
#include "std/smart_ptr.h"

#ifndef LOG_WRAPPER_MAX_SIZE_PER_LINE
#define LOG_WRAPPER_MAX_SIZE_PER_LINE (1024 * 1024 * 2)
#endif

#ifndef LOG_WRAPPER_CATEGORIZE_SIZE
#define LOG_WRAPPER_CATEGORIZE_SIZE 32
#endif

namespace util {
    namespace log {
        class log_wrapper {
        public:
            struct categorize_t {
                enum type {
                    DEFAULT = 0, // 服务框架
                    MAX = LOG_WRAPPER_CATEGORIZE_SIZE
                };
            };

            struct level_t {
                enum type {
                    LOG_LW_DISABLED = 0, // 关闭日志
                    LOG_LW_FATAL,        // 强制输出
                    LOG_LW_ERROR,        // 错误
                    LOG_LW_WARNING,
                    LOG_LW_INFO,
                    LOG_LW_NOTICE,
                    LOG_LW_DEBUG,
                };
            };

            struct options_t {
                enum type {
                    OPT_AUTO_UPDATE_TIME = 0, // 是否自动更新时间（会降低性能）
                    OPT_PRINT_FILE_NAME,      // 打印文件名
                    OPT_PRINT_FUNCTION_NAME,  // 打印函数名
                    OPT_PRINT_LEVEL,          // 打印日志级别
                    OPT_MAX
                };
            };

        public:
            struct caller_info_t {
                level_t::type level_id;
                const char *level_name;
                const char *file_path;
                uint32_t line_number;
                const char *func_name;

                caller_info_t(level_t::type lid, const char *lname, const char *fpath, uint32_t lnum, const char *fnname);
            };

            typedef std::function<void(const caller_info_t &caller, const char *content, size_t content_size)> log_handler_t;
            typedef struct {
                level_t::type level_min;
                level_t::type level_max;
                log_handler_t handle;
            } log_router_t;

        protected:
            log_wrapper();
            virtual ~log_wrapper();

        public:
            // 初始化
            int32_t init(level_t::type level = level_t::LOG_LW_DEBUG);

            static void update();

            void log(const caller_info_t &caller,
#ifdef _MSC_VER
                     _In_z_ _Printf_format_string_ const char *fmt, ...);
#elif(defined(__clang__) && __clang_major__ >= 3) || (defined(__GNUC__) && __GNUC__ >= 4)
                     // 格式检查(成员函数有个隐含的this参数)
                     const char *fmt, ...) __attribute__((format(printf, 3, 4)));
#else
                     const char *fmt, ...);
#endif

            // 一般日志级别检查
            inline bool check(level_t::type level) const { return log_level_ >= level; }

            static inline bool check(const log_wrapper *logger, level_t::type level) {
                if (NULL == logger) {
                    return false;
                }
                return logger->log_level_ >= level;
            }

            inline const std::list<log_router_t> &get_sinks() const { return log_sinks_; }

            /**
             * @brief 添加落地接口
             */
            void add_sink(log_handler_t h, level_t::type level_min = level_t::LOG_LW_FATAL,
                          level_t::type level_max = level_t::LOG_LW_DEBUG);

            inline void set_level(level_t::type l) { log_level_ = l; }

            inline level_t::type get_level() const { return log_level_; }

            inline const std::string &get_time_format() const { return time_format_; }

            inline void set_time_format(const std::string &time_format) { time_format_ = time_format; }

            inline bool get_option(options_t::type t) const {
                if (t < options_t::OPT_MAX) {
                    return false;
                }

                return options_.test(t);
            };

            inline void set_option(options_t::type t, bool v) const {
                if (t < options_t::OPT_MAX) {
                    return;
                }

                return options_.set(t, v);
            };

            /**
             * @brief 实际写出到落地接口
             */
            void write_log(const caller_info_t &caller, const char *content, size_t content_size);

            // TODO 白名单及用户指定日志输出以后有需要再说

            static log_wrapper *mutable_log_cat(uint32_t cats = categorize_t::DEFAULT);

        private:
            level_t::type log_level_;
            std::list<log_router_t> log_sinks_;
            std::string time_format_;
            std::bitset<options_t::OPT_MAX> options_;

            static bool destroyed_; // log模块进入释放阶段，进入释放阶段后log功能会被关闭
        };
    }
}

#define WLOG_LEVELID(lv) static_cast<util::log::log_wrapper::level_t::type>(lv)

#define WDTLOGGETCAT(cat) util::log::log_wrapper::getLogCat(cat)
#define WDTLOGFILENF(lv, name) util::log::log_wrapper::caller_info_t(lv, name, __FILE__, __LINE__, __FUNCTION__)

#define WLOG_INIT(cat, lv) NULL != WDTLOGGETCAT(cat) ? WDTLOGGETCAT(cat)->init(lv) : -1

#define WLOG_GETCAT(cat) util::log::log_wrapper::getLogCat(cat)

// 按分类日志输出工具
#ifdef _MSC_VER

#define WCLOGDEFLV(lv, lv_name, cat, ...) \
    if (util::log::log_wrapper::check(WDTLOGGETCAT(cat), lv)) WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#define WCLOGDEBUG(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_DEBUG, "Debug", cat, __VA_ARGS__)
#define WCLOGNOTICE(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_NOTICE, "Notice", cat, __VA_ARGS__)
#define WCLOGINFO(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_INFO, "Info", cat, __VA_ARGS__)
#define WCLOGWARNING(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_WARNING, "Warning", cat, __VA_ARGS__)
#define WCLOGERROR(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_ERROR, "Error", cat, __VA_ARGS__)
#define WCLOGFATAL(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_FATAL, "Fatal", cat, __VA_ARGS__)

#else

#define WCLOGDEFLV(lv, lv_name, cat, args...) \
    if (util::log::log_wrapper::check(WDTLOGGETCAT(cat), lv)) WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), ##args);

#define WCLOGDEBUG(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_DEBUG, "Debug", __VA_ARGS__)
#define WCLOGNOTICE(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_NOTICE, "Notice", __VA_ARGS__)
#define WCLOGINFO(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_INFO, "Info", __VA_ARGS__)
#define WCLOGWARNING(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_WARNING, "Warning", __VA_ARGS__)
#define WCLOGERROR(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_ERROR, "Error", __VA_ARGS__)
#define WCLOGFATAL(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_FATAL, "Fatal", __VA_ARGS__)

#endif

// 默认日志输出工具
#define WLOGDEBUG(...) WCLOGDEBUG(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGNOTICE(...) WCLOGNOTICE(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGINFO(...) WCLOGINFO(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGWARNING(...) WCLOGWARNING(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGERROR(...) WCLOGERROR(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGFATAL(...) WCLOGFATAL(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)


// 控制台输出工具
#ifdef WIN32
#define PSTDTERMCOLOR(code, fmt) fmt
#else
#define PSTDTERMCOLOR(code, fmt) "\033[" #code ";1m" fmt "\033[0m"
#endif

#ifdef _MSC_VER

#define PSTDINFO(fmt, ...) printf("Info: " fmt, __VA_ARGS__)
#define PSTDNOTICE(fmt, ...) printf(PSTDTERMCOLOR(36, "Notice: " fmt), __VA_ARGS__)
#define PSTDWARNING(fmt, ...) printf(PSTDTERMCOLOR(33, "Warning: " fmt), __VA_ARGS__)
#define PSTDERROR(fmt, ...) printf(PSTDTERMCOLOR(31, "Error: " fmt), __VA_ARGS__)
#define PSTDFATAL(fmt, ...) printf(PSTDTERMCOLOR(31, "Fatal: " fmt), __VA_ARGS__)
#define PSTDOK(fmt, ...) printf(PSTDTERMCOLOR(32, "OK: " fmt), __VA_ARGS__)
//
#ifndef NDEBUG
#define PSTDDEBUG(fmt, ...) printf(PSTDTERMCOLOR(35, "Debug: " fmt), __VA_ARGS__)
#define PSTDMARK printf(PSTDTERMCOLOR(35, "Mark: %s:%s (function %s)"), __FILE__, __LINE__, __FUNCTION__)
#else
#define PSTDDEBUG(fmt, ...)
#define PSTDMARK
#endif

#else
#define PSTDINFO(fmt, args...) printf("Info: " fmt, ##args)
#define PSTDNOTICE(fmt, args...) printf(PSTDTERMCOLOR(36, "Notice: " fmt), ##args)
#define PSTDWARNING(fmt, args...) printf(PSTDTERMCOLOR(33, "Warning: " fmt), ##args)
#define PSTDERROR(fmt, args...) printf(PSTDTERMCOLOR(31, "Error: " fmt), ##args)
#define PSTDFATAL(fmt, args...) printf(PSTDTERMCOLOR(31, "Fatal: " fmt), ##args)
#define PSTDOK(fmt, args...) printf(PSTDTERMCOLOR(32, "OK: " fmt), ##args)
//
#ifndef NDEBUG
#define PSTDDEBUG(fmt, args...) printf(PSTDTERMCOLOR(35, "Debug: " fmt), ##args)
#define PSTDMARK printf(PSTDTERMCOLOR(35, "Mark: %s:%s (function %s)"), __FILE__, __LINE__, __FUNCTION__)
#else
#define PSTDDEBUG(fmt, args...)
#define PSTDMARK
#endif

#endif

#endif // _UTIL_LOG_LOG_WRAPPER_H_
