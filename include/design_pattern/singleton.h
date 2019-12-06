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
 *   2019-12-05 移除对noncopyable的继承链，以适应用于dllexport时自带的 util::design_pattern::noncopyable 未导出的问题
 *              增加nomovable实现
 */

#ifndef UTILS_DESIGNPATTERN_SINGLETON_H
#define UTILS_DESIGNPATTERN_SINGLETON_H

#pragma once

#include <cstddef>
#include <memory>

#include <config/compiler_features.h>
#include <config/compile_optimize.h>

#include "lock/lock_holder.h"
#include "lock/spin_lock.h"
#include "std/smart_ptr.h"

/**
 * @brief if you are under Windows, you may want to declare import/export of singleton<T>
 * @usage UTIL_DESIGN_PATTERN_SINGLETON_IMPORT(Your Class Name)
 *        UTIL_DESIGN_PATTERN_SINGLETON_EXPORT(Your Class Name)
 */
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
    #define UTIL_DESIGN_PATTERN_SINGLETON_IMPORT(T)                                                 \
        template class UTIL_SYMBOL_IMPORT ::util::design_pattern::wrapper::singleton_wrapper<T>;    \
        template class UTIL_SYMBOL_IMPORT ::util::design_pattern::singleton<T>;
        
    #define UTIL_DESIGN_PATTERN_SINGLETON_EXPORT(T)                                                 \
        template class UTIL_SYMBOL_EXPORT ::util::design_pattern::wrapper::singleton_wrapper<T>;    \
        template class UTIL_SYMBOL_EXPORT ::util::design_pattern::singleton<T>;

#else
    #define UTIL_DESIGN_PATTERN_SINGLETON_IMPORT(T)
    #define UTIL_DESIGN_PATTERN_SINGLETON_EXPORT(T)
#endif

namespace util {
    namespace design_pattern {

        namespace wrapper {
            template <class T>
            class UTIL_SYMBOL_VISIBLE singleton_wrapper : public T {
            public:
                static bool destroyed_;
                ~singleton_wrapper() { destroyed_ = true; }
            };

            template <class T>
            bool singleton_wrapper<T>::destroyed_ = false;
        } // namespace wrapper

        template <typename T>
        class UTIL_SYMBOL_VISIBLE singleton {
        public:
            /**
             * @brief 自身类型声明
             */
            typedef T                          self_type;
            typedef std::shared_ptr<self_type> ptr_t;

        private:
            singleton(const singleton &) UTIL_CONFIG_DELETED_FUNCTION;
            singleton &operator=(const singleton &) UTIL_CONFIG_DELETED_FUNCTION;
#if defined(UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES) && UTIL_CONFIG_COMPILER_CXX_RVALUE_REFERENCES
            singleton(singleton &&) UTIL_CONFIG_DELETED_FUNCTION;
            singleton &operator=(singleton &&) UTIL_CONFIG_DELETED_FUNCTION;
#endif
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
            static T &get_instance() { return *me(); }

            /**
             * @brief 获取单件对象常量引用
             * @return const T& instance
             */
            static const T &get_const_instance() { return get_instance(); }

            /**
             * @brief 获取实例指针
             * @return T* instance
             */
            static self_type *instance() { return me().get(); }

            /**
             * @brief 获取原始指针
             * @return T* instance
             */
            static ptr_t &me() {
                static ptr_t inst;
                if (!inst) {
                    static util::lock::spin_lock                   lock;
                    util::lock::lock_holder<util::lock::spin_lock> lock_opr(lock);

                    UTIL_LOCK_ATOMIC_THREAD_FENCE(::util::lock::memory_order_acquire);
                    do {
                        if (inst) {
                            break;
                        }

                        ptr_t new_data = std::make_shared<wrapper::singleton_wrapper<self_type> >();
                        inst           = new_data;
                    } while (false);

                    UTIL_LOCK_ATOMIC_THREAD_FENCE(::util::lock::memory_order_release);
                    use(*inst);
                }

                return inst;
            }

            /**
             * @brief 判断是否已被析构
             * @return bool
             */
            static bool is_instance_destroyed() { return wrapper::singleton_wrapper<T>::destroyed_; }
        };
    } // namespace design_pattern
} // namespace util
#endif
