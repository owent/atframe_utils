/**
 * @file noncopyable.h
 * @brief 禁止复制基类,继承该类的子类不允许复制
 *
 *
 * @version 1.0
 * @author owent
 * @date 2012.02.21
 *
 * @history
 *      2017-01-21: using c++11 delete keyword in modern compiler
 *
 */

#ifndef UTILS_DESIGNPATTERN_NONCOPYABLE_H
#define UTILS_DESIGNPATTERN_NONCOPYABLE_H

#pragma once

#include <config/compiler_features.h>

namespace util {
    namespace design_pattern {

        class noncopyable {
        protected:
            noncopyable() {}
            ~noncopyable() {}

        private:
            noncopyable(const noncopyable &) UTIL_CONFIG_DELETED_FUNCTION;
            const noncopyable &operator=(const noncopyable &) UTIL_CONFIG_DELETED_FUNCTION;
            // we has defined copy constructor, so move constructor will not generated
        };
    }
}

#endif
