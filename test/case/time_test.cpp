// Copyright 2021 atframework

#include <algorithm>
#include <cstring>
#include <ctime>

#include <time/jiffies_timer.h>
#include "frame/test_macros.h"
#include "time/time_utility.h"

CASE_TEST(time_test, global_offset) {
  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  time_t now = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::set_global_now_offset(
      std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(5)));
  CASE_EXPECT_EQ(now + 5, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now());
  CASE_EXPECT_EQ(std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(5)).count(),
                 LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_global_now_offset().count());
  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::reset_global_now_offset();
  CASE_EXPECT_EQ(now, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now());
}

CASE_TEST(time_test, sys_now) {
  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  time_t now = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::set_global_now_offset(
      std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(5)));
  CASE_EXPECT_EQ(now + 5, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now());
  CASE_EXPECT_EQ(std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(5)).count(),
                 LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_global_now_offset().count());

  CASE_EXPECT_EQ(now, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_sys_now());

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::reset_global_now_offset();
  CASE_EXPECT_EQ(now, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now());
}

CASE_TEST(time_test, zone_offset) {
  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  time_t offset = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_sys_zone_offset() -
                  5 * LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::HOUR_SECONDS;
  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::set_zone_offset(offset);
  CASE_EXPECT_EQ(offset, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_zone_offset());
  CASE_EXPECT_NE(offset, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_sys_zone_offset());

  // 恢复时区设置
  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::set_zone_offset(
      LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_sys_zone_offset());
}

CASE_TEST(time_test, sys_zone_offset) {
  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  time_t offset = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_sys_zone_offset();
  time_t now = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_sys_now();

  tm local_tm, gmt_tm;
  time_t local_now = now;
  time_t gmt_now = now - offset;
  UTIL_STRFUNC_LOCALTIME_S(&local_now, &local_tm);
  UTIL_STRFUNC_GMTIME_S(&gmt_now, &gmt_tm);

  CASE_EXPECT_EQ(local_tm.tm_year, gmt_tm.tm_yday);
  CASE_EXPECT_EQ(local_tm.tm_mon, gmt_tm.tm_mon);
  CASE_EXPECT_EQ(local_tm.tm_mday, gmt_tm.tm_mday);
  CASE_EXPECT_EQ(local_tm.tm_hour, gmt_tm.tm_hour);
  CASE_EXPECT_EQ(local_tm.tm_min, gmt_tm.tm_min);
  CASE_EXPECT_EQ(local_tm.tm_sec, gmt_tm.tm_sec);
}

CASE_TEST(time_test, today_offset) {
  using std::abs;
  struct tm tobj;
  time_t tnow, loffset, cnow;
  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();

  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);
  loffset = tobj.tm_hour * LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::HOUR_SECONDS +
            tobj.tm_min * LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::MINITE_SECONDS + tobj.tm_sec;
  cnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_today_offset(loffset);

  // 只有闰秒误差，肯定在5秒以内
  // 容忍夏时令误差，所以要加一小时
  time_t toff = (cnow + LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::DAY_SECONDS - tnow) %
                LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::DAY_SECONDS;
  if (tobj.tm_isdst > 0) {
    CASE_EXPECT_LE(toff, 3605);
  } else {
    CASE_EXPECT_LE(toff, 5);
  }
}

CASE_TEST(time_test, is_same_day) {
  struct tm tobj;
  time_t lt, rt;
  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  lt = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&lt, &tobj);

  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 5;
  lt = mktime(&tobj);
  rt = lt + 5;
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, rt));

  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 55;
  rt = mktime(&tobj);
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, rt));

  lt = rt - 5;
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, rt));
  // 容忍夏时令误差
  lt = rt + 3610;
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, rt));
}

CASE_TEST(time_test, is_same_day_with_offset) {
  struct tm tobj;
  time_t lt, rt;
  int zero_hore = 5;
  time_t day_offset = zero_hore * LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::HOUR_SECONDS;

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  lt = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&lt, &tobj);

  tobj.tm_isdst = 0;
  tobj.tm_hour = zero_hore;
  tobj.tm_min = 0;
  tobj.tm_sec = 5;
  lt = mktime(&tobj);
  rt = lt + 5;
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, rt, day_offset));

  tobj.tm_isdst = 0;
  tobj.tm_hour = zero_hore - 1;
  tobj.tm_min = 59;
  tobj.tm_sec = 55;
  rt = mktime(&tobj);
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, rt, day_offset));
}

