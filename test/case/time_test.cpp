#include <algorithm>
#include <cstring>
#include <ctime>

#include "frame/test_macros.h"
#include "time/time_utility.h"
#include <time/jiffies_timer.h>

CASE_TEST(time_test, zone_offset) {
    util::time::time_utility::update();
    time_t offset = util::time::time_utility::get_sys_zone_offset() - 5 * util::time::time_utility::HOUR_SECONDS;
    util::time::time_utility::set_zone_offset(offset);
    CASE_EXPECT_EQ(offset, util::time::time_utility::get_zone_offset());
    CASE_EXPECT_NE(offset, util::time::time_utility::get_sys_zone_offset());

    // 恢复时区设置
    util::time::time_utility::set_zone_offset(util::time::time_utility::get_sys_zone_offset());
}

CASE_TEST(time_test, today_offset) {
    using std::abs;
    struct tm tobj;
    time_t tnow, loffset, cnow;
    util::time::time_utility::update();

    tnow = util::time::time_utility::get_now();
    UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);
    loffset = tobj.tm_hour * util::time::time_utility::HOUR_SECONDS + tobj.tm_min * util::time::time_utility::MINITE_SECONDS + tobj.tm_sec;
    cnow = util::time::time_utility::get_today_offset(loffset);

    // 只有闰秒误差，肯定在5秒以内
    // 容忍夏时令误差，所以要加一小时
    CASE_EXPECT_LE(abs(cnow - tnow), 3605);
}

CASE_TEST(time_test, is_same_day) {
    struct tm tobj;
    time_t lt, rt;
    util::time::time_utility::update();
    lt = util::time::time_utility::get_now();
    UTIL_STRFUNC_LOCALTIME_S(&lt, &tobj);

    tobj.tm_isdst = 0;
    tobj.tm_hour = 0;
    tobj.tm_min = 0;
    tobj.tm_sec = 5;
    lt = mktime(&tobj);
    rt = lt + 5;
    CASE_EXPECT_TRUE(util::time::time_utility::is_same_day(lt, rt));

    tobj.tm_isdst = 0;
    tobj.tm_hour = 23;
    tobj.tm_min = 59;
    tobj.tm_sec = 55;
    rt = mktime(&tobj);
    CASE_EXPECT_TRUE(util::time::time_utility::is_same_day(lt, rt));

    lt = rt - 5;
    CASE_EXPECT_TRUE(util::time::time_utility::is_same_day(lt, rt));
    // 容忍夏时令误差
    lt = rt + 3610;
    CASE_EXPECT_FALSE(util::time::time_utility::is_same_day(lt, rt));
}

CASE_TEST(time_test, is_same_day_with_offset) {
    struct tm tobj;
    time_t lt, rt;
    int zero_hore = 5;
    time_t day_offset = zero_hore * util::time::time_utility::HOUR_SECONDS;

    util::time::time_utility::update();
    lt = util::time::time_utility::get_now();
    UTIL_STRFUNC_LOCALTIME_S(&lt, &tobj);

    tobj.tm_isdst = 0;
    tobj.tm_hour = zero_hore;
    tobj.tm_min = 0;
    tobj.tm_sec = 5;
    lt = mktime(&tobj);
    rt = lt + 5;
    CASE_EXPECT_TRUE(util::time::time_utility::is_same_day(lt, rt, day_offset));

    tobj.tm_isdst = 0;
    tobj.tm_hour = zero_hore - 1;
    tobj.tm_min = 59;
    tobj.tm_sec = 55;
    rt = mktime(&tobj);
    CASE_EXPECT_FALSE(util::time::time_utility::is_same_day(lt, rt, day_offset));
}

CASE_TEST(time_test, is_same_week) {
    struct tm tobj;
    time_t lt, rt, tnow;

    util::time::time_utility::update();
    tnow = util::time::time_utility::get_now();
    UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

    tobj.tm_isdst = 0;
    tobj.tm_hour = 0;
    tobj.tm_min = 0;
    tobj.tm_sec = 5;
    lt = mktime(&tobj);
    lt -= util::time::time_utility::DAY_SECONDS * tobj.tm_wday;
    rt = lt + util::time::time_utility::WEEK_SECONDS;
    CASE_EXPECT_FALSE(util::time::time_utility::is_same_week(lt, rt));

    rt -= 10;
    CASE_EXPECT_TRUE(util::time::time_utility::is_same_week(lt, rt));
    CASE_EXPECT_TRUE(util::time::time_utility::is_same_week(lt, tnow));
}

