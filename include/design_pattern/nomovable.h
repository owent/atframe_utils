// Copyright 2021 atframework
// Created by owent on 2019-12-05

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>
#include <config/compiler_features.h>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
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
LIBATFRAME_UTILS_NAMESPACE_END

/**
 * @brief 侵入式的禁止move实现，有一些场景下需要使用dllexport或者-fvisibility=hidden
 */
#define UTIL_DESIGN_PATTERN_NOMOVABLE(CLAZZ) \
 private:                                    \
  CLAZZ(CLAZZ &&) = delete;                  \
  CLAZZ &operator=(CLAZZ &&) = delete;
