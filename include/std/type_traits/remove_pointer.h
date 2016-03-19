/**
 * Copyright (c) 2014, Tencent
 * All rights reserved.
 *
 * @file remove_pointer.h
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

#ifndef _UTIL_TYPE_TRAITS_REMOVE_POINTER_H_
#define _UTIL_TYPE_TRAITS_REMOVE_POINTER_H_

#include "remove_cv.h"

namespace util
{
    namespace type_traits
    {
        /**
         * type traits - remove pointer
         */
        namespace detail
        {
            template<typename Ty, typename>
            struct _remove_pointer
            {
                typedef Ty type;
            };

            template<typename Ty, typename TUP>
            struct _remove_pointer<Ty, TUP*>
            {
                typedef TUP type;
            };
        }

        template<typename Ty>
        struct remove_pointer
        {
            typedef typename detail::_remove_pointer<Ty, typename remove_cv<Ty>::type>::type type;
        };
    }
}


#endif /* _UTIL_TYPE_TRAITS_REMOVE_POINTER_H_ */
