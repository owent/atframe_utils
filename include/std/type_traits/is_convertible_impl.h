/**
 * @file is_convertible_impl.h
 * @brief
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT, owt5008137@live.com
 * @date 2014年3月28日
 *
 * @history
 *
 */

#ifndef UTIL_TYPE_TRAITS_IS_CONVERTIBLE_IMPL_H
#define UTIL_TYPE_TRAITS_IS_CONVERTIBLE_IMPL_H


#pragma once

#include "is_xxx_impl.h"

#if (defined(_MSC_VER) && defined(_MSC_FULL_VER) && (_MSC_FULL_VER >= 140050215))
#define UTIL_TYPE_TRAITS_IS_CONVERTIBLE(TB, TC) (__is_convertible_to(TB, TC) || is_same<TB, TC>::value)

#elif defined(__clang__) && defined(__has_feature)
#if __has_feature(is_convertible_to)
#define UTIL_TYPE_TRAITS_IS_CONVERTIBLE(TB, TC) __is_convertible_to(TB, TC)
#endif

#elif defined(__CODEGEARC__)
#define UTIL_TYPE_TRAITS_IS_CONVERTIBLE(TB, TC) (__is_convertible(TB, TC) || is_void<TC>::value)

#endif

#if !defined(UTIL_TYPE_TRAITS_IS_CONVERTIBLE)
#define UTIL_TYPE_TRAITS_IS_CONVERTIBLE(TB, TC) detail::_is_convertible<TB, TC>::value
#endif


namespace util {
    namespace type_traits {
        namespace detail {
            template <typename TFrom, typename TTo, bool BV = (is_void<TFrom>::value || is_array<TTo>::value)>
            struct _is_convertible : public integral_constant<bool, is_void<TTo>::value> {};

            template <typename TFrom, typename TTo>
            struct _is_convertible<TFrom, TTo, false> : public integral_constant<bool, sizeof(type_checker<TTo>::_m_check(*((TFrom *)0))) ==
                                                                                           sizeof(typename type_checker<TTo>::yes_type)> {};
        } // namespace detail

        /**
         * type traits - is convertible
         */
        template <typename TFrom, typename TTo>
        struct is_convertible : public integral_constant<bool, UTIL_TYPE_TRAITS_IS_CONVERTIBLE(TFrom, TTo)> {};
    } // namespace type_traits
} // namespace util


#ifdef UTIL_TYPE_TRAITS_IS_CONVERTIBLE
#undef UTIL_TYPE_TRAITS_IS_CONVERTIBLE
#endif

#endif /* _UTIL_TYPE_TRAITS_IS_CONVERTIBLE_IMPL_H_ */