CASE_TEST(time_test, is_same_week) {
  struct tm tobj;
  time_t lt, rt, tnow;

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 5;
  lt = mktime(&tobj);
  lt -= LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::DAY_SECONDS * tobj.tm_wday;
  rt = lt + LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::WEEK_SECONDS;
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_week(lt, rt));

  rt -= 10;
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_week(lt, rt));
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_week(lt, tnow));
}

CASE_TEST(time_test, get_week_day) {
  struct tm tobj;
  time_t lt, rt, tnow;

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

  bool isdst = tobj.tm_isdst > 0;

  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 5;
  lt = mktime(&tobj);

  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 55;
  rt = mktime(&tobj);

  CASE_MSG_INFO() << "lt=" << lt << ",tnow=" << tnow << ",rt=" << rt << std::endl;
  // 夏时令会导致lt和rt可能提前一天
  if (isdst && lt > tnow + 5) {
    lt -= LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::DAY_SECONDS;
    rt -= LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::DAY_SECONDS;
  }

  CASE_EXPECT_EQ(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_week_day(lt),
                 LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_week_day(tnow));
  CASE_EXPECT_EQ(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_week_day(lt),
                 LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_week_day(rt));
}

CASE_TEST(time_test, is_same_year) {
  // nothing todo use libc now
  struct tm tobj;
  time_t lt, rt, tnow;

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

  tobj.tm_mday = 1;
  tobj.tm_mon = 0;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 1;
  lt = mktime(&tobj);

  tobj.tm_yday = 364;
  // 闰年多一天
  if (LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_leap_year(tobj.tm_year + 1900)) {
    ++tobj.tm_yday;
  }
  tobj.tm_mon = 11;
  tobj.tm_mday = 31;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 58;
  rt = mktime(&tobj);

  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_year(lt, rt));
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_year(lt, rt + 3));
}

CASE_TEST(time_test, get_year_day) {
  struct tm tobj;
  time_t lt, rt, tnow;

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

  tobj.tm_yday = 0;
  tobj.tm_mon = 0;
  tobj.tm_mday = 1;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 1;
  lt = mktime(&tobj);

  tobj.tm_yday = 364;
  // 闰年多一天
  if (LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_leap_year(tobj.tm_year + 1900)) {
    ++tobj.tm_yday;
  }
  tobj.tm_mon = 11;
  tobj.tm_mday = 31;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 58;
  rt = mktime(&tobj);

  CASE_EXPECT_EQ(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_year_day(lt), 0);
  CASE_EXPECT_EQ(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_year_day(rt), tobj.tm_yday);
  CASE_EXPECT_EQ(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_year_day(rt + 3), 0);
}

CASE_TEST(time_test, is_same_month) {
  struct tm tobj;
  time_t lt, rt, tnow;

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

  tobj.tm_mon = 7;
  tobj.tm_mday = 1;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 1;
  lt = mktime(&tobj);

  tobj.tm_mday = 31;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 58;
  rt = mktime(&tobj);

  // nothing todo use libc now
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_month(lt, rt));
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_month(lt, rt + 3));
}

CASE_TEST(time_test, get_month_day) {
  struct tm tobj;
  time_t lt, rt, tnow;

  LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::update();
  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

  tobj.tm_mday = 1;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 1;
  lt = mktime(&tobj);

  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 58;
  rt = mktime(&tobj);

  CASE_EXPECT_EQ(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(lt),
                 LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(rt));
  CASE_EXPECT_NE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(lt),
                 LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(rt + 3));
  CASE_EXPECT_EQ(1, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(rt));
  CASE_EXPECT_EQ(2, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(rt + 3));

  tobj.tm_mon = 7;
  tobj.tm_mday = 31;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 1;
  lt = mktime(&tobj);

  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 58;
  rt = mktime(&tobj);

  CASE_EXPECT_EQ(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(lt),
                 LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(rt));
  CASE_EXPECT_NE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(lt),
                 LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(rt + 3));
  CASE_EXPECT_EQ(31, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(rt));
  CASE_EXPECT_EQ(1, LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_day(rt + 3));
}

CASE_TEST(time_test, get_day_start_time) {
  time_t lt, rt, tnow;
  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  lt = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_day_start_time(tnow);
  rt = lt + LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::DAY_SECONDS;

  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, tnow));
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, lt + 1));
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, lt - 1));
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, rt));
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_day(lt, rt - 1));
}

