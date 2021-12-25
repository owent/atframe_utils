/**
 * @file tquerystring.h
 * @brief 支持嵌套结构的Web Querystring类，生成和解析方式类似PHP <br />
 *        依赖智能指针库 <br />
 *        注: 解析参数字符串的时候，除最后*[]=...外的所有符合结构将全部解析为Object类型 <br />
 *            数据和key中不允许出现[和]
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2012.08.01
 *
 * @history
 *      2014.05.20 增加类似php的rawurlencode和urlencode函数
 *      2020.08.14 增加优先使用unordered_map
 */

#ifndef UTIL_URI_TQUERYSTRING_H
#define UTIL_URI_TQUERYSTRING_H

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <cstddef>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

#if defined(LIBATFRAME_UTILS_ENABLE_UNORDERED_MAP_SET) && LIBATFRAME_UTILS_ENABLE_UNORDERED_MAP_SET
#  include <unordered_map>
#else
#  include <map>
#endif

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace uri {
/**
 * @brief 编码URI，类似Javascript的encodeURI函数
 * @param [in] content 待编码内容指针
 * @param [in] sz      编码内容大小（默认当作字符串）
 * @return 编码后的字符串
 */
LIBATFRAME_UTILS_API std::string encode_uri(const char *content, std::size_t sz = 0);

/**
 * @brief 解码URI，类似Javascript的decodeURI函数
 * @param [in] uri     待解码内容指针
 * @param [in] sz      解码内容大小（默认当作字符串）
 * @return 解码后的字符串
 */
LIBATFRAME_UTILS_API std::string decode_uri(const char *uri, std::size_t sz = 0);

/**
 * @brief 编码并转义URI，类似Javascript的decodeURIComponent函数
 * @param [in] content 待编码内容指针
 * @param [in] sz      编码内容大小（默认当作字符串）
 * @return 编码后的字符串
 */
LIBATFRAME_UTILS_API std::string encode_uri_component(const char *content, std::size_t sz = 0);

/**
 * @brief 解码转义的URI，类似Javascript的encodeURIComponent函数
 * @param [in] uri     待解码内容指针
 * @param [in] sz      解码内容大小（默认当作字符串）
 * @return 编码后的字符串
 */
LIBATFRAME_UTILS_API std::string decode_uri_component(const char *uri, std::size_t sz = 0);

/**
 * @brief 编码并转义URL，类似php的rawurlencode函数
 * 根据RFC 3986 编码字符串
 * @param [in] content 待编码内容指针
 * @param [in] sz      编码内容大小（默认当作字符串）
 * @return 编码后的字符串
 */
LIBATFRAME_UTILS_API std::string raw_encode_url(const char *content, std::size_t sz = 0);

/**
 * @brief 解码转义的URL，类似php的rawurldecode函数
 * @param [in] uri     待解码内容指针
 * @param [in] sz      解码内容大小（默认当作字符串）
 * @return 编码后的字符串
 */
LIBATFRAME_UTILS_API std::string raw_decode_url(const char *uri, std::size_t sz = 0);

/**
 * @brief 编码并转义URL，类似php的urlencode函数
 * 根据 application/x-www-form-urlencoded 编码字符串
 * @param [in] content 待编码内容指针
 * @param [in] sz      编码内容大小（默认当作字符串）
 * @return 编码后的字符串
 */
LIBATFRAME_UTILS_API std::string encode_url(const char *content, std::size_t sz = 0);

/**
 * @brief 解码转义的URL，类似php的urldecode函数
 * @param [in] uri     待解码内容指针
 * @param [in] sz      解码内容大小（默认当作字符串）
 * @return 编码后的字符串
 */
LIBATFRAME_UTILS_API std::string decode_url(const char *uri, std::size_t sz = 0);

/**
 * @brief 字符串转换为任意类型
 * @param [in] str     字符串表示的数据内容
 * @return 任意类型
 */
template <typename T>
LIBATFRAME_UTILS_API_HEAD_ONLY T query_string_to_any(const char *str) {
  T ret;
  std::stringstream ss;
  ss << str;
  ss >> ret;
  return ret;
}

/**
 * @brief 任意类型转换为字符串
 * @param [in] val     任意类型数据
 * @return 对应的字符串
 */
template <typename T>
LIBATFRAME_UTILS_API_HEAD_ONLY std::string any_to_query_string(const T &val) {
  std::stringstream ss;
  ss << val;
  return ss.str();
}
}  // namespace uri

