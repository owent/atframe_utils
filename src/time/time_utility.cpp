
// Copyright 2026 atframework

#include "time/time_utility.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace time {

namespace {
struct ATFW_UTIL_SYMBOL_LOCAL time_utility_global_shared_cache {
  // 当前时间
  time_utility::raw_time_t now = time_utility::raw_time_t::clock::now();

  // 当前时间(Unix时间戳)
  time_t now_unix = std::chrono::system_clock::to_time_t(time_utility::raw_time_t::clock::now());

  // 当前时间(微妙，非精确)
  int32_t now_usec = 0;

  // 当前时间(纳秒，非精确)
  int32_t now_nanos = 0;

  platform::cpu_time_counter now_cpu_counter = platform::cpu_time_counter::now();

  // 时区时间的人为偏移
  time_t custom_zone_offset = -time_utility::YEAR_SECONDS;

  // 时间的全局偏移（Debug功能）
  std::chrono::system_clock::duration global_now_offset = std::chrono::system_clock::duration::zero();
};

inline static time_utility_global_shared_cache &get_global_shared_cache() noexcept {
  static time_utility_global_shared_cache cache;
  return cache;
}
}  // namespace

time_utility::time_utility() noexcept {}
time_utility::~time_utility() noexcept {}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void time_utility::update(raw_time_t *t) noexcept {
  // raw_time_t prev_tp = get_global_shared_cache().now;
  time_utility_global_shared_cache &cache = get_global_shared_cache();
  if (nullptr != t) {
    cache.now = *t + cache.global_now_offset;
  } else {
    cache.now = std::chrono::system_clock::now() + cache.global_now_offset;
  }
  cache.now_cpu_counter = platform::cpu_time_counter::now();

  // reset unix timestamp
  cache.now_unix = std::chrono::system_clock::to_time_t(cache.now);

  // reset usec
  ATFRAMEWORK_UTILS_NAMESPACE_ID::time::time_utility::raw_time_t padding_time =
      ATFRAMEWORK_UTILS_NAMESPACE_ID::time::time_utility::raw_time_t::clock::from_time_t(cache.now_unix);
  std::chrono::nanoseconds::rep nanos = std::chrono::duration_cast<std::chrono::nanoseconds>(
                                            ATFRAMEWORK_UTILS_NAMESPACE_ID::time::time_utility::now() - padding_time)
                                            .count();

  if (nanos < 0) {
    nanos = 0;
  } else if (nanos >= 1000000000) {
    nanos = 999999999;
  }
  cache.now_nanos = static_cast<int32_t>(nanos);
  cache.now_usec = static_cast<int32_t>(nanos) / 1000;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_utility::raw_time_t time_utility::now() noexcept {
  return get_global_shared_cache().now;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD int32_t time_utility::get_now_usec() noexcept {
  return get_global_shared_cache().now_usec;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD int32_t time_utility::get_now_nanos() noexcept {
  return get_global_shared_cache().now_nanos;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_now() noexcept {
  return get_global_shared_cache().now_unix;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_utility::raw_time_t time_utility::sys_now() noexcept {
  return get_global_shared_cache().now - get_global_shared_cache().global_now_offset;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_sys_now() noexcept {
  return std::chrono::system_clock::to_time_t(sys_now());
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD platform::cpu_time_counter
time_utility::sys_now_cpu_counter() noexcept {
  return get_global_shared_cache().now_cpu_counter;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_utility::raw_time_t time_utility::sys_now_realtime() noexcept {
  auto &cache = get_global_shared_cache();
  auto now_cpu_counter = platform::cpu_time_counter::now();

  // 如果cpu counter轮转了则刷新一次时间缓存
  if ATFW_UTIL_UNLIKELY_CONDITION (cache.now_cpu_counter > now_cpu_counter) {
    update(nullptr);
    now_cpu_counter = platform::cpu_time_counter::now();
  }
  auto offset = now_cpu_counter - cache.now_cpu_counter;
  return sys_now() + offset.to_duration<std::chrono::system_clock::duration>();
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void time_utility::set_global_now_offset(
    const std::chrono::system_clock::duration &offset) noexcept {
  raw_time_t old_now = now() - get_global_shared_cache().global_now_offset;
  get_global_shared_cache().global_now_offset = offset;
  update(&old_now);
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD std::chrono::system_clock::duration
time_utility::get_global_now_offset() noexcept {
  return get_global_shared_cache().global_now_offset;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void time_utility::reset_global_now_offset() noexcept {
  raw_time_t old_now = now() - get_global_shared_cache().global_now_offset;
  get_global_shared_cache().global_now_offset = std::chrono::system_clock::duration::zero();
  update(&old_now);
}

// ====================== 后面的函数都和时区相关 ======================
ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_sys_zone_offset() noexcept {
  // 部分地区当前时间时区和70年不一样，所以要基于当前时间算
  time_t utc_timepoint = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  tm t;
  UTIL_STRFUNC_GMTIME_S(&utc_timepoint, &t);
  time_t local_timepoint = mktime(&t);
  return local_timepoint - utc_timepoint;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_zone_offset() noexcept {
  if (get_global_shared_cache().custom_zone_offset <= -YEAR_SECONDS) {
    return get_global_shared_cache().custom_zone_offset = get_sys_zone_offset();
  }

  return get_global_shared_cache().custom_zone_offset;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD void time_utility::set_zone_offset(time_t t) noexcept {
  get_global_shared_cache().custom_zone_offset = t;
}

ATFRAMEWORK_UTILS_API ATFW_UTIL_SANITIZER_NO_THREAD time_t time_utility::get_today_now_offset() noexcept {
  time_t curr_time = get_now();
  curr_time -= get_zone_offset();

  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  if (curr_time < 0) {
    // 保证返回值为正
    curr_time = DAY_SECONDS - ((-curr_time) % DAY_SECONDS);
    return curr_time;
  }

  return curr_time % DAY_SECONDS;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_day(time_t left, time_t right) noexcept {
  return is_same_day(left, right, 0);
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_day(time_t left, time_t right, time_t offset) noexcept {
  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  left -= get_zone_offset() + offset;
  right -= get_zone_offset() + offset;

  return left / DAY_SECONDS == right / DAY_SECONDS;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_greater_day(time_t left, time_t right) noexcept {
  return is_greater_day(left, right, 0);
}

ATFRAMEWORK_UTILS_API bool time_utility::is_greater_day(time_t left, time_t right, time_t offset) noexcept {
  if (left >= right) {
    return false;
  }

  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  left -= get_zone_offset() + offset;
  right -= get_zone_offset() + offset;

  return left / DAY_SECONDS < right / DAY_SECONDS;
}

ATFRAMEWORK_UTILS_API time_t time_utility::get_today_offset(time_t offset) noexcept {
  return get_any_day_offset(get_now(), offset);
}

ATFRAMEWORK_UTILS_API time_t time_utility::get_any_day_offset(time_t checked, time_t offset) noexcept {
  checked -= get_zone_offset();
  checked -= checked % DAY_SECONDS;

  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  return checked + offset + get_zone_offset();
}

ATFRAMEWORK_UTILS_API time_utility::raw_time_desc_t time_utility::get_local_tm(time_t t) noexcept {
  return get_gmt_tm(t - get_zone_offset());
}

ATFRAMEWORK_UTILS_API time_utility::raw_time_desc_t time_utility::get_gmt_tm(time_t t) noexcept {
  std::tm ttm;
  UTIL_STRFUNC_GMTIME_S(&t, &ttm);  // lgtm [cpp/potentially-dangerous-function]
  return ttm;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_leap_year(int year) noexcept {
  if (year & 0x03) {
    return false;
  }

  return year % 100 != 0 || (year % 400 == 0 && year % 3200 != 0) || year % 172800 == 0;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_year(time_t left, time_t right) noexcept {
  std::tm left_tm = get_local_tm(left);
  std::tm right_tm = get_local_tm(right);

  return left_tm.tm_year == right_tm.tm_year;
}

ATFRAMEWORK_UTILS_API int time_utility::get_year_day(time_t t) noexcept {
  std::tm ttm = get_local_tm(t);
  return ttm.tm_yday;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_month(time_t left, time_t right) noexcept {
  std::tm left_tm = get_local_tm(left);
  std::tm right_tm = get_local_tm(right);

  return left_tm.tm_year == right_tm.tm_year && left_tm.tm_mon == right_tm.tm_mon;
}

ATFRAMEWORK_UTILS_API int time_utility::get_month_day(time_t t) noexcept {
  std::tm ttm = get_local_tm(t);
  return ttm.tm_mday;
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_week(time_t left, time_t right, time_t week_first) noexcept {
  return is_same_week_point(left, right, 0, week_first);
}

ATFRAMEWORK_UTILS_API bool time_utility::is_same_week_point(time_t left, time_t right, time_t offset,
                                                            time_t week_first) noexcept {
  left -= get_zone_offset() + offset;
  right -= get_zone_offset() + offset;

  // 仅考虑时区, 不是标准意义上的当天时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  // 1970年1月1日是周四
  // 周日是一周的第一天
  return (left - ((week_first - 4) * DAY_SECONDS)) / WEEK_SECONDS ==
         (right - ((week_first - 4) * DAY_SECONDS)) / WEEK_SECONDS;
}

ATFRAMEWORK_UTILS_API int time_utility::get_week_day(time_t t) noexcept {
  t -= get_zone_offset();

  // 仅考虑时区, 不是标准意义上的时间，忽略记闰秒之类的偏移(偏移量很少，忽略不计吧)
  // 1970年1月1日是周四
  // 周日是一周的第一天
  t %= WEEK_SECONDS;
  t /= DAY_SECONDS;
  return static_cast<int>((t + 4) % 7);
}

ATFRAMEWORK_UTILS_API time_t time_utility::get_day_start_time(time_t t) noexcept {
  if (0 == t) {
    t = get_now();
  }

  return get_any_day_offset(t, 0);
}

ATFRAMEWORK_UTILS_API time_t time_utility::get_week_start_time(time_t t, time_t week_first) noexcept {
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

ATFRAMEWORK_UTILS_API time_t time_utility::get_month_start_time(time_t t) noexcept {
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