CASE_TEST(time_test, get_week_start_time) {
  time_t lt, rt, tnow;
  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  lt = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_week_start_time(tnow, 1);
  rt = lt + LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::WEEK_SECONDS;

  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_week(lt, tnow, 1));
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_week(lt, lt + 1, 1));
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_week(lt, lt - 1, 1));
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_week(lt, rt, 1));
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_week(lt, rt - 1, 1));
}

CASE_TEST(time_test, get_month_start_time) {
  time_t lt, rt, tnow;
  tnow = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_now();
  lt = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_start_time(tnow);
  rt = LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::get_month_start_time(
      lt + 32 * LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::DAY_SECONDS);

  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_month(lt, tnow));
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_month(lt, lt + 1));
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_month(lt, lt - 1));
  CASE_EXPECT_FALSE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_month(lt, rt));
  CASE_EXPECT_TRUE(LIBATFRAME_UTILS_NAMESPACE_ID::time::time_utility::is_same_month(lt, rt - 1));
}

using short_timer_t = LIBATFRAME_UTILS_NAMESPACE_ID::time::jiffies_timer<6, 3, 4>;
struct jiffies_timer_fn {
  void *check_priv_data;
  jiffies_timer_fn(void *pd) : check_priv_data(pd) {}

  void operator()(time_t, const short_timer_t::timer_t &timer) {
    if (nullptr != check_priv_data) {
      CASE_EXPECT_EQ(short_timer_t::get_timer_private_data(timer), check_priv_data);
    } else if (nullptr != short_timer_t::get_timer_private_data(timer)) {
      ++(*reinterpret_cast<int *>(short_timer_t::get_timer_private_data(timer)));
    }

    CASE_MSG_INFO() << "jiffies_timer " << short_timer_t::get_timer_sequence(timer) << " actived" << std::endl;
  }
};

CASE_TEST(time_test, jiffies_timer_basic) {
  short_timer_t short_timer;
  int count = 0;
  time_t max_tick = short_timer.get_max_tick_distance() + 1;

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_NOT_INITED,
                 short_timer.add_timer(123, jiffies_timer_fn(nullptr), nullptr));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_NOT_INITED, short_timer.tick(456));

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.init(max_tick));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_ALREADY_INITED, short_timer.init(max_tick));

  CASE_EXPECT_EQ(32767, short_timer.get_max_tick_distance());
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_TIMEOUT_EXTENDED,
                 short_timer.add_timer(short_timer.get_max_tick_distance() + 1, jiffies_timer_fn(nullptr), nullptr));

  // 理论上会被放在（数组下标: 2^6*3=192）
  CASE_EXPECT_EQ(
      short_timer_t::error_type_t::EN_JTET_SUCCESS,
      short_timer.add_timer(short_timer.get_max_tick_distance(), jiffies_timer_fn(&short_timer), &short_timer));
  // 理论上会被放在（数组下标: 2^6*4-1=255）
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(short_timer.get_max_tick_distance() - short_timer_t::LVL_GRAN(3),
                                       jiffies_timer_fn(&short_timer), &short_timer));

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(-123, jiffies_timer_fn(nullptr), &count));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(30, jiffies_timer_fn(nullptr), &count));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(40, jiffies_timer_fn(nullptr), &count));
  CASE_EXPECT_EQ(5, static_cast<int>(short_timer.size()));

  CASE_EXPECT_EQ(0, count);
  short_timer.tick(max_tick);
  CASE_EXPECT_EQ(0, count);
  short_timer.tick(max_tick + 1);
  CASE_EXPECT_EQ(1, count);
  CASE_EXPECT_EQ(4, static_cast<int>(short_timer.size()));

  // +30的触发点是+31。因为添加定时器的时候会认为当前时间是+0.XXX，并且由于触发只会延后不会提前
  short_timer.tick(max_tick + 30);
  CASE_EXPECT_EQ(1, count);
  short_timer.tick(max_tick + 31);
  CASE_EXPECT_EQ(2, count);

  // 跨tick
  short_timer.tick(max_tick + 64);
  CASE_EXPECT_EQ(3, count);
  CASE_EXPECT_EQ(2, static_cast<int>(short_timer.size()));

  // 非第一层、非第一个定时器组.（512+64*5-1 = 831）
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(831, jiffies_timer_fn(nullptr), &count));
  short_timer.tick(max_tick + 64 + 831);
  CASE_EXPECT_EQ(3, count);
  // 768-831 share tick
  short_timer.tick(max_tick + 64 + 832);
  CASE_EXPECT_EQ(4, count);

  // 32767
  short_timer.tick(32767 + max_tick);

  // 这个应该会放在数组下标为0的位置
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(0, jiffies_timer_fn(nullptr), &count));

  // 环状数组的复用
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(1000, jiffies_timer_fn(nullptr), &count));

  // 全部执行掉
  short_timer.tick(32767 + max_tick + 2000);
  CASE_EXPECT_EQ(6, count);
  CASE_EXPECT_EQ(0, static_cast<int>(short_timer.size()));
}

