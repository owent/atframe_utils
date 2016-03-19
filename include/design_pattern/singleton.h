/**
 * @file singleton.h
 * @brief 单件模式基类, 参考 boost::serialization::singleton,去除了Debug锁操作 <br />
 *        实例的初始化会在模块载入（如果是动态链接库则是载入动态链接库）时启动 <br />
 *        在模块卸载时会自动析构 <br />
 *
 * Note that this singleton class is thread-safe.
 *
 * @version 1.0
 * @author owent
 * @date 2012.02.13
 *
 * @history
 *   2012.07.20 为线程安全而改进实现方式
 *   2015.01.10 改为使用双检锁实现线程安全
 *   2015.11.02 增加内存屏障，保证极端情况下多线程+编译优化导致的指令乱序问题
 */

#ifndef _UTILS_DESIGNPATTERN_SINGLETON_H_
#define _UTILS_DESIGNPATTERN_SINGLETON_H_

#pragma once

#include "noncopyable.h"

#include "lock/spin_lock.h"
#include "std/smart_ptr.h"
#include <cstddef>
#include <memory>

namespace util {
    namespace design_pattern {

        namespace wrapper {
            template<class T>
            class singleton_wrapper : public T {
            public:
                static bool destroyed_;
                ~singleton_wrapper() {
                    destroyed_ = true;
                }
            };

            template<class T>
            bool singleton_wrapper< T >::destroyed_ = false;

        }

        template <typename T>
        class singleton : public noncopyable {
        public:
            /**
             * @brief 自身类型声明
             */
            typedef T self_type;
            typedef std::shared_ptr<self_type> ptr_t;

        protected:

            /**
             * @brief 虚类，禁止直接构造
             */
            singleton() {}

            /**
             * @brief 用于在初始化阶段强制构造单件实例
             */
            static void use(self_type const &) {}

        public:
            /**
             * @brief 获取单件对象引用
             * @return T& instance
             */
            static T& get_instance() {
                return *me();
            }

            /**
             * @brief 获取单件对象常量引用
             * @return const T& instance
             */
            static const T& get_const_instance() {
                return get_instance();
            }

            /**
             * @brief 获取实例指针
             * @return T* instance
             */
            static self_type* instance() {
                return me().get();
            }

            /**
            * @brief 获取原始指针
            * @return T* instance
            */
            static ptr_t& me() {
                static ptr_t inst;
                if (!inst) {
                    static util::lock::spin_lock lock;
                    lock.lock();

#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && (_MSC_VER > 1700 || (defined(_HAS_CPP0X) && _HAS_CPP0X)))
                    std::atomic_thread_fence(std::memory_order_acquire);
#elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
                    __atomic_thread_fence(__ATOMIC_ACQUIRE);
#endif
                    do {
                        if (inst) {
                            break;
                        }

                        ptr_t new_data = std::make_shared<wrapper::singleton_wrapper<self_type> >();
                        inst = new_data;
                    } while (false);

#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && (_MSC_VER > 1700 || (defined(_HAS_CPP0X) && _HAS_CPP0X)))
                    std::atomic_thread_fence(std::memory_order_release);
#elif defined(__UTIL_LOCK_SPINLOCK_ATOMIC_GCC_ATOMIC)
                    __atomic_thread_fence(__ATOMIC_RELEASE);
#endif
                    lock.unlock();
                    use(*inst);
                }

                return inst;
            }

            /**
             * @brief 判断是否已被析构
             * @return bool
             */
            static bool is_instance_destroyed() {
                return wrapper::singleton_wrapper<T>::destroyed_;
            }
        };

    }
}
#endif
