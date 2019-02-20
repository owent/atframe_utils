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

#ifndef UTIL_LOG_LOG_WRAPPER_H
#define UTIL_LOG_LOG_WRAPPER_H

#pragma once

#include "std/functional.h"
#include "std/smart_ptr.h"
#include <bitset>
#include <list>


#include "cli/shell_font.h"

#include "lock/spin_rw_lock.h"

#include "log_formatter.h"

#ifndef LOG_WRAPPER_MAX_SIZE_PER_LINE
#define LOG_WRAPPER_MAX_SIZE_PER_LINE (1024 * 1024 * 2)
#endif

#ifndef LOG_WRAPPER_CATEGORIZE_SIZE
#define LOG_WRAPPER_CATEGORIZE_SIZE 16
#endif

namespace util {
    namespace log {
        class log_wrapper {
        public:
            typedef std::shared_ptr<log_wrapper> ptr_t;

            struct categorize_t {
                enum type {
                    DEFAULT = 0, // 服务框架
                    MAX     = LOG_WRAPPER_CATEGORIZE_SIZE
                };
            };

            struct options_t {
                enum type {
                    OPT_AUTO_UPDATE_TIME = 0, // 是否自动更新时间（会降低性能）
                    OPT_USER_MAX,             // 允许外部接口修改的flag范围
                    OPT_IS_GLOBAL,            // 是否是全局log的tag
                    OPT_MAX
                };
            };

        public:
            typedef log_formatter::level_t       level_t;
            typedef log_formatter::caller_info_t caller_info_t;

            typedef std::function<void(const caller_info_t &caller, const char *content, size_t content_size)> log_handler_t;
            typedef struct {
                level_t::type level_min;
                level_t::type level_max;
                log_handler_t handle;
            } log_router_t;

        private:
            struct construct_helper_t {};
            log_wrapper();

        public:
            log_wrapper(construct_helper_t &h);
            virtual ~log_wrapper();

        public:
            // 初始化
            int32_t init(level_t::type level = level_t::LOG_LW_DEBUG);

            static void update();

            void log(const caller_info_t &caller,
#ifdef _MSC_VER
                     _In_z_ _Printf_format_string_ const char *fmt, ...);
#elif (defined(__clang__) && __clang_major__ >= 3)
                     const char *fmt, ...) __attribute__((__format__(__printf__, 3, 4)));
#elif (defined(__GNUC__) && __GNUC__ >= 4)
// 格式检查(成员函数有个隐含的this参数)
#if defined(__MINGW32__) || defined(__MINGW64__)
                     const char *fmt, ...) __attribute__((format(__MINGW_PRINTF_FORMAT, 3, 4)));
#else
                     const char *fmt, ...) __attribute__((format(printf, 3, 4)));
#endif
#else
                     const char *fmt, ...);
#endif

            // 一般日志级别检查
            inline bool check_level(level_t::type level) const { return log_level_ >= level; }

            static inline bool check_level(const log_wrapper *logger, level_t::type level) {
                if (NULL == logger) {
                    return false;
                }
                return logger->log_level_ >= level;
            }

            size_t sink_size() const;

            /**
             * @brief 添加后端接口
             * @param h 输出函数
             * @param level_min 最低日志级别
             * @param level_min 最高日志级别
             */
            void add_sink(log_handler_t h, level_t::type level_min = level_t::LOG_LW_FATAL,
                          level_t::type level_max = level_t::LOG_LW_DEBUG);

            /**
             * @brief 移除最后一个后端接口
             */
            void pop_sink();

            /**
             * @brief 设置后端接口的日志级别
             * @param idx 后端接口索引
             * @param level_min 最低日志级别
             * @param level_min 最高日志级别
             * @return 如果没找到则返回false，成功返回true
             */
            bool set_sink(size_t idx, level_t::type level_min = level_t::LOG_LW_FATAL, level_t::type level_max = level_t::LOG_LW_DEBUG);

            /**
             * @brief 移除所有后端, std::function无法比较，所以只能全清
             */
            void clear_sinks();

            inline void set_level(level_t::type l) { log_level_ = l; }

            inline level_t::type get_level() const { return log_level_; }

            inline const std::string &set_prefix_format() const { return prefix_format_; }

            inline void set_prefix_format(const std::string &prefix) { prefix_format_ = prefix; }

            inline bool get_option(options_t::type t) const {
                if (t < options_t::OPT_MAX) {
                    return false;
                }

                return options_.test(t);
            };

            inline void set_option(options_t::type t, bool v) {
                if (t >= options_t::OPT_USER_MAX) {
                    return;
                }

                options_.set(t, v);
            };

            void                                                  set_stacktrace_level(level_t::type level_max = level_t::LOG_LW_DISABLED,
                                                                                       level_t::type level_min = level_t::LOG_LW_DISABLED);
            inline const std::pair<level_t::type, level_t::type> &get_stacktrace_level() const { return stacktrace_level_; }

            /**
             * @brief 实际写出到落地接口
             */
            void write_log(const caller_info_t &caller, const char *content, size_t content_size);

            // 白名单及用户指定日志输出可以针对哪个用户创建log_wrapper实例

            static log_wrapper *mutable_log_cat(uint32_t cats = categorize_t::DEFAULT);
            static ptr_t        create_user_logger();

        private:
            level_t::type                           log_level_;
            std::pair<level_t::type, level_t::type> stacktrace_level_;
            std::string                             prefix_format_;
            std::bitset<options_t::OPT_MAX>         options_;
            std::list<log_router_t>                 log_sinks_;
            mutable util::lock::spin_rw_lock        log_sinks_lock_;
        };
    } // namespace log
} // namespace util