CASE_TEST(time_test, jiffies_timer_slot) {
  size_t timer_list_count[short_timer_t::WHEEL_SIZE] = {0};
  time_t blank_area[short_timer_t::WHEEL_SIZE / short_timer_t::LVL_SIZE] = {0};
  time_t max_tick = short_timer_t::get_max_tick_distance();
  for (time_t i = 0; i <= max_tick; ++i) {
    size_t idx = short_timer_t::calc_wheel_index(i, 0);
    CASE_EXPECT_LT(idx, static_cast<size_t>(short_timer_t::WHEEL_SIZE));
    ++timer_list_count[idx];
  }

  // 每个idx的计数器测试
  for (size_t i = 0; i < short_timer_t::WHEEL_SIZE; ++i) {
    size_t timer_count = timer_list_count[i];
    // 除了第一个定时器区间外，每个定时器区间都有一段空白区域
    if (0 == timer_count) {
      ++blank_area[i / short_timer_t::LVL_SIZE];
    } else {
      CASE_EXPECT_EQ(timer_count, static_cast<size_t>(short_timer_t::LVL_GRAN(i / short_timer_t::LVL_SIZE)));
    }
  }

  // 定时器空白区间的个数应该等于重合区域
  for (size_t i = 1; i < short_timer_t::WHEEL_SIZE / short_timer_t::LVL_SIZE; ++i) {
    CASE_EXPECT_EQ(blank_area[i], short_timer_t::LVL_START(i) / short_timer_t::LVL_GRAN(i));
  }
}

CASE_TEST(time_test, jiffies_timer_remove) {
  short_timer_t short_timer;
  int count = 0;
  time_t max_tick = short_timer.get_max_tick_distance() + 1;

  short_timer_t::timer_wptr_t timers[5];

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_NOT_INITED,
                 short_timer.add_timer(123, jiffies_timer_fn(nullptr), nullptr));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_NOT_INITED, short_timer.tick(456));

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.init(max_tick));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_ALREADY_INITED, short_timer.init(max_tick));

  CASE_EXPECT_EQ(32767, short_timer.get_max_tick_distance());
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_TIMEOUT_EXTENDED,
                 short_timer.add_timer(short_timer.get_max_tick_distance() + 1, jiffies_timer_fn(nullptr), nullptr));

  // 理论上会被放在（数组下标: 2^6*3=192）
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(short_timer.get_max_tick_distance(), jiffies_timer_fn(&short_timer),
                                       &short_timer, &timers[0]));
  // 理论上会被放在（数组下标: 2^6*4-1=255）
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(short_timer.get_max_tick_distance() - short_timer_t::LVL_GRAN(3),
                                       jiffies_timer_fn(&short_timer), &short_timer, &timers[1]));

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(-123, jiffies_timer_fn(nullptr), &count, &timers[2]));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(30, jiffies_timer_fn(nullptr), &count, &timers[3]));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(40, jiffies_timer_fn(nullptr), &count, &timers[4]));
  CASE_EXPECT_EQ(5, static_cast<int>(short_timer.size()));

  short_timer_t::set_timer_flags(*timers[4].lock(), short_timer_t::timer_flag_t::EN_JTTF_DISABLED);
  short_timer_t::remove_timer(*timers[2].lock());
  short_timer_t::remove_timer(*timers[3].lock());

  short_timer.tick(max_tick + 1);
  CASE_EXPECT_EQ(0, count);
  CASE_EXPECT_EQ(3, static_cast<int>(short_timer.size()));

  short_timer.tick(max_tick + 31);
  // 跨tick
  short_timer.tick(max_tick + 64);
  CASE_EXPECT_EQ(0, count);
  CASE_EXPECT_EQ(2, static_cast<int>(short_timer.size()));

  short_timer_t::remove_timer(*timers[0].lock());
  CASE_EXPECT_EQ(1, static_cast<int>(short_timer.size()));

  // 全部执行掉
  short_timer.tick(32767 + max_tick);
  CASE_EXPECT_EQ(0, count);
  CASE_EXPECT_EQ(0, static_cast<int>(short_timer.size()));
}

