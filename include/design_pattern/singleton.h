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
 *   2020-11-27 增加基于C++11标准 N2660 的实现
 *              优化销毁判定内存区的放置
 *              优化初始化顺序
 *              增加符号不导出的单例基类 local_singleton<T>
 *              单例接口返回const的智能指针，不再允许替换单例对象
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
#  define UTIL_DESIGN_PATTERN_SINGLETON_IMPORT(T)                                                      \
    UTIL_SYMBOL_IMPORT bool ::util::design_pattern::wrapper::singleton_wrapper<T>::destroyed_ = false; \
    template class UTIL_SYMBOL_IMPORT ::util::design_pattern::singleton<T>;

#  define UTIL_DESIGN_PATTERN_SINGLETON_EXPORT(T)                                                      \
    UTIL_SYMBOL_EXPORT bool ::util::design_pattern::wrapper::singleton_wrapper<T>::destroyed_ = false; \
    template class UTIL_SYMBOL_EXPORT ::util::design_pattern::singleton<T>;

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
      static LABEL bool __is_destroyed;                                                                                \
      struct deleter {                                                                                                 \
        void operator()(singleton_wrapper_permission_t<CLAZZ> *p) const {                                              \
          __is_destroyed = true;                                                                                       \
          UTIL_LOCK_ATOMIC_THREAD_FENCE(::util::lock::memory_order_release);                                           \
          delete p;                                                                                                    \
        }                                                                                                              \
      };                                                                                                               \
      static LABEL const ptr_t &me() {                                                                                 \
        static ptr_t data =                                                                                            \
            std::static_pointer_cast<CLAZZ>(ptr_permission_t(new singleton_wrapper_permission_t<CLAZZ>(), deleter())); \
        return data;                                                                                                   \
      }                                                                                                                \
    };                                                                                                                 \
                                                                                                                       \
   private:                                                                                                            \
    friend class singleton_wrapper_t;

#else

#  define UTIL_DESIGN_PATTERN_SINGLETON_DATA_IMPL(LABEL, CLAZZ, BASE_CLAZZ)                \
   private:                                                                                \
    template <class TCLASS>                                                                \
    class singleton_wrapper_permission_t : public TCLASS {};                               \
    class LABEL singleton_data_t {                                                         \
     public:                                                                               \
      std::shared_ptr<CLAZZ> instance;                                                     \
      util::lock::spin_lock lock;                                                          \
    };                                                                                     \
    class LABEL singleton_wrapper_t {                                                      \
     public:                                                                               \
      using ptr_permission_t = std::shared_ptr<singleton_wrapper_permission_t<CLAZZ> >;    \
      using ptr_t = std::shared_ptr<CLAZZ>;                                                \
      static LABEL bool __is_destroyed;                                                    \
      struct deleter {                                                                     \
        void operator()(singleton_wrapper_permission_t<CLAZZ> *p) const {                  \
          __is_destroyed = true;                                                           \
          UTIL_LOCK_ATOMIC_THREAD_FENCE(::util::lock::memory_order_release);               \
          delete p;                                                                        \
        }                                                                                  \
      };                                                                                   \
      static LABEL const ptr_t &me() {                                                     \
        if (!__data().instance) {                                                          \
          util::lock::lock_holder<util::lock::spin_lock> lock_opr(__data().lock);          \
          UTIL_LOCK_ATOMIC_THREAD_FENCE(::util::lock::memory_order_acquire);               \
          do {                                                                             \
            if (__data().instance) {                                                       \
              break;                                                                       \
            }                                                                              \
            ptr_t new_data = std::static_pointer_cast<CLAZZ>(                              \
                ptr_permission_t(new singleton_wrapper_permission_t<CLAZZ>(), deleter())); \
            __data().instance = new_data;                                                  \
          } while (false);                                                                 \
          UTIL_LOCK_ATOMIC_THREAD_FENCE(::util::lock::memory_order_release);               \
          __use(*__data().instance);                                                       \
        }                                                                                  \
        return __data().instance;                                                          \
      }                                                                                    \
    };                                                                                     \
                                                                                           \
   private:                                                                                \
    friend class singleton_wrapper_t;                                                      \
    static LABEL void __use(CLAZZ const &) {}                                              \
    static LABEL singleton_data_t &__data() {                                              \
      static singleton_data_t data;                                                        \
      return data;                                                                         \
    }

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

#define UTIL_DESIGN_PATTERN_SINGLETON_IMPORT_DECL(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_DEF_FUNCS(UTIL_SYMBOL_IMPORT, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_EXPORT_DECL(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_DEF_FUNCS(UTIL_SYMBOL_EXPORT, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DECL(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_DEF_FUNCS(UTIL_SYMBOL_VISIBLE, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_LOCAL_DECL(CLAZZ) \
  UTIL_DESIGN_PATTERN_SINGLETON_DEF_FUNCS(UTIL_SYMBOL_LOCAL, CLAZZ, CLAZZ)

#define UTIL_DESIGN_PATTERN_SINGLETON_IMPORT_DATA_DEFINITION(CLAZZ) \
  UTIL_SYMBOL_IMPORT bool CLAZZ::singleton_wrapper_t::__is_destroyed = false

#define UTIL_DESIGN_PATTERN_SINGLETON_EXPORT_DATA_DEFINITION(CLAZZ) \
  UTIL_SYMBOL_EXPORT bool CLAZZ::singleton_wrapper_t::__is_destroyed = false

#define UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DATA_DEFINITION(CLAZZ) \
  UTIL_SYMBOL_VISIBLE bool CLAZZ::singleton_wrapper_t::__is_destroyed = false

#define UTIL_DESIGN_PATTERN_SINGLETON_LOCAL_DATA_DEFINITION(CLAZZ) \
  UTIL_SYMBOL_LOCAL bool CLAZZ::singleton_wrapper_t::__is_destroyed = false

namespace util {
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
UTIL_DESIGN_PATTERN_SINGLETON_VISIBLE_DATA_DEFINITION(singleton<T>);

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
UTIL_DESIGN_PATTERN_SINGLETON_LOCAL_DATA_DEFINITION(local_singleton<T>);
}  // namespace design_pattern
}  // namespace util
#endif
