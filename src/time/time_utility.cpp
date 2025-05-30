
#include "time/time_utility.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace time {
ATFRAMEWORK_UTILS_API time_utility::raw_time_t time_utility::now_;
ATFRAMEWORK_UTILS_API time_t time_utility::now_unix_;
ATFRAMEWORK_UTILS_API int32_t time_utility::now_usec_ = 0;
ATFRAMEWORK_UTILS_API int32_t time_utility::now_nanos_ = 0;
ATFRAMEWORK_UTILS_API time_t time_utility::custom_zone_offset_ = -time_utility::YEAR_SECONDS;
ATFRAMEWORK_UTILS_API std::chrono::system_clock::duration time_utility::global_now_offset_ =
    std::chrono::system_clock::duration::zero();

time_utility::time_utility() {}
time_utility::~time_utility() {}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void time_utility::update(raw_time_t *t) {
  // raw_time_t prev_tp = now_;
  if (nullptr != t) {
    now_ = *t + global_now_offset_;
  } else {
    now_ = std::chrono::system_clock::now() + global_now_offset_;
  }

  // reset unix timestamp
  now_unix_ = std::chrono::system_clock::to_time_t(now_);

  // reset usec
  ATFRAMEWORK_UTILS_NAMESPACE_ID::time::time_utility::raw_time_t padding_time =
      ATFRAMEWORK_UTILS_NAMESPACE_ID::time::time_utility::raw_time_t::clock::from_time_t(now_unix_);
  std::chrono::nanoseconds::rep nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                            ATFRAMEWORK_UTILS_NAMESPACE_ID::time::time_utility::now() - padding_time)
                                            .count();

  if (nanos < 0) {
    nanos = 0;
  } else if (nanos >= 1000000000) {
    nanos = 999999999;
  }
  now_nanos_ = static_cast<int32_t>(nanos);
  now_usec_ = static_cast<int32_t>(nanos) / 1000;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_utility::raw_time_t time_utility::now() { return now_; }

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD int32_t time_utility::get_now_usec() { return now_usec_; }

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD int32_t time_utility::get_now_nanos() { return now_nanos_; }

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_now() { return now_unix_; }

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_utility::raw_time_t time_utility::sys_now() {
  return now_ - global_now_offset_;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_sys_now() {
  return std::chrono::system_clock::to_time_t(sys_now());
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void time_utility::set_global_now_offset(
    const std::chrono::system_clock::duration &offset) {
  raw_time_t old_now = now() - global_now_offset_;
  global_now_offset_ = offset;
  update(&old_now);
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD std::chrono::system_clock::duration
time_utility::get_global_now_offset() {
  return global_now_offset_;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void time_utility::reset_global_now_offset() {
  raw_time_t old_now = now() - global_now_offset_;
  global_now_offset_ = std::chrono::system_clock::duration::zero();
  update(&old_now);
}

// ====================== 后面的函数都和时区相关 ======================
ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_sys_zone_offset() {
  // 部分地区当前时间时区和70年不一样，所以要基于当前时间算
  time_t utc_timepoint = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  tm t;
  UTIL_STRFUNC_GMTIME_S(&utc_timepoint, &t);
  time_t local_timepoint = mktime(&t);
  return local_timepoint - utc_timepoint;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_zone_offset() {
  if (custom_zone_offset_ <= -YEAR_SECONDS) {
    return custom_zone_offset_ = get_sys_zone_offset();
  }

  return custom_zone_offset_;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void time_utility::set_zone_offset(time_t t) {
  custom_zone_offset_ = t;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_today_now_offset() {
  time_t curr_time = get_now();
  curr_time -= get_zone_offset();

  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  if (curr_time < 0) {
    // 保证返回值为正
    curr_time = DAY_SECONDS - (-curr_time) % DAY_SECONDS;
    return curr_time;
  } else {
    return curr_time % DAY_SECONDS;
  }
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_day(time_t left, time_t right) { return is_same_day(left, right, 0); }

ATFRAMEWORK_UTILS_API bool time_utility::is_same_day(time_t left, time_t right, time_t offset) {
  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  left -= get_zone_offset() + offset;
  right -= get_zone_offset() + offset;

  return left / DAY_SECONDS == right / DAY_SECONDS;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_greater_day(time_t left, time_t right) {
  return is_greater_day(left, right, 0);
}

ATFRAMEWORK_UTILS_API bool time_utility::is_greater_day(time_t left, time_t right, time_t offset) {
  if (left >= right) {
    return false;
  }

  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  left -= get_zone_offset() + offset;
  right -= get_zone_offset() + offset;

  return left / DAY_SECONDS < right / DAY_SECONDS;
}

ATFRAMEWORK_UTILS_API time_t time_utility::get_today_offset(time_t offset) {
  return get_any_day_offset(get_now(), offset);
}

ATFRAMEWORK_UTILS_API time_t time_utility::get_any_day_offset(time_t checked, time_t offset) {
  checked -= get_zone_offset();
  checked -= checked % DAY_SECONDS;

  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  return checked + offset + get_zone_offset();
}

ATFRAMEWORK_UTILS_API time_utility::raw_time_desc_t time_utility::get_local_tm(time_t t) {
  return get_gmt_tm(t - get_zone_offset());
}

ATFRAMEWORK_UTILS_API time_utility::raw_time_desc_t time_utility::get_gmt_tm(time_t t) {
  std::tm ttm;
  UTIL_STRFUNC_GMTIME_S(&t, &ttm);  // lgtm [cpp/potentially-dangerous-function]
  return ttm;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_leap_year(int year) {
  if (year & 0x03) {
    return false;
  }

  return year % 100 != 0 || (year % 400 == 0 && year % 3200 != 0) || year % 172800 == 0;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_year(time_t left, time_t right) {
  std::tm left_tm = get_local_tm(left);
  std::tm right_tm = get_local_tm(right);

  return left_tm.tm_year == right_tm.tm_year;
}

ATFRAMEWORK_UTILS_API int time_utility::get_year_day(time_t t) {
  std::tm ttm = get_local_tm(t);
  return ttm.tm_yday;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_month(time_t left, time_t right) {
  std::tm left_tm = get_local_tm(left);
  std::tm right_tm = get_local_tm(right);

  return left_tm.tm_year == right_tm.tm_year && left_tm.tm_mon == right_tm.tm_mon;
}

ATFRAMEWORK_UTILS_API int time_utility::get_month_day(time_t t) {
  std::tm ttm = get_local_tm(t);
  return ttm.tm_mday;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_week(time_t left, time_t right, time_t week_first) {
  return is_same_week_point(left, right, 0, week_first);
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_week_point(time_t left, time_t right, time_t offset,
                                                            time_t week_first) {
  left -= get_zone_offset() + offset;
  right -= get_zone_offset() + offset;

  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  // 1970年1月1日是周四
  // 周日是一周的第一天
  return (left - (week_first - 4) * DAY_SECONDS) / WEEK_SECONDS ==
         (right - (week_first - 4) * DAY_SECONDS) / WEEK_SECONDS;
}

ATFRAMEWORK_UTILS_API int time_utility::get_week_day(time_t t) {
  t -= get_zone_offset();

  // 仅考虑时区, 不是标准意义上的时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  // 1970年1月1日是周四
  // 周日是一周的第一天
  t %= WEEK_SECONDS;
  t /= DAY_SECONDS;
  return static_cast<int>((t + 4) % 7);
}

ATFRAMEWORK_UTILS_API time_t time_utility::get_day_start_time(time_t t) {
  if (0 == t) {
    t = get_now();
  }

  return get_any_day_offset(t, 0);
}

ATFRAMEWORK_UTILS_API time_t time_utility::get_week_start_time(time_t t, time_t week_first) {
  if (0 == t) {
    t = get_now();
  }

  time_t ct = t - get_zone_offset();

  if (week_first >= 7 || week_first < 0) {
    week_first %= 7;
  }

  ct += (4 - week_first) * DAY_SECONDS;
  ct %= WEEK_SECONDS;
  return t - ct;
}

ATFRAMEWORK_UTILS_API time_t time_utility::get_month_start_time(time_t t) {
  if (0 == t) {
    t = get_now();
  }

  // Maybe we have change the default offset
  time_t local_offset = get_zone_offset() - get_sys_zone_offset();

  std::tm ttm = get_local_tm(t - local_offset);
  ttm.tm_sec = 0;
  ttm.tm_min = 0;
  ttm.tm_hour = 0;
  ttm.tm_mday = 1;
  return mktime(&ttm) + local_offset;
}
}  // namespace time
ATFRAMEWORK_UTILS_NAMESPACE_END
