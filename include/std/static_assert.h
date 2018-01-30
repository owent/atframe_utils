/**
 * @file static_assert.h
 * @brief 导入静态断言（STD_STATIC_ASSERT）<br />
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT, owt5008137@live.com
 * @date 2013-12-25
 *
 * @history
 *
 */
#ifndef STD_STATIC_ASSERT_H
#define STD_STATIC_ASSERT_H
#pragma once

// ============================================================
// 公共包含部分
// 自动导入TR1库
// ============================================================

/**
 * 导入静态断言（STD_STATIC_ASSERT）
 * 如果是G++且GCC版本高于4.3, 使用关键字static_assert
 * 否则会启用自定义简化的静态断言
 *
 * 如果是VC++且VC++版本高于10.0, 使用关键字static_assert
 * 否则会启用自定义简化的静态断言
 *
 * 如果指定载入了boost库，则启用BOOST中的静态断言
 *
 * 自定义简化的静态断言参照BOOST_STATIC_ASSERT
 *
 */

// VC10.0 SP1以上分支判断
#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1600) || \
    (defined(__GNUC__) && defined(__GXX_EXPERIMENTAL_CXX0X__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)))
#define STD_STATIC_ASSERT(exp) static_assert(exp, #exp)
#define STD_STATIC_ASSERT_MSG(exp, msg) static_assert(exp, msg)

#elif defined(STD_WITH_BOOST_HPP) || defined(STD_ENABLE_BOOST_STATIC_ASSERT)
#include <boost/static_assert.hpp>
#define STD_STATIC_ASSERT(exp) BOOST_STATIC_ASSERT(exp)
#define STD_STATIC_ASSERT_MSG(exp, msg) BOOST_STATIC_ASSERT_MSG(exp, msg)
#else

//
// If the compiler issues warnings about old C style casts,
// then enable this:
//
#if defined(__GNUC__) && ((__GNUC__ > 3) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 4)))
#define STD_STATIC_ASSERT_BOOL_CAST(x) ((x) == 0 ? false : true)
#else
#define STD_STATIC_ASSERT_BOOL_CAST(x) (bool)(x)
#endif

//
// Helper macro STD_STATIC_ASSERT_JOIN:
// The following piece of macro magic joins the two
// arguments together, even when one of the arguments is
// itself a macro (see 16.3.1 in C++ standard).  The key
// is that macro expansion of macro arguments does not
// occur in STD_STATIC_ASSERT_DO_JOIN2 but does in STD_STATIC_ASSERT_DO_JOIN.
//
#define STD_STATIC_ASSERT_JOIN(X, Y) STD_STATIC_ASSERT_DO_JOIN(X, Y)
#define STD_STATIC_ASSERT_DO_JOIN(X, Y) STD_STATIC_ASSERT_DO_JOIN2(X, Y)
#define STD_STATIC_ASSERT_DO_JOIN2(X, Y) X##Y

namespace util {
    namespace std {
        // HP aCC cannot deal with missing names for template value parameters
        template <bool x>
        struct STATIC_ASSERTION_FAILURE;

        template <>
        struct STATIC_ASSERTION_FAILURE<true> {
            enum { value = 1 };
        };

        // HP aCC cannot deal with missing names for template value parameters
        template <int x>
        struct static_assert_test {};
    } // namespace std
} // namespace util

#if defined(_MSC_VER) && (_MSC_VER < 1300)
  // __LINE__ macro broken when -ZI is used see Q199057
  // fortunately MSVC ignores duplicate typedef's.
#define STD_STATIC_ASSERT(B) \
    typedef ::util::std::static_assert_test<sizeof(::util::std::STATIC_ASSERTION_FAILURE<(bool)(B)>)> std_static_assert_typedef_
#elif defined(_MSC_VER)
#define STD_STATIC_ASSERT(B)                                                                                               \
    typedef ::util::std::static_assert_test<sizeof(::util::std::STATIC_ASSERTION_FAILURE<STD_STATIC_ASSERT_BOOL_CAST(B)>)> \
        STD_STATIC_ASSERT_JOIN(std_static_assert_typedef_, __COUNTER__)
#else
// generic version
#define STD_STATIC_ASSERT(B)                                                                                               \
    typedef ::util::std::static_assert_test<sizeof(::util::std::STATIC_ASSERTION_FAILURE<STD_STATIC_ASSERT_BOOL_CAST(B)>)> \
        STD_STATIC_ASSERT_JOIN(std_static_assert_typedef_, __LINE__)

#endif

// 自定义静态断言忽略自定义消息
#define STD_STATIC_ASSERT_MSG(exp, msg) STD_STATIC_ASSERT(exp)

#endif

#endif /* _STD_STATIC_ASSERT_H_ */
