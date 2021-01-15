/**
 * @file random_mt_core.h
 * @brief 伪随机数生成器  - 梅森旋转算法
 *
 * Licensed under the MIT licenses.
 * @note 提供mt19937、mt19937-64和mt11213b标准算法
 *
 * @version 1.0
 * @author OWenT
 * @date 2013年8月5日
 *
 * @history
 *
 */


#ifndef UTIL_RANDOM_RANDOMMTCORE_H
#define UTIL_RANDOM_RANDOMMTCORE_H

#pragma once

#include <config/atframe_utils_build_feature.h>

namespace util {
    namespace random {
        namespace core {
            /**
             * 伪随机数生成器
             * 梅森旋转算法
             * @note 参考自GCC random源码, boost库及维基百科
             * http://zh.wikipedia.org/wiki/%E6%A2%85%E6%A3%AE%E6%97%8B%E8%BD%AC%E7%AE%97%E6%B3%95
             * @tparam MAX_STATUS_N_SIZE  The degree of recursion.
             * @tparam RD_M         The period parameter.
             * @tparam SPB_IDX      The separation point bit index.
             * @tparam RD_A         The last row of the twist matrix.
             * @tparam RD_U         The first right-shift tempering matrix parameter.
             * @tparam RD_D         The first right-shift tempering matrix mask.
             * @tparam RD_S         The first left-shift tempering matrix parameter.
             * @tparam RD_B         The first left-shift tempering matrix mask.
             * @tparam RD_T         The second left-shift tempering matrix parameter.
             * @tparam RD_C         The second left-shift tempering matrix mask.
             * @tparam RD_L         The second right-shift tempering matrix parameter.
             * @tparam INIT_SEED_F  Initialization multiplier.
             */
            template <typename UIntType, std::size_t MAX_STATUS_N_SIZE, std::size_t RD_M, std::size_t SPB_IDX, UIntType RD_A,
                      std::size_t RD_U, UIntType RD_D, std::size_t RD_S, UIntType RD_B, std::size_t RD_T, UIntType RD_C, std::size_t RD_L,
                      UIntType INIT_SEED_F>
            class LIBATFRAME_UTILS_API_HEAD_ONLY mersenne_twister {
            public:
                typedef UIntType result_type;

            private:
                UIntType    mt_status[MAX_STATUS_N_SIZE];
                std::size_t mt_index;

                void twist() {
                    const UIntType mt_upper_mask = (~static_cast<UIntType>(0)) << SPB_IDX;
                    const UIntType mt_lower_mask = ~mt_upper_mask;

                    const std::size_t mt_unroll_factor = 6;
                    const std::size_t mt_unroll_extra1 = (MAX_STATUS_N_SIZE - RD_M) % mt_unroll_factor;
                    const std::size_t mt_unroll_extra2 = (RD_M - 1) % mt_unroll_factor;

                    // 这里为了减少取模操作把一个循环拆成了四个
                    for (std::size_t i = 0; i < MAX_STATUS_N_SIZE - RD_M - mt_unroll_extra1; ++i) {
                        UIntType y   = (mt_status[i] & mt_upper_mask) | (mt_status[i + 1] & mt_lower_mask);
                        mt_status[i] = mt_status[i + RD_M] ^ (y >> 1) ^ ((mt_status[i + 1] & 1) * RD_A);
                    }
                    for (std::size_t i = MAX_STATUS_N_SIZE - RD_M - mt_unroll_extra1; i < MAX_STATUS_N_SIZE - RD_M; ++i) {
                        UIntType y   = (mt_status[i] & mt_upper_mask) | (mt_status[i + 1] & mt_lower_mask);
                        mt_status[i] = mt_status[i + RD_M] ^ (y >> 1) ^ ((mt_status[i + 1] & 1) * RD_A);
                    }
                    for (std::size_t i = MAX_STATUS_N_SIZE - RD_M; i < MAX_STATUS_N_SIZE - 1 - mt_unroll_extra2; ++i) {
                        UIntType y   = (mt_status[i] & mt_upper_mask) | (mt_status[i + 1] & mt_lower_mask);
                        mt_status[i] = mt_status[i - (MAX_STATUS_N_SIZE - RD_M)] ^ (y >> 1) ^ ((mt_status[i + 1] & 1) * RD_A);
                    }
                    for (std::size_t i = MAX_STATUS_N_SIZE - 1 - mt_unroll_extra2; i < MAX_STATUS_N_SIZE - 1; ++i) {
                        UIntType y   = (mt_status[i] & mt_upper_mask) | (mt_status[i + 1] & mt_lower_mask);
                        mt_status[i] = mt_status[i - (MAX_STATUS_N_SIZE - RD_M)] ^ (y >> 1) ^ ((mt_status[i + 1] & 1) * RD_A);
                    }
                    // last iteration
                    UIntType y                       = (mt_status[MAX_STATUS_N_SIZE - 1] & mt_upper_mask) | (mt_status[0] & mt_lower_mask);
                    mt_status[MAX_STATUS_N_SIZE - 1] = mt_status[RD_M - 1] ^ (y >> 1) ^ ((mt_status[0] & 1) * RD_A);
                    mt_index                         = 0;
                }

