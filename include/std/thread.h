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
#pragma once
#endif

// ===================================== thread local storage =====================================
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
#if !defined(THREAD_TLS) && defined(_MSC_VER)
#if _MSC_VER >= 1900
#define THREAD_TLS thread_local
#define THREAD_TLS_ENABLED 1
#elif _MSC_VER >= 1300
#define THREAD_TLS __declspec(thread)
#define THREAD_TLS_ENABLED 1
#else
#define THREAD_TLS __thread
#define THREAD_TLS_ENABLED 1
#endif
#endif

#if !defined(THREAD_TLS)
#define THREAD_TLS
#endif


// ===================================== thread sleep & yield =====================================
#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1800)
#include <thread>
#define THREAD_SLEEP_MS(x) std::this_thread::sleep_for(std::chrono::milliseconds(x))
#define THREAD_YIELD() std::this_thread::yield()

#elif defined(_MSC_VER)
#include <Windows.h>
#define THREAD_SLEEP_MS(x) Sleep(x)
#define THREAD_YIELD() YieldProcessor()

#else
#include <unistd.h>

#define THREAD_SLEEP_MS(x)                      \
    ((x > 1000) ? sleep(x / 1000) : usleep(0)); \
    usleep((x % 1000) * 1000)
#if defined(__linux__) || defined(__unix__)
#include <sched.h>
#define THREAD_YIELD() sched_yield()
#elif defined(__GNUC__) || defined(__clang__)
#if defined(__i386__) || defined(__x86_64__)
/**
* See: Intel(R) 64 and IA-32 Architectures Software Developer's Manual V2
* PAUSE-Spin Loop Hint, 4-57
* http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.html?wapkw=instruction+set+reference
*/
#define THREAD_YIELD() __asm__ __volatile__("pause")
#elif defined(__ia64__) || defined(__ia64)
/**
* See: Intel(R) Itanium(R) Architecture Developer's Manual, Vol.3
* hint - Performance Hint, 3:145
* http://www.intel.com/content/www/us/en/processors/itanium/itanium-architecture-vol-3-manual.html
*/
#define THREAD_YIELD() __asm__ __volatile__("hint @pause")
#elif defined(__arm__) && !defined(__ANDROID__)
/**
* See: ARM Architecture Reference Manuals (YIELD)
* http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.subset.architecture.reference/index.html
*/
#define THREAD_YIELD() __asm__ __volatile__("yield")
#else
#define THREAD_YIELD()
#endif
#else
#define THREAD_YIELD()
#endif

#endif

#endif
