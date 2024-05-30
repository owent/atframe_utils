// Copyright 2021 atframework
// Created by owent on 2012-02-13

/**
 * @brief 单件模式基类, 参考 boost::serialization::singleton,去除了Debug锁操作 <br />
 *        实例的初始化会在模块载入（如果是动态链接库则是载入动态链接库）时启动 <br />
 *        在模块卸载时会自动析构 <br />
 *
 * Note that this singleton class is thread-safe.
 *
 *
 * @history
 *   2012.07.20 为线程安全而改进实现方式
 *   2015.01.10 改为使用双检锁实现线程安全
 *   2015.11.02 增加内存屏障，保证极端情况下多线程+编译优化导致的指令乱序问题
 *   2019-12-05 移除对noncopyable的继承链，以适应用于dllexport时自带的
 * util::design_pattern::noncopyable 未导出的问题 增加nomovable实现 2020-11-27
 * 增加基于C++11标准 N2660 的实现 优化销毁判定内存区的放置 优化初始化顺序 增加符号不导出的单例基类 local_singleton<T>
 *              单例接口返回const的智能指针，不再允许替换单例对象
 *   2023-07-09 修订宏接口，修订更严谨的符号导出问题
 *
 * @note 如果在Windows下dll请使用宏来生成函数
 *          UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DECL(类名): visiable 标记(.lib)
 *          UTIL_DESIGN_PATTERN_SINGLETON_IMPORT_DECL(类名): __attribute__((__dllimport__))/__declspec(dllimport)
 * 标记(从其他dll中导入) UTIL_DESIGN_PATTERN_SINGLETON_EXPORT_DECL(类名):
 * __attribute__((__dllexport__))/__declspec(dllexport) 标记(dll导出符号)
 *       然后需要在源文件中请使用宏来定义部分静态数据的存放位置
 *          UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DATA_DEFINITION(类名): visiable 标记(.lib)
 *          UTIL_DESIGN_PATTERN_SINGLETON_IMPORT_DATA_DEFINITION(类名):
 * __attribute__((__dllimport__))/__declspec(dllimport) 标记(从其他dll中导入)
 *          UTIL_DESIGN_PATTERN_SINGLETON_EXPORT_DATA_DEFINITION(类名):
 * __attribute__((__dllexport__))/__declspec(dllexport) 标记(dll导出符号)
 *       如果单例对象不需要导出则也可以直接util::design_pattern::singleton<类名>或者用VISIBLE导出规则
 * @example
 *      // singleton_class.h
 *      class singleton_class : public util::design_pattern::singleton<singleton_class> {};
 * @example
 *      // singleton_class.h
 *      class singleton_class {
 *      #if (defined(LIB_EXPORT) && LIB_EXPORT) || (defined(EXE_EXPORT) && EXE_EXPORT)
 *         UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DECL(singleton_class)
 *      #elif defined(DLL_EXPORT) && DLL_EXPORT
 *         UTIL_DESIGN_PATTERN_SINGLETON_EXPORT_DECL(singleton_class)
 *      #else
 *         UTIL_DESIGN_PATTERN_SINGLETON_IMPORT_DECL(singleton_class)
 *      #endif
 *      };
 *      // singleton_class.cpp
 *      #if (defined(LIB_EXPORT) && LIB_EXPORT) || (defined(EXE_EXPORT) && EXE_EXPORT)
 *          UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DATA_DEFINITION(singleton_class);
 *      #elif defined(DLL_EXPORT) && DLL_EXPORT
 *          UTIL_DESIGN_PATTERN_SINGLETON_IMPORT_DATA_DEFINITION(singleton_class);
 *      #else
 *          UTIL_DESIGN_PATTERN_SINGLETON_EXPORT_DATA_DEFINITION(singleton_class);
 *      #endif
 */

#ifndef UTILS_DESIGNPATTERN_SINGLETON_H
#define UTILS_DESIGNPATTERN_SINGLETON_H

#pragma once

#include <config/compile_optimize.h>
#include <config/compiler_features.h>

#include <cstddef>
#include <memory>

#include "lock/lock_holder.h"
#include "lock/spin_lock.h"

/**
 * @brief if you are under Windows, you may want to declare import/export of singleton<T>
 * @usage UTIL_DESIGN_PATTERN_SINGLETON_IMPORT(Your Class Name)
 *        UTIL_DESIGN_PATTERN_SINGLETON_EXPORT(Your Class Name)
 */
