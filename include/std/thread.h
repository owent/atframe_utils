/**
* @file thread.h
* @brief 导入多线程特性支持兼容层
* Licensed under the MIT licenses.
*
* @version 1.0
* @author OWenT, owt5008137@live.com
* @date 2014.03.13
*
* @history
*   2015-06-06: 适配Android和IOS
*
*/

#ifndef _STD_THREAD_H_
#define _STD_THREAD_H_


#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

/**
 * 导入线程特性支持 (thread)
 * 规则如下
 * @see http://en.wikipedia.org/wiki/Thread-local_storage#C.2B.2B
 * @note 不支持 C++ Builder 编译器
 */
// IOS 不支持tls
#if defined(__APPLE__)
    #include <TargetConditionals.h>

    #if TARGET_OS_IPHONE || TARGET_OS_EMBEDDED || TARGET_IPHONE_SIMULATOR
        #define THREAD_TLS
    #endif
#endif

// android 不支持tls 
#if !defined(THREAD_TLS) && defined(__ANDROID__)
    #define THREAD_TLS
#endif

#if !defined(THREAD_TLS) && defined(__clang__)
    #if __has_feature(cxx_thread_local)
        #define THREAD_TLS thread_local
        #define THREAD_TLS_ENABLED 1
    #elif __has_feature(c_thread_local) || __has_extension(c_thread_local)
        #define THREAD_TLS _Thread_local
        #define THREAD_TLS_ENABLED 1
    #else
        #define THREAD_TLS __thread
        #define THREAD_TLS_ENABLED 1
    #endif
#endif

#if !defined(THREAD_TLS) && defined(__cplusplus) && __cplusplus >= 201103L
    #define THREAD_TLS thread_local
    #define THREAD_TLS_ENABLED 1
#endif

// VC 2003
#if !defined(THREAD_TLS)
    #if defined(_MSC_VER) && (_MSC_VER >= 1300)
        #define THREAD_TLS __declspec( thread )
        #define THREAD_TLS_ENABLED 1
    #else
        #define THREAD_TLS __thread
        #define THREAD_TLS_ENABLED 1
    #endif
#endif

#if !defined(THREAD_TLS)
    #define THREAD_TLS
#endif

#endif