#define WLOG_LEVELID(lv) static_cast<util::log::log_wrapper::level_t::type>(lv)

#define WDTLOGGETCAT(cat) util::log::log_wrapper::mutable_log_cat(cat)
#define WDTLOGFILENF(lv, name) util::log::log_wrapper::caller_info_t(lv, name, __FILE__, __LINE__, __FUNCTION__)

#define WLOG_INIT(cat, lv) NULL != WDTLOGGETCAT(cat) ? WDTLOGGETCAT(cat)->init(lv) : -1

#define WLOG_GETCAT(cat) util::log::log_wrapper::mutable_log_cat(cat)

// 按分类日志输出工具
#ifdef _MSC_VER

/** 全局日志输出工具 **/
#define WCLOGDEFLV(lv, lv_name, cat, ...) \
    if (util::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#define WCLOGTRACE(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_TRACE, NULL, cat, __VA_ARGS__)
#define WCLOGDEBUG(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_DEBUG, NULL, cat, __VA_ARGS__)
#define WCLOGNOTICE(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_NOTICE, NULL, cat, __VA_ARGS__)
#define WCLOGINFO(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_INFO, NULL, cat, __VA_ARGS__)
#define WCLOGWARNING(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_WARNING, NULL, cat, __VA_ARGS__)
#define WCLOGERROR(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_ERROR, NULL, cat, __VA_ARGS__)
#define WCLOGFATAL(cat, ...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_FATAL, NULL, cat, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 **/

#define WINSTLOGDEFLV(lv, lv_name, inst, ...) \
    if ((inst).check_level(lv)) (inst).log(WDTLOGFILENF(lv, lv_name), __VA_ARGS__);

#define WINSTLOGTRACE(inst, ...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_TRACE, NULL, inst, __VA_ARGS__)
#define WINSTLOGDEBUG(inst, ...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_DEBUG, NULL, inst, __VA_ARGS__)
#define WINSTLOGNOTICE(inst, ...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_NOTICE, NULL, inst, __VA_ARGS__)
#define WINSTLOGINFO(inst, ...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_INFO, NULL, inst, __VA_ARGS__)
#define WINSTLOGWARNING(inst, ...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_WARNING, NULL, inst, __VA_ARGS__)
#define WINSTLOGERROR(inst, ...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_ERROR, NULL, inst, __VA_ARGS__)
#define WINSTLOGFATAL(inst, ...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_FATAL, NULL, inst, __VA_ARGS__)

#else

/** 全局日志输出工具 **/
#define WCLOGDEFLV(lv, lv_name, cat, args...) \
    if (util::log::log_wrapper::check_level(WDTLOGGETCAT(cat), lv)) WDTLOGGETCAT(cat)->log(WDTLOGFILENF(lv, lv_name), ##args);

#define WCLOGTRACE(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_TRACE, NULL, __VA_ARGS__)
#define WCLOGDEBUG(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_DEBUG, NULL, __VA_ARGS__)
#define WCLOGNOTICE(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_NOTICE, NULL, __VA_ARGS__)
#define WCLOGINFO(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_INFO, NULL, __VA_ARGS__)
#define WCLOGWARNING(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_WARNING, NULL, __VA_ARGS__)
#define WCLOGERROR(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_ERROR, NULL, __VA_ARGS__)
#define WCLOGFATAL(...) WCLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_FATAL, NULL, __VA_ARGS__)

/** 对指定log_wrapper的日志输出工具 **/
#define WINSTLOGDEFLV(lv, lv_name, inst, args...) \
    if ((inst).check_level(lv)) (inst).log(WDTLOGFILENF(lv, lv_name), ##args);

#define WINSTLOGTRACE(...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_TRACE, NULL, __VA_ARGS__)
#define WINSTLOGDEBUG(...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_DEBUG, NULL, __VA_ARGS__)
#define WINSTLOGNOTICE(...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_NOTICE, NULL, __VA_ARGS__)
#define WINSTLOGINFO(...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_INFO, NULL, __VA_ARGS__)
#define WINSTLOGWARNING(...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_WARNING, NULL, __VA_ARGS__)
#define WINSTLOGERROR(...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_ERROR, NULL, __VA_ARGS__)
#define WINSTLOGFATAL(...) WINSTLOGDEFLV(util::log::log_wrapper::level_t::LOG_LW_FATAL, NULL, __VA_ARGS__)

#endif

// 默认日志输出工具
#define WLOGTRACE(...) WCLOGTRACE(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGDEBUG(...) WCLOGDEBUG(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGNOTICE(...) WCLOGNOTICE(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGINFO(...) WCLOGINFO(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGWARNING(...) WCLOGWARNING(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGERROR(...) WCLOGERROR(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)
#define WLOGFATAL(...) WCLOGFATAL(util::log::log_wrapper::categorize_t::DEFAULT, __VA_ARGS__)


// 控制台输出工具
#ifdef _MSC_VER
#define PSTDTERMCOLOR(os_ident, code, fmt, ...)                                        \
                                                                                       \
    {                                                                                  \
        util::cli::shell_stream::shell_stream_opr log_wrapper_pstd_ss(&std::os_ident); \
        log_wrapper_pstd_ss.open(code);                                                \
        log_wrapper_pstd_ss.close();                                                   \
        printf(fmt, __VA_ARGS__);                                                      \
    }

#else
#define PSTDTERMCOLOR(os_ident, code, fmt, args...)                                    \
                                                                                       \
    {                                                                                  \
        util::cli::shell_stream::shell_stream_opr log_wrapper_pstd_ss(&std::os_ident); \
        log_wrapper_pstd_ss.open(code);                                                \
        log_wrapper_pstd_ss.close();                                                   \
        printf(fmt, ##args);                                                           \
    }

#endif

#define PSTDINFO(...) printf(__VA_ARGS__)
#define PSTDNOTICE(...) PSTDTERMCOLOR(cout, util::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW, __VA_ARGS__)
#define PSTDWARNING(...)                                                                                                          \
    PSTDTERMCOLOR(cerr, util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD | util::cli::shell_font_style::SHELL_FONT_COLOR_YELLOW, \
                  __VA_ARGS__)
#define PSTDERROR(...) \
    PSTDTERMCOLOR(cerr, util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD | util::cli::shell_font_style::SHELL_FONT_COLOR_RED, __VA_ARGS__)
#define PSTDFATAL(...) PSTDTERMCOLOR(cerr, util::cli::shell_font_style::SHELL_FONT_COLOR_MAGENTA, __VA_ARGS__)
#define PSTDOK(...) PSTDTERMCOLOR(cout, util::cli::shell_font_style::SHELL_FONT_COLOR_GREEN, __VA_ARGS__)
//
#ifndef NDEBUG
#define PSTDTRACE(...) PSTDTERMCOLOR(cout, util::cli::shell_font_style::SHELL_FONT_COLOR_CYAN, __VA_ARGS__)
#define PSTDDEBUG(...) PSTDTERMCOLOR(cout, util::cli::shell_font_style::SHELL_FONT_COLOR_CYAN, __VA_ARGS__)
#define PSTDMARK                                                                                                               \
    PSTDTERMCOLOR(cout, util::cli::shell_font_style::SHELL_FONT_SPEC_BOLD | util::cli::shell_font_style::SHELL_FONT_COLOR_RED, \
                  "Mark: %s:%s (function %s)\n", __FILE__, __LINE__, __FUNCTION__)
#else
#define PSTDTRACE(...)
#define PSTDDEBUG(...)
#define PSTDMARK

#endif

#endif // _UTIL_LOG_LOG_WRAPPER_H_
