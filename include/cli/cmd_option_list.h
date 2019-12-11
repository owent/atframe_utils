#ifndef CMDOPTIONLIST_H
#define CMDOPTIONLIST_H

#pragma once

/*
 * cmd_option_list.h
 *
 *  Created on: 2011-12-29
 *      Author: OWenT
 *
 * 应用程序命令处理
 *
 */

#include <map>
#include <string>
#include <vector>

#include "cli/cmd_option_value.h"
#include "std/smart_ptr.h"

namespace util {
    namespace cli {
        class cmd_option_list;
        typedef cmd_option_list &callback_param;

        namespace binder {
            struct UTIL_SYMBOL_VISIBLE unspecified {};

            // 绑定器接口
            class UTIL_SYMBOL_VISIBLE cmd_option_bind_base : public std::enable_shared_from_this<cmd_option_bind_base> {
            protected:
                static LIBATFRAME_UTILS_API const char *ROOT_NODE_CMD;
                typedef struct {
                    std::vector<std::string> cmd_paths;
                    std::string all_cmds;
                    std::string description;
                    std::shared_ptr<cmd_option_bind_base> binded_obj;
                } help_msg_t;
                typedef std::vector<help_msg_t> help_list_t;

                std::string help_msg_;
                LIBATFRAME_UTILS_API virtual ~cmd_option_bind_base();

                static LIBATFRAME_UTILS_API bool sort_by_all_cmds(const help_msg_t &l, const help_msg_t &r);

            public:
                // 定义参数类型
                typedef callback_param param_type;

                LIBATFRAME_UTILS_API virtual void operator()(callback_param arg) = 0;

                // 获取绑定器的帮助信息
                LIBATFRAME_UTILS_API virtual std::string get_help_msg(const char *prefix_data = "") const;

                // 设置绑定器的帮助信息
                LIBATFRAME_UTILS_API virtual std::shared_ptr<cmd_option_bind_base> set_help_msg(const char *help_msg);

                // 增加绑定器的帮助信息
                LIBATFRAME_UTILS_API virtual std::shared_ptr<cmd_option_bind_base> add_help_msg(const char *help_msg);
            };
        } // namespace binder


        class UTIL_SYMBOL_VISIBLE cmd_option_list {
        public:
            // 类型定义
            typedef std::vector<std::pair<std::string, std::shared_ptr<binder::cmd_option_bind_base> > > cmd_array_type; // 大小类型
            typedef std::shared_ptr<cmd_option_value> value_type;                                                        // 值类型
            typedef std::vector<value_type>::size_type size_type;                                                        // 大小类型

        protected:
            std::shared_ptr<std::map<std::string, std::shared_ptr<cmd_option_value> > > key_value_;
            std::vector<std::shared_ptr<cmd_option_value> > keys_;
            cmd_array_type cmd_array_;
            void *ext_param_;

            // 初始化Key-Value映射（用于第一次调用get(key)时调用）
            LIBATFRAME_UTILS_API void init_key_value_map();

        public:
            // 构造函数
            LIBATFRAME_UTILS_API cmd_option_list();
            LIBATFRAME_UTILS_API cmd_option_list(int argv, const char *argc[]);
            LIBATFRAME_UTILS_API cmd_option_list(const std::vector<std::string> &cmds);

            // 增加选项
            LIBATFRAME_UTILS_API void add(const char *param);

            // 删除全部选项
            LIBATFRAME_UTILS_API void clear();

            // 读取指令集
            LIBATFRAME_UTILS_API void load_cmd_array(const cmd_array_type &cmds);

            // 添加指令
            LIBATFRAME_UTILS_API void append_cmd(const char *cmd_content, std::shared_ptr<binder::cmd_option_bind_base> base_node);

            // 移除末尾指令
            LIBATFRAME_UTILS_API void pop_cmd();

            LIBATFRAME_UTILS_API const cmd_array_type &get_cmd_array() const;

            // 根据键值获取选项指针，如果不存在返回默认值
            LIBATFRAME_UTILS_API value_type get(std::string key, const char *default_val);

            // 根据键值获取选项指针，如果不存在返回空指针
            LIBATFRAME_UTILS_API value_type get(std::string key);

            // 根据下标获取选项指针，如果不存在会出现运行时错误
            LIBATFRAME_UTILS_API value_type get(size_type index) const;

            // 操作符重载，功能和上面一样
            LIBATFRAME_UTILS_API value_type operator[](size_type index) const;

            // 获取参数数量
            LIBATFRAME_UTILS_API size_type get_params_number() const;

            // 重置Key-Value映射表
            // # 在第一次调用get(字符串[, 默认值])后会建立映射表
            // # 如果这之后add了参数而没有调用此函数重置映射表
            // # 新的变量将不会进入映射表
            LIBATFRAME_UTILS_API void reset_key_value_map();

            // 设置透传参数列表
            LIBATFRAME_UTILS_API void set_ext_param(void *param);

            // 获取透传参数列表
            LIBATFRAME_UTILS_API void *get_ext_param() const;
        };
    } // namespace cli
} // namespace util

#endif /* _CMDOPTIONLIST_H_ */
