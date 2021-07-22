/**
 * @file nomovable.h
 * @brief 禁止移动基类和宏,继承禁止移动基类的子类不允许移动构造
 *
 *
 * @version 1.0
 * @author owent
 * @date 2019.12.05
 *
 * @history
 *      2019-12-05: created
 */

#ifndef UTILS_DESIGNPATTERN_NONMOVABLE_H
#define UTILS_DESIGNPATTERN_NONMOVABLE_H

#pragma once

#include <config/compile_optimize.h>
#include <config/compiler_features.h>

namespace util {
namespace design_pattern {

class UTIL_SYMBOL_VISIBLE nomovable {
 protected:
  nomovable() {}
  ~nomovable() {}

 private:
  nomovable(nomovable &&) = delete;
  nomovable &operator=(nomovable &&) = delete;
  // we has defined copy constructor, so move constructor will not generated
};
}  // namespace design_pattern
}  // namespace util

/**
 * @brief 侵入式的禁止move实现，有一些场景下需要使用dllexport或者-fvisibility=hidden
 */
#define UTIL_DESIGN_PATTERN_NOMOVABLE(CLAZZ) \
 private:                                    \
  CLAZZ(CLAZZ &&) = delete;                  \
  CLAZZ &operator=(CLAZZ &&) = delete;

#endif