#if defined(_WIN32) || defined(__WIN32__) || defined(WIN32) || defined(__CYGWIN__)
#  define UTIL_DESIGN_PATTERN_SINGLETON_IMPORT(T)                                                                      \
    UTIL_SYMBOL_IMPORT bool LIBATFRAME_UTILS_NAMESPACE_ID::design_pattern::wrapper::singleton_wrapper<T>::destroyed_ = \
        false;                                                                                                         \
    template class UTIL_SYMBOL_IMPORT LIBATFRAME_UTILS_NAMESPACE_ID::design_pattern::singleton<T>;

#  define UTIL_DESIGN_PATTERN_SINGLETON_EXPORT(T)                                                                      \
    UTIL_SYMBOL_EXPORT bool LIBATFRAME_UTILS_NAMESPACE_ID::design_pattern::wrapper::singleton_wrapper<T>::destroyed_ = \
        false;                                                                                                         \
    template class UTIL_SYMBOL_EXPORT LIBATFRAME_UTILS_NAMESPACE_ID::design_pattern::singleton<T>;

#else
#  define UTIL_DESIGN_PATTERN_SINGLETON_IMPORT(T)
#  define UTIL_DESIGN_PATTERN_SINGLETON_EXPORT(T)
#endif

#define UTIL_DESIGN_PATTERN_SINGLETON_NOMAVLBLE(CLAZZ) \
  CLAZZ(CLAZZ &&) = delete;                            \
  CLAZZ &operator=(CLAZZ &&) = delete;

#if defined(__cpp_threadsafe_static_init)
#  if __cpp_threadsafe_static_init >= 200806L
#    define UTIL_DESIGN_PATTERN_SINGLETON_DATA_IMPL_N2660 1
#  endif
#elif defined(__cplusplus)
// @see https://gcc.gnu.org/projects/cxx-status.html
// @see http://clang.llvm.org/cxx_status.html
#  if __cplusplus >= 201103L
#    define UTIL_DESIGN_PATTERN_SINGLETON_DATA_IMPL_N2660 1
#  endif
#elif defined(_MSC_VER) && defined(_MSVC_LANG)
// @see https://docs.microsoft.com/en-us/cpp/build/reference/zc-threadsafeinit-thread-safe-local-static-initialization
#  if _MSC_VER >= 1900 && _MSVC_LANG >= 201103L
#    define UTIL_DESIGN_PATTERN_SINGLETON_DATA_IMPL_N2660 1
#  endif
#endif

#if defined(UTIL_DESIGN_PATTERN_SINGLETON_DATA_IMPL_N2660) && UTIL_DESIGN_PATTERN_SINGLETON_DATA_IMPL_N2660

// @see https://wg21.link/n2660
#  define UTIL_DESIGN_PATTERN_SINGLETON_DATA_IMPL(LABEL, CLAZZ, BASE_CLAZZ)                                            \
   private:                                                                                                            \
    template <class TCLASS>                                                                                            \
    class singleton_wrapper_permission_t : public TCLASS {};                                                           \
    class LABEL singleton_wrapper_t {                                                                                  \
     public:                                                                                                           \
      using ptr_permission_t = std::shared_ptr<singleton_wrapper_permission_t<CLAZZ> >;                                \
      using ptr_t = std::shared_ptr<CLAZZ>;                                                                            \
      static bool __is_destroyed;                                                                                      \
      struct deleter {                                                                                                 \
        void operator()(singleton_wrapper_permission_t<CLAZZ> *p) const {                                              \
          __is_destroyed = true;                                                                                       \
          UTIL_LOCK_ATOMIC_THREAD_FENCE(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_release);                    \
          delete p;                                                                                                    \
        }                                                                                                              \
      };                                                                                                               \
      friend struct deleter;                                                                                           \
      static const ptr_t &me() {                                                                                       \
        static ptr_t data =                                                                                            \
            std::static_pointer_cast<CLAZZ>(ptr_permission_t(new singleton_wrapper_permission_t<CLAZZ>(), deleter())); \
        return data;                                                                                                   \
      }                                                                                                                \
    };                                                                                                                 \
                                                                                                                       \
   private:                                                                                                            \
    friend class singleton_wrapper_t;

