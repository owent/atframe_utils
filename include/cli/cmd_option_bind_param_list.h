#ifndef UTIL_CLI_CMDOPTIONBINDPARAMLIST_H
#define UTIL_CLI_CMDOPTIONBINDPARAMLIST_H

#pragma once

/*
 * cmd_option_bind_param_list.h
 *
 *  Created on: 2014-01-22
 *      Author: OWenT
 *
 * 绑定器参数列表类
 */

#include <tuple>

#include "cli/cmd_option_bindt_cc.h"
#include "cli/cmd_option_bindt_mf_cc.h"
#include "cli/cmd_option_list.h"

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace cli {
namespace binder {

/**
 *  Maps member pointers into instances of _Mem_fn but leaves all
 *  primary template handles the non--member-pointer case.
 */
template <typename _Tp>
struct LIBATFRAME_UTILS_API_HEAD_ONLY maybe_wrap_member_pointer {
  using type = _Tp;
  using caller_type = cmd_option_bindt_cc_caller<_Tp>;

  static constexpr const _Tp &do_wrap(const _Tp &__x) { return __x; }

  static constexpr _Tp &do_wrap(_Tp &__x) { return __x; }
};

/**
 *  Maps member pointers into instances of _Mem_fn but leaves all
 *  partial specialization handles the member pointer case.
 */
template <typename _Tp, typename _Class>
struct LIBATFRAME_UTILS_API_HEAD_ONLY maybe_wrap_member_pointer<_Tp _Class::*> {
  using type = _Tp _Class::*;
  using caller_type = cmd_option_bindt_mf_cc_caller<_Class, type>;

  static type do_wrap(_Tp _Class::*__pm) { return type(__pm); }
};

// ============================
// ===       参数列表       ===
// ============================
template <typename... _Args>
class LIBATFRAME_UTILS_API_HEAD_ONLY cmd_option_bind_param_list {
 private:
  /**
   * 用于创建存储对象索引和解包索引[0, 1, 2, ..., sizeof...(_Args) - 1]
   */
  template <std::size_t... _Index>
  struct index_args_var_list {};

  template <std::size_t N, std::size_t... _Index>
  struct build_args_index : build_args_index<N - 1, _Index..., sizeof...(_Index)> {};

  template <std::size_t... _Index>
  struct build_args_index<0, _Index...> {
    using type = index_args_var_list<_Index...>;
  };

 private:
  template <class _F, std::size_t... _Indexes>
  void _do_call(_F &f, callback_param args, index_args_var_list<_Indexes...>) {
    f(args, std::get<_Indexes>(args_)...);
  }

 private:
  std::tuple<_Args...> args_;

 public:
  cmd_option_bind_param_list(_Args... args) : args_(args...) {}

  template <class _F>
  void operator()(_F &f, callback_param args, int) {
    using _index_type = typename build_args_index<sizeof...(_Args)>::type;
    _do_call(f, args, _index_type());
  }
};

}  // namespace binder
}  // namespace cli
LIBATFRAME_UTILS_NAMESPACE_END
#endif /* _CMDOPTIONBINDPARAMLIST_H_ */
