// Copyright 2021 atframework
// Created by owent on 2012-02-21
// @history
//   2017-01-21: using c++11 delete keyword in modern compiler
//   2019-12-05: 增加 UTIL_DESIGN_PATTERN_NOCOPYABLE(类名) 宏，用于dllexport时自带的
// atfw::util::design_pattern::noncopyable 未导出的问题 增加 UTIL_DESIGN_PATTERN_NOCOPYABLE(类名)
// 宏，用于dllexport时自带的 atfw::util::design_pattern::noncopyable 未导出的问题

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>
#include <config/compiler_features.h>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace design_pattern {

class UTIL_SYMBOL_VISIBLE noncopyable {
 protected:
  noncopyable() {}
  ~noncopyable() {}

 private:
  noncopyable(const noncopyable &) = delete;
  noncopyable &operator=(const noncopyable &) = delete;
  // we has defined copy constructor, so move constructor will not generated
};
}  // namespace design_pattern
ATFRAMEWORK_UTILS_NAMESPACE_END

/**
 * @brief 侵入式的禁止copy实现，有一些场景下需要使用dllexport或者-fvisibility=hidden
 */
#define ATFW_UTIL_DESIGN_PATTERN_NOCOPYABLE(CLAZZ) \
 private:                                     \
  CLAZZ(const CLAZZ &) = delete;              \
  CLAZZ &operator=(const CLAZZ &) = delete;

// Legacy macros
#ifndef UTIL_DESIGN_PATTERN_NOCOPYABLE
#  define UTIL_DESIGN_PATTERN_NOCOPYABLE(CLAZZ) ATFW_UTIL_DESIGN_PATTERN_NOCOPYABLE(CLAZZ)
#endif