#  define UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DECL_IMPL(LABEL, CLAZZ) \
   private:                                                            \
    class UTIL_SYMBOL_LOCAL singleton_wrapper_type {                   \
     public:                                                           \
      using ptr_t = std::shared_ptr<CLAZZ>;                            \
                                                                       \
      struct deleter {                                                 \
        deleter();                                                     \
        deleter(const deleter &);                                      \
        ~deleter();                                                    \
        void operator()(CLAZZ *p) const noexcept;                      \
      };                                                               \
      static const ptr_t &me();                                        \
      static bool is_instance_destroyed() noexcept;                    \
                                                                       \
     private:                                                          \
      static bool __is_destroyed;                                      \
    };                                                                 \
                                                                       \
   private:                                                            \
    friend class singleton_wrapper_type;

#  define UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DEFINITION_IMPL(LABEL, CLAZZ)                                          \
    UTIL_SYMBOL_LOCAL CLAZZ::singleton_wrapper_type::deleter::deleter() {}                                            \
    UTIL_SYMBOL_LOCAL CLAZZ::singleton_wrapper_type::deleter::deleter(const deleter &) {}                             \
    UTIL_SYMBOL_LOCAL CLAZZ::singleton_wrapper_type::deleter::~deleter() {}                                           \
    UTIL_SYMBOL_LOCAL void CLAZZ::singleton_wrapper_type::deleter::operator()(CLAZZ *p) const noexcept {              \
      __is_destroyed = true;                                                                                          \
      UTIL_LOCK_ATOMIC_THREAD_FENCE(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_release);                       \
      delete p;                                                                                                       \
    }                                                                                                                 \
    UTIL_SYMBOL_LOCAL const CLAZZ::singleton_wrapper_type::ptr_t &CLAZZ::singleton_wrapper_type::me() {               \
      static ptr_t data = ptr_t(new CLAZZ(), deleter());                                                              \
      return data;                                                                                                    \
    }                                                                                                                 \
    UTIL_SYMBOL_LOCAL bool CLAZZ::singleton_wrapper_type::is_instance_destroyed() noexcept { return __is_destroyed; } \
    UTIL_SYMBOL_LOCAL bool CLAZZ::singleton_wrapper_type::__is_destroyed = false

#else

#  define UTIL_DESIGN_PATTERN_SINGLETON_DATA_IMPL(LABEL, CLAZZ, BASE_CLAZZ)                                          \
   private:                                                                                                          \
    template <class TCLASS>                                                                                          \
    class singleton_wrapper_permission_t : public TCLASS {};                                                         \
    class LABEL singleton_data_t {                                                                                   \
     public:                                                                                                         \
      std::shared_ptr<CLAZZ> instance;                                                                               \
      LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_lock lock;                                                           \
    };                                                                                                               \
    class LABEL singleton_wrapper_t {                                                                                \
     public:                                                                                                         \
      using ptr_permission_t = std::shared_ptr<singleton_wrapper_permission_t<CLAZZ> >;                              \
      using ptr_t = std::shared_ptr<CLAZZ>;                                                                          \
      static bool __is_destroyed;                                                                                    \
      struct deleter {                                                                                               \
        void operator()(singleton_wrapper_permission_t<CLAZZ> *p) const {                                            \
          __is_destroyed = true;                                                                                     \
          UTIL_LOCK_ATOMIC_THREAD_FENCE(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_release);                  \
          delete p;                                                                                                  \
        }                                                                                                            \
      };                                                                                                             \
      friend struct deleter;                                                                                         \
      static const ptr_t &me() {                                                                                     \
        if (!__data().instance) {                                                                                    \
          LIBATFRAME_UTILS_NAMESPACE_ID::lock::lock_holder<LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_lock> lock_opr( \
              __data().lock);                                                                                        \
          UTIL_LOCK_ATOMIC_THREAD_FENCE(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acquire);                  \
          do {                                                                                                       \
            if (__data().instance) {                                                                                 \
              break;                                                                                                 \
            }                                                                                                        \
            ptr_t new_data = std::static_pointer_cast<CLAZZ>(                                                        \
                ptr_permission_t(new singleton_wrapper_permission_t<CLAZZ>(), deleter()));                           \
            __data().instance = new_data;                                                                            \
          } while (false);                                                                                           \
          UTIL_LOCK_ATOMIC_THREAD_FENCE(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_release);                  \
          __use(*__data().instance);                                                                                 \
        }                                                                                                            \
        return __data().instance;                                                                                    \
      }                                                                                                              \
    };                                                                                                               \
                                                                                                                     \
   private:                                                                                                          \
    friend class singleton_wrapper_t;                                                                                \
    static LABEL void __use(CLAZZ const &) {}                                                                        \
    static LABEL singleton_data_t &__data() {                                                                        \
      static singleton_data_t data;                                                                                  \
      return data;                                                                                                   \
    }

