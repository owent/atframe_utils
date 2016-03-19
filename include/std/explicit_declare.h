/**
* @file explicit_declare.h
* @brief 导入继承关系约束<br />
* Licensed under the MIT licenses.
*
* @version 1.0
* @author OWenT, owt5008137@live.com
* @date 2013-12-25
*
* @history
*
*/
#ifndef _STD_EXPLICIT_DECLARE_H_
#define _STD_EXPLICIT_DECLARE_H_


#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

// ============================================================
// 公共包含部分
// C++0x/11/14 显示申明
// ============================================================

#define PARAM_IN
#define PARAM_OUT
#define PARAM_INOUT

// default,delete 显示生成默认构造函数申明
// VC11.0 以上分支判断
#if defined(_MSC_VER) && _MSC_VER >= 1700 && _HAS_CPP0X
    #define FUNC_DEFAULT = default
    #define FUNC_DELETE = delete
#elif defined(__clang__) && __clang_major__ >= 3 && defined(__cplusplus) && __cplusplus >= 201103L
    // clang
    #define FUNC_DEFAULT = default
    #define FUNC_DELETE = delete
#elif defined(__GNUC__) && __GNUC__ >= 4 && (__GNUC__ > 4 || __GNUC_MINOR__ >= 4) && defined(__cplusplus)
    // 采用GCC
    #if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
        #define FUNC_DEFAULT = default
        #define FUNC_DELETE = delete
    #else
        #define FUNC_DEFAULT {}
        #define FUNC_DELETE
    #endif
#else
    #define FUNC_DEFAULT {}
    #define FUNC_DELETE
#endif

// override ,final 显示生成默认构造函数申明
// VC10.0 以上分支判断
#if defined(_MSC_VER) && _MSC_VER >= 1600 && _HAS_CPP0X
    #define CLASS_OVERRIDE override
    #if _MSC_VER >= 1700
        #define CLASS_FINAL final
    #else
        #define CLASS_FINAL sealed
    #endif
#elif defined(__GNUC__) && __GNUC__ >= 4 && (__GNUC__ > 4 || __GNUC_MINOR__ >= 7) && defined(__cplusplus)
    // 采用GCC
    #if __cplusplus >= 201103L || defined(__GXX_EXPERIMENTAL_CXX0X__)
        #define CLASS_OVERRIDE override
        #define CLASS_FINAL final
    #else
        #define CLASS_OVERRIDE
        #define CLASS_FINAL
    #endif
#else
        #define CLASS_OVERRIDE
        #define CLASS_FINAL
#endif


// deprecated
// using: DEPRECATED_ATTR int a; class DEPRECATED_ATTR a;
// using: DEPRECATED_MSG("there is better choose") int a; class DEPRECATED_MSG("there is better choose") a;
#if defined(__cplusplus) && __cplusplus >= 201402L
#define DEPRECATED_ATTR [[deprecated]]
#elif defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#define DEPRECATED_ATTR __attribute__((deprecated))
#elif defined(_MSC_VER) && _MSC_VER >= 1400 //vs 2005 or higher
#define DEPRECATED_ATTR __declspec(deprecated) 
#else
#define DEPRECATED_ATTR
#endif 

#if defined(__cplusplus) && __cplusplus >= 201402L
#define DEPRECATED_MSG(msg) [[deprecated(msg)]]
#elif defined(__GNUC__) && ((__GNUC__ >= 4) || ((__GNUC__ == 3) && (__GNUC_MINOR__ >= 1)))
#define DEPRECATED_MSG(msg) __attribute__((deprecated(msg)))
#elif defined(_MSC_VER) && _MSC_VER >= 1400 //vs 2005 or higher
#define DEPRECATED_MSG(msg) __declspec(deprecated(msg)) 
#else
#define DEPRECATED_MSG(msg)
#endif 

#endif