            public:
                mersenne_twister() {}

                mersenne_twister(UIntType mt_seed) { init_seed(mt_seed); }

                /**
                 * 初始化随机数种子
                 * @param [in] mt_seed 随机数种子
                 */
                void init_seed(UIntType mt_seed) {
                    // New seeding algorithm from
                    // http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/emt19937ar.html
                    // In the previous versions, MSBs of the seed affected only MSBs of the
                    // state mt_status[].

#ifdef max
#undef max
#endif
                    const UIntType mask = std::numeric_limits<UIntType>::max();

                    mt_status[0] = mt_seed & mask;
                    for (mt_index = 1; mt_index < MAX_STATUS_N_SIZE; ++mt_index) {
                        // See Knuth "The Art of Computer Programming"
                        // Vol. 2, 3rd ed., page 106
                        mt_status[mt_index] =
                            (INIT_SEED_F * (mt_status[mt_index - 1] ^ (mt_status[mt_index - 1] >> (sizeof(UIntType) * 8 - 2))) + mt_index) &
                            mask;
                    }
                }

                /**
                 * 初始化随机数种子
                 * @note 取值范围为 [first, last)
                 * @param [in] first 随机数种子散列值起始位置
                 * @param [in] last 随机数种子散列值结束位置
                 */
                template <class It>
                LIBATFRAME_UTILS_API_HEAD_ONLY void init_seed(It &first, It last) {
                    It begin = first;
                    for (mt_index = 0; mt_index < MAX_STATUS_N_SIZE && begin != last; ++mt_index, ++begin) {
                        mt_status[mt_index] = static_cast<UIntType>(*begin);
                    }
                    mt_index = MAX_STATUS_N_SIZE;

                    // fix up the state if it's all zeroes.
                    if ((mt_status[0] & (~static_cast<UIntType>(0) << SPB_IDX)) == 0) {
                        for (std::size_t i = 1; i < MAX_STATUS_N_SIZE; ++i) {
                            if (mt_status[i] != 0) {
                                return;
                            }
                            mt_status[0] = static_cast<UIntType>(1) << (sizeof(UIntType) * 8 - 1);
                        }
                    }
                }

                inline size_t block_size() const UTIL_CONFIG_NOEXCEPT { return sizeof(mt_status) + sizeof(mt_index); }

                inline bool dump(unsigned char *output, size_t size) const UTIL_CONFIG_NOEXCEPT {
                    if (NULL == output || size < block_size()) {
                        return false;
                    }

                    memcpy(output, mt_status, sizeof(mt_status));
                    memcpy(output + sizeof(mt_status), &mt_index, sizeof(mt_index));
                    return true;
                }

                inline bool load(const unsigned char *input, size_t size) UTIL_CONFIG_NOEXCEPT {
                    if (NULL == input || size < block_size()) {
                        return false;
                    }

                    memcpy(mt_status, input, sizeof(mt_status));
                    memcpy(&mt_index, input + sizeof(mt_status), sizeof(mt_index));
                    return true;
                }

                /**
                 * 产生一个随机数
                 * @return 产生的随机数
                 */
                result_type random() {
                    // 每MAX_STATUS_N_SIZE次随机数重新生成缓存
                    if (mt_index >= MAX_STATUS_N_SIZE) {
                        twist();
                    }

                    // 生成随机数
                    UIntType mt_ret = mt_status[mt_index];
                    ++mt_index;
                    mt_ret ^= ((mt_ret >> RD_U) & RD_D);
                    mt_ret ^= ((mt_ret << RD_S) & RD_B);
                    mt_ret ^= ((mt_ret << RD_T) & RD_C);
                    mt_ret ^= (mt_ret >> RD_L);

                    return mt_ret;
                }

                /**
                 * 产生一个随机数
                 * @return 产生的随机数
                 */
                result_type operator()() { return random(); }
            };
        } // namespace core
    }     // namespace random
} // namespace util


#endif /* RANDOMMTCORE_H_ */
