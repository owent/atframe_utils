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
#include <memory>
#include <string>
#include <vector>

#include "cli/cmd_option_value.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace cli {
class cmd_option_list;
using callback_param = cmd_option_list &;

namespace binder {
struct UTIL_SYMBOL_VISIBLE unspecified{};

// 绑定器接口
class UTIL_SYMBOL_VISIBLE cmd_option_bind_base : public std::enable_shared_from_this<cmd_option_bind_base> {
 protected:
  static ATFRAMEWORK_UTILS_API const char *ROOT_NODE_CMD;
  struct help_msg_t {
    std::vector<std::string> cmd_paths;
    std::string all_cmds;
    std::string description;
    std::shared_ptr<cmd_option_bind_base> binded_obj;
  };
  using help_list_t = std::vector<help_msg_t>;

  std::string help_msg_;
  ATFRAMEWORK_UTILS_API cmd_option_bind_base();
  ATFRAMEWORK_UTILS_API virtual ~cmd_option_bind_base();

  static ATFRAMEWORK_UTILS_API bool sort_by_all_cmds(const help_msg_t &l, const help_msg_t &r);

 private:
  ATFRAMEWORK_UTILS_API cmd_option_bind_base(const cmd_option_bind_base &other);
  ATFRAMEWORK_UTILS_API cmd_option_bind_base &operator=(const cmd_option_bind_base &other);

 public:
  // 定义参数类型
  using param_type = callback_param;

  ATFRAMEWORK_UTILS_API virtual void operator()(callback_param arg) = 0;

  // 获取绑定器的帮助信息
  ATFRAMEWORK_UTILS_API virtual std::string get_help_msg(const char *prefix_data = "") const;

  // 设置绑定器的帮助信息
  ATFRAMEWORK_UTILS_API virtual std::shared_ptr<cmd_option_bind_base> set_help_msg(const char *help_msg);

  // 增加绑定器的帮助信息
  ATFRAMEWORK_UTILS_API virtual std::shared_ptr<cmd_option_bind_base> add_help_msg(const char *help_msg);
};
}  // namespace binder

class UTIL_SYMBOL_VISIBLE cmd_option_list {
 public:
  // 类型定义
  using cmd_array_type =
      std::vector<std::pair<std::string, std::shared_ptr<binder::cmd_option_bind_base> > >;  // 大小类型
  using value_type = std::shared_ptr<cmd_option_value>;                                      // 值类型
  using size_type = std::vector<value_type>::size_type;                                      // 大小类型

 protected:
  std::shared_ptr<std::map<std::string, std::shared_ptr<cmd_option_value> > > key_value_;
  std::vector<std::shared_ptr<cmd_option_value> > keys_;
  cmd_array_type cmd_array_;
  void *ext_param_;

  // 初始化Key-Value映射（用于第一次调用get(key)时调用）
  ATFRAMEWORK_UTILS_API void init_key_value_map();

 public:
  // 构造函数
  ATFRAMEWORK_UTILS_API cmd_option_list();
  ATFRAMEWORK_UTILS_API cmd_option_list(int argv, const char *argc[]);
  ATFRAMEWORK_UTILS_API cmd_option_list(const std::vector<std::string> &cmds);

  // 增加选项
  ATFRAMEWORK_UTILS_API void add(const char *param);

  // 删除全部选项
  ATFRAMEWORK_UTILS_API void clear();

  // 读取指令集
  ATFRAMEWORK_UTILS_API void load_cmd_array(const cmd_array_type &cmds);

  // 添加指令
  ATFRAMEWORK_UTILS_API void append_cmd(const char *cmd_content,
                                        std::shared_ptr<binder::cmd_option_bind_base> base_node);

  // 移除末尾指令
  ATFRAMEWORK_UTILS_API void pop_cmd();

  ATFRAMEWORK_UTILS_API const cmd_array_type &get_cmd_array() const;

  // 根据键值获取选项指针，如果不存在返回默认值
  ATFRAMEWORK_UTILS_API value_type get(std::string key, const char *default_val);

  // 根据键值获取选项指针，如果不存在返回空指针
  ATFRAMEWORK_UTILS_API value_type get(std::string key);

  // 根据下标获取选项指针，如果不存在会出现运行时错误
  ATFRAMEWORK_UTILS_API value_type get(size_type index) const;

  // 操作符重载，功能和上面一样
  ATFRAMEWORK_UTILS_API value_type operator[](size_type index) const;

  // 获取参数数量
  ATFRAMEWORK_UTILS_API size_type get_params_number() const;

  // 重置Key-Value映射表
  // # 在第一次调用get(字符串[, 默认值])后会建立映射表
  // # 如果这之后add了参数而没有调用此函数重置映射表
  // # 新的变量将不会进入映射表
  ATFRAMEWORK_UTILS_API void reset_key_value_map();

  // 设置透传参数列表
  ATFRAMEWORK_UTILS_API void set_ext_param(void *param);

  // 获取透传参数列表
  ATFRAMEWORK_UTILS_API void *get_ext_param() const;
};
}  // namespace cli
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif /* _CMDOPTIONLIST_H_ */
