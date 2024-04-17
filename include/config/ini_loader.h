/**
 * @file config/ini_loader.h
 * @brief ini解析器
 * Licensed under the MIT licenses.
 *
 * @note 与标准ini有略微不同，请注意
 * 1. 支持Section
 * 2. Secion支持父子关系,即 [ A : B : C ] 语法
 * 3. 支持多重父子关系,如 C.B.A = d
 * 4. 支持字符串转义，其中以'包裹的不进行转义，以"包裹的可进行转义,如 C.B.A = "Hello \r\n World!\0"
 * 5. 配置的Key名称不能包含引号('和")、点(.)、冒号(:)和方括号([])
 * 6. 配置的Key名称区分大小写
 * 7. #符号也将是做行注释，与 ; 等价
 *
 * @version 1.0.1.0
 * @author owentou, owt5008137@live.com
 * @date 2013年11月16日
 *
 * @history
 *   2014-07-14: 修正空值问题, 优化API
 *   2015-02-02: 修正字符串未配置会导致崩溃的BUG
 *   2016-04-14: Section部分也支持使用.来分割层级
 */

#ifndef UTIL_CONFIG_INI_INILOADER_H
#define UTIL_CONFIG_INI_INILOADER_H

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <nostd/string_view.h>

#include <stdint.h>
#include <cstddef>
#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace config {
// ================= 错误码 =================
enum EN_INILOADER_ERROR_CODE {
  EIEC_SUCCESS = 0,
  EIEC_OPENFILE = -1,
};
// 时间区间
struct LIBATFRAME_UTILS_API_HEAD_ONLY duration_value {
  time_t sec;   // 秒
  time_t nsec;  // 纳秒
};
// ----------------- 错误码 -----------------

// ================= 存储层 =================
class ini_value : public std::enable_shared_from_this<ini_value> {
 public:
  using ptr_t = std::shared_ptr<ini_value>;
  using node_type = LIBATFRAME_UTILS_AUTO_SELETC_MAP(std::string, ptr_t);

 private:
  std::vector<std::string> data_;
  node_type children_nodes_;

  template <typename _Tt>
  UTIL_FORCEINLINE static void clear_data(_Tt &) {}

  UTIL_FORCEINLINE static void clear_data(float &data) { data = 0.0f; }
  UTIL_FORCEINLINE static void clear_data(double &data) { data = 0.0; }
  UTIL_FORCEINLINE static void clear_data(char *&data) { data = nullptr; }
  UTIL_FORCEINLINE static void clear_data(std::string &data) { data.clear(); }
  UTIL_FORCEINLINE static void clear_data(bool &data) { data = false; }
  UTIL_FORCEINLINE static void clear_data(char &data) { data = 0; }
  UTIL_FORCEINLINE static void clear_data(short &data) { data = 0; }  // NOLINT: runtime/int
  UTIL_FORCEINLINE static void clear_data(int &data) { data = 0; }
  UTIL_FORCEINLINE static void clear_data(long &data) { data = 0; }       // NOLINT: runtime/int
  UTIL_FORCEINLINE static void clear_data(long long &data) { data = 0; }  // NOLINT: runtime/int
  UTIL_FORCEINLINE static void clear_data(unsigned char &data) { data = 0; }
  UTIL_FORCEINLINE static void clear_data(unsigned short &data) { data = 0; }  // NOLINT: runtime/int
  UTIL_FORCEINLINE static void clear_data(unsigned int &data) { data = 0; }
  UTIL_FORCEINLINE static void clear_data(unsigned long &data) { data = 0; }       // NOLINT: runtime/int
  UTIL_FORCEINLINE static void clear_data(unsigned long long &data) { data = 0; }  // NOLINT: runtime/int
  UTIL_FORCEINLINE static void clear_data(duration_value &data) {
    data.sec = 0;
    data.nsec = 0;
  }

 public:
  LIBATFRAME_UTILS_API ini_value();
  LIBATFRAME_UTILS_API ~ini_value();
  LIBATFRAME_UTILS_API ini_value(const ini_value &other);
  LIBATFRAME_UTILS_API ini_value &operator=(const ini_value &other);

  LIBATFRAME_UTILS_API void add(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view val);
  LIBATFRAME_UTILS_API void add(const char *begin, const char *end);