#  define UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DECL_IMPL(LABEL, CLAZZ) \
   private:                                                            \
    class UTIL_SYMBOL_LOCAL singleton_data_type {                      \
     public:                                                           \
      std::shared_ptr<CLAZZ> instance;                                 \
      LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_lock lock;             \
    };                                                                 \
    class UTIL_SYMBOL_LOCAL singleton_wrapper_type {                   \
     public:                                                           \
      using ptr_t = std::shared_ptr<CLAZZ>;                            \
                                                                       \
      struct deleter {                                                 \
        deleter();                                                     \
        deleter(const deleter &);                                      \
        ~deleter();                                                    \
        void operator()(CLAZZ *p) const noexcept;                      \
      };                                                               \
      static const ptr_t &me();                                        \
      static bool is_instance_destroyed();                             \
                                                                       \
     private:                                                          \
      static bool __is_destroyed;                                      \
      static void __use(CLAZZ const &);                                \
      static singleton_data_type &__data();                            \
    };                                                                 \
                                                                       \
   private:                                                            \
    friend class singleton_wrapper_type;

#  define UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DEFINITION_IMPL(LABEL, CLAZZ)                                       \
    UTIL_SYMBOL_LOCAL CLAZZ::singleton_wrapper_type::deleter::deleter() {}                                         \
    UTIL_SYMBOL_LOCAL CLAZZ::singleton_wrapper_type::deleter::deleter(const deleter &) {}                          \
    UTIL_SYMBOL_LOCAL CLAZZ::singleton_wrapper_type::deleter::~deleter() {}                                        \
    UTIL_SYMBOL_LOCAL void CLAZZ::singleton_wrapper_type::deleter::operator()(CLAZZ *p) const noexcept {           \
      __is_destroyed = true;                                                                                       \
      UTIL_LOCK_ATOMIC_THREAD_FENCE(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_release);                    \
      delete p;                                                                                                    \
    }                                                                                                              \
    UTIL_SYMBOL_LOCAL const CLAZZ::singleton_wrapper_type::ptr_t &CLAZZ::singleton_wrapper_type::me() {            \
      if (!__data().instance) {                                                                                    \
        LIBATFRAME_UTILS_NAMESPACE_ID::lock::lock_holder<LIBATFRAME_UTILS_NAMESPACE_ID::lock::spin_lock> lock_opr( \
            __data().lock);                                                                                        \
        UTIL_LOCK_ATOMIC_THREAD_FENCE(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_acquire);                  \
        do {                                                                                                       \
          if (__data().instance) {                                                                                 \
            break;                                                                                                 \
          }                                                                                                        \
          ptr_t new_data = ptr_t(new CLAZZ(), deleter());                                                          \
          __data().instance = new_data;                                                                            \
        } while (false);                                                                                           \
        UTIL_LOCK_ATOMIC_THREAD_FENCE(LIBATFRAME_UTILS_NAMESPACE_ID::lock::memory_order_release);                  \
        __use(*__data().instance);                                                                                 \
      }                                                                                                            \
      return __data().instance;                                                                                    \
    }                                                                                                              \
    UTIL_SYMBOL_LOCAL void CLAZZ::singleton_wrapper_type::__use(CLAZZ const &) {}                                  \
    UTIL_SYMBOL_LOCAL CLAZZ::singleton_data_type &CLAZZ::singleton_wrapper_type::__data() {                        \
      static singleton_data_type data;                                                                             \
      return data;                                                                                                 \
    }                                                                                                              \
    UTIL_SYMBOL_LOCAL bool CLAZZ::singleton_wrapper_type::__is_destroyed = false

#endif