struct jiffies_timer_remove_in_callback_fn {
  jiffies_timer_remove_in_callback_fn() {}

  void operator()(time_t, const short_timer_t::timer_t &timer) {
    CASE_MSG_INFO() << "jiffies_timer " << short_timer_t::get_timer_sequence(timer) << " removed in callback"
                    << std::endl;
  }
};

CASE_TEST(time_test, jiffies_timer_remove_in_callback) {
  short_timer_t short_timer;
  time_t max_tick = short_timer.get_max_tick_distance() + 1;
  short_timer_t::timer_wptr_t timer_holer;
  short_timer_t::timer_ptr_t timer_ptr;

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.init(max_tick));

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(0, jiffies_timer_remove_in_callback_fn(), nullptr));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(30, jiffies_timer_remove_in_callback_fn(), nullptr));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(40, jiffies_timer_remove_in_callback_fn(), &short_timer, &timer_holer));
  CASE_EXPECT_EQ(3, static_cast<int>(short_timer.size()));

  timer_ptr = timer_holer.lock();
  CASE_EXPECT_EQ(&short_timer, short_timer_t::set_timer_private_data(*timer_ptr, &timer_holer));
  CASE_EXPECT_EQ(&timer_holer, short_timer_t::get_timer_private_data(*timer_ptr));

  short_timer.tick(max_tick + 1);
  CASE_EXPECT_EQ(2, static_cast<int>(short_timer.size()));

  short_timer.tick(max_tick + 64);
  CASE_EXPECT_EQ(0, static_cast<int>(short_timer.size()));

  // 重复调用remove_timer是安全的
  short_timer_t::remove_timer(*timer_ptr);
}

struct jiffies_timer_add_in_callback_fn {
  jiffies_timer_add_in_callback_fn() {}

  void operator()(time_t, const short_timer_t::timer_t &timer) {
    CASE_MSG_INFO() << "jiffies_timer " << short_timer_t::get_timer_sequence(timer) << " add timer in callback"
                    << std::endl;
    short_timer_t *owner = reinterpret_cast<short_timer_t *>(short_timer_t::get_timer_private_data(timer));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   owner->add_timer(0, jiffies_timer_fn(nullptr), nullptr));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   owner->add_timer(short_timer_t::LVL_SIZE - 5 - 1, jiffies_timer_fn(nullptr), nullptr));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   owner->add_timer(short_timer_t::LVL_SIZE - 5, jiffies_timer_fn(nullptr), nullptr));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   owner->add_timer(short_timer_t::LVL_SIZE - 1, jiffies_timer_fn(nullptr), nullptr));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   owner->add_timer(short_timer_t::LVL_SIZE, jiffies_timer_fn(nullptr), nullptr));
  }
};

CASE_TEST(time_test, jiffies_timer_add_in_callback) {
  short_timer_t short_timer;
  time_t max_tick = short_timer.get_max_tick_distance() + 1;
  short_timer_t::timer_wptr_t timer_holer;
  short_timer_t::timer_ptr_t timer_ptr;

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.init(max_tick));

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(5, jiffies_timer_add_in_callback_fn(), &short_timer, &timer_holer));
  CASE_EXPECT_EQ(1, static_cast<int>(short_timer.size()));

  timer_ptr = timer_holer.lock();
  CASE_EXPECT_EQ(&short_timer, short_timer_t::get_timer_private_data(*timer_ptr));

  short_timer.tick(max_tick + 6);
  CASE_EXPECT_EQ(5, static_cast<int>(short_timer.size()));

  short_timer.tick(max_tick + 6 + short_timer_t::LVL_SIZE);
  CASE_EXPECT_EQ(1, static_cast<int>(short_timer.size()));

  short_timer.tick(max_tick + 6 + short_timer_t::LVL_SIZE + short_timer_t::LVL_CLK_DIV);
  CASE_EXPECT_EQ(0, static_cast<int>(short_timer.size()));
}
