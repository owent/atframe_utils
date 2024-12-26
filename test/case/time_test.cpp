// Copyright 2021 atframework

#include <algorithm>
#include <cstring>
#include <ctime>

#include <time/jiffies_timer.h>
#include "frame/test_macros.h"
#include "time/time_utility.h"

CASE_TEST(time_test, global_offset) {
  atfw::util::time::time_utility::update();
  time_t now = atfw::util::time::time_utility::get_now();

  atfw::util::time::time_utility::set_global_now_offset(
      std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(5)));
  CASE_EXPECT_EQ(now + 5, atfw::util::time::time_utility::get_now());
  CASE_EXPECT_EQ(std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(5)).count(),
                 atfw::util::time::time_utility::get_global_now_offset().count());
  atfw::util::time::time_utility::reset_global_now_offset();
  CASE_EXPECT_EQ(now, atfw::util::time::time_utility::get_now());
}

CASE_TEST(time_test, sys_now) {
  atfw::util::time::time_utility::update();
  time_t now = atfw::util::time::time_utility::get_now();

  atfw::util::time::time_utility::set_global_now_offset(
      std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(5)));
  CASE_EXPECT_EQ(now + 5, atfw::util::time::time_utility::get_now());
  CASE_EXPECT_EQ(std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::seconds(5)).count(),
                 atfw::util::time::time_utility::get_global_now_offset().count());

  CASE_EXPECT_EQ(now, atfw::util::time::time_utility::get_sys_now());

  atfw::util::time::time_utility::reset_global_now_offset();
  CASE_EXPECT_EQ(now, atfw::util::time::time_utility::get_now());
}

CASE_TEST(time_test, zone_offset) {
  atfw::util::time::time_utility::update();
  time_t offset =
      atfw::util::time::time_utility::get_sys_zone_offset() - 5 * atfw::util::time::time_utility::HOUR_SECONDS;
  atfw::util::time::time_utility::set_zone_offset(offset);
  CASE_EXPECT_EQ(offset, atfw::util::time::time_utility::get_zone_offset());
  CASE_EXPECT_NE(offset, atfw::util::time::time_utility::get_sys_zone_offset());

  // 恢复时区设置
  atfw::util::time::time_utility::set_zone_offset(atfw::util::time::time_utility::get_sys_zone_offset());
}

CASE_TEST(time_test, sys_zone_offset) {
  atfw::util::time::time_utility::update();
  time_t offset = atfw::util::time::time_utility::get_sys_zone_offset();
  time_t now = atfw::util::time::time_utility::get_sys_now();

  tm local_tm, gmt_tm;
  time_t local_now = now;
  time_t gmt_now = now - offset;
  UTIL_STRFUNC_LOCALTIME_S(&local_now, &local_tm);
  UTIL_STRFUNC_GMTIME_S(&gmt_now, &gmt_tm);

  CASE_EXPECT_EQ(local_tm.tm_year, gmt_tm.tm_year);
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
  atfw::util::time::time_utility::update();

  tnow = atfw::util::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);
  loffset = tobj.tm_hour * atfw::util::time::time_utility::HOUR_SECONDS +
            tobj.tm_min * atfw::util::time::time_utility::MINITE_SECONDS + tobj.tm_sec;
  cnow = atfw::util::time::time_utility::get_today_offset(loffset);

  // 只有闰秒误差，肯定在5秒以内
  // 容忍夏时令误差，所以要加一小时
  time_t toff =
      (cnow + atfw::util::time::time_utility::DAY_SECONDS - tnow) % atfw::util::time::time_utility::DAY_SECONDS;
  if (tobj.tm_isdst > 0) {
    CASE_EXPECT_LE(toff, 3605);
  } else {
    CASE_EXPECT_LE(toff, 5);
  }
}

CASE_TEST(time_test, is_same_day) {
  struct tm tobj;
  time_t lt, rt;
  atfw::util::time::time_utility::update();
  lt = atfw::util::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&lt, &tobj);

  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 5;
  lt = mktime(&tobj);
  rt = lt + 5;
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_day(lt, rt));

  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 55;
  rt = mktime(&tobj);
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_day(lt, rt));

  lt = rt - 5;
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_day(lt, rt));
  // 容忍夏时令误差
  lt = rt + 3610;
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_day(lt, rt));
}

