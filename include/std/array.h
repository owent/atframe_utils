/**
 * @file array.h
 * @brief 导入支持STL的数组类型
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT, owt5008137@live.com
 * @date 2012.08.02
 *
 * @history
 *
 */

#ifndef _STD_ARRAY_H_
#define _STD_ARRAT_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

// ============================================================
// 公共包含部分
// 自动导入TR1库
// ============================================================

/**
* 导入数组（array）
* 如果是G++且支持c++0x草案1（tr1版本）的array[GCC版本高于4.0]
* 则会启用GNU-C++的数组
*
* 如果是VC++且支持c++0x草案1（tr1版本）的array[VC++版本高于9.0 SP1]
* 则会启用VC++的数组
*
* 否则启用boost中的array库（如果是这种情况需要加入boost库）
*/

// VC9.0 SP1以上分支判断
#if defined(_MSC_VER) && ((_MSC_VER == 1500 && defined(_HAS_TR1)) || _MSC_VER > 1500)
// 采用VC std::tr1库
#include <array>
#elif defined(__clang__) && __clang_major__ >= 3
// 采用Clang c++11库
#include <array>
#elif defined(__GNUC__) && __GNUC__ >= 4
// 采用G++ std::tr1库
#if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <array>
#else
#include <tr1/array>
namespace std {
    using tr1::array;
    using tr1::get;
    using tr1::tuple_element;
    using tr1::tuple_size;
}
#endif

#else
// 采用boost库
#include <boost/tr1/array.hpp>
namespace std {
    using tr1::array;
    using tr1::get;
    using tr1::tuple_element;
    using tr1::tuple_size;
}
#endif

#endif
