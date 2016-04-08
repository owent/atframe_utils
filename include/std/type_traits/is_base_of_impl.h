/**
 * Copyright (c) 2014, Tencent
 * All rights reserved.
 *
 * @file is_base_of_impl.h
 * @brief 
 *
 *
 * @version 1.0
 * @author owentou, owentou@tencent.com
 * @date 2014年3月28日
 *
 * @history
 *
 */

#ifndef _UTIL_TYPE_TRAITS_IS_BASE_OF_IMPL_H_
#define _UTIL_TYPE_TRAITS_IS_BASE_OF_IMPL_H_

#include "is_convertible_impl.h"

#if (defined(_MSC_VER) && defined(_MSC_FULL_VER) && (_MSC_FULL_VER >=140050215))
    #define UTIL_TYPE_TRAITS_IS_BASE_OF(TB, TC) __is_base_of(TB, TC)

#elif defined(__clang__) && defined(__has_feature)
    #if __has_feature(is_base_of)
        #define UTIL_TYPE_TRAITS_IS_BASE_OF(TB, TC) __is_base_of(TB, TC)
    #endif

#elif defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3) && !defined(__GCCXML__)))
    #define UTIL_TYPE_TRAITS_IS_BASE_OF(TB, TC) __is_base_of(TB, TC)

#elif defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
    #define UTIL_TYPE_TRAITS_IS_BASE_OF(TB, TC) __is_base_of(TB, TC)

#elif defined(__ghs__) && (__GHS_VERSION_NUMBER >= 600)
    #define UTIL_TYPE_TRAITS_IS_BASE_OF(TB, TC) __is_base_of(TB, TC)

#elif defined(__CODEGEARC__)
    #define UTIL_TYPE_TRAITS_IS_BASE_OF(TB, TC) __is_base_of(TB, TC)

#endif

#if !defined(UTIL_TYPE_TRAITS_IS_BASE_OF)
    #define UTIL_TYPE_TRAITS_IS_BASE_OF(TB, TC) \
        is_convertible< \
            typename remove_cv<TC>::type*, \
            typename remove_cv<TB>::type*  \
        >::value

#endif


namespace util
{
    namespace type_traits
    {
        // 简单 c++ 11 type_traits std::is_base_of<LogicTaskBase, base_type>::value 解决方案
        // 以下是和 c++ 11 type_traits std::is_base_of 的不同之处
        // [逻辑保证base_type和LogicTaskBase是一个类] 忽略对 base_type 和 LogicTaskBase 是否是class的检测

        /**
         * type traits - is base of
         */
        template <typename TB, typename TC>
        struct is_base_of: public integral_constant< bool, UTIL_TYPE_TRAITS_IS_BASE_OF(TB, TC) >
        {
        };
    }
}


#ifdef UTIL_TYPE_TRAITS_IS_BASE_OF
    #undef UTIL_TYPE_TRAITS_IS_BASE_OF
#endif

#endif /* _TYPE_TRAITS_IS_BASE_OF_IMPL_H_ */
