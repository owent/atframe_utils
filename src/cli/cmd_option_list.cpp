/*
 * cmd_option_list.cpp
 *
 *  Created on: 2011-12-29
 *      Author: OWenT
 *
 * 应用程序命令处理
 *
 */

#include "cli/cmd_option_list.h"
#include "cli/cmd_option.h"

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace cli {
LIBATFRAME_UTILS_API const char *binder::cmd_option_bind_base::ROOT_NODE_CMD = "@Root";

LIBATFRAME_UTILS_API binder::cmd_option_bind_base::cmd_option_bind_base() {}

LIBATFRAME_UTILS_API binder::cmd_option_bind_base::~cmd_option_bind_base() {}

LIBATFRAME_UTILS_API bool binder::cmd_option_bind_base::sort_by_all_cmds(const help_msg_t &l, const help_msg_t &r) {
  return l.all_cmds < r.all_cmds;
}

LIBATFRAME_UTILS_API std::string binder::cmd_option_bind_base::get_help_msg(const char *prefix_data) const {
  return prefix_data + help_msg_;
}

LIBATFRAME_UTILS_API std::shared_ptr<binder::cmd_option_bind_base> binder::cmd_option_bind_base::set_help_msg(
    const char *help_msg) {
  help_msg_ = help_msg;
  return shared_from_this();
}

LIBATFRAME_UTILS_API std::shared_ptr<binder::cmd_option_bind_base> binder::cmd_option_bind_base::add_help_msg(
    const char *help_msg) {
  help_msg_ += help_msg;
  return shared_from_this();
}

LIBATFRAME_UTILS_API void cmd_option_list::init_key_value_map() {
  using key_map_type = std::map<std::string, value_type>;
  using keys_type = std::vector<value_type>;
  // 已经初始化，跳过
  if (key_value_.get() != nullptr) return;

  key_value_ = std::shared_ptr<key_map_type>(new key_map_type());

  for (keys_type::size_type i = 0; i < keys_.size(); ++i) {
    const char *str_key = keys_[i]->to_string();
    if (nullptr == str_key) {
      continue;
    }

    const char *str_val = str_key;
    for (; *str_val && ':' != *str_val && '=' != *str_val; ++str_val);
    if (*str_val) {
      ++str_val;
    } else {
      continue;
    }

    std::string val;
    cmd_option::get_segment(str_val, val);
    (*key_value_)[std::string(str_key, str_val - 1)] = std::make_shared<cmd_option_value>(val);
  }
}

LIBATFRAME_UTILS_API cmd_option_list::cmd_option_list() : ext_param_(nullptr) {}

LIBATFRAME_UTILS_API cmd_option_list::cmd_option_list(int argv, const char *argc[]) : ext_param_(nullptr) {
  for (int i = 0; i < argv; ++i) keys_.push_back(std::make_shared<cmd_option_value>(argc[i]));
}

LIBATFRAME_UTILS_API cmd_option_list::cmd_option_list(const std::vector<std::string> &cmds) : ext_param_(nullptr) {
  std::vector<std::string>::size_type uSize = cmds.size();
  for (std::vector<std::string>::size_type i = 0; i < uSize; ++i) {
    keys_.push_back(std::make_shared<cmd_option_value>(cmds[i].c_str()));
  }
}

LIBATFRAME_UTILS_API void cmd_option_list::add(const char *param) {
  keys_.push_back(std::make_shared<cmd_option_value>(param));
}

LIBATFRAME_UTILS_API void cmd_option_list::clear() {
  key_value_.reset();    // 删除key-value映射
  keys_.clear();         // 删除索引下标映射
  cmd_array_.clear();    // 删除指令栈集合
  ext_param_ = nullptr;  // 透传参数置空
}

LIBATFRAME_UTILS_API void cmd_option_list::load_cmd_array(const cmd_array_type &cmds) { cmd_array_ = cmds; }

LIBATFRAME_UTILS_API void cmd_option_list::append_cmd(const char *cmd_content,
                                                      std::shared_ptr<binder::cmd_option_bind_base> base_node) {
  cmd_array_.push_back(std::make_pair(cmd_content, base_node));
}

LIBATFRAME_UTILS_API void cmd_option_list::pop_cmd() { cmd_array_.pop_back(); }

LIBATFRAME_UTILS_API const cmd_option_list::cmd_array_type &cmd_option_list::get_cmd_array() const {
  return cmd_array_;
}

LIBATFRAME_UTILS_API cmd_option_list::value_type cmd_option_list::get(std::string key, const char *default_val) {
  value_type ret_ptr = get(key);
  if (ret_ptr.get() == nullptr) return std::make_shared<cmd_option_value>(default_val);
  return ret_ptr;
}

LIBATFRAME_UTILS_API cmd_option_list::value_type cmd_option_list::get(std::string key) {
  init_key_value_map();

  std::map<std::string, value_type>::const_iterator itr;
  itr = key_value_->find(key);
  if (itr == key_value_->end()) return value_type();
  return itr->second;
}

LIBATFRAME_UTILS_API cmd_option_list::value_type cmd_option_list::get(size_type index) const { return keys_[index]; }

// 操作符重载，功能和上面一样
LIBATFRAME_UTILS_API cmd_option_list::value_type cmd_option_list::operator[](size_type index) const {
  return keys_[index];
}

// 获取参数数量
LIBATFRAME_UTILS_API cmd_option_list::size_type cmd_option_list::get_params_number() const { return keys_.size(); }

// 重置Key-Value映射表
LIBATFRAME_UTILS_API void cmd_option_list::reset_key_value_map() { key_value_.reset(); }

LIBATFRAME_UTILS_API void cmd_option_list::set_ext_param(void *param) { ext_param_ = param; }

LIBATFRAME_UTILS_API void *cmd_option_list::get_ext_param() const { return ext_param_; }
}  // namespace cli
LIBATFRAME_UTILS_NAMESPACE_END