  // 节点操作
  LIBATFRAME_UTILS_API bool empty() const;     // like stl
  LIBATFRAME_UTILS_API bool has_data() const;  // like stl
  LIBATFRAME_UTILS_API size_t size() const;    // like stl
  LIBATFRAME_UTILS_API void clear();           // like stl
  LIBATFRAME_UTILS_API ini_value &operator[](LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view key);
  LIBATFRAME_UTILS_API node_type &get_children();
  LIBATFRAME_UTILS_API const node_type &get_children() const;

  LIBATFRAME_UTILS_API ptr_t get_child_by_path(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path) const;

  static LIBATFRAME_UTILS_API const std::string &get_empty_string();

 private:
  template <typename _Tt, typename _TVOID>
  struct as_helper;

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<LIBATFRAME_UTILS_NAMESPACE_ID::config::duration_value, _TVOID> {
    UTIL_FORCEINLINE static LIBATFRAME_UTILS_NAMESPACE_ID::config::duration_value convert(const ini_value &val,
                                                                                          size_t index) {
      return val.as_duration(index);
    }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<int8_t, _TVOID> {
    UTIL_FORCEINLINE static int8_t convert(const ini_value &val, size_t index) { return val.as_int8(index); }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<uint8_t, _TVOID> {
    UTIL_FORCEINLINE static uint8_t convert(const ini_value &val, size_t index) { return val.as_uint8(index); }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<int16_t, _TVOID> {
    UTIL_FORCEINLINE static int16_t convert(const ini_value &val, size_t index) { return val.as_int16(index); }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<uint16_t, _TVOID> {
    UTIL_FORCEINLINE static uint16_t convert(const ini_value &val, size_t index) { return val.as_uint16(index); }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<int32_t, _TVOID> {
    UTIL_FORCEINLINE static int32_t convert(const ini_value &val, size_t index) { return val.as_int32(index); }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<uint32_t, _TVOID> {
    UTIL_FORCEINLINE static uint32_t convert(const ini_value &val, size_t index) { return val.as_uint32(index); }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<int64_t, _TVOID> {
    UTIL_FORCEINLINE static int64_t convert(const ini_value &val, size_t index) { return val.as_int64(index); }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<uint64_t, _TVOID> {
    UTIL_FORCEINLINE static uint64_t convert(const ini_value &val, size_t index) { return val.as_uint64(index); }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<const char *, _TVOID> {
    UTIL_FORCEINLINE static const char *convert(const ini_value &val, size_t index) { return val.as_string(index); }
  };

  template <typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper<std::string, _TVOID> {
    UTIL_FORCEINLINE static std::string convert(const ini_value &val, size_t index) { return val.as_cpp_string(index); }
  };

  template <typename _Tt, typename _TVOID>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY as_helper {
    UTIL_FORCEINLINE static _Tt string2any(const std::string &data) {
      _Tt ret;
      ini_value::clear_data(ret);
      if (!data.empty()) {
        std::stringstream s_stream;
        s_stream.str(data);
        s_stream >> ret;
      }
      return ret;
    }

    UTIL_FORCEINLINE static _Tt convert(const ini_value &val, size_t index) {
      return string2any(val.as_cpp_string(index));
    }
  };

 public:
  // 数值转换操作
  template <typename _Tt>
  UTIL_FORCEINLINE _Tt as(size_t index = 0) const {
    return as_helper<_Tt, void>::convert(*this, index);
  }

  // 获取存储对象的字符串
  LIBATFRAME_UTILS_API const std::string &as_cpp_string(size_t index = 0) const;

  LIBATFRAME_UTILS_API char as_char(size_t index = 0) const;

  LIBATFRAME_UTILS_API short as_short(size_t index = 0) const;  // NOLINT: runtime/int

  LIBATFRAME_UTILS_API int as_int(size_t index = 0) const;

  LIBATFRAME_UTILS_API long as_long(size_t index = 0) const;  // NOLINT: runtime/int

  LIBATFRAME_UTILS_API long long as_longlong(size_t index = 0) const;  // NOLINT: runtime/int

  LIBATFRAME_UTILS_API double as_double(size_t index = 0) const;

  LIBATFRAME_UTILS_API float as_float(size_t index = 0) const;

  LIBATFRAME_UTILS_API const char *as_string(size_t index = 0) const;

  LIBATFRAME_UTILS_API unsigned char as_uchar(size_t index = 0) const;

  LIBATFRAME_UTILS_API unsigned short as_ushort(size_t index = 0) const;  // NOLINT: runtime/int

  LIBATFRAME_UTILS_API unsigned int as_uint(size_t index = 0) const;

  LIBATFRAME_UTILS_API unsigned long as_ulong(size_t index = 0) const;  // NOLINT: runtime/int

  LIBATFRAME_UTILS_API unsigned long long as_ulonglong(size_t index = 0) const;  // NOLINT: runtime/int

  LIBATFRAME_UTILS_API int8_t as_int8(size_t index = 0) const;

  LIBATFRAME_UTILS_API uint8_t as_uint8(size_t index = 0) const;

  LIBATFRAME_UTILS_API int16_t as_int16(size_t index = 0) const;

  LIBATFRAME_UTILS_API uint16_t as_uint16(size_t index = 0) const;

  LIBATFRAME_UTILS_API int32_t as_int32(size_t index = 0) const;

  LIBATFRAME_UTILS_API uint32_t as_uint32(size_t index = 0) const;

  LIBATFRAME_UTILS_API int64_t as_int64(size_t index = 0) const;

  LIBATFRAME_UTILS_API uint64_t as_uint64(size_t index = 0) const;

  LIBATFRAME_UTILS_API duration_value as_duration(size_t index = 0) const;
};
// ----------------- 存储层 -----------------

class ini_loader {
 private:
  ini_value::ptr_t root_node_;  // root node
  ini_value *current_node_ptr_;

 public:
  LIBATFRAME_UTILS_API ini_loader();
  LIBATFRAME_UTILS_API ini_loader(const ini_loader &other);
  LIBATFRAME_UTILS_API ini_loader &operator=(const ini_loader &other);
  LIBATFRAME_UTILS_API ~ini_loader();

  /**
   * @brief 从流读取数据
   * @param in 输入流
   * @param is_append 是否是追加模式
   * @return 返回码
   * @see EN_INILOADER_ERROR_CODE
   */
  LIBATFRAME_UTILS_API int load_stream(std::istream &in, bool is_append = false);

  /**
   * @brief 从文件取数据
   * @param file_path 输入文件
   * @param is_append 是否是追加模式
   * @return 返回码
   * @see EN_INILOADER_ERROR_CODE
   */
  LIBATFRAME_UTILS_API int load_file(const char *file_path, bool is_append = false);

  /**
   * @brief 从文件取数据
   * @param file_path 输入文件
   * @param is_append 是否是追加模式
   * @return 返回码
   * @see EN_INILOADER_ERROR_CODE
   */
  LIBATFRAME_UTILS_API int load_file(const std::string &file_path, bool is_append = false);

  /**
   * @brief 清空
   */
  LIBATFRAME_UTILS_API void clear();

  /**
   * @brief 设置当前配置结构根节点路径
   * @param path 路径
   */
  LIBATFRAME_UTILS_API void set_section(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path);

  /**
   * @brief 获取当前配置结构根节点
   * @return 当前配置结构根节点
   */
  LIBATFRAME_UTILS_API ini_value &get_section();

  /**
   * @brief 获取当前配置结构根节点
   * @return 当前配置结构根节点
   */
  LIBATFRAME_UTILS_API const ini_value &get_section() const;

  /**
   * @brief 获取根节点
   * @return 根节点
   */
  LIBATFRAME_UTILS_API ini_value &get_root_node();

  /**
   * @brief 获取根节点
   * @return 根节点
   */
  LIBATFRAME_UTILS_API const ini_value &get_root_node() const;

  /**
   * @brief 根据目录获取子节点
   * @param path 节点相对路径
   * @param father_ptr 父节点，设为空则相对于根节点
   * @return 子节点
   * @note 如果子节点不存在会创建空列表节点
   */
  LIBATFRAME_UTILS_API ini_value &get_node(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path,
                                           ini_value *father_ptr = nullptr);

  /**
   * @brief 根据子节点名称获取子节点
   * @param path 子节点名称（注意不是路径）
   * @param father_ptr 父节点，设为空则相对于根节点
   * @return 子节点
   * @note 如果子节点不存在会创建空列表节点
   */
  LIBATFRAME_UTILS_API ini_value &get_child_node(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path,
                                                 ini_value *father_ptr = nullptr);

  // ========================= 单值容器转储 =========================

  /**
   * @brief 配置转储
   * @param path 配置路径
   * @param val 转储目标
   * @param is_force 是否是强制转储，强制在找不到路径对应的配置项时会尝试清空数据
   * @param index 转储索引，默认是第一个值
   */
  template <typename Ty>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump_to(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path, Ty &val,
                                              bool is_force = false, size_t index = 0) {
    ini_value &cur_node = get_node(path);
    if (cur_node.has_data() || is_force) {
      val = cur_node.as<Ty>(index);
    }
  }

  /**
   * @brief 配置转储
   * @param path 配置路径
   * @param val 转储目标
   * @param is_force 是否是强制转储，强制在找不到路径对应的配置项时会尝试清空数据
   * @param index 转储索引，默认是第一个值
   */
  LIBATFRAME_UTILS_API void dump_to(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path, bool &val,
                                    bool is_force = false, size_t index = 0);

  /**
   * @brief 配置转储
   * @param path 配置路径
   * @param val 转储目标
   * @param is_force 是否是强制转储，强制在找不到路径对应的配置项时会尝试清空数据
   * @param index 转储索引，默认是第一个值
   */
  LIBATFRAME_UTILS_API void dump_to(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path, std::string &val,
                                    bool is_force = false, size_t index = 0);

  /**
   * @brief 配置转储 - 字符串
   * @param path 配置路径
   * @param begin 转储目标起始地址
   * @param end 转储目标边界地址
   * @param is_force 是否是强制转储，强制在找不到路径对应的配置项时会尝试清空数据
   * @param index 转储索引，默认是第一个值
   */
  LIBATFRAME_UTILS_API void dump_to(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path, char *begin, char *end,
                                    bool is_force = false, size_t index = 0);

  /**
   * @brief 配置转储 - 字符串
   * @param path 配置路径
   * @param val 转储目标
   * @param is_force 是否是强制转储，强制在找不到路径对应的配置项时会尝试清空数据
   * @param index 转储索引，默认是第一个值
   */
  template <size_t MAX_COUNT>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump_to(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path,
                                              char (&val)[MAX_COUNT], bool is_force = false, size_t index = 0) {
    dump_to(path, val, val + MAX_COUNT, is_force, index);
  }

  // ========================= 多值容器转储 =========================
  /**
   * @brief 配置转储 - 容器
   * @param path 配置路径
   * @param val 转储目标
   * @param is_force 是否是强制转储，强制在找不到路径对应的配置项时会尝试清空数据
   * @note 容器足够大时，会尝试转储所有配置
   */
  template <typename Ty>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump_to(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path,
                                              std::vector<Ty> &val, bool is_force = false) {
    if (is_force) {
      val.clear();
    }

    ini_value &cur_node = get_node(path);
    for (size_t i = 0; i < cur_node.size(); ++i) {
      val.push_back(Ty());
      Ty &new_node = val.back();
      dump_to(path, new_node, is_force, i);
    }
  }

  /**
   * @brief 配置转储 - 容器
   * @param path 配置路径
   * @param val 转储目标
   * @param is_force 是否是强制转储，强制在找不到路径对应的配置项时会尝试清空数据
   * @note 容器足够大时，会尝试转储所有配置
   */
  template <typename Ty>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump_to(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path,
                                              std::list<Ty> &val, bool is_force = false) {
    if (is_force) {
      val.clear();
    }

    ini_value &cur_node = get_node(path);
    for (size_t i = 0; i < cur_node.size(); ++i) {
      val.push_back(Ty());
      Ty &new_node = val.back();
      dump_to(path, new_node, is_force, i);
    }
  }

  /**
   * @brief 配置转储 - 数组
   * @param path 配置路径
   * @param val 转储目标
   * @param is_force 是否是强制转储，强制在找不到路径对应的配置项时会尝试清空数据
   * @note 容器足够大时，会尝试转储所有配置
   */
  template <typename Ty, size_t MAX_COUNT>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump_to(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path,
                                              Ty (&val)[MAX_COUNT], bool is_force = false) {
    ini_value &cur_node = get_node(path);
    for (size_t i = 0; i < cur_node.size() && i < MAX_COUNT; ++i) {
      dump_to(path, val[i], is_force, i);
    }
  }

  /**
   * @brief 配置转储 - 迭代器
   * @param path 配置路径
   * @param begin 转储目标起始迭代器
   * @param end 转储目标边界迭代器
   * @param is_force 是否是强制转储，强制在找不到路径对应的配置项时会尝试清空数据
   * @note 容器足够大时，会尝试转储所有配置
   */
  template <typename TIter>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump_to(LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view path, TIter begin,
                                              TIter end, bool is_force = false) {
    size_t index = 0;
    ini_value &cur_node = get_node(path);
    for (TIter i = begin; i != end && index < cur_node.size(); ++index, ++i) {
      dump_to(path, *i, is_force, index);
    }
  }
};
}  // namespace config
LIBATFRAME_UTILS_NAMESPACE_END

#endif