CASE_TEST(time_test, is_same_day_with_offset) {
  struct tm tobj;
  time_t lt, rt;
  int zero_hore = 5;
  time_t day_offset = zero_hore * atfw::util::time::time_utility::HOUR_SECONDS;

  atfw::util::time::time_utility::update();
  lt = atfw::util::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&lt, &tobj);

  tobj.tm_isdst = 0;
  tobj.tm_hour = zero_hore;
  tobj.tm_min = 0;
  tobj.tm_sec = 5;
  lt = mktime(&tobj);
  rt = lt + 5;
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_day(lt, rt, day_offset));

  tobj.tm_isdst = 0;
  tobj.tm_hour = zero_hore - 1;
  tobj.tm_min = 59;
  tobj.tm_sec = 55;
  rt = mktime(&tobj);
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_day(lt, rt, day_offset));
}

CASE_TEST(time_test, is_same_week) {
  struct tm tobj;
  time_t lt, rt, tnow;

  atfw::util::time::time_utility::update();
  tnow = atfw::util::time::time_utility::get_now();
  UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

  tobj.tm_isdst = 0;
  tobj.tm_hour = 0;
  tobj.tm_min = 0;
  tobj.tm_sec = 5;
  lt = mktime(&tobj);
  lt -= atfw::util::time::time_utility::DAY_SECONDS * tobj.tm_wday;
  rt = lt + atfw::util::time::time_utility::WEEK_SECONDS;
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_week(lt, rt));

  rt -= 10;
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_week(lt, rt));
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_week(lt, tnow));
}

CASE_TEST(time_test, get_week_day) {
  struct tm tobj;
  time_t lt, rt, tnow;

  atfw::util::time::time_utility::update();
  tnow = atfw::util::time::time_utility::get_now();
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
    lt -= atfw::util::time::time_utility::DAY_SECONDS;
    rt -= atfw::util::time::time_utility::DAY_SECONDS;
  }

  CASE_EXPECT_EQ(atfw::util::time::time_utility::get_week_day(lt), atfw::util::time::time_utility::get_week_day(tnow));
  CASE_EXPECT_EQ(atfw::util::time::time_utility::get_week_day(lt), atfw::util::time::time_utility::get_week_day(rt));
}

CASE_TEST(time_test, is_same_year) {
  // nothing todo use libc now
  struct tm tobj;
  time_t lt, rt, tnow;

  atfw::util::time::time_utility::update();
  tnow = atfw::util::time::time_utility::get_now();
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
  if (atfw::util::time::time_utility::is_leap_year(tobj.tm_year + 1900)) {
    ++tobj.tm_yday;
  }
  tobj.tm_mon = 11;
  tobj.tm_mday = 31;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 58;
  rt = mktime(&tobj);

  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_year(lt, rt));
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_year(lt, rt + 3));
}

CASE_TEST(time_test, get_year_day) {
  struct tm tobj;
  time_t lt, rt, tnow;

  atfw::util::time::time_utility::update();
  tnow = atfw::util::time::time_utility::get_now();
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
  if (atfw::util::time::time_utility::is_leap_year(tobj.tm_year + 1900)) {
    ++tobj.tm_yday;
  }
  tobj.tm_mon = 11;
  tobj.tm_mday = 31;
  tobj.tm_isdst = 0;
  tobj.tm_hour = 23;
  tobj.tm_min = 59;
  tobj.tm_sec = 58;
  rt = mktime(&tobj);

  CASE_EXPECT_EQ(atfw::util::time::time_utility::get_year_day(lt), 0);
  CASE_EXPECT_EQ(atfw::util::time::time_utility::get_year_day(rt), tobj.tm_yday);
  CASE_EXPECT_EQ(atfw::util::time::time_utility::get_year_day(rt + 3), 0);
}

CASE_TEST(time_test, is_same_month) {
  struct tm tobj;
  time_t lt, rt, tnow;

  atfw::util::time::time_utility::update();
  tnow = atfw::util::time::time_utility::get_now();
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
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_month(lt, rt));
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_month(lt, rt + 3));
}

