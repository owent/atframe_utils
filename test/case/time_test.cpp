#include <algorithm>
#include <cstring>
#include <ctime>

#include "frame/test_macros.h"
#include "time/time_utility.h"

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
    CASE_EXPECT_TRUE(abs(cnow - tnow) <= 5);
}

CASE_TEST(time_test, is_same_day) {
    struct tm tobj;
    time_t lt, rt;
    util::time::time_utility::update();
    lt = util::time::time_utility::get_now();
    UTIL_STRFUNC_LOCALTIME_S(&lt, &tobj);

    tobj.tm_hour = 0;
    tobj.tm_min = 0;
    tobj.tm_sec = 5;
    lt = mktime(&tobj);
    tobj.tm_sec = 10;
    rt = lt + 5;
    CASE_EXPECT_TRUE(util::time::time_utility::is_same_day(lt, rt));

    tobj.tm_hour = 23;
    tobj.tm_min = 59;
    tobj.tm_sec = 55;
    rt = mktime(&tobj);
    CASE_EXPECT_TRUE(util::time::time_utility::is_same_day(lt, rt));

    lt = rt - 5;
    CASE_EXPECT_TRUE(util::time::time_utility::is_same_day(lt, rt));
    lt = rt + 10;
    CASE_EXPECT_FALSE(util::time::time_utility::is_same_day(lt, rt));
}

CASE_TEST(time_test, is_same_week) {
    struct tm tobj;
    time_t lt, rt, tnow;

    util::time::time_utility::update();
    tnow = util::time::time_utility::get_now();
    UTIL_STRFUNC_LOCALTIME_S(&tnow, &tobj);

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

    tobj.tm_hour = 0;
    tobj.tm_min = 0;
    tobj.tm_sec = 5;
    lt = mktime(&tobj);

    tobj.tm_hour = 23;
    tobj.tm_min = 59;
    tobj.tm_sec = 55;
    rt = mktime(&tobj);

    CASE_EXPECT_EQ(util::time::time_utility::get_week_day(lt), util::time::time_utility::get_week_day(tnow));
    CASE_EXPECT_EQ(util::time::time_utility::get_week_day(lt), util::time::time_utility::get_week_day(rt));
}

CASE_TEST(time_test, is_same_month) {
    // nothing todo use libc now
}
