/**
 * @file noncopyable.h
 * @brief 禁止复制基类和宏,继承禁止复制基类的子类不允许复制
 *
 *
 * @version 1.0
 * @author owent
 * @date 2012.02.21
 *
 * @history
 *      2017-01-21: using c++11 delete keyword in modern compiler
 *      2019-12-05: 增加 UTIL_DESIGN_PATTERN_NOCOPYABLE(类名) 宏，用于dllexport时自带的 util::design_pattern::noncopyable 未导出的问题
 *                  增加 UTIL_DESIGN_PATTERN_NOCOPYABLE(类名) 宏，用于dllexport时自带的 util::design_pattern::noncopyable 未导出的问题
 */

#ifndef UTILS_DESIGNPATTERN_NONCOPYABLE_H
#define UTILS_DESIGNPATTERN_NONCOPYABLE_H

#pragma once

#include <config/compiler_features.h>
#include <config/compile_optimize.h>

namespace util {
    namespace design_pattern {

        class UTIL_SYMBOL_VISIBLE noncopyable {
        protected:
            noncopyable() {}
            ~noncopyable() {}

        private:
            noncopyable(const noncopyable &) UTIL_CONFIG_DELETED_FUNCTION;
            noncopyable &operator=(const noncopyable &) UTIL_CONFIG_DELETED_FUNCTION;
            // we has defined copy constructor, so move constructor will not generated
        };
    } // namespace design_pattern
} // namespace util

/**
 * @brief 侵入式的禁止copy实现，有一些场景下需要使用dllexport或者-fvisibility=hidden
 */
#define UTIL_DESIGN_PATTERN_NOCOPYABLE(CLAZZ)          \
private:                                               \
    CLAZZ(const CLAZZ &) UTIL_CONFIG_DELETED_FUNCTION; \
    CLAZZ &operator=(const CLAZZ &) UTIL_CONFIG_DELETED_FUNCTION;


#endif
