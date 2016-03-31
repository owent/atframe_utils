/**
 * @file spin_lock.h
 * @brief 自旋锁
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2013-3-18
 *
 * @note VC 2012+, GCC 4.4 + 使用C++0x/11实现实现原子操作
 * @note 低版本 VC使用InterlockedExchange等实现原子操作
 * @note 低版本 GCC采用__sync_lock_test_and_set等实现原子操作
 *
 * @history
 *     2013-12-20
 *         1. add support for clang & intel compiler
 *         2. add try unlock function
 *         3. fix atom operator
 *         4. add gcc atomic support
 *    2014-07-08
 *         1. add yield operation
 */

#ifndef _UTIL_LOCK_SPINLOCK_H_
#define _UTIL_LOCK_SPINLOCK_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#include <cstddef>
#if defined(__cplusplus) && __cplusplus >= 201103L
#include <atomic>
#define __UTIL_LOCK_SPINLOCK_ATOMIC_STD
#elif defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 1)) && __cplusplus >= 201103L
#include <atomic>
#define __UTIL_LOCK_SPINLOCK_ATOMIC_STD
#elif defined(_MSC_VER) && (_MSC_VER > 1700 || (defined(_HAS_CPP0X) && _HAS_CPP0X))
#include <atomic>
#define __UTIL_LOCK_SPINLOCK_ATOMIC_STD
#elif defined(__GNUC__) && ((__GNUC__ == 4 && __GNUC_MINOR__ >= 5) || __GNUC__ > 4) && defined(__GXX_EXPERIMENTAL_CXX0X__)
#include <atomic>
#define __UTIL_LOCK_SPINLOCK_ATOMIC_STD
#endif


/**
 * ==============================================
 * ======            asm pause             ======
 * ==============================================
 */
#if defined(_MSC_VER)
#include <windows.h> // YieldProcessor

/*
 * See: http://msdn.microsoft.com/en-us/library/windows/desktop/ms687419(v=vs.85).aspx
 * Not for intel c++ compiler, so ignore http://software.intel.com/en-us/forums/topic/296168
 */
#define __UTIL_LOCK_SPIN_LOCK_PAUSE() YieldProcessor()

#elif defined(__GNUC__) || defined(__clang__)
#if defined(__i386__) || defined(__x86_64__)
/**
 * See: Intel(R) 64 and IA-32 Architectures Software Developer's Manual V2
 * PAUSE-Spin Loop Hint, 4-57
 * http://www.intel.com/content/www/us/en/architecture-and-technology/64-ia-32-architectures-software-developer-instruction-set-reference-manual-325383.html?wapkw=instruction+set+reference
 */
#define __UTIL_LOCK_SPIN_LOCK_PAUSE() __asm__ __volatile__("pause")
#elif defined(__ia64__) || defined(__ia64)
/**
 * See: Intel(R) Itanium(R) Architecture Developer's Manual, Vol.3
 * hint - Performance Hint, 3:145
 * http://www.intel.com/content/www/us/en/processors/itanium/itanium-architecture-vol-3-manual.html
 */
#define __UTIL_LOCK_SPIN_LOCK_PAUSE() __asm__ __volatile__("hint @pause")
#elif defined(__arm__) && !defined(__ANDROID__)
/**
 * See: ARM Architecture Reference Manuals (YIELD)
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.subset.architecture.reference/index.html
 */
#define __UTIL_LOCK_SPIN_LOCK_PAUSE() __asm__ __volatile__("yield")
#endif

#endif /*compilers*/

// set pause do nothing
#if !defined(__UTIL_LOCK_SPIN_LOCK_PAUSE)
#define __UTIL_LOCK_SPIN_LOCK_PAUSE()
#endif /*!defined(CAPO_SPIN_LOCK_PAUSE)*/


/**
 * ==============================================
 * ======            cpu yield             ======
 * ==============================================
 */
#if defined(_MSC_VER)
#define __UTIL_LOCK_SPIN_LOCK_CPU_YIELD() SwitchToThread()

#elif defined(__linux__) || defined(__unix__)
#include <sched.h>
#define __UTIL_LOCK_SPIN_LOCK_CPU_YIELD() sched_yield()
#endif

#ifndef __UTIL_LOCK_SPIN_LOCK_CPU_YIELD
#define __UTIL_LOCK_SPIN_LOCK_CPU_YIELD() __UTIL_LOCK_SPIN_LOCK_PAUSE()
#endif

/**
 * ==============================================
 * ======           thread yield           ======
 * ==============================================
 */
#if defined(__UTIL_LOCK_SPINLOCK_ATOMIC_STD)
#include <thread>
#include <chrono>
#define __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD() ::std::this_thread::yield()
#define __UTIL_LOCK_SPIN_LOCK_THREAD_SLEEP() ::std::this_thread::sleep_for(::std::chrono::milliseconds(1))
#elif defined(_MSC_VER)
#define __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD() Sleep(0)
#define __UTIL_LOCK_SPIN_LOCK_THREAD_SLEEP() Sleep(1)
#endif

#ifndef __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD
#define __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD()
#define __UTIL_LOCK_SPIN_LOCK_THREAD_SLEEP() __UTIL_LOCK_SPIN_LOCK_CPU_YIELD()
#endif

/**
 * ==============================================
 * ======           spin lock wait         ======
 * ==============================================
 * @note
 *   1. busy-wait
 *   2. asm pause
 *   3. thread give up cpu time slice but will not switch to another process
 *   4. thread give up cpu time slice (may switch to another process)
 *   5. sleep (will switch to another process when necessary)
 */