CASE_TEST(time_test, get_month_day) {
  struct tm tobj;
  time_t lt, rt, tnow;

  atfw::util::time::time_utility::update();
  tnow = atfw::util::time::time_utility::get_now();
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

  CASE_EXPECT_EQ(atfw::util::time::time_utility::get_month_day(lt), atfw::util::time::time_utility::get_month_day(rt));
  CASE_EXPECT_NE(atfw::util::time::time_utility::get_month_day(lt),
                 atfw::util::time::time_utility::get_month_day(rt + 3));
  CASE_EXPECT_EQ(1, atfw::util::time::time_utility::get_month_day(rt));
  CASE_EXPECT_EQ(2, atfw::util::time::time_utility::get_month_day(rt + 3));

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

  CASE_EXPECT_EQ(atfw::util::time::time_utility::get_month_day(lt), atfw::util::time::time_utility::get_month_day(rt));
  CASE_EXPECT_NE(atfw::util::time::time_utility::get_month_day(lt),
                 atfw::util::time::time_utility::get_month_day(rt + 3));
  CASE_EXPECT_EQ(31, atfw::util::time::time_utility::get_month_day(rt));
  CASE_EXPECT_EQ(1, atfw::util::time::time_utility::get_month_day(rt + 3));
}

CASE_TEST(time_test, get_day_start_time) {
  time_t lt, rt, tnow;
  tnow = atfw::util::time::time_utility::get_now();
  lt = atfw::util::time::time_utility::get_day_start_time(tnow);
  rt = lt + atfw::util::time::time_utility::DAY_SECONDS;

  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_day(lt, tnow));
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_day(lt, lt + 1));
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_day(lt, lt - 1));
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_day(lt, rt));
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_day(lt, rt - 1));
}

CASE_TEST(time_test, get_week_start_time) {
  time_t lt, rt, tnow;
  tnow = atfw::util::time::time_utility::get_now();
  lt = atfw::util::time::time_utility::get_week_start_time(tnow, 1);
  rt = lt + atfw::util::time::time_utility::WEEK_SECONDS;

  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_week(lt, tnow, 1));
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_week(lt, lt + 1, 1));
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_week(lt, lt - 1, 1));
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_week(lt, rt, 1));
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_week(lt, rt - 1, 1));
}

CASE_TEST(time_test, get_month_start_time) {
  time_t lt, rt, tnow;
  tnow = atfw::util::time::time_utility::get_now();
  lt = atfw::util::time::time_utility::get_month_start_time(tnow);
  rt = atfw::util::time::time_utility::get_month_start_time(lt + 32 * atfw::util::time::time_utility::DAY_SECONDS);

  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_month(lt, tnow));
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_month(lt, lt + 1));
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_month(lt, lt - 1));
  CASE_EXPECT_FALSE(atfw::util::time::time_utility::is_same_month(lt, rt));
  CASE_EXPECT_TRUE(atfw::util::time::time_utility::is_same_month(lt, rt - 1));
}

using short_timer_t = atfw::util::time::jiffies_timer<6, 3, 4>;
struct jiffies_timer_fn {
  void *check_priv_data;
  jiffies_timer_fn(void *pd) : check_priv_data(pd) {}

  void operator()(time_t tick, const short_timer_t::timer_t &timer) {
    if (nullptr != check_priv_data) {
      CASE_EXPECT_EQ(short_timer_t::get_timer_private_data(timer), check_priv_data);
    } else if (nullptr != short_timer_t::get_timer_private_data(timer)) {
      ++(*reinterpret_cast<int *>(short_timer_t::get_timer_private_data(timer)));
    }

    CASE_EXPECT_EQ(short_timer_t::get_timer_timeout(timer), tick);

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

  {
    short_timer_t::timer_wptr_t timer_watcher;
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   short_timer.add_timer(-123, jiffies_timer_fn(nullptr), &count, &timer_watcher));

    CASE_EXPECT_EQ(short_timer.get_last_tick() + 1, short_timer_t::get_timer_timeout(*timer_watcher.lock()));
  }
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

  short_timer.tick(max_tick + 29);
  CASE_EXPECT_EQ(1, count);
  short_timer.tick(max_tick + 30);
  CASE_EXPECT_EQ(2, count);

  // 跨tick
  short_timer.tick(max_tick + 64);
  CASE_EXPECT_EQ(3, count);
  CASE_EXPECT_EQ(2, static_cast<int>(short_timer.size()));

