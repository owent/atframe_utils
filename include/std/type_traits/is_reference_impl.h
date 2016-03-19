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

#ifndef _UTIL_TYPE_TRAITS_IS_REFERENCE_IMPL_H_
#define _UTIL_TYPE_TRAITS_IS_REFERENCE_IMPL_H_

#include "const_common.h"


#if defined(__cplusplus) && __cplusplus >= 201103L
    #define UTIL_TYPE_TRAITS_HAS_RVALUE_REFS

#elif defined(__MWERKS__)
    #if __MWERKS__ > 0x3206 && __option(rvalue_refs)
        #define UTIL_TYPE_TRAITS_HAS_RVALUE_REFS
    #endif

#elif defined(__clang__) && defined(__has_feature)
    #if __has_feature(cxx_rvalue_references)
        #define UTIL_TYPE_TRAITS_HAS_RVALUE_REFS
    #endif

#elif defined(__GNUC__) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ > 2)) && defined(__GXX_EXPERIMENTAL_CXX0X__)
    #define UTIL_TYPE_TRAITS_HAS_RVALUE_REFS

#elif defined(__INTEL_COMPILER) || defined(__ICL) || defined(__ICC) || defined(__ECC)

    #if defined(__INTEL_COMPILER)
        #define _INTEL_CXX_VERSION __INTEL_COMPILER
    #elif defined(__ICL)
        #define _INTEL_CXX_VERSION __ICL
    #elif defined(__ICC)
        #define _INTEL_CXX_VERSION __ICC
    #elif defined(__ECC)
        #define _INTEL_CXX_VERSION __ECC
    #endif

    #if (!(defined(_WIN32) || defined(_WIN64)) && defined(__STDC_HOSTED__) && (__STDC_HOSTED__ && (_INTEL_CXX_VERSION <= 1200))) || defined(__GXX_EXPERIMENTAL_CPP0X__) || defined(__GXX_EXPERIMENTAL_CXX0X__)
        #define _INTEL_STDCXX0X
    #endif
    #if defined(_MSC_VER) && (_MSC_VER >= 1600)
        #define _INTEL_STDCXX0X
    #endif


    #if defined(_INTEL_STDCXX0X) && (_INTEL_CXX_VERSION >= 1200)
        #define UTIL_TYPE_TRAITS_HAS_RVALUE_REFS
    #endif

    #undef _INTEL_CXX_VERSION
    #ifdef _INTEL_STDCXX0X
        #undef _INTEL_STDCXX0X
    #endif

#elif defined(_MSC_VER) && _MSC_VER >= 1600
    #define UTIL_TYPE_TRAITS_HAS_RVALUE_REFS
#endif

namespace util
{
    namespace type_traits
    {
        namespace detail
        {
        }

        template <typename Ty>
        struct is_lvalue_reference : public false_type {};

        template <typename Ty>
        struct is_lvalue_reference<Ty&> : public true_type {};

        template <typename Ty>
        struct is_rvalue_reference : public false_type {};

        #ifdef UTIL_TYPE_TRAITS_HAS_RVALUE_REFS
        template <typename Ty>
        struct is_rvalue_reference<Ty&&> : public true_type {};
        #endif

        template <typename Ty>
        struct is_reference : public integral_constant<bool,
            ( is_lvalue_reference<Ty>::value ||
              is_rvalue_reference<Ty>::value
            )
        > {};

    }
}

#ifdef UTIL_TYPE_TRAITS_IS_UNION
    #undef UTIL_TYPE_TRAITS_IS_UNION
#endif

#endif /* _UTIL_TYPE_TRAITS_IS_REFERENCE_IMPL_H_ */
