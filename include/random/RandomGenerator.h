/**
 * @file RandomGenerator.h
 * @brief 伪随机数生成器
 * 
 * Licensed under the MIT licenses.
 * @version 1.0
 * @author OWenT
 * @date 2013年8月5日
 *
 * @history
 *
 */

#ifndef _UTIL_RANDOM_GENERATOR_H_
#define _UTIL_RANDOM_GENERATOR_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

#include <stdint.h>
#include <ctime>

#include <cstddef>
#include <limits>
#include <numeric>

#include "RandomMTCore.h"
#include "RandomXorCombineCore.h"

namespace util
{
    namespace random
    {
        /**
         * 随机数包装类，用以提供高级功能
         */
        template<typename CoreType>
        class RandomManagerWrapper
        {
        public:
            typedef CoreType core_type;
            typedef typename core_type::result_type result_type;

        private:
            core_type m_stCore;

        public:
            RandomManagerWrapper(){}
            RandomManagerWrapper(result_type tSeed): m_stCore(tSeed)
            {
            }
            
            inline core_type& GetCore() { return m_stCore; }
            inline const core_type& GetCore() const { return m_stCore; }

            /**
             * 初始化随机数种子
             * @param [in] tSeed 随机数种子
             */
            void InitSeed(result_type tSeed)
            {
                m_stCore.InitSeed(tSeed);
            }

            /**
             * 初始化随机数种子
             * @note 取值范围为 [first, last)
             * @param [in] first 随机数种子散列值起始位置
             * @param [in] last 随机数种子散列值结束位置
             */
            template<class It>
            void InitSeed(It& first, It last)
            {
                m_stCore.InitSeed(first, last);
            }

            /**
             * 产生一个随机数
             * @return 产生的随机数
             */
            result_type Random()
            {
                return m_stCore();
            }

            /**
             * 产生一个随机数
             * @return 产生的随机数
             */
            result_type operator()()
            {
                return Random();
            }

            /**
             * 产生一个随机数
             * @param [in] lowest 下限
             * @param [in] highest 上限(必须大于lowest)
             * @note 取值范围 [lowest, highest)
             * @return 产生的随机数
             */
            template<typename ResaultType>
            ResaultType RandomBetween(ResaultType lowest, ResaultType highest)
            {
                result_type res = (*this)();
                return static_cast<ResaultType>(res % static_cast<result_type>(highest - lowest)) + lowest;
            }
        };


        // ============== 随机数生成器 - 梅森旋转算法(STL 标准算法) ==============
        typedef RandomManagerWrapper<core::MersenneTwister<
            uint32_t,
            351, 175, 19,
            0xccab8ee7, 11,
            0xffffffff, 7,
            0x31b6ab00, 15,
            0xffe50000, 17,
            1812433253
        > > MT11213B;

        typedef RandomManagerWrapper<core::MersenneTwister<
            uint32_t,
            624, 397, 31,
            0x9908b0df, 11,
            0xffffffff, 7,
            0x9d2c5680, 15,
            0xefc60000, 18,
            1812433253
        > > MT19937;

        typedef RandomManagerWrapper<core::MersenneTwister<
            uint64_t,
            312, 156, 31,
            0xb5026f5aa96619e9ULL, 29,
            0x5555555555555555ULL, 17,
            0x71d67fffeda60000ULL, 37,
            0xfff7eee000000000ULL, 43,
            6364136223846793005ULL
        > > MT19937_64;

        // ============== 随机数生成器 - taus 算法(比梅森旋转算法消耗更少的内存，但是循环节更小) ==============
        typedef RandomManagerWrapper<core::XorCombineEngine<
            core::XorCombineEngine<
                core::LinearFeedbackShiftEngine<uint32_t, 32, 31, 13, 12>, 0,
                core::LinearFeedbackShiftEngine<uint32_t, 32, 29, 2, 4>, 0>, 0,
            core::LinearFeedbackShiftEngine<uint32_t, 32, 28, 3, 17>, 0>
        > TAUS88;
    }
}

#endif /* _UTIL_RANDOM_GENERATOR_H_ */
