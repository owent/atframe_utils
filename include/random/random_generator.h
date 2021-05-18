/**
 * @file random_generator.h
 * @brief 伪随机数生成器
 *
 * Licensed under the MIT licenses.
 * @version 1.0
 * @author OWenT
 * @date 2018年09月30日
 *
 * @history
 *
 */

#ifndef UTIL_RANDOM_GENERATOR_H
#define UTIL_RANDOM_GENERATOR_H

#pragma once

#include <stdint.h>
#include <ctime>

#include <algorithm>
#include <cstddef>
#include <limits>
#include <numeric>

#include <config/atframe_utils_build_feature.h>
#include <config/compiler_features.h>

#include "random_mt_core.h"
#include "random_xor_combine_core.h"
#include "random_xoshiro_core.h"

#ifdef max
#  undef max
#endif

#ifdef min
#  undef min
#endif

namespace util {
namespace random {
/**
 * 随机数包装类，用以提供高级功能
 * 支持 UniformRandomBitGenerator
 * @see https://en.cppreference.com/w/cpp/named_req/UniformRandomBitGenerator
 * 支持 std::random_shuffle 里的 RandomFunc
 * @see https://en.cppreference.com/w/cpp/algorithm/random_shuffle
 */
template <typename CoreType>
class LIBATFRAME_UTILS_API_HEAD_ONLY random_manager_wrapper {
 public:
#if defined(UTIL_CONFIG_COMPILER_CXX_ALIAS_TEMPLATES) && UTIL_CONFIG_COMPILER_CXX_ALIAS_TEMPLATES
  using core_type = CoreType;
  using result_type = typename core_type::result_type;
#else
  typedef CoreType core_type;
  typedef typename core_type::result_type result_type;
#endif

 private:
  core_type core_;

 public:
  random_manager_wrapper() {}
  random_manager_wrapper(result_type rd_seed) : core_(rd_seed) {}

  inline core_type &get_core() UTIL_CONFIG_NOEXCEPT { return core_; }
  inline const core_type &get_core() const UTIL_CONFIG_NOEXCEPT { return core_; }

  /**
   * 初始化随机数种子
   * @param [in] rd_seed 随机数种子
   */
  void init_seed(result_type rd_seed) { core_.init_seed(rd_seed); }

  /**
   * 获取转储数据块的长度
   * @return 获取转储数据块的长度
   */
  inline size_t block_size() const UTIL_CONFIG_NOEXCEPT { return core_.block_size(); }

  /**
   * 转储随机数引擎的状态数据，可用于以后恢复
   * @param [out] output 输出数据地址
   * @param [in] size 输出数据大小,必须>=block_size()
   * @return output为空或数据长度错误返回false，其他情况返回true
   */
  inline bool dump(unsigned char *output, size_t size) const UTIL_CONFIG_NOEXCEPT { return core_.dump(output, size); }

  /**
   * 恢复随机数引擎的状态数据
   * @param [in] input 输入数据地址
   * @param [in] size 输入数据大小,必须>=block_size()
   * @return input为空或数据长度错误返回false，其他情况返回true
   */
  inline bool load(const unsigned char *input, size_t size) UTIL_CONFIG_NOEXCEPT { return core_.load(input, size); }

  /**
   * 初始化随机数种子
   * @note 取值范围为 [first, last)
   * @param [in] first 随机数种子散列值起始位置
   * @param [in] last 随机数种子散列值结束位置
   */
  template <class It>
  LIBATFRAME_UTILS_API_HEAD_ONLY void init_seed(It &first, It last) {
    core_.init_seed(first, last);
  }

  /**
   * 产生一个随机数
   * @return 产生的随机数
   */
  result_type random() UTIL_CONFIG_NOEXCEPT { return core_(); }

  /**
   * 产生一个随机数
   * @return 产生的随机数
   */
  result_type operator()() UTIL_CONFIG_NOEXCEPT { return random(); }

  /**
   * 产生一个随机数
   * @param [in] lowest 下限
   * @param [in] highest 上限(必须大于lowest)
   * @note 取值范围 [lowest, highest)
   * @return 产生的随机数
   */
  template <typename ResultType>
  LIBATFRAME_UTILS_API_HEAD_ONLY ResultType random_between(ResultType lowest, ResultType highest) UTIL_CONFIG_NOEXCEPT {
    if (highest <= lowest) {
      return lowest;
    }
    result_type res = (*this)();
    return static_cast<ResultType>(res % static_cast<result_type>(highest - lowest)) + lowest;
  }

 public:
  // ------------ Support for UniformRandomBitGenerator ------------
  static inline UTIL_CONFIG_CONSTEXPR result_type min() UTIL_CONFIG_NOEXCEPT {
    return std::numeric_limits<result_type>::min();
  }
  static inline UTIL_CONFIG_CONSTEXPR result_type max() UTIL_CONFIG_NOEXCEPT {
    return std::numeric_limits<result_type>::max();
  }
  inline result_type g() UTIL_CONFIG_NOEXCEPT { return random(); }

  // ------------ Support for RandomFunc ------------
  /**
   * 产生一个随机数
   * @return 产生的随机数
   */
  inline result_type operator()(result_type mod) UTIL_CONFIG_NOEXCEPT {
    if (0 == mod) {
      return random();
    } else {
      return random() % mod;
    }
  }