  // 非第一层、非第一个定时器组.（512+64*5-1 = 831）
  short_timer_t::timer_wptr_t timer_watcher_831;
  short_timer_t::timer_wptr_t timer_watcher_832;
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(831, jiffies_timer_fn(nullptr), &count, &timer_watcher_831));
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(832, jiffies_timer_fn(nullptr), &count, &timer_watcher_832));
  size_t wheel_idx_831 = short_timer_t::get_timer_wheel_index(*timer_watcher_831.lock());
  size_t wheel_idx_832 = short_timer_t::get_timer_wheel_index(*timer_watcher_832.lock());
  CASE_EXPECT_NE(wheel_idx_831, wheel_idx_832);
  CASE_EXPECT_GE(wheel_idx_831, 128);
  CASE_EXPECT_GE(wheel_idx_832, 128);

  // 768-831 share tick, it will move timer but will not trigger it
  short_timer.tick(max_tick + 64 + 830);
  size_t wheel_idx_831_moved = short_timer_t::get_timer_wheel_index(*timer_watcher_831.lock());
  CASE_EXPECT_NE(wheel_idx_831, wheel_idx_831_moved);
  CASE_EXPECT_LT(wheel_idx_831_moved, 64);

  CASE_EXPECT_EQ(3, count);
  short_timer.tick(max_tick + 64 + 831);
  CASE_EXPECT_EQ(4, count);
  short_timer.tick(max_tick + 64 + 832);
  CASE_EXPECT_EQ(5, count);

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
  CASE_EXPECT_EQ(7, count);
  CASE_EXPECT_EQ(0, static_cast<int>(short_timer.size()));
}

CASE_TEST(time_test, jiffies_timer_slot) {
  size_t timer_list_count[short_timer_t::WHEEL_SIZE] = {0};
  time_t blank_area[short_timer_t::WHEEL_SIZE / short_timer_t::LVL_SIZE] = {0};
  time_t max_tick = short_timer_t::get_max_tick_distance();
  for (time_t i = 1; i <= max_tick; ++i) {
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
                   owner->add_timer(short_timer_t::LVL_SIZE - 4 - 1, jiffies_timer_fn(nullptr), nullptr));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   owner->add_timer(short_timer_t::LVL_SIZE - 4, jiffies_timer_fn(nullptr), nullptr));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   owner->add_timer(short_timer_t::LVL_SIZE, jiffies_timer_fn(nullptr), nullptr));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   owner->add_timer(short_timer_t::LVL_SIZE + 1, jiffies_timer_fn(nullptr), nullptr));
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

  short_timer.tick(max_tick + 5);
  CASE_EXPECT_EQ(5, static_cast<int>(short_timer.size()));

  short_timer.tick(max_tick + 5 + short_timer_t::LVL_SIZE);
  CASE_EXPECT_EQ(1, static_cast<int>(short_timer.size()));

  short_timer.tick(max_tick + 5 + short_timer_t::LVL_SIZE + short_timer_t::LVL_CLK_DIV);
  CASE_EXPECT_EQ(0, static_cast<int>(short_timer.size()));
}

struct jiffies_timer_keep_order_fn {
  jiffies_timer_keep_order_fn() {}

  void operator()(time_t, const short_timer_t::timer_t &timer) {
    auto sequence = short_timer_t::get_timer_sequence(timer);
    short_timer_t *owner = reinterpret_cast<short_timer_t *>(short_timer_t::get_timer_private_data(timer));
    uint32_t *check_order = reinterpret_cast<uint32_t *>(owner->get_private_data());
    CASE_MSG_INFO() << "jiffies_timer " << sequence << " add timer in callback" << std::endl;

    CASE_EXPECT_GT(sequence, *check_order);
    *check_order = sequence;
  }
};

