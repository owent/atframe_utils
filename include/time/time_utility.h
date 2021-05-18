/**
 * @file time_utility.h
 * @brief 时间相关得通用代码
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2015-06-29
 * @history
 */

#ifndef UTIL_TIME_TIME_UTILITY_H
#define UTIL_TIME_TIME_UTILITY_H

#pragma once

#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <ctime>

#include "std/chrono.h"

#include <config/atframe_utils_build_feature.h>

#if (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || defined(__STDC_LIB_EXT1__)
#  define UTIL_STRFUNC_LOCALTIME_S(time_t_ptr, tm_ptr) localtime_s(time_t_ptr, tm_ptr)
#  define UTIL_STRFUNC_GMTIME_S(time_t_ptr, tm_ptr) gmtime_s(time_t_ptr, tm_ptr)

#elif defined(_MSC_VER) && _MSC_VER >= 1300
#  define UTIL_STRFUNC_LOCALTIME_S(time_t_ptr, tm_ptr) localtime_s(tm_ptr, time_t_ptr)
#  define UTIL_STRFUNC_GMTIME_S(time_t_ptr, tm_ptr) gmtime_s(tm_ptr, time_t_ptr)

#elif defined(__STDC_VERSION__)
#  define UTIL_STRFUNC_LOCALTIME_S(time_t_ptr, tm_ptr) localtime_r(time_t_ptr, tm_ptr)
#  define UTIL_STRFUNC_GMTIME_S(time_t_ptr, tm_ptr) gmtime_r(time_t_ptr, tm_ptr)

