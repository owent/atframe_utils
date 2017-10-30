/**
 * @file remove_pointer.h
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

#ifndef UTIL_TYPE_TRAITS_REMOVE_POINTER_H
#define UTIL_TYPE_TRAITS_REMOVE_POINTER_H

#pragma once

#include "remove_cv.h"

namespace util {
    namespace type_traits {
        /**
         * type traits - remove pointer
         */
        namespace detail {
            template <typename Ty, typename>
            struct _remove_pointer {
                typedef Ty type;
            };

            template <typename Ty, typename TUP>
            struct _remove_pointer<Ty, TUP *> {
                typedef TUP type;
            };
        } // namespace detail

        template <typename Ty>
        struct remove_pointer {
            typedef typename detail::_remove_pointer<Ty, typename remove_cv<Ty>::type>::type type;
        };
    } // namespace type_traits
} // namespace util


#endif /* _UTIL_TYPE_TRAITS_REMOVE_POINTER_H_ */
