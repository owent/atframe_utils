
#include "time/time_utility.h"

namespace util {
    namespace time {
        time_utility::raw_time_t time_utility::now_;
        time_t time_utility::now_unix_;
        time_t time_utility::custom_zone_offset_ = -YEAR_SECONDS;

        time_utility::time_utility() {}
        time_utility::~time_utility() {}

        void time_utility::update(raw_time_t *t) {
            if (NULL != t) {
                now_ = *t;
            } else {
                now_ = std::chrono::system_clock::now();
            }

            now_unix_ = std::chrono::system_clock::to_time_t(now_);
        }

        time_utility::raw_time_t time_utility::now() { return now_; }

        time_t time_utility::get_now() { return now_unix_; }

        // ====================== 后面的函数都和时区相关 ======================
        time_t time_utility::get_sys_zone_offset() {
            time_t ret = 0;
            struct tm t;
            memset(&t, 0, sizeof(t));
            t.tm_year = 70;
            t.tm_mon = 0;
            t.tm_mday = 2; // VC 在时区offset是负数的时候会出错，所以改成从第二天开始然后减一天
            t.tm_hour = 0;
            t.tm_min = 0;
            t.tm_sec = 0;

            ret = mktime(&t);
            return ret - DAY_SECONDS;
        }

        time_t time_utility::get_zone_offset() {
            if (custom_zone_offset_ <= -YEAR_SECONDS) {
                return custom_zone_offset_ = get_sys_zone_offset();
            }

            return custom_zone_offset_;
        }

        void time_utility::set_zone_offset(time_t t) { custom_zone_offset_ = t; }

        time_t time_utility::get_today_now_offset() {
            time_t curr_time = get_now();
            curr_time -= get_sys_zone_offset();

            // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
            if (curr_time < 0) {
                // 保证返回值为正
                curr_time = DAY_SECONDS - (-curr_time) % DAY_SECONDS;
                return curr_time;
            } else {
                return curr_time % DAY_SECONDS;
            }
        }

        bool time_utility::is_same_day(time_t left, time_t right) {
            // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
            left -= get_zone_offset();
            right -= get_zone_offset();

            return left / DAY_SECONDS == right / DAY_SECONDS;
        }

        time_t time_utility::get_today_offset(time_t offset) {
            time_t now = get_now();
            now -= get_zone_offset();
            now -= now % DAY_SECONDS;

            // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
            return now + offset + get_zone_offset();
        }

        bool time_utility::is_same_month(time_t left, time_t right) {
            std::tm left_tm;
            std::tm right_tm;
            UTIL_STRFUNC_LOCALTIME_S(&left, &left_tm);
            UTIL_STRFUNC_LOCALTIME_S(&right, &right_tm);

            return left_tm.tm_year == right_tm.tm_year && left_tm.tm_mon == right_tm.tm_mon;
        }

        bool time_utility::is_same_week(time_t left, time_t right, time_t week_first) {
            left -= get_zone_offset();
            right -= get_zone_offset();

            // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
            // 1970年1月1日是周四
            // 周日是一周的第一天
            return (left - (week_first - 4) * DAY_SECONDS) / WEEK_SECONDS == (right - (week_first - 4) * DAY_SECONDS) / WEEK_SECONDS;
        }

        int time_utility::get_week_day(time_t t) {
            t -= get_zone_offset();

            // 仅考虑时区, 不是标准意义上的时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
            // 1970年1月1日是周四
            // 周日是一周的第一天
            t %= WEEK_SECONDS;
            t /= DAY_SECONDS;
            return static_cast<int>((t + 4) % 7);
        }
    }
}