#else
#  define UTIL_STRFUNC_LOCALTIME_S(time_t_ptr, tm_ptr) (*(tm_ptr) = *localtime(time_t_ptr))
#  define UTIL_STRFUNC_GMTIME_S(time_t_ptr, tm_ptr) (*(tm_ptr) = *gmtime(time_t_ptr))

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
  typedef std::chrono::system_clock::duration raw_duration_t;
  typedef std::chrono::system_clock::period raw_period_t;
  typedef struct tm raw_time_desc_t;

 private:
  time_utility();
  ~time_utility();

 public:
  /**
   * @brief 更新时间
   * @param t 可以指定时间对象
   */
  static LIBATFRAME_UTILS_API void update(raw_time_t *t = NULL);

  /**
   * @brief 获取原始系统时间对象
   * @return 最后一次update的时间
   */
  static LIBATFRAME_UTILS_API raw_time_t now();

  /**
   * @brief 获取当前Unix时间戳
   * @return 最后一次update的时间
   */
  static LIBATFRAME_UTILS_API time_t get_now();

  /**
   * @brief 获取当前时间的微秒部分
   * @note 为了减少系统调用，这里仅在update时更新缓存，并且使用偏移值进行计算，所以大部分情况下都会偏小一些。
   *       这里仅为能够容忍误差的时间相关的功能提供一个时间参考，如果需要使用精确时间，请使用系统调用
   * @note update接口不加锁，所以一般情况下，返回值在[0,
   * 1000000)之间，极端情况下（特别是多线程调用时）可能出现大于1000000
   * @return 当前时间的微妙部分
   */
  static LIBATFRAME_UTILS_API time_t get_now_usec();

  /**
   * @brief 获取原始系统时间对象,不受set_global_now_offset()影响
   * @return 最后一次update的时间
   */
  static LIBATFRAME_UTILS_API raw_time_t sys_now();

  /**
   * @brief 获取系统当前Unix时间戳,不受set_global_now_offset()影响
   * @return 最后一次update的时间
   */
  static LIBATFRAME_UTILS_API time_t get_sys_now();

  /**
   * @brief 设置时间的全局偏移（Debug功能）
   * @note 影响now()、get_now()和get_now_usec()的返回结果，用于模拟时间
   */
  static LIBATFRAME_UTILS_API void set_global_now_offset(const std::chrono::system_clock::duration &offset);

  /**
   * @brief 设置时间的全局偏移（Debug功能）
   * @note 影响now()、get_now()和get_now_usec()的返回结果，用于模拟时间
   * @return 当前的时间全局偏移
   */
  static LIBATFRAME_UTILS_API std::chrono::system_clock::duration get_global_now_offset();

  /**
   * @brief 重置时间的全局偏移（Debug功能）
   * @note 影响now()、get_now()和get_now_usec()的返回结果，还原成真实时间
   */
  static LIBATFRAME_UTILS_API void reset_global_now_offset();

  // ====================== 后面的函数都和时区相关 ======================
  /**
   * @brief 获取系统时区时间偏移(忽略自定义偏移)
   * @return 系统时区的时间偏移
   */
  static LIBATFRAME_UTILS_API time_t get_sys_zone_offset();

  /**
   * @brief 获取时区时间偏移（如果设置过自定义偏移，使用自定义的值，否则和get_sys_zone_offset()的结果一样)
   * @return 时区的时间偏移
   */
  static LIBATFRAME_UTILS_API time_t get_zone_offset();

  /**
   * @brief 设置时区时间偏移
   * @param t 可以指定时区时间偏移
   * @note 比如要设置凌晨5点视为0点: set_zone_offset(get_sys_zone_offset() - 5 * HOUR_SECONDS)
   */
  static LIBATFRAME_UTILS_API void set_zone_offset(time_t t);

  /**
   * @brief 获取当前时区今天过了多少秒(不计自定义时区)
   * @return 目前时间的相对今天的偏移
   */
  static LIBATFRAME_UTILS_API time_t get_today_now_offset();

  /**
   * @brief [快速非严格]判定当前时区，两个时间是否是同一天
   * @param left (必须大于0)
   * @param right (必须大于0)
   * @return 同一天返回 true
   */
  static LIBATFRAME_UTILS_API bool is_same_day(time_t left, time_t right);

  /**
   * @brief [快速非严格]判定当前时区，两个时间是否跨同一天的指定时间点
   * @param left (必须大于0)
   * @param right (必须大于0)
   * @param offset 时间点偏移
   * @note 比如某个功能在21:00刷新，那么判定是否需要刷新则使用 is_same_day([上一次刷新时间], [要检测的时间], 75600)
   * @note offset = 0时等同于is_same_day(time_t left, time_t right)
   * @return 不跨同一天的该时间点返回 true
   */
  static LIBATFRAME_UTILS_API bool is_same_day(time_t left, time_t right, time_t offset);

  /**
   * @brief [快速非严格]判定当前时区，right是否和left不是同一天且right大于left
   * @param left (必须大于0)
   * @param right (必须大于0)
   * @return right是否和left不是同一天且right大于left返回 true
   */
  static LIBATFRAME_UTILS_API bool is_greater_day(time_t left, time_t right);

  /**
   * @brief [快速非严格]判定当前时区，right是否和left不是同一天且right大于left
   * @param left (必须大于0)
   * @param right (必须大于0)
   * @param offset 时间点偏移
   * @note 比如某个功能在21:00刷新，那么判定是否需要刷新则使用 is_greater_day([上一次刷新时间], [要检测的时间], 75600)
   * @note offset = 0时等同于is_greater_day(time_t left, time_t right)
   * @return right是否和left不是同一天且right大于left返回 true
   */
  static LIBATFRAME_UTILS_API bool is_greater_day(time_t left, time_t right, time_t offset);

  /**
   * @brief 获取当前时区相对于今天零点之后offset秒的Unix时间戳
   * @param offset 时间偏移值
   * @return 今天0点后offset的时间戳
   */
  static LIBATFRAME_UTILS_API time_t get_today_offset(time_t offset);

  /**
   * @brief 获取当前时区相对于指定时间得那一天的零点之后offset秒的Unix时间戳
   * @param checked 指定时间，用于计算基于哪一天
   * @param offset 时间偏移值
   * @return 今天0点后offset的时间戳
   */
  static LIBATFRAME_UTILS_API time_t get_any_day_offset(time_t checked, time_t offset = 0);

  /**
   * @brief 获取当前时区指定时间的详细信息
   * @param t (必须大于0)
   * @return 时间描述
   */
  static LIBATFRAME_UTILS_API raw_time_desc_t get_local_tm(time_t t);

  /**
   * @brief 获取时指定太平洋时间的详细信息
   * @note 建议使用这个接口来代替gmtime_s/gmtime_s，这样里面会适配跨平台的调用且不会报warning
   * @param t (必须大于0)
   * @return 时间描述
   */
  static LIBATFRAME_UTILS_API raw_time_desc_t get_gmt_tm(time_t t);

  /**
   * @brief 判定当前年是否是闰年
   * @return 闰年返回 true
   */
  static LIBATFRAME_UTILS_API bool is_leap_year(int year);

  /**
   * @brief 判定当前时区时间是否是同一个年
   * @return 同一月返回 true
   */
  static LIBATFRAME_UTILS_API bool is_same_year(time_t left, time_t right);

  /**
   * @brief 判定当前时区时间是本年第几天
   * @param t (必须大于0)
   * @return 本年第几天，0-365，和struct tm的tm_yday保持一致
   */
  static LIBATFRAME_UTILS_API int get_year_day(time_t t);

  /**
   * @brief 判定当前时区时间是否是同一个月
   * @return 同一月返回 true
   */
  static LIBATFRAME_UTILS_API bool is_same_month(time_t left, time_t right);

  /**
   * @brief 判定当前时区时间是本月第几天
   * @param t (必须大于0)
   * @return 本月第几天，1-31，和struct tm的tm_mday保持一致
   */
  static LIBATFRAME_UTILS_API int get_month_day(time_t t);

  /**
   * @brief [快速非严格]判定当前时区时间是否是同一个周
   * @param left (必须大于0)
   * @param right (必须大于0)
   * @param week_first 一周的第一天，0表示周日，1表示周一，以此类推
   * @return 同一周返回 true
   */
  static LIBATFRAME_UTILS_API bool is_same_week(time_t left, time_t right, time_t week_first = 0);

  /**
   * @brief [快速非严格]判定当前时区，两个时间是否跨同一周的指定时间点
   * @param left (必须大于0)
   * @param right (必须大于0)
   * @param offset 时间点偏移
   * @note 比如某个功能在每周的第一天的21:00刷新，那么判定是否需要刷新则使用 is_same_week_point([上一次刷新时间],
   * [要检测的时间], 75600, [一周的第一天])
   * @note offset = 0时等同于is_same_week(time_t left, time_t right, time_t week_first)
   * @return 不跨同一天的该时间点返回 true
   */
  static LIBATFRAME_UTILS_API bool is_same_week_point(time_t left, time_t right, time_t offset, time_t week_first = 0);

  /**
   * @brief [快速非严格]判定当前时区时间是本周第几天
   * @param t (必须大于0)
   * @return 本周第几天，周日返回0,周一返回1,周二返回2,以此类推
   */
  static LIBATFRAME_UTILS_API int get_week_day(time_t t);

  /**
   * @brief 获取一天的开始时间的时间戳
   * @param t 基于该时间的天,填0使用get_now()返回的时间
   * @return t所在天的开始时间戳
   */
  static LIBATFRAME_UTILS_API time_t get_day_start_time(time_t t = 0);

  /**
   * @brief 获取一天的开始时间的时间戳
   * @param t 基于该时间的周,填0使用get_now()返回的时间
   * @param week_first 一周的第一天，0表示周日，1表示周一，以此类推
   * @return t所在周的开始时间戳
   */
  static LIBATFRAME_UTILS_API time_t get_week_start_time(time_t t = 0, time_t week_first = 0);

  /**
   * @brief 获取一天的开始时间的时间戳
   * @param t 基于该时间的月,填0使用get_now()返回的时间
   * @return t所在月的开始时间戳
   */
  static LIBATFRAME_UTILS_API time_t get_month_start_time(time_t t = 0);

 private:
  // 当前时间
  static LIBATFRAME_UTILS_API raw_time_t now_;

  // 当前时间(Unix时间戳)
  static LIBATFRAME_UTILS_API time_t now_unix_;

  // 当前时间(微妙，非精确)
  static LIBATFRAME_UTILS_API time_t now_usec_;

  // 时区时间的人为偏移
  static LIBATFRAME_UTILS_API time_t custom_zone_offset_;

  // 时间的全局偏移（Debug功能）
  static LIBATFRAME_UTILS_API std::chrono::system_clock::duration global_now_offset_;
};
}  // namespace time
}  // namespace util

#endif  // _UTIL_TIME_TIME_UTILITY_H_