CASE_TEST(time_test, jiffies_timer_keep_order) {
  short_timer_t short_timer;
  uint32_t check_order = 0;
  short_timer.set_private_data(reinterpret_cast<void *>(&check_order));

  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.init(0));

  time_t timeout_tick = 4165;
  short_timer_t::timer_wptr_t timer_holer1;
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(timeout_tick - short_timer.get_last_tick(), jiffies_timer_keep_order_fn(),
                                       &short_timer, &timer_holer1));
  size_t wheel_idx1 = short_timer_t::get_timer_wheel_index(*timer_holer1.lock());
  CASE_EXPECT_GE(wheel_idx1, 192);
  short_timer.tick(4096);

  short_timer_t::timer_wptr_t timer_holer2;
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(timeout_tick - short_timer.get_last_tick(), jiffies_timer_keep_order_fn(),
                                       &short_timer, &timer_holer2));

  size_t wheel_idx2 = short_timer_t::get_timer_wheel_index(*timer_holer2.lock());
  CASE_EXPECT_GE(wheel_idx2, 64);
  CASE_EXPECT_NE(wheel_idx1, wheel_idx2);
  wheel_idx1 = short_timer_t::get_timer_wheel_index(*timer_holer1.lock());
  CASE_EXPECT_EQ(wheel_idx1, wheel_idx2);

  short_timer.tick(timeout_tick - 1);

  short_timer_t::timer_wptr_t timer_holer3;
  CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                 short_timer.add_timer(timeout_tick - short_timer.get_last_tick(), jiffies_timer_keep_order_fn(),
                                       &short_timer, &timer_holer3));

  size_t wheel_idx3 = short_timer_t::get_timer_wheel_index(*timer_holer3.lock());
  CASE_EXPECT_LT(wheel_idx3, 64);
  CASE_EXPECT_NE(wheel_idx1, wheel_idx3);
  wheel_idx1 = short_timer_t::get_timer_wheel_index(*timer_holer1.lock());
  wheel_idx2 = short_timer_t::get_timer_wheel_index(*timer_holer2.lock());
  CASE_EXPECT_EQ(wheel_idx3, wheel_idx1);
  CASE_EXPECT_EQ(wheel_idx3, wheel_idx1);

  CASE_EXPECT_EQ(3, static_cast<int>(short_timer.size()));
  short_timer.tick(timeout_tick);
  CASE_EXPECT_EQ(0, static_cast<int>(short_timer.size()));
}

using default_timer_t = atfw::util::time::jiffies_timer<6, 3, 8>;
struct jiffies_default_timer_keep_order_fn {
  jiffies_default_timer_keep_order_fn() {}

  void operator()(time_t, const default_timer_t::timer_t &timer) {
    auto sequence = default_timer_t::get_timer_sequence(timer);
    default_timer_t *owner = reinterpret_cast<default_timer_t *>(default_timer_t::get_timer_private_data(timer));
    uint32_t *check_order = reinterpret_cast<uint32_t *>(owner->get_private_data());
    CASE_MSG_INFO() << "jiffies_timer " << sequence << " add timer in callback" << std::endl;

    CASE_EXPECT_GT(sequence, *check_order);
    *check_order = sequence;
  }
};

CASE_TEST(time_test, reinsert_lower_wheel) {
  default_timer_t test_timer;
  uint32_t check_order = 0;
  test_timer.set_private_data(reinterpret_cast<void *>(&check_order));

  CASE_EXPECT_EQ(default_timer_t::error_type_t::EN_JTET_SUCCESS, test_timer.init(16959102784));

  default_timer_t::timer_wptr_t timer_holer1;
  CASE_EXPECT_EQ(default_timer_t::error_type_t::EN_JTET_SUCCESS,
                 test_timer.add_timer(16959103301 - test_timer.get_last_tick(), jiffies_default_timer_keep_order_fn(),
                                      &test_timer, &timer_holer1));
  size_t wheel_idx1 = default_timer_t::get_timer_wheel_index(*timer_holer1.lock());
  CASE_EXPECT_EQ(wheel_idx1, 133);

  default_timer_t::timer_wptr_t timer_holer2;
  CASE_EXPECT_EQ(default_timer_t::error_type_t::EN_JTET_SUCCESS,
                 test_timer.add_timer(16959107395 - test_timer.get_last_tick(), jiffies_default_timer_keep_order_fn(),
                                      &test_timer, &timer_holer2));

  size_t wheel_idx2 = default_timer_t::get_timer_wheel_index(*timer_holer2.lock());
  CASE_EXPECT_EQ(wheel_idx2, 248);

  test_timer.tick(16959103300);
  test_timer.insert_timer(timer_holer2.lock());

  test_timer.tick(16959103301);

  wheel_idx2 = default_timer_t::get_timer_wheel_index(*timer_holer2.lock());
  CASE_EXPECT_EQ(wheel_idx2, 133);
}
