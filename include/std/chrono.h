/**
 *
 * @file chrono.h
 * @brief 导入高精度时间库
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT, owt5008137@live.com
 * @date 2016.03.25
 * @history
 *
 */

#ifndef STD_CHRONO_H
#define STD_CHRONO_H

#pragma once

// ============================================================
// 公共包含部分
// 自动导入TR1库
// ============================================================

/**
 * 导入时间库
 * VC 9.0 SP1以上支持是我猜的，因为我手边最老的版本是 VC 11(VS 2012)已经支持了
 * GCC 4.4 以上已经确认支持,4.2 不支持，所以这里先写4.4吧。懒得去查了
 */

#if __cplusplus >= 201103L
// C++ 11 and upper
#  include <chrono>

#elif defined(_MSC_VER) && ((_MSC_VER == 1500 && defined(_HAS_TR1)) || _MSC_VER > 1500)
// VC9.0 SP1以上分支判断
// 采用VC std::tr1库
#  include <chrono>

#elif defined(__clang__) && __clang_major__ >= 3
// 采用Clang c++11库
#  include <chrono>

#elif defined(__GNUC__) && __GNUC__ >= 4 && (__GNUC__ > 4 || __GNUC_MINOR__ >= 4) && defined(__GXX_EXPERIMENTAL_CXX0X__)
// 采用G++ std::tr1库
#  include <chrono>

#else
// 采用boost.chrono库
#  include <boost/chrono.hpp>
namespace std {
namespace chrono {
using boost::chrono::duration;
using boost::chrono::time_point;

using boost::chrono::duration_cast;

using boost::chrono::high_resolution_clock;
using boost::chrono::steady_clock;
using boost::chrono::system_clock;
// 下面两个是boost独有，并不在std中
// using boost::chrono::process_cpu_clock
// using boost::chrono::thread_clock

using boost::chrono::hours;
using boost::chrono::microseconds;
using boost::chrono::milliseconds;
using boost::chrono::minutes;
using boost::chrono::nanoseconds;
using boost::chrono::seconds;
}  // namespace chrono
}  // namespace std
#endif

#endif
