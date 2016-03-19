/**
 * Copyright (c) 2014, Tencent
 * All rights reserved.
 *
 * @file is_union_impl.h
 * @brief 
 *
 *
 * @version 1.0
 * @author owentou, owentou@tencent.com
 * @date 2014年3月27日
 *
 * @history
 *
 */

#ifndef _UTIL_TYPE_TRAITS_IS_UNION_IMPL_H_
#define _UTIL_TYPE_TRAITS_IS_UNION_IMPL_H_

#include "remove_cv.h"

#if defined(__MSL_CPP__) && (__MSL_CPP__ >= 0x8000)
    #include <msl_utility>
    #define UTIL_TYPE_TRAITS_IS_UNION(T) Metrowerks::is_union<T>::value

#elif (defined(_MSC_VER) && defined(_MSC_FULL_VER) && (_MSC_FULL_VER >=140050215))
    #define UTIL_TYPE_TRAITS_IS_UNION(T) __is_union(T)

#elif (__DMC__) && (__DMC__ >= 0x848)
    #define UTIL_TYPE_TRAITS_IS_UNION(T) (__typeinfo(T) & 0x400)

#elif defined(__clang__) && defined(__has_feature)
    #if __has_feature(is_union)
        #define UTIL_TYPE_TRAITS_IS_UNION(T) __is_union(T)
    #endif

#elif defined(__GNUC__) && ((__GNUC__ > 4) || ((__GNUC__ == 4) && (__GNUC_MINOR__ >= 3) && !defined(__GCCXML__)))
    #define UTIL_TYPE_TRAITS_IS_UNION(T) __is_union(T)

#elif defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)
    #define UTIL_TYPE_TRAITS_IS_UNION(T) __is_union(T)

#elif defined(__ghs__) && (__GHS_VERSION_NUMBER >= 600)
    #define UTIL_TYPE_TRAITS_IS_UNION(T) __is_union(T)

#elif defined(__CODEGEARC__)
    #define UTIL_TYPE_TRAITS_IS_UNION(T) __is_union(T)

#endif

namespace util
{
    namespace type_traits
    {
        namespace detail
        {
            #ifndef __GNUC__
                template <typename T>
                struct _is_union: public
                #ifdef UTIL_TYPE_TRAITS_IS_UNION
                    integral_constant<bool,
                        UTIL_TYPE_TRAITS_IS_UNION(typename remove_cv<T>::type)
                    >
                #else
                    false_type
                #endif
                { };
            #else
                //
                // using remove_cv here generates a whole load of needless
                // warnings with gcc, since it doesn't do any good with gcc
                // in any case (at least at present), just remove it:
                //
                template <typename T>
                struct _is_union: public
                #ifdef UTIL_TYPE_TRAITS_IS_UNION
                    integral_constant<bool,
                        UTIL_TYPE_TRAITS_IS_UNION(T)
                    >
                #else
                    false_type
                #endif
                { };
            #endif
        }

        template <typename T>
        struct is_union : public detail::_is_union<T> {};
    }
}

#ifdef UTIL_TYPE_TRAITS_IS_UNION
    #undef UTIL_TYPE_TRAITS_IS_UNION
#endif

#endif /* _UTIL_TYPE_TRAITS_IS_UNION_IMPL_H_ */