namespace types {
/**
 * @brief 数据类型枚举，小于ITEM_TYPE_QUERYSTRING的为元类型
 */
enum ITEM_TYPE { ITEM_TYPE_STRING = 0, ITEM_TYPE_QUERYSTRING = 1, ITEM_TYPE_ARRAY = 2, ITEM_TYPE_OBJECT = 3 };

/**
 * @brief 数据类型抽象接口
 */
class LIBATFRAME_UTILS_API item_impl {
 protected:
  item_impl() {}

  /**
   * @brief 添加到字符串
   * @param [in] target   填充目标
   * @param [in] key      key值
   * @param [in] value    value值
   */
  void append_to(std::string &target, const std::string &key, const std::string &value) const;

 public:
  /**
   * @brief 对应数据类型的智能指针类型
   */
  using ptr_type = std::shared_ptr<item_impl>;

  virtual ~item_impl();

  /**
   * @brief 获取是否为空内容
   * @return 是否为空
   */
  virtual bool empty() const = 0;

  /**
   * @brief 获取数据数量
   * @return 数据长度
   */
  virtual std::size_t size() const = 0;

  /**
   * @brief 获取数据类型
   * @return 表示类型的枚举类型
   */
  virtual ITEM_TYPE type() const = 0;

  /**
   * @brief 把数据转为字符串
   * @param [in] prefix 数据项前缀
   * @return 字符串表示
   */
  virtual std::string to_string(const char *prefix = "") const = 0;

  /**
   * @brief 编码并追加目标字符串尾部
   * @param [out] output 输出内容
   * @param [in] prefix 编码内容前缀
   * @return 如果成功，返回true，否则返回false
   */
  virtual bool encode(std::string &output, const char *prefix = "") const = 0;

  /**
   * @brief 数据解码
   * @param [in] keys   key列表
   * @param [in] index    当前解码位置下标
   * @param [in] value 目标值
   * @return 如果成功，返回true，否则返回false
   */
  virtual bool parse(const std::vector<std::string> &keys, std::size_t index, const std::string &value) = 0;
};

/**
 * @brief 普通字符串类型
 */
class item_string : public item_impl {
 protected:
  std::string data_;

 public:
  /**
   * @brief 对应数据类型的智能指针类型
   */
  using ptr_type = std::shared_ptr<item_string>;

  LIBATFRAME_UTILS_API item_string();

  LIBATFRAME_UTILS_API item_string(const std::string &data);

  LIBATFRAME_UTILS_API virtual ~item_string();

  /**
   * @brief 创建自身类型的实例
   * @return 新实例的智能指针
   */
  static LIBATFRAME_UTILS_API ptr_type create();

  /**
   * @brief 创建自身类型的实例
   * @param [in] data 初始数据
   * @return 新实例的智能指针
   */
  static LIBATFRAME_UTILS_API ptr_type create(const std::string &data);

  LIBATFRAME_UTILS_API virtual bool empty() const;

  LIBATFRAME_UTILS_API virtual std::size_t size() const;

  LIBATFRAME_UTILS_API virtual types::ITEM_TYPE type() const;

  LIBATFRAME_UTILS_API virtual std::string to_string(const char *prefix = "") const;

  LIBATFRAME_UTILS_API virtual bool encode(std::string &output, const char *prefix = "") const;

  LIBATFRAME_UTILS_API virtual bool parse(const std::vector<std::string> &keys, std::size_t index,
                                          const std::string &value);

  LIBATFRAME_UTILS_API const std::string &data() const;

  /**
   * @breif 类型转换操作
   */
  LIBATFRAME_UTILS_API operator std::string();

  /**
   * @breif 兼容赋值操作
   * @param [in] data 原始数据
   * @return 依据等号操作符规则返回自身引用
   */
  LIBATFRAME_UTILS_API item_string &operator=(const std::string &data);

  /**
   * @breif 设置数据
   * @param [in] data 原始数据
   */
  LIBATFRAME_UTILS_API void set(const std::string &data);