#define UTIL_DESIGN_PATTERN_SINGLETON_DEF_FUNCS(LABEL, CLAZZ, BASE_CLAZZ)                   \
  UTIL_DESIGN_PATTERN_SINGLETON_DATA_IMPL(LABEL, CLAZZ, BASE_CLAZZ)                         \
  BASE_CLAZZ(const BASE_CLAZZ &) = delete;                                                  \
  BASE_CLAZZ &operator=(const BASE_CLAZZ &) = delete;                                       \
  UTIL_DESIGN_PATTERN_SINGLETON_NOMAVLBLE(BASE_CLAZZ)                                       \
 public:                                                                                    \
  static LABEL CLAZZ &get_instance() { return *singleton_wrapper_t::me(); }                 \
  static LABEL const CLAZZ &get_const_instance() { return get_instance(); }                 \
  static LABEL CLAZZ *instance() { return singleton_wrapper_t::me().get(); }                \
  static LABEL const std::shared_ptr<CLAZZ> &me() { return singleton_wrapper_t::me(); }     \
  static LABEL bool is_instance_destroyed() { return singleton_wrapper_t::__is_destroyed; } \
                                                                                            \
 private:

#define UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DECL(LABEL, CLAZZ, BASE_CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DECL_IMPL(LABEL, CLAZZ)              \
  BASE_CLAZZ(const BASE_CLAZZ &) = delete;                                  \
  BASE_CLAZZ &operator=(const BASE_CLAZZ &) = delete;                       \
  UTIL_DESIGN_PATTERN_SINGLETON_NOMAVLBLE(BASE_CLAZZ)                       \
 public:                                                                    \
  static LABEL CLAZZ &get_instance();                                       \
  static LABEL const CLAZZ &get_const_instance();                           \
  static LABEL CLAZZ *instance();                                           \
  static LABEL const std::shared_ptr<CLAZZ> &me();                          \
  static LABEL bool is_instance_destroyed();                                \
                                                                            \
 private:

#define UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DEFINITION(LABEL, CLAZZ, BASE_CLAZZ)          \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DEFINITION_IMPL(LABEL, CLAZZ);                      \
  LABEL CLAZZ &CLAZZ::get_instance() { return *singleton_wrapper_type::me(); }             \
  LABEL const CLAZZ &CLAZZ::get_const_instance() { return get_instance(); }                \
  LABEL CLAZZ *CLAZZ::instance() { return singleton_wrapper_type::me().get(); }            \
  LABEL const std::shared_ptr<CLAZZ> &CLAZZ::me() { return singleton_wrapper_type::me(); } \
  LABEL bool CLAZZ::is_instance_destroyed() { return singleton_wrapper_type::is_instance_destroyed(); }

#define UTIL_DESIGN_PATTERN_SINGLETON_IMPORT_DECL(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DECL(UTIL_SYMBOL_IMPORT, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_EXPORT_DECL(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DECL(UTIL_SYMBOL_EXPORT, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DECL(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DECL(UTIL_SYMBOL_VISIBLE, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_LOCAL_DECL(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DECL(UTIL_SYMBOL_LOCAL, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_IMPORT_DATA_DEFINITION(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DEFINITION(UTIL_SYMBOL_IMPORT, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_EXPORT_DATA_DEFINITION(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DEFINITION(UTIL_SYMBOL_EXPORT, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DATA_DEFINITION(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DEFINITION(UTIL_SYMBOL_VISIBLE, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_LOCAL_DATA_DEFINITION(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_MEMBER_DEFINITION(UTIL_SYMBOL_LOCAL, CLAZZ, CLAZZ)

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace design_pattern {
template <class T>
class singleton {
 public:
  /**
   * @brief 自身类型声明
   */
  using self_type = T;
  using ptr_t = std::shared_ptr<self_type>;

  UTIL_SYMBOL_VISIBLE singleton() {}
  UTIL_SYMBOL_VISIBLE ~singleton() {}

  UTIL_DESIGN_PATTERN_SINGLETON_DEF_FUNCS(UTIL_SYMBOL_VISIBLE, self_type, singleton)
};
template <class T>
UTIL_SYMBOL_VISIBLE bool singleton<T>::singleton_wrapper_t::__is_destroyed = false;

template <class T>
class local_singleton {
 public:
  /**
   * @brief 自身类型声明
   */
  using self_type = T;
  using ptr_t = std::shared_ptr<self_type>;

  UTIL_SYMBOL_LOCAL local_singleton() {}
  UTIL_SYMBOL_LOCAL ~local_singleton() {}

  UTIL_DESIGN_PATTERN_SINGLETON_DEF_FUNCS(UTIL_SYMBOL_LOCAL, self_type, local_singleton)
};
template <class T>
UTIL_SYMBOL_LOCAL bool local_singleton<T>::singleton_wrapper_t::__is_destroyed = false;

}  // namespace design_pattern
LIBATFRAME_UTILS_NAMESPACE_END
#endif