#define __UTIL_LOCK_SPIN_LOCK_WAIT(x)                                 \
    {                                                                 \
        unsigned char try_lock_times = static_cast<unsigned char>(x); \
        if (try_lock_times < 4) {                                     \
        } else if (try_lock_times < 16) {                             \
            __UTIL_LOCK_SPIN_LOCK_PAUSE();                            \
        } else if (try_lock_times < 32) {                             \
            __UTIL_LOCK_SPIN_LOCK_THREAD_YIELD();                     \
        } else if (try_lock_times < 64) {                             \
            __UTIL_LOCK_SPIN_LOCK_CPU_YIELD();                        \
        } else {                                                      \
            __UTIL_LOCK_SPIN_LOCK_THREAD_SLEEP();                     \
        }                                                             \
    }


namespace util {
    namespace lock {
#ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_STD
        /**
         * @brief 自旋锁 - C++ 0x/11版实现
         * @see http://www.boost.org/doc/libs/1_60_0/doc/html/atomic/usage_examples.html#boost_atomic.usage_examples.example_spinlock
         */
        class spin_lock {
        private:
            typedef enum { UNLOCKED = 0, LOCKED = 1 } lock_state_t;
            ::std::atomic_uint lock_status_;

        public:
            spin_lock() { lock_status_.store(UNLOCKED); }

            void lock() {
                unsigned char try_times = 0;
                while (lock_status_.exchange(static_cast<unsigned int>(LOCKED), ::std::memory_order_acq_rel) == LOCKED)
                    __UTIL_LOCK_SPIN_LOCK_WAIT(try_times++); /* busy-wait */
            }

            void unlock() { lock_status_.store(static_cast<unsigned int>(UNLOCKED), ::std::memory_order_release); }

            bool is_locked() { return lock_status_.load(::std::memory_order_acquire) == LOCKED; }

            bool try_lock() { return lock_status_.exchange(static_cast<unsigned int>(LOCKED), ::std::memory_order_acq_rel) == UNLOCKED; }

            bool try_unlock() { return lock_status_.exchange(static_cast<unsigned int>(UNLOCKED), ::std::memory_order_acq_rel) == LOCKED; }
        };
#else

#if defined(__clang__)
#if !defined(__GCC_ATOMIC_INT_LOCK_FREE) && (!defined(__GNUC__) || __GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1))
#error Clang version is too old
#endif
#if defined(__GCC_ATOMIC_INT_LOCK_FREE)
#define __UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC 1
#else
#define __UTIL_LOCK_SPINLOCK_ATOMIC_GCC 1
#endif
#elif defined(_MSC_VER)
#include <WinBase.h>
#define __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC 1

#elif defined(__GNUC__) || defined(__clang__) || defined(__clang__) || defined(__INTEL_COMPILER)
#if defined(__GNUC__) && (__GNUC__ < 4 || (__GNUC__ == 4 && __GNUC_MINOR__ < 1))
#error GCC version must be greater or equal than 4.1
#endif

#if defined(__INTEL_COMPILER) && __INTEL_COMPILER < 1100
#error Intel Compiler version must be greater or equal than 11.0
#endif

#if defined(__GCC_ATOMIC_INT_LOCK_FREE)
#define __UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC 1
#else
#define __UTIL_LOCK_SPINLOCK_ATOMIC_GCC 1
#endif
#else
#error Currently only gcc, msvc, intel compiler & llvm-clang are supported
#endif

        class spin_lock {
        private:
            typedef enum { UNLOCKED = 0, LOCKED = 1 } lock_state_t;
            volatile unsigned int lock_status_;

        public:
            spin_lock() : lock_status_(UNLOCKED) {}

            void lock() {
                unsigned char try_times = 0;
#ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
                while (InterlockedExchange(&lock_status_, LOCKED) == LOCKED)
                    __UTIL_LOCK_SPIN_LOCK_WAIT(try_times++); /* busy-wait */
#elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
                while (__atomic_exchange_n(&lock_status_, LOCKED, __ATOMIC_ACQ_REL) == LOCKED)
                    __UTIL_LOCK_SPIN_LOCK_WAIT(try_times++); /* busy-wait */
#else
                while (__sync_lock_test_and_set(&lock_status_, LOCKED) == LOCKED)
                    __UTIL_LOCK_SPIN_LOCK_WAIT(try_times++); /* busy-wait */
#endif
            }

            void unlock() {
#ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
                InterlockedExchange(&lock_status_, UNLOCKED);
#elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
                __atomic_store_n(&lock_status_, UNLOCKED, __ATOMIC_RELEASE);
#else
                __sync_lock_release(&lock_status_, UNLOCKED);
#endif
            }

            bool is_locked() {
#ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
                return InterlockedExchangeAdd(&lock_status_, 0) == LOCKED;
#elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
                return __atomic_load_n(&lock_status_, __ATOMIC_ACQUIRE) == LOCKED;
#else
                return __sync_add_and_fetch(&lock_status_, 0) == LOCKED;
#endif
            }

            bool try_lock() {

#ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
                return InterlockedExchange(&lock_status_, LOCKED) == UNLOCKED;
#elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
                return __atomic_exchange_n(&lock_status_, LOCKED, __ATOMIC_ACQ_REL) == UNLOCKED;
#else
                return __sync_bool_compare_and_swap(&lock_status_, UNLOCKED, LOCKED);
#endif
            }

            bool try_unlock() {
#ifdef __UTIL_LOCK_SPINLOCK_ATOMIC_MSVC
                return InterlockedExchange(&lock_status_, UNLOCKED) == LOCKED;
#elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
                return __atomic_exchange_n(&lock_status_, UNLOCKED, __ATOMIC_ACQ_REL) == LOCKED;
#else
                return __sync_bool_compare_and_swap(&lock_status_, LOCKED, UNLOCKED);
#endif
            }
        };

#endif
    }
}

#endif /* SPINLOCK_H_ */