  /**
   * @breif 获取数据
   * @return 数据内容
   */
  LIBATFRAME_UTILS_API std::string &get();
};

/**
 * @brief 数组类型
 */
class item_array : public item_impl {
 protected:
  std::vector<std::shared_ptr<item_impl> > data_;

 public:
  /**
   * @brief 对应数据类型的智能指针类型
   */
  using ptr_type = std::shared_ptr<item_array>;

  LIBATFRAME_UTILS_API item_array();

  LIBATFRAME_UTILS_API virtual ~item_array();

  /**
   * @brief 创建自身类型的实例
   * @param [in] data 初始数据
   * @return 新实例的智能指针
   */
  static LIBATFRAME_UTILS_API ptr_type create();

  LIBATFRAME_UTILS_API virtual bool empty() const;

  LIBATFRAME_UTILS_API virtual std::size_t size() const;

  LIBATFRAME_UTILS_API virtual types::ITEM_TYPE type() const;

  LIBATFRAME_UTILS_API virtual std::string to_string(const char *prefix = "") const;

  LIBATFRAME_UTILS_API virtual bool encode(std::string &output, const char *prefix = "") const;

  LIBATFRAME_UTILS_API virtual bool parse(const std::vector<std::string> &keys, std::size_t index,
                                          const std::string &value);

  /**
   * @breif 依据下标获取数据
   * @param [in] uIndex 下标
   * @return 数据内容的智能指针
   */
  LIBATFRAME_UTILS_API std::shared_ptr<item_impl> get(std::size_t uIndex);

  /**
   * @breif 依据下标获取数据的字符串值
   * @param [in] uIndex 下标
   * @return 数据内容的字符串表示
   */
  LIBATFRAME_UTILS_API std::string get_string(std::size_t uIndex) const;

  /**
   * @breif 设置值
   * @param [in] uIndex 下标
   * @param [in] value 值的智能指针
   */
  LIBATFRAME_UTILS_API void set(std::size_t uIndex, const std::shared_ptr<item_impl> &value);

  /**
   * @breif 设置字符串值
   * @param [in] uIndex 下标
   * @param [in] value 字符串值
   */
  LIBATFRAME_UTILS_API void set(std::size_t uIndex, const std::string &value);

  /**
   * @breif 添加值
   * @param [in] uIndex 下标
   * @param [in] value 值的智能指针
   */
  LIBATFRAME_UTILS_API void append(const std::shared_ptr<item_impl> &value);

  /**
   * @breif 添加字符串值
   * @param [in] uIndex 下标
   * @param [in] value 字符串值
   */
  LIBATFRAME_UTILS_API void append(const std::string &value);

  /**
   * @breif 清除最后一项数据
   */
  LIBATFRAME_UTILS_API void pop_back();

  /**
   * @breif 清空数据
   */
  LIBATFRAME_UTILS_API void clear();
};

/**
 * @brief 映射类型
 */
class item_object : public item_impl {
 public:
  using data_map_t = LIBATFRAME_UTILS_AUTO_SELETC_MAP(std::string, std::shared_ptr<item_impl>);
  using data_iterator = data_map_t::iterator;
  using data_const_iterator = data_map_t::const_iterator;
  using ptr_type = std::shared_ptr<item_object>;

  data_map_t data_;

  /**
   * @brief 创建自身类型的实例
   * @param [in] data 初始数据
   * @return 新实例的智能指针
   */
  static LIBATFRAME_UTILS_API ptr_type create();

  LIBATFRAME_UTILS_API item_object();

  LIBATFRAME_UTILS_API virtual ~item_object();

  LIBATFRAME_UTILS_API virtual bool empty() const;

  LIBATFRAME_UTILS_API virtual std::size_t size() const;

  LIBATFRAME_UTILS_API virtual types::ITEM_TYPE type() const;

  LIBATFRAME_UTILS_API virtual std::string to_string(const char *prefix = "") const;

  LIBATFRAME_UTILS_API virtual bool encode(std::string &output, const char *prefix = "") const;

  LIBATFRAME_UTILS_API virtual bool parse(const std::vector<std::string> &keys, std::size_t index,
                                          const std::string &value);

  LIBATFRAME_UTILS_API std::vector<std::string> keys() const;

