#ifndef UTIL_CLI_CMDOPTIONBINDT_CC_H
#define UTIL_CLI_CMDOPTIONBINDT_CC_H

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compiler_features.h>

/*
 * cmd_option_bindt_cc.h
 *
 *  Created on: 2012-01-18
 *      Author: OWenT
 *
 * 自由函数绑定器
 */

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace cli {
// 绑定器集合
namespace binder {
// ============================
// ===       函数绑定       ===
// ============================
template <typename _TF>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY cmd_option_bindt_cc_caller {
 private:
  _TF func_;

 public:
  cmd_option_bindt_cc_caller(_TF f) : func_(f) {}

  template <typename _TCBP, typename... _Args>
  void operator()(_TCBP &param, _Args &...args) {
    func_(param, args...);
  }
};
}  // namespace binder
}  // namespace cli
ATFRAMEWORK_UTILS_NAMESPACE_END
#endif /* cmd_option_bindt_cc_caller */
