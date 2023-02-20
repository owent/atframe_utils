/**
 * @file random_xor_combine_core.h
 * @brief 伪随机数生成器  - xoshiro算法核心
 * @see http://xoshiro.di.unimi.it
 * @note 2018年的新型的全功能型伪随机数算法，也是lua 5.4内建的伪随机数算法
 *
 * Licensed under the MIT licenses.
 * @version 1.0
 * @author OWenT
 * @date 2018年09月30日
 *
 * @history
 *  2021-01-20: 修正拼写错误
 *
 */

#ifndef UTIL_RANDOM_XOSHIRO_CORE_H
#define UTIL_RANDOM_XOSHIRO_CORE_H

#pragma once

#include <cstring>
#include <memory>

#include <config/atframe_utils_build_feature.h>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace random {
namespace core {
/**
 * @breif just for
 *          xoshiro256**
 *          xoshiro256+
 *          xoroshiro128**
 *          xoroshiro128+
 * @note not support for xoroshiro64** 、xoroshiro64*、xoroshiro512** 、xoroshiro512*、xoroshiro1024** 、xoroshiro1024*
 */
template <class UIntType, bool is_plus, int iidx, int n1, int n2>
class LIBATFRAME_UTILS_API_HEAD_ONLY xoshiro_engine {
 public:
  using result_type = UIntType;
  using seed_type = result_type[4];

 private:
  /// \endcond
  seed_type xoshiro_seed_;

  static inline result_type rotl(const result_type x, int k) noexcept {
    return (static_cast<result_type>(x) << k) |
           (static_cast<result_type>(x) >>
            (static_cast<result_type>(sizeof(result_type) * 8) - static_cast<result_type>(k)));
  }

  template <class, bool>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY next_init;

  template <class T>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY next_init<T, true> {
    static inline result_type call(seed_type &s) noexcept { return s[0] + s[3]; }
  };

  template <class T>
  struct LIBATFRAME_UTILS_API_HEAD_ONLY next_init<T, false> {
    static inline result_type call(seed_type &s) noexcept { return rotl(s[iidx] * 5, 7) * 9; }
  };

 protected:
  result_type next() noexcept {
    const result_type ret = next_init<UIntType, is_plus>::call(xoshiro_seed_);
    const result_type t = xoshiro_seed_[1] << n1;

    xoshiro_seed_[2] ^= xoshiro_seed_[0];
    xoshiro_seed_[3] ^= xoshiro_seed_[1];
    xoshiro_seed_[1] ^= xoshiro_seed_[2];
    xoshiro_seed_[0] ^= xoshiro_seed_[3];

    xoshiro_seed_[2] ^= t;
    xoshiro_seed_[3] = rotl(xoshiro_seed_[3], n2);

    return ret;
  }

  void jump(const seed_type &JUMP) noexcept {
    result_type s0 = 0;
    result_type s1 = 0;
    result_type s2 = 0;
    result_type s3 = 0;
    for (size_t i = 0; i < sizeof(JUMP) / sizeof(JUMP[0]); i++) {
      for (size_t b = 0; b < sizeof(result_type) * 8; b++) {
        if (JUMP[i] & result_type(1) << b) {
          s0 ^= xoshiro_seed_[0];
          s1 ^= xoshiro_seed_[1];
          s2 ^= xoshiro_seed_[2];
          s3 ^= xoshiro_seed_[3];
        }
        next();
      }
    }

    xoshiro_seed_[0] = s0;
    xoshiro_seed_[1] = s1;
    xoshiro_seed_[2] = s2;
    xoshiro_seed_[3] = s3;
  }

 public:
  xoshiro_engine() {
    xoshiro_seed_[0] = 0;
    xoshiro_seed_[1] = 0;
    xoshiro_seed_[2] = 0;
    xoshiro_seed_[3] = 0;
  }
  xoshiro_engine(result_type s) {
    xoshiro_seed_[0] = 0;
    xoshiro_seed_[1] = 0;
    xoshiro_seed_[2] = 0;
    xoshiro_seed_[3] = 0;
    init_seed(s);
  }

  void init_seed(result_type s) noexcept {
    xoshiro_seed_[0] = s;
    xoshiro_seed_[1] = 0xff;
    xoshiro_seed_[2] = 0;
    xoshiro_seed_[3] = 0;

    // just like in lua 5.4
    for (int i = 0; i < 16; ++i) {
      next();
    }
  }

  template <class It>
  LIBATFRAME_UTILS_API_HEAD_ONLY void init_seed(It &first, It last) {
    It begin = first;
    for (int i = 0; i < 4; ++i) {
      if (begin != last) {
        xoshiro_seed_[i] = *begin;
        ++begin;
      } else {
        xoshiro_seed_[i] = 0;
      }
    }

    // just like in lua 5.4
    for (int i = 0; i < 16; ++i) {
      next();
    }
  }

  inline size_t block_size() const noexcept { return sizeof(xoshiro_seed_); }

  inline bool dump(unsigned char *output, size_t size) const noexcept {
    if (nullptr == output || size < block_size()) {
      return false;
    }

    memcpy(output, xoshiro_seed_, sizeof(xoshiro_seed_));
    return true;
  }

  inline bool load(const unsigned char *input, size_t size) noexcept {
    if (nullptr == input || size < block_size()) {
      return false;
    }

    memcpy(xoshiro_seed_, input, sizeof(xoshiro_seed_));
    return true;
  }

  result_type random() noexcept { return next(); }

  result_type operator()() noexcept { return random(); }

  inline const seed_type &get_seed() const noexcept { return xoshiro_seed_; }
};

template <bool is_plus>
class LIBATFRAME_UTILS_API_HEAD_ONLY xoshiro_engine_128 : public xoshiro_engine<uint32_t, is_plus, 0, 9, 11> {
 public:
  using base_type = xoshiro_engine<uint32_t, is_plus, 0, 9, 11>;
  using result_type = typename base_type::result_type;
  using seed_type = typename base_type::seed_type;

 public:
  xoshiro_engine_128() {}
  xoshiro_engine_128(result_type s) : base_type(s) {}

  using base_type::jump;

  /**
   * @brief just like call next() for 2^64 times
   */
  void jump() noexcept {
    static constexpr const result_type jump_params[4] = {0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b};
    jump(jump_params);
  }
};

template <bool is_plus>
class LIBATFRAME_UTILS_API_HEAD_ONLY xoshiro_engine_256 : public xoshiro_engine<uint64_t, is_plus, 1, 17, 45> {
 public:
  using base_type = xoshiro_engine<uint64_t, is_plus, 1, 17, 45>;
  using result_type = typename base_type::result_type;
  using seed_type = typename base_type::seed_type;

 public:
  xoshiro_engine_256() {}
  xoshiro_engine_256(result_type s) : base_type(s) {}

  using base_type::jump;

  /**
   * @brief just like call next() for 2^128 times
   */
  void jump() noexcept {
    static constexpr const result_type jump_params[4] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa,
                                                         0x39abdc4529b1661c};
    jump(jump_params);
  }

  /**
   * @brief just like call next() for 2^192 times
   */
  void long_jump() noexcept {
    static constexpr const result_type jump_params[4] = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241,
                                                         0x39109bb02acbe635};
    jump(jump_params);
  }
};
}  // namespace core
}  // namespace random
LIBATFRAME_UTILS_NAMESPACE_END

#endif /* UTIL_RANDOM_XOSHIRO_CORE_H */
