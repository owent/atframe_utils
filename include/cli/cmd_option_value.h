#ifndef UTIL_CLI_CMDOPTIONVALUE_H
#define UTIL_CLI_CMDOPTIONVALUE_H

#pragma once

/*
 * cmd_option_value.h
 *
 *  Created on: 2011-12-29
 *      Author: OWenT
 *
 * 应用程序命令处理
 *
 */

#include <stdint.h>
#include <cstring>
#include <memory>
#include <sstream>
#include <string>
#include <type_traits>
#include <vector>

#include <common/string_oprs.h>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace cli {
template <typename Tt>
struct string2any;

template <>
struct string2any<std::string> {
  UTIL_FORCEINLINE std::string operator()(const std::string &s) const { return s; }
};

template <>
struct string2any<char> {
  UTIL_FORCEINLINE char operator()(const std::string &s) const { return s.empty() ? 0 : s[0]; }
};

template <>
struct string2any<unsigned char> {
  UTIL_FORCEINLINE unsigned char operator()(const std::string &s) const {
    return static_cast<unsigned char>(s.empty() ? 0 : s[0]);
  }
};

template <>
struct string2any<int16_t> {
  UTIL_FORCEINLINE int16_t operator()(const std::string &s) const {
    return LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<int16_t>(s.c_str());
  }
};

template <>
struct string2any<uint16_t> {
  UTIL_FORCEINLINE uint16_t operator()(const std::string &s) const {
    return LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<uint16_t>(s.c_str());
  }
};

template <>
struct string2any<int32_t> {
  UTIL_FORCEINLINE int32_t operator()(const std::string &s) const {
    return LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<int32_t>(s.c_str());
  }
};

template <>
struct string2any<uint32_t> {
  UTIL_FORCEINLINE uint32_t operator()(const std::string &s) const {
    return LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<uint32_t>(s.c_str());
  }
};

template <>
struct string2any<int64_t> {
  UTIL_FORCEINLINE int64_t operator()(const std::string &s) const {
    return LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<int64_t>(s.c_str());
  }
};

template <>
struct string2any<uint64_t> {
  UTIL_FORCEINLINE uint64_t operator()(const std::string &s) const {
    return LIBATFRAME_UTILS_NAMESPACE_ID::string::to_int<uint64_t>(s.c_str());
  }
};

template <>
struct string2any<bool> {
  UTIL_FORCEINLINE bool operator()(const std::string &s) const { return !s.empty() && "0" != s; }
};

template <typename Tt>
struct string2any {
  UTIL_FORCEINLINE Tt operator()(const std::string &s) const {
    Tt ret;
    std::stringstream ss;
    ss.str(s);
    ss >> ret;
    return ret;
  }
};

class cmd_option_value {
 protected:
  std::string data_;

 public:
  LIBATFRAME_UTILS_API cmd_option_value(const char *str_data);
  LIBATFRAME_UTILS_API cmd_option_value(const char *begin, const char *end);
  LIBATFRAME_UTILS_API cmd_option_value(const std::string &str_data);

  template <typename Tr>
  LIBATFRAME_UTILS_API_HEAD_ONLY Tr to() const {
    using cv_type = typename ::std::remove_cv<Tr>::type;
    return string2any<cv_type>()(data_);
  }

  // 获取存储对象的字符串
  LIBATFRAME_UTILS_API const std::string &to_cpp_string() const;

  LIBATFRAME_UTILS_API bool to_bool() const;

  LIBATFRAME_UTILS_API char to_char() const;

  LIBATFRAME_UTILS_API short to_short() const;

  LIBATFRAME_UTILS_API int to_int() const;

  LIBATFRAME_UTILS_API long to_long() const;

  LIBATFRAME_UTILS_API long long to_longlong() const;

  LIBATFRAME_UTILS_API double to_double() const;

  LIBATFRAME_UTILS_API float to_float() const;

  LIBATFRAME_UTILS_API const char *to_string() const;

  LIBATFRAME_UTILS_API unsigned char to_uchar() const;

  LIBATFRAME_UTILS_API unsigned short to_ushort() const;

  LIBATFRAME_UTILS_API unsigned int to_uint() const;

  LIBATFRAME_UTILS_API unsigned long to_ulong() const;

  LIBATFRAME_UTILS_API unsigned long long to_ulonglong() const;

  LIBATFRAME_UTILS_API int8_t to_int8() const;

  LIBATFRAME_UTILS_API uint8_t to_uint8() const;

  LIBATFRAME_UTILS_API int16_t to_int16() const;

  LIBATFRAME_UTILS_API uint16_t to_uint16() const;

  LIBATFRAME_UTILS_API int32_t to_int32() const;

  LIBATFRAME_UTILS_API uint32_t to_uint32() const;

  LIBATFRAME_UTILS_API int64_t to_int64() const;

  LIBATFRAME_UTILS_API uint64_t to_uint64() const;

  // ============ logic operation ============
  LIBATFRAME_UTILS_API bool to_logic_bool() const;

  LIBATFRAME_UTILS_API void split(char delim, std::vector<cmd_option_value> &out);
};
}  // namespace cli
LIBATFRAME_UTILS_NAMESPACE_END

#endif /* _CMDOPTIONVALUE_H_ */
