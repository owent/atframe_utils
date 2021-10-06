#ifndef UTIL_CLI_CMDOPTIONBINDT_MF_CC_H
#define UTIL_CLI_CMDOPTIONBINDT_MF_CC_H

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compiler_features.h>

/*
 * cmd_option_bindt_mf_cc.h
 *
 *  Created on: 2012-01-18
 *      Author: OWenT
 *
 * 成员函数绑定器
 */

namespace util {
namespace cli {
// 绑定器集合
namespace binder {
// ============================
// ===        类绑定        ===
// ============================

template <typename _T, typename _F>
class LIBATFRAME_UTILS_API_HEAD_ONLY cmd_option_bindt_mf_cc_caller {
 private:
  _F mem_func_;

 public:
  cmd_option_bindt_mf_cc_caller(_F f) : mem_func_(f) {}

  template <typename _TCBP, typename... _Args>
  void operator()(_TCBP &param, _T *arg0, _Args &...args) {
    (arg0->*mem_func_)(param, args...);
  }

  template <typename _TCBP, typename... _Args>
  void operator()(_TCBP &param, _T &arg0, _Args &...args) {
    (arg0.*mem_func_)(param, args...);
  }
};
}  // namespace binder
}  // namespace cli
}  // namespace util
#endif /* _CMDOPTIONBINDT_MF_CC_H_ */
