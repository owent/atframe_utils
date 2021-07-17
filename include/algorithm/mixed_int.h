/**
 * @file mixed_int.h
 * @brief 混淆整数类型
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2014.12.15
 *
 * @history
 *
 *
 */

#ifndef UTIL_ALGORITHM_MIXEDINT_H
#define UTIL_ALGORITHM_MIXEDINT_H

// 目测主流编译器都支持且有优化， gcc 3.4 and upper, vc, clang, c++ builder xe3, intel c++ and etc.
#pragma once

#include <stddef.h>
#include <stdint.h>
#include <type_traits>

#ifdef __cpp_impl_three_way_comparison
#  include <compare>
#endif

#include <common/compiler_message.h>
#include <config/atframe_utils_build_feature.h>

// 定义ENABLE_MIXEDINT_MAGIC_MASK=[数字]以设置整数混淆功能
// 如果ENABLE_MIXEDINT_MAGIC_MASK=1，则刚好是ZigZag编码规则

#if defined(ENABLE_MIXEDINT_MAGIC_MASK)
#  if ENABLE_MIXEDINT_MAGIC_MASK <= 0
#    undef ENABLE_MIXEDINT_MAGIC_MASK

#  elif ENABLE_MIXEDINT_MAGIC_MASK >= 64
COMPILER_MSG_ERROR("ENABLE_MIXEDINT_MAGIC_MASK must be less than 64");
#  endif

#endif

namespace util {
namespace mixed_int {
#if defined(ENABLE_MIXEDINT_MAGIC_MASK)
namespace detail {
template <typename TSINT>
class LIBATFRAME_UTILS_API_HEAD_ONLY mixed_signed_int {
 public:
  using value_type = TSINT;

  static const size_t TYPE_BIT = 8 * sizeof(value_type);
  static const size_t TYPE_LEFT_BIT = ENABLE_MIXEDINT_MAGIC_MASK % TYPE_BIT;
  static const size_t TYPE_RIGHT_BIT = TYPE_BIT - TYPE_LEFT_BIT;
  static const value_type RIGHT_MASK = (((value_type)1) << TYPE_RIGHT_BIT) - 1;
  static const value_type SYMBOL_MASK = ((value_type)1) << (TYPE_LEFT_BIT - 1);

 public:
  static value_type encode(value_type d) { return (d << TYPE_LEFT_BIT) ^ (d >> TYPE_RIGHT_BIT); }

  static value_type decode(value_type d) {
    return (d << TYPE_RIGHT_BIT) | (RIGHT_MASK & (d & SYMBOL_MASK) ? ~(d >> TYPE_LEFT_BIT) : (d >> TYPE_LEFT_BIT));
  }
};

template <typename TUSINT>
class LIBATFRAME_UTILS_API_HEAD_ONLY mixed_unsigned_int {
 public:
  typedef TUSINT value_type;

  static const size_t TYPE_BIT = 8 * sizeof(value_type);
  static const size_t TYPE_LEFT_BIT = ENABLE_MIXEDINT_MAGIC_MASK % TYPE_BIT;
  static const size_t TYPE_RIGHT_BIT = TYPE_BIT - TYPE_LEFT_BIT;
  static const value_type RIGHT_MASK = (((value_type)1) << TYPE_RIGHT_BIT) - 1;
  static const value_type SYMBOL_MASK = ((value_type)1) << (TYPE_LEFT_BIT - 1);

 public:
  static value_type encode(value_type d) { return (d << TYPE_LEFT_BIT) ^ (d >> TYPE_RIGHT_BIT); }

