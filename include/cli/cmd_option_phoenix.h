#ifndef UTIL_CLI_CMDOPTION_PHOENIX_H
#define UTIL_CLI_CMDOPTION_PHOENIX_H

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
            struct LIBATFRAME_UTILS_API_HEAD_ONLY assign_t {
                T &var;
                assign_t(T &t) : var(t) {}

                void operator()(util::cli::callback_param params) {
                    if (params.get_params_number() > 0) {
                        var = params[0]->to<T>();
                    }
                }
            };

            template <typename T>
            LIBATFRAME_UTILS_API_HEAD_ONLY assign_t<T> assign(T &t) {
                return assign_t<T>(std::ref(t));
            }

            /**
             * @brief 通用赋值动作 - 容器push_back操作
             */
            template <typename T>
            struct LIBATFRAME_UTILS_API_HEAD_ONLY push_back_t {
                T &var;
                push_back_t(T &t) : var(t) {}

                void operator()(util::cli::callback_param params) {
                    for (util::cli::cmd_option_list::size_type i = 0; i < params.get_params_number(); ++i) {
                        var.push_back(params[i]->to<typename T::value_type>());
                    }
                }
            };

            template <typename T>
            LIBATFRAME_UTILS_API_HEAD_ONLY push_back_t<T> push_back(T &t) {
                return push_back_t<T>(std::ref(t));
            }

            /**
             * @brief 通用赋值动作 - 容器push_front操作
             */
            template <typename T>
            struct LIBATFRAME_UTILS_API_HEAD_ONLY push_front_t {
                T &var;
                push_front_t(T &t) : var(t) {}

                void operator()(util::cli::callback_param params) {
                    for (util::cli::cmd_option_list::size_type i = 0; i < params.get_params_number(); ++i) {
                        var.push_front(params[i]->to<typename T::value_type>());
                    }
                }
            };

            template <typename T>
            LIBATFRAME_UTILS_API_HEAD_ONLY push_front_t<T> push_front(T &t) {
                return push_front_t<T>(std::ref(t));
            }

            /**
             * @brief 通用赋值动作 - 容器insert操作
             */
            template <typename T>
            struct LIBATFRAME_UTILS_API_HEAD_ONLY insert_t {
                T &var;
                insert_t(T &t) : var(t) {}

                void operator()(util::cli::callback_param params) {
                    for (util::cli::cmd_option_list::size_type i = 0; i < params.get_params_number(); ++i) {
                        var.insert(params[i]->to<typename T::value_type>());
                    }
                }
            };

            template <typename T>
            LIBATFRAME_UTILS_API_HEAD_ONLY insert_t<T> insert(T &t) {
                return insert_t<T>(std::ref(t));
            }

            /**
             * @brief 通用赋值动作 - 设置变量值为某个固定值
             */
            template <typename T>
            struct LIBATFRAME_UTILS_API_HEAD_ONLY set_const_t {
                T &var;
                T  val;
                set_const_t(T &t, const T &v) : var(t), val(v) {}

                void operator()(util::cli::callback_param) { var = val; }
            };

            template <typename T>
            LIBATFRAME_UTILS_API_HEAD_ONLY set_const_t<T> set_const(T &t, const T &v) {
                return set_const_t<T>(std::ref(t), std::cref(v));
            }

            /**
             * @brief 通用赋值动作 - 设置一个变量为bool值并检查语义
             * @note no, false, disabled, disable, 0 都会被判定为false，其他为true
             */
            template <typename T>
            struct LIBATFRAME_UTILS_API_HEAD_ONLY assign_logic_bool_t {
                T &var;
                assign_logic_bool_t(T &t) : var(t) {}

                void operator()(util::cli::callback_param params) {
                    if (params.get_params_number() > 0) {
                        var = params[0]->to_logic_bool();
                    } else {
                        var = false;
                    }
                }
            };

            template <typename T>
            LIBATFRAME_UTILS_API_HEAD_ONLY assign_logic_bool_t<T> assign_logic_bool(T &t) {
                return assign_logic_bool_t<T>(std::ref(t));
            }
        } // namespace phoenix
    }     // namespace cli
} // namespace util
#endif /* CMDOPTION_H_ */
