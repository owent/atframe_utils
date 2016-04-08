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

#ifndef _UTIL_TIME_TIME_UTILITY_H_
#define _UTIL_TIME_TIME_UTILITY_H_

#pragma once

#include <cstddef>
#include <cstring>
#include <ctime>
#include <stdint.h>

#include "std/chrono.h"

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || defined(__STDC_LIB_EXT1__)
#define UTIL_STRFUNC_LOCALTIME_S(time_t_ptr, tm_ptr) localtime_s(time_t_ptr, tm_ptr)
#define UTIL_STRFUNC_GMTIME_S(time_t_ptr, tm_ptr) gmtime_s(time_t_ptr, tm_ptr)

#elif defined(_MSC_VER) && _MSC_VER >= 1300
#define UTIL_STRFUNC_LOCALTIME_S(time_t_ptr, tm_ptr) localtime_s(tm_ptr, time_t_ptr)
#define UTIL_STRFUNC_GMTIME_S(time_t_ptr, tm_ptr) gmtime_s(tm_ptr, time_t_ptr)

#elif defined(__STDC_VERSION__)
#define UTIL_STRFUNC_LOCALTIME_S(time_t_ptr, tm_ptr) localtime_r(time_t_ptr, tm_ptr)
#define UTIL_STRFUNC_GMTIME_S(time_t_ptr, tm_ptr) gmtime_r(time_t_ptr, tm_ptr)

#else
#define UTIL_STRFUNC_LOCALTIME_S(time_t_ptr, tm_ptr) (*(tm_ptr) = *localtime(time_t_ptr))
#define UTIL_STRFUNC_GMTIME_S(time_t_ptr, tm_ptr) (*(tm_ptr) = gmtime(time_t_ptr))

#endif

namespace util {
    namespace time {
        class time_utility {
        public:
            enum {
                MINITE_SECONDS = 60,
                HOUR_SECONDS = MINITE_SECONDS * 60,
                DAY_SECONDS = HOUR_SECONDS * 24,
                WEEK_SECONDS = DAY_SECONDS * 7,
                YEAR_SECONDS = DAY_SECONDS * 365,
            };

            typedef std::chrono::system_clock::time_point raw_time_t;

        private:
            time_utility();
            ~time_utility();

        public:
            /**
             * @brief 更新时间
             * @param 可以指定时间对象
             */
            static void update(raw_time_t *t = NULL);

            /**
             * @brief 获取原始系统时间对象
             * @return 最后一次update的时间
             */
            static raw_time_t now();

            /**
             * @brief 获取当前Unix时间戳
             * @return 最后一次update的时间
             */
            static time_t get_now();

            // ====================== 后面的函数都和时区相关 ======================
            /**
             * @brief 获取系统时区时间偏移(忽略自定义偏移)
             * @return 系统时区的时间偏移
             */
            static time_t get_sys_zone_offset();

            /**
             * @brief 获取时区时间偏移（如果设置过自定义偏移，使用自定义的值，否则和get_sys_zone_offset()的结果一样)
             * @return 时区的时间偏移
             */
            static time_t get_zone_offset();

            /**
             * @brief 设置时区时间偏移
             * @param t 可以指定时区时间偏移
             * @note 比如要设置凌晨5点视为0点: set_zone_offset(get_sys_zone_offset() - 5 * HOUR_SECONDS)
             */
            static void set_zone_offset(time_t t);

            /**
             * @brief 获取当前时区今天过了多少秒
             * @return 目前时间的相对今天的偏移
             */
            static time_t get_today_now_offset();

            /**
             * @brief [快速非严格]判定当前时区，两个时间是否是同一天
             * @param left (必须大于0)
             * @param right (必须大于0)
             * @return 同一天返回 true
             */
            static bool is_same_day(time_t left, time_t right);

            /**
             * @brief 获取当前时区相对于今天零点之后offset秒的Unix时间戳
             * @param offset 时间偏移值
             * @return 今天0点后offset的时间戳
             */
            static time_t get_today_offset(time_t offset);

            /**
             * @brief 判定当前时区时间是否是同一个月
             * @return 同一月返回 true
             */
            static bool is_same_month(time_t left, time_t right);

            /**
             * @brief [快速非严格]判定当前时区时间是否是同一个周
             * @param left (必须大于0)
             * @param right (必须大于0)
             * @param week_first 一周的第一天，0表示周日，1表示周一，以此类推
             * @return 同一周返回 true
             */
            static bool is_same_week(time_t left, time_t right, time_t week_first = 0);

            /**
             * @brief [快速非严格]判定当前时区时间是本周第几天
             * @param t (必须大于0)
             * @return 本周第几天，周日返回0,周一返回1,周二返回2,以此类推
             */
            static int get_week_day(time_t t);

        private:
            // 当前时间
            static raw_time_t now_;
            // 当前时间
            static time_t now_unix_;
            // 时区时间的人为偏移
            static time_t custom_zone_offset_;
        };
    }
}

#endif // _UTIL_TIME_TIME_UTILITY_H_