  static value_type decode(value_type d) { return (d << TYPE_RIGHT_BIT) ^ (d >> TYPE_LEFT_BIT); }
};

template <typename TINT>
class LIBATFRAME_UTILS_API_HEAD_ONLY mixed_int
    : public std::conditional<std::is_unsigned<TINT>::value, mixed_unsigned_int<TINT>, mixed_signed_int<TINT> >::type {
 public:
  typedef TINT value_type;
  typedef mixed_int<value_type> self_type;
  typedef
      typename std::conditional<std::is_unsigned<TINT>::value, mixed_unsigned_int<TINT>, mixed_signed_int<TINT> >::type
          base_type;

  using base_type::decode;
  using base_type::encode;

 public:
  // constructure & destructure
  mixed_int() {}
  mixed_int(const mixed_int &other) : data_(other.data_) {}
  mixed_int(value_type d) : data_(encode(d)) {}

  // assign & assign copy
  mixed_int &operator=(const mixed_int &other) {
    data_ = other.data_;
    return (*this);
  }

  mixed_int &operator=(value_type d) {
    data_ = encode(d);
    return (*this);
  }

  // type convert
  operator value_type() const { return decode(data_); }

  operator bool() const { return !!data_; }

  // compare
  friend bool operator==(const self_type &l, const self_type &r) { return l.data_ == r.data_; }
  template <typename TL>
  friend bool operator==(const TL &l, const self_type &r) {
    return unwrapper(l) == decode(r.data_);
  }
  template <typename TR>
  friend bool operator==(const self_type &l, const TR &r) {
    return decode(l.data_) == unwrapper(r);
  }

#  if defined(__cpp_impl_three_way_comparison) && !defined(_MSC_VER)
  friend std::strong_ordering operator<=>(const self_type &l, const self_type &r) {
    return decode(l.data_) <=> decode(r.data_);
  }
  template <typename TL>
  friend std::strong_ordering operator<=>(const TL &l, const self_type &r) {
    return unwrapper(l) <=> decode(r.data_);
  }
  template <typename TR>
  friend std::strong_ordering operator<=>(const self_type &l, const TR &r) {
    return decode(l.data_) <=> unwrapper(r);
  }
#  else
  friend bool operator!=(const self_type &l, const self_type &r) { return !(l == r); }
  template <typename TL>
  friend bool operator!=(const TL &l, const self_type &r) {
    return !(l == r);
  }
  template <typename TR>
  friend bool operator!=(const self_type &l, const TR &r) {
    return !(l == r);
  }

  friend bool operator<(const self_type &l, const self_type &r) { return decode(l.data_) < decode(r.data_); }
  template <typename TL>
  friend bool operator<(const TL &l, const self_type &r) {
    return unwrapper(l) < decode(r.data_);
  }
  template <typename TR>
  friend bool operator<(const self_type &l, const TR &r) {
    return decode(l.data_) < unwrapper(r);
  }

  friend bool operator<=(const self_type &l, const self_type &r) { return decode(l.data_) <= decode(r.data_); }
  template <typename TL>
  friend bool operator<=(const TL &l, const self_type &r) {
    return unwrapper(l) <= decode(r.data_);
  }
  template <typename TR>
  friend bool operator<=(const self_type &l, const TR &r) {
    return decode(l.data_) <= unwrapper(r);
  }

  friend bool operator>(const self_type &l, const self_type &r) { return decode(l.data_) > decode(r.data_); }
  template <typename TL>
  friend bool operator>(const TL &l, const self_type &r) {
    return unwrapper(l) > decode(r.data_);
  }
  template <typename TR>
  friend bool operator>(const self_type &l, const TR &r) {
    return decode(l.data_) > unwrapper(r);
  }

  friend bool operator>=(const self_type &l, const self_type &r) { return decode(l.data_) >= decode(r.data_); }
  template <typename TL>
  friend bool operator>=(const TL &l, const self_type &r) {
    return unwrapper(l) >= decode(r.data_);
  }
  template <typename TR>
  friend bool operator>=(const self_type &l, const TR &r) {
    return decode(l.data_) >= unwrapper(r);
  }
#  endif

  // calc
  template <typename TR>
  self_type &operator+=(const TR &other) {
    return (*this) = (value_type)(*this) + unwrapper(other);
  }
  template <typename TR>
  self_type &operator-=(const TR &other) {
    return (*this) = (value_type)(*this) - unwrapper(other);
  }
  template <typename TR>
  self_type &operator*=(const TR &other) {
    return (*this) = (value_type)(*this) * unwrapper(other);
  }
  template <typename TR>
  self_type &operator/=(const TR &other) {
    return (*this) = (value_type)(*this) / unwrapper(other);
  }
  template <typename TR>
  self_type &operator%=(const TR &other) {
    return (*this) = (value_type)(*this) % unwrapper(other);
  }

  self_type &operator++() { return (*this) = (value_type)(*this) + 1; }
  self_type operator++(int) {
    self_type ret = (*this);
    ++(*this);
    return ret;
  }
  self_type &operator--() { return (*this) = (value_type)(*this) - 1; }
  self_type operator--(int) {
    self_type ret = (*this);
    --(*this);
    return ret;
  }

  friend value_type operator+(const self_type &l, const self_type &r) { return (value_type)l + (value_type)r; }
  template <typename TL>
  friend value_type operator+(const TL &l, const self_type &r) {
    return unwrapper(l) + (value_type)r;
  }
  template <typename TR>
  friend value_type operator+(const self_type &l, const TR &r) {
    return (value_type)l + unwrapper(r);
  }

  friend value_type operator-(const self_type &l, const self_type &r) { return (value_type)l - (value_type)r; }
  template <typename TL>
  friend value_type operator-(const TL &l, const self_type &r) {
    return unwrapper(l) - (value_type)r;
  }
  template <typename TR>
  friend value_type operator-(const self_type &l, const TR &r) {
    return (value_type)l - unwrapper(r);
  }

  friend value_type operator*(const self_type &l, const self_type &r) { return (value_type)l * (value_type)r; }
  template <typename TL>
  friend value_type operator*(const TL &l, const self_type &r) {
    return unwrapper(l) * (value_type)r;
  }
  template <typename TR>
  friend value_type operator*(const self_type &l, const TR &r) {
    return (value_type)l * unwrapper(r);
  }

  friend value_type operator/(const self_type &l, const self_type &r) { return (value_type)l / (value_type)r; }
  template <typename TL>
  friend value_type operator/(const TL &l, const self_type &r) {
    return unwrapper(l) / (value_type)r;
  }
  template <typename TR>
  friend value_type operator/(const self_type &l, const TR &r) {
    return (value_type)l / unwrapper(r);
  }

  friend value_type operator%(const self_type &l, const self_type &r) { return (value_type)l % (value_type)r; }
  template <typename TL>
  friend value_type operator%(const TL &l, const self_type &r) {
    return unwrapper(l) % (value_type)r;
  }
  template <typename TR>
  friend value_type operator%(const self_type &l, const TR &r) {
    return (value_type)l % unwrapper(r);
  }

  // bit operator
  template <typename TR>
  self_type &operator&=(const TR &other) {
    return (*this) = (value_type)(*this) & unwrapper(other);
  }
  template <typename TR>
  self_type &operator|=(const TR &other) {
    return (*this) = (value_type)(*this) | unwrapper(other);
  }
  template <typename TR>
  self_type &operator^=(const TR &other) {
    return (*this) = (value_type)(*this) ^ unwrapper(other);
  }
  template <typename TR>
  self_type &operator>>=(const TR &other) {
    return (*this) = (value_type)(*this) >> unwrapper(other);
  }
  template <typename TR>
  self_type &operator<<=(const TR &other) {
    return (*this) = (value_type)(*this) << unwrapper(other);
  }

  friend value_type operator&(const self_type &l, const self_type &r) { return (value_type)l & (value_type)r; }
  template <typename TL>
  friend value_type operator&(const TL &l, const self_type &r) {
    return unwrapper(l) & (value_type)r;
  }
  template <typename TR>
  friend value_type operator&(const self_type &l, const TR &r) {
    return (value_type)l & unwrapper(r);
  }

  friend value_type operator|(const self_type &l, const self_type &r) { return (value_type)l | (value_type)r; }
  template <typename TL>
  friend value_type operator|(const TL &l, const self_type &r) {
    return unwrapper(l) | (value_type)r;
  }
  template <typename TR>
  friend value_type operator|(const self_type &l, const TR &r) {
    return (value_type)l | unwrapper(r);
  }

  friend value_type operator^(const self_type &l, const self_type &r) { return (value_type)l ^ (value_type)r; }
  template <typename TL>
  friend value_type operator^(const TL &l, const self_type &r) {
    return unwrapper(l) ^ (value_type)r;
  }
  template <typename TR>
  friend value_type operator^(const self_type &l, const TR &r) {
    return (value_type)l ^ unwrapper(r);
  }

  friend value_type operator>>(const self_type &l, const self_type &r) { return (value_type)l >> (value_type)r; }
  template <typename TL>
  friend value_type operator>>(const TL &l, const self_type &r) {
    return unwrapper(l) >> (value_type)r;
  }
  template <typename TR>
  friend value_type operator>>(const self_type &l, const TR &r) {
    return (value_type)l >> unwrapper(r);
  }

  friend value_type operator<<(const self_type &l, const self_type &r) { return (value_type)l << (value_type)r; }
  template <typename TL>
  friend value_type operator<<(const TL &l, const self_type &r) {
    return unwrapper(l) << (value_type)r;
  }
  template <typename TR>
  friend value_type operator<<(const self_type &l, const TR &r) {
    return (value_type)l << unwrapper(r);
  }

 private:
  template <typename TMI>
  static inline value_type unwrapper(const mixed_int<TMI> &x) {
    return mixed_int<TMI>::decode(x.data_);
  }

  template <typename TI>
  static inline value_type unwrapper(const TI &x) {
    return static_cast<value_type>(x);
  }

  value_type data_;
};
}  // namespace detail

typedef util::mixed_int::detail::mixed_int<int8_t> mixed_int8_t;
typedef util::mixed_int::detail::mixed_int<int16_t> mixed_int16_t;
typedef util::mixed_int::detail::mixed_int<int32_t> mixed_int32_t;
typedef util::mixed_int::detail::mixed_int<int64_t> mixed_int64_t;
typedef util::mixed_int::detail::mixed_int<uint8_t> mixed_uint8_t;
typedef util::mixed_int::detail::mixed_int<uint16_t> mixed_uint16_t;
typedef util::mixed_int::detail::mixed_int<uint32_t> mixed_uint32_t;
typedef util::mixed_int::detail::mixed_int<uint64_t> mixed_uint64_t;

#else
typedef int8_t mixed_int8_t;
typedef int16_t mixed_int16_t;
typedef int32_t mixed_int32_t;
typedef int64_t mixed_int64_t;
typedef uint8_t mixed_uint8_t;
typedef uint16_t mixed_uint16_t;
typedef uint32_t mixed_uint32_t;
typedef uint64_t mixed_uint64_t;
#endif
}  // namespace mixed_int
}  // namespace util

typedef util::mixed_int::mixed_int8_t mixed_int8_t;
typedef util::mixed_int::mixed_int16_t mixed_int16_t;
typedef util::mixed_int::mixed_int32_t mixed_int32_t;
typedef util::mixed_int::mixed_int64_t mixed_int64_t;
typedef util::mixed_int::mixed_uint8_t mixed_uint8_t;
typedef util::mixed_int::mixed_uint16_t mixed_uint16_t;
typedef util::mixed_int::mixed_uint32_t mixed_uint32_t;
typedef util::mixed_int::mixed_uint64_t mixed_uint64_t;

#endif /* _UTIL_ALGORITHM_MIXEDINT_H_ */
