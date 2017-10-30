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

#ifndef UTIL_LOG_LOG_FORMATTER_H
#define UTIL_LOG_LOG_FORMATTER_H

#pragma once

#include <cstdlib>
#include <cstdio>
#include <stdint.h>
#include <string>
#include <inttypes.h>
#include <ctime>
#include <cstring>

namespace util {
    namespace log {
        /**
         * @brief 日志格式化数据
         */
        class log_formatter {
        public:
            struct flag_t {
                enum type {
                    INDEX = 0x01, // 日志rotation序号
                    DATE = 0x02,  // 日期
                    TIME = 0x04, // 时间
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

            struct caller_info_t {
                level_t::type level_id;
                const char *level_name;
                const char *file_path;
                uint32_t line_number;
                const char *func_name;
                uint32_t rotate_index;

                caller_info_t();
                caller_info_t(level_t::type lid, const char *lname, const char *fpath, uint32_t lnum, const char *fnname);
                caller_info_t(level_t::type lid, const char *lname, const char *fpath, uint32_t lnum, const char *fnname, uint32_t ridx);
            };

        public:
            static bool check(int32_t flags, int32_t checked);

            /**
             * @brief 格式化到缓冲区，如果缓冲区不足忽略后面的数据
             * @note 如果返回值大于0，本函数保证输出的数据结尾有'\0'，且返回的长度不计这个'\0'
             * @return 返回消耗的缓存区长度
             * @see http://en.cppreference.com/w/c/chrono/strftime
             * @note 支持的格式规则
             *            %Y:  	writes year as a 4 digit decimal number
             *            %y:   writes last 2 digits of year as a decimal number (range [00,99])
             *            %m:  	writes month as a decimal number (range [01,12])
             *            %j:   writes day of the year as a decimal number (range [001,366])
             *            %d:  	writes day of the month as a decimal number (range [01,31])
             *            %w:  	writes weekday as a decimal number, where Sunday is 0 (range [0-6])
             *            %H:   writes hour as a decimal number, 24 hour clock (range [00-23])
             *            %I:  	writes hour as a decimal number, 12 hour clock (range [01,12])
             *            %M:  	writes minute as a decimal number (range [00,59])
             *            %S:  	writes second as a decimal number (range [00,60])
             *            %F:  	equivalent to "%Y-%m-%d" (the ISO 8601 date format)
             *            %T:  	equivalent to "%H:%M:%S" (the ISO 8601 time format)
             *            %R:  	equivalent to "%H:%M"
             *            %f:  	小于秒的时间标识 TODO: 目前为了速度是clock中的单位为毫秒的部分，但不是真正的毫秒数，以后改成真正的毫秒数
             *            %L:  	日志级别名称
             *            %l:  	日志级别ID
             *            %s:  	调用处源码文件路径
             *            %k:  	调用处源码文件名
             *            %n:  	调用处源码行号
             *            %C:  	调用处函数名称
             *            %N:   轮询序号(仅在内部接口有效)
             */
            static size_t format(char *buff, size_t bufz, const char *fmt, size_t fmtz, const caller_info_t &caller);

            static bool check_rotation_var(const char *fmt, size_t fmtz);

            static bool has_format(const char *fmt, size_t fmtz);

            /**
             * @brief 设置工程目录，会影响format时的%s参数，如果文件路径以工程目录开头，则会用~替换
             */
            static void set_project_directory(const char* dirbuf, size_t dirsz);
        private:
            static struct tm *get_iso_tm();
            static std::string project_dir_;
        };
    }
}

#endif
