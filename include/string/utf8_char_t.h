// Copyright 2021 atframework
// Created by owent on 2017-06-05

#pragma once

#include <assert.h>
#include <stdint.h>
#include <cstddef>
#include <cstring>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <type_traits>

#ifdef __cpp_impl_three_way_comparison
#  include <compare>
#endif

#include "common/string_oprs.h"

#include "config/atframe_utils_build_feature.h"

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace string {
struct LIBATFRAME_UTILS_API_HEAD_ONLY utf8_char_t {
  explicit utf8_char_t(const char *str) {
    size_t len = length(str);
    for (size_t i = 0; i < len; ++i) {
      data[i] = str[i];
    }
  }

  explicit utf8_char_t(const std::string &str) {
    size_t len = length(str.c_str());
    for (size_t i = 0; i < len; ++i) {
      data[i] = str[i];
    }
  }

  utf8_char_t(char c) {
    c &= 0x7f; // protect invalid char
    data[0] = c;
  }

  char data[8];

  static inline size_t length(const char *s) {
    size_t ret = 1;
    char c = 0;
    if (nullptr != s) {
      c = *s;
    }

    if (!(c & 0x80)) {
      return ret;
    }

    for (; ret < 6; ++ret, c <<= 1) {
      if (!(c & 0x40)) {
        break;
      }
    }

    return ret;
  }

  inline char operator[](size_t idx) const { return idx < sizeof(data) ? data[idx] : 0; }
  inline char operator[](int idx) const {
    return (idx >= 0 && static_cast<size_t>(idx) < sizeof(data)) ? data[idx] : 0;
  }
  inline char operator[](int64_t idx) const {
    return (idx >= 0 && static_cast<size_t>(idx) < sizeof(data)) ? data[idx] : 0;
  }

  inline size_t length() const { return length(&data[0]); }

  friend bool operator==(const utf8_char_t &l, const utf8_char_t &r) {
    size_t len = l.length();

    if (l.length() != r.length()) {
      return false;
    }

    for (size_t i = 0; i < len; ++i) {
      if (l.data[i] != r.data[i]) {
        return false;
      }
    }

    return true;
  }

#ifdef __cpp_impl_three_way_comparison
  friend std::strong_ordering operator<=>(const utf8_char_t &l, const utf8_char_t &r) {
    size_t len = l.length();

    if (l.length() != r.length()) {
      if (l.length() < r.length()) {
        return std::strong_ordering::less;
      } else {
        return std::strong_ordering::greater;
      }
    }

    for (size_t i = 0; i < len; ++i) {
      if (l.data[i] != r.data[i]) {
        if (l.data[i] < r.data[i]) {
          return std::strong_ordering::less;
        } else {
          return std::strong_ordering::greater;
        }
      }
    }

    return std::strong_ordering::equal;
  }
#else
  friend bool operator!=(const utf8_char_t &l, const utf8_char_t &r) { return !(l == r); }

  friend bool operator<(const utf8_char_t &l, const utf8_char_t &r) {
    size_t len = l.length();

    if (l.length() != r.length()) {
      return l.length() < r.length();
    }

    for (size_t i = 0; i < len; ++i) {
      if (l.data[i] != r.data[i]) {
        return l.data[i] < r.data[i];
      }
    }

    return false;
  }

  friend bool operator<=(const utf8_char_t &l, const utf8_char_t &r) {
    size_t len = l.length();

    if (l.length() != r.length()) {
      return l.length() < r.length();
    }

    for (size_t i = 0; i < len; ++i) {
      if (l.data[i] != r.data[i]) {
        return l.data[i] <= r.data[i];
      }
    }

    return true;
  }

  friend bool operator>(const utf8_char_t &l, const utf8_char_t &r) { return !(l <= r); }

  friend bool operator>=(const utf8_char_t &l, const utf8_char_t &r) { return !(l < r); }
#endif

  template <typename CH, typename CHT>
  LIBATFRAME_UTILS_API_HEAD_ONLY friend std::basic_ostream<CH, CHT> &operator<<(std::basic_ostream<CH, CHT> &os,
                                                                                const utf8_char_t &self) {
    os.write((CH *)self.data, self.length());
    return os;
  }

  template <typename CH, typename CHT>
  LIBATFRAME_UTILS_API_HEAD_ONLY friend std::basic_istream<CH, CHT> &operator>>(std::basic_istream<CH, CHT> &is,
                                                                                utf8_char_t &self) {
    self.data[0] = 0;
    is.read((CH *)&self.data[0], 1);
    size_t len = self.length();
    if (len > 1) {
      is.read((CH *)&self.data[1], len - 1);
    }
    return is;
  }
};
}  // namespace string
LIBATFRAME_UTILS_NAMESPACE_END

namespace std {
template <>
struct hash<LIBATFRAME_UTILS_NAMESPACE_ID::string::utf8_char_t> {
  std::size_t operator()(const LIBATFRAME_UTILS_NAMESPACE_ID::string::utf8_char_t &c) const noexcept {
    std::hash<char> hasher = std::hash<char>{};
    std::size_t result = hasher(c[0]);
    std::size_t length = c.length();
    for (std::size_t i = 1; i < length; ++i) {
      result = (result << 1) ^ hasher(c[i]);
    }
    return result;
  }
};
}  // namespace std