  LIBATFRAME_UTILS_API const LIBATFRAME_UTILS_AUTO_SELETC_MAP(std::string, std::shared_ptr<item_impl>) & data() const;
  /**
   * @breif 依据Key获取数据
   * @param [in] key Key
   * @return 数据内容的智能指针
   */
  LIBATFRAME_UTILS_API std::shared_ptr<item_impl> get(const std::string &key);

  /**
   * @breif 依据Key获取数据的字符串值
   * @param [in] key Key
   * @return 数据内容的字符串表示
   */
  LIBATFRAME_UTILS_API std::string get_string(const std::string &key) const;

  /**
   * @breif 添加或设置值
   * @param [in] key Key
   * @param [in] value 值的智能指针
   */
  LIBATFRAME_UTILS_API void set(const std::string &key, const std::shared_ptr<item_impl> &value);

  /**
   * @breif 添加或设置字符串值
   * @param [in] key Key
   * @param [in] value 字符串值
   */
  LIBATFRAME_UTILS_API void set(const std::string &key, const std::string &value);

  /**
   * @breif 删除数据
   * @param [in] key Key
   */
  LIBATFRAME_UTILS_API void remove(const std::string &key);

  /**
   * @breif 清空数据
   */
  LIBATFRAME_UTILS_API void clear();
};
}  // namespace types

class tquerystring : public types::item_object {
 public:
  using data_map_t = types::item_object::data_map_t;
  using data_iterator = types::item_object::data_iterator;
  using data_const_iterator = types::item_object::data_const_iterator;

 protected:
  std::string spliter_;

  LIBATFRAME_UTILS_API bool decode_record(const char *content, std::size_t sz);

 public:
  /**
   * @brief 对应数据类型的智能指针类型
   */
  using ptr_type = std::shared_ptr<tquerystring>;

  LIBATFRAME_UTILS_API tquerystring();

  LIBATFRAME_UTILS_API tquerystring(const std::string &spliter);

  LIBATFRAME_UTILS_API virtual ~tquerystring();

  /**
   * @brief 创建自身类型的实例
   * @return 新实例的智能指针
   */
  static LIBATFRAME_UTILS_API ptr_type create();

  /**
   * @brief 创建自身类型的实例
   * @param [in] spliter 默认分隔符
   * @return 新实例的智能指针
   */
  static LIBATFRAME_UTILS_API ptr_type create(const std::string &spliter);

  LIBATFRAME_UTILS_API virtual bool empty() const;

  LIBATFRAME_UTILS_API virtual std::size_t size() const;

  LIBATFRAME_UTILS_API virtual types::ITEM_TYPE type() const;

  LIBATFRAME_UTILS_API virtual std::string to_string(const char *prefix = "") const;

  LIBATFRAME_UTILS_API virtual bool encode(std::string &output, const char *prefix = "") const;

  /**
   * @breif 解码数据
   * @param [in] content 数据指针
   * @param [in] sz      数据长度
   * @return 成功返回true
   */
  LIBATFRAME_UTILS_API bool decode(const char *content, std::size_t sz = 0);

  /**
   * @breif 根据ID获取数据
   * @param [in] key Key
   * @return 存在返回对应的智能指针，否则返回空智能指针
   */
  LIBATFRAME_UTILS_API std::shared_ptr<types::item_impl> operator[](const std::string &key);

  /**
   * @breif 设置数据分隔符
   * @param [in] spliter 分割符，每个字符都是单独的分隔符
   */
  LIBATFRAME_UTILS_API void set_spliter(const std::string &spliter);

  /**
   * @breif 创建字符串实例
   * @return 新实例指针
   */
  LIBATFRAME_UTILS_API types::item_string::ptr_type create_string();

  /**
   * @breif 创建字符串实例
   * @param [in] val 初始值
   * @return 新实例指针
   */
  LIBATFRAME_UTILS_API types::item_string::ptr_type create_string(const std::string &val);

  /**
   * @breif 创建数组实例
   * @return 新实例指针
   */
  LIBATFRAME_UTILS_API types::item_array::ptr_type create_array();

  /**
   * @breif 创建Object实例
   * @return 新实例指针
   */
  LIBATFRAME_UTILS_API types::item_object::ptr_type create_object();
};
LIBATFRAME_UTILS_NAMESPACE_END

#endif