CASE_TEST(time_test, get_week_day) {
    struct tm tobj;
    time_t lt, rt, tnow;

    util::time::time_utility::update();
    tnow = util::time::time_utility::get_now();
    UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

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
    CASE_EXPECT_EQ(util::time::time_utility::get_week_day(lt), util::time::time_utility::get_week_day(tnow));
    CASE_EXPECT_EQ(util::time::time_utility::get_week_day(lt), util::time::time_utility::get_week_day(rt));
}

CASE_TEST(time_test, is_same_month) {
    // nothing todo use libc now
}

typedef util::time::jiffies_timer<6, 3, 4> short_timer_t;
struct jiffies_timer_fn {
    void *check_priv_data;
    jiffies_timer_fn(void *pd) : check_priv_data(pd) {}

    void operator()(time_t, const short_timer_t::timer_t &timer) {
        if (NULL != check_priv_data) {
            CASE_EXPECT_EQ(short_timer_t::get_timer_private_data(timer), check_priv_data);
        } else if (NULL != short_timer_t::get_timer_private_data(timer)) {
            ++(*reinterpret_cast<int *>(short_timer_t::get_timer_private_data(timer)));
        }

        CASE_MSG_INFO() << "jiffies_timer " << short_timer_t::get_timer_sequence(timer) << " actived" << std::endl;
    }
};

CASE_TEST(time_test, jiffies_timer_basic) {
    short_timer_t short_timer;
    int count = 0;
    time_t max_tick = short_timer.get_max_tick_distance() + 1;

    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_NOT_INITED, short_timer.add_timer(123, jiffies_timer_fn(NULL), NULL));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_NOT_INITED, short_timer.tick(456));

    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.init(max_tick));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_ALREADY_INITED, short_timer.init(max_tick));

    CASE_EXPECT_EQ(32767, short_timer.get_max_tick_distance());
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_TIMEOUT_EXTENDED,
                   short_timer.add_timer(short_timer.get_max_tick_distance() + 1, jiffies_timer_fn(NULL), NULL));

    // 理论上会被放在（数组下标: 2^6*3=192）
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   short_timer.add_timer(short_timer.get_max_tick_distance(), jiffies_timer_fn(&short_timer), &short_timer));
    // 理论上会被放在（数组下标: 2^6*4-1=255）
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS,
                   short_timer.add_timer(short_timer.get_max_tick_distance() - short_timer_t::LVL_GRAN(3), jiffies_timer_fn(&short_timer),
                                         &short_timer));

    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.add_timer(-123, jiffies_timer_fn(NULL), &count));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.add_timer(30, jiffies_timer_fn(NULL), &count));
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.add_timer(40, jiffies_timer_fn(NULL), &count));
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
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.add_timer(831, jiffies_timer_fn(NULL), &count));
    short_timer.tick(max_tick + 64 + 831);
    CASE_EXPECT_EQ(3, count);
    // 768-831 share tick
    short_timer.tick(max_tick + 64 + 832);
    CASE_EXPECT_EQ(4, count);

    // 32767
    short_timer.tick(32767 + max_tick);

    // 这个应该会放在数组下标为0的位置
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.add_timer(0, jiffies_timer_fn(NULL), &count));

    // 环状数组的复用
    CASE_EXPECT_EQ(short_timer_t::error_type_t::EN_JTET_SUCCESS, short_timer.add_timer(1000, jiffies_timer_fn(NULL), &count));

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
            CASE_EXPECT_EQ(timer_count, short_timer_t::LVL_GRAN(i / short_timer_t::LVL_SIZE));
        }
    }

    // 定时器空白区间的个数应该等于重合区域
    for (size_t i = 1; i < short_timer_t::WHEEL_SIZE / short_timer_t::LVL_SIZE; ++i) {
        CASE_EXPECT_EQ(blank_area[i], short_timer_t::LVL_START(i) / short_timer_t::LVL_GRAN(i));
    }
}