  template <typename RandomIt>
  LIBATFRAME_UTILS_API_HEAD_ONLY void shuffle(RandomIt first, RandomIt last) {
#if defined(__cplusplus) && __cplusplus >= 201103L
    std::shuffle(first, last, std::move(*this));
#elif defined(_MSVC_LANG) && _MSVC_LANG >= 201402L
    std::shuffle(first, last, std::move(*this));
#else
    std::random_shuffle(first, last, *this);
#endif
  }
};

// ============== 随机数生成器 - 梅森旋转算法(STL 标准算法) ==============
#if defined(UTIL_CONFIG_COMPILER_CXX_ALIAS_TEMPLATES) && UTIL_CONFIG_COMPILER_CXX_ALIAS_TEMPLATES
typedef random_manager_wrapper<core::mersenne_twister<uint32_t, 351, 175, 19, 0xccab8ee7, 11, 0xffffffff, 7, 0x31b6ab00,
                                                      15, 0xffe50000, 17, 1812433253> >
    mt11213b;

typedef random_manager_wrapper<core::mersenne_twister<uint32_t, 624, 397, 31, 0x9908b0df, 11, 0xffffffff, 7, 0x9d2c5680,
                                                      15, 0xefc60000, 18, 1812433253> >
    mt19937;

typedef random_manager_wrapper<
    core::mersenne_twister<uint64_t, 312, 156, 31, 0xb5026f5aa96619e9ULL, 29, 0x5555555555555555ULL, 17,
                           0x71d67fffeda60000ULL, 37, 0xfff7eee000000000ULL, 43, 6364136223846793005ULL> >
    mt19937_64;
#else
using mt11213b = random_manager_wrapper<core::mersenne_twister<uint32_t, 351, 175, 19, 0xccab8ee7, 11, 0xffffffff, 7,
                                                               0x31b6ab00, 15, 0xffe50000, 17, 1812433253> >;
using mt19937 = random_manager_wrapper<core::mersenne_twister<uint32_t, 624, 397, 31, 0x9908b0df, 11, 0xffffffff, 7,
                                                              0x9d2c5680, 15, 0xefc60000, 18, 1812433253> >;
using mt19937_64 = random_manager_wrapper<
    core::mersenne_twister<uint64_t, 312, 156, 31, 0xb5026f5aa96619e9ULL, 29, 0x5555555555555555ULL, 17,
                           0x71d67fffeda60000ULL, 37, 0xfff7eee000000000ULL, 43, 6364136223846793005ULL> >;
#endif
// ============== 随机数生成器 - taus 算法(比梅森旋转算法消耗更少的内存，但是循环节更小) ==============
#if defined(UTIL_CONFIG_COMPILER_CXX_ALIAS_TEMPLATES) && UTIL_CONFIG_COMPILER_CXX_ALIAS_TEMPLATES
using taus88 = random_manager_wrapper<
    core::xor_combine_engine<core::xor_combine_engine<core::linear_feedback_shift_engine<uint32_t, 32, 31, 13, 12>, 0,
                                                      core::linear_feedback_shift_engine<uint32_t, 32, 29, 2, 4>, 0>,
                             0, core::linear_feedback_shift_engine<uint32_t, 32, 28, 3, 17>, 0> >;
#else
typedef random_manager_wrapper<
    core::xor_combine_engine<core::xor_combine_engine<core::linear_feedback_shift_engine<uint32_t, 32, 31, 13, 12>, 0,
                                                      core::linear_feedback_shift_engine<uint32_t, 32, 29, 2, 4>, 0>,
                             0, core::linear_feedback_shift_engine<uint32_t, 32, 28, 3, 17>, 0> >
    taus88;
#endif

// ============== 随机数生成器 - xoshiro 算法(比梅森旋转算法消耗更少的内存，但是循环节更小，随机性比taus好)
// ==============
// @see http://xoshiro.di.unimi.it
#if defined(UTIL_CONFIG_COMPILER_CXX_ALIAS_TEMPLATES) && UTIL_CONFIG_COMPILER_CXX_ALIAS_TEMPLATES
using xoroshiro128_starstar = random_manager_wrapper<core::xoshiro_engine_128<false> >;
using xoroshiro128_plus = random_manager_wrapper<core::xoshiro_engine_128<true> >;
using xoshiro256_starstar = random_manager_wrapper<core::xoshiro_engine_256<false> >;
using xoshiro256_plus = random_manager_wrapper<core::xoshiro_engine_256<true> >;
#else
// 循环节： 2^128 − 1
typedef random_manager_wrapper<core::xoshiro_engine_128<false> > xoroshiro128_starstar;
// 循环节： 2^128 − 1，少一次旋转，更快一点点
typedef random_manager_wrapper<core::xoshiro_engine_128<true> > xoroshiro128_plus;
// 循环节： 2^256 − 1
typedef random_manager_wrapper<core::xoshiro_engine_256<false> > xoshiro256_starstar;
// 循环节： 2^256 − 1，少一次旋转，更快一点点
typedef random_manager_wrapper<core::xoshiro_engine_256<true> > xoshiro256_plus;
#endif
}  // namespace random
}  // namespace util

#endif /* _UTIL_RANDOM_GENERATOR_H_ */
