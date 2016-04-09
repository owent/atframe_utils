#ifndef _UTIL_CLI_CMDOPTION_PHOENIX_H_
#define _UTIL_CLI_CMDOPTION_PHOENIX_H_

#pragma once

/**
 * cmd_option_bind<TCmdStr>.h
 *
 *  Version: 2.0.0
 *  Created on: 2011-12-29
 *  Last edit : 2016-04-09
 *      Author: OWenT
 *
 * 应用程序命令处理
 * 快捷工具
 *
 */

#include "cmd_option.h"

namespace util {
    namespace cli {
        namespace phoenix {
            /**
             * @brief 通用赋值动作 - 设置一个变量值
             */
            template <typename T>
            struct assign {
                T &var;
                assign(T &t) : var(t) {}

                void operator()(util::cli::callback_param params) {
                    if (params.get_params_number() > 0) {
                        var = params[0]->to<T>();
                    }
                }
            };

            /**
             * @brief 通用赋值动作 - 容器push_back操作
             */
            template <typename T>
            struct push_back {
                T &var;
                push_back(T &t) : var(t) {}

                void operator()(util::cli::callback_param params) {
                    for (util::cli::cmd_option_list::size_type i = 0; i < params.get_params_number(); ++i) {
                        var.push_back(params[i]->as_cpp_string());
                    }
                }
            };

            /**
             * @brief 通用赋值动作 - 设置变量值为某个固定值
             */
            template <typename T>
            struct set_const {
                T &var;
                T val;
                set_const(T &t, const T &v) : var(t), val(v) {}

                void operator()(util::cli::callback_param params) { var = val; }
            };

            /**
             * @brief 通用赋值动作 - 设置一个变量为bool值并检查语义
             * @note no, false, disabled, disable, 0 都会被判定为false，其他为true
             */
            template <typename T>
            struct assign_logic_bool {
                T &var;
                assign_logic_bool(T &t) : var(t) {}

                void operator()(util::cli::callback_param params) {
                    if (params.get_params_number() > 0) {
                        var = params[0]->to_logic_bool();
                    } else {
                        var = false;
                    }
                }
            };
        }
    }
}
#endif /* CMDOPTION_H_ */
