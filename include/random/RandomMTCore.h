/**
 * @file RandomMTCore.h
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
 

#ifndef _UTIL_RANDOM_RANDOMMTCORE_H_
#define _UTIL_RANDOM_RANDOMMTCORE_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

namespace util
{
    namespace random
    {
        namespace core
        {
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
            template<typename UIntType,
                std::size_t MAX_STATUS_N_SIZE, std::size_t RD_M, std::size_t SPB_IDX,
                UIntType RD_A, std::size_t RD_U, UIntType RD_D,
                std::size_t RD_S, UIntType RD_B, std::size_t RD_T,
                UIntType RD_C, std::size_t RD_L, UIntType INIT_SEED_F>
            class MersenneTwister
            {
            public:
                typedef UIntType result_type;

            private:
                UIntType m_arrStatus[MAX_STATUS_N_SIZE];
                std::size_t m_uIndex;


                void twist()
                {
                    const UIntType uUpperMask = (~static_cast<UIntType>(0)) << SPB_IDX;
                    const UIntType uLowerMask = ~uUpperMask;

                    const std::size_t uUnrollFactor = 6;
                    const std::size_t uUnrollExtra1 = (MAX_STATUS_N_SIZE - RD_M) % uUnrollFactor;
                    const std::size_t uUnrollExtra2 = (RD_M - 1) % uUnrollFactor;

                    // 这里为了减少取模操作把一个循环拆成了四个
                    for(std::size_t i = 0; i < MAX_STATUS_N_SIZE - RD_M - uUnrollExtra1; ++ i)
                    {
                        UIntType y = (m_arrStatus[i] & uUpperMask) | (m_arrStatus[i + 1] & uLowerMask);
                        m_arrStatus[i] = m_arrStatus[i + RD_M] ^ (y >> 1) ^ ((m_arrStatus[i + 1] & 1 ) * RD_A);
                    }
                    for(std::size_t i = MAX_STATUS_N_SIZE - RD_M - uUnrollExtra1; i < MAX_STATUS_N_SIZE - RD_M; ++ i)
                    {
                        UIntType y = (m_arrStatus[i] & uUpperMask) | (m_arrStatus[i + 1] & uLowerMask);
                        m_arrStatus[i] = m_arrStatus[i + RD_M] ^ (y >> 1) ^ ((m_arrStatus[i + 1] & 1) * RD_A);
                    }
                    for(std::size_t i = MAX_STATUS_N_SIZE - RD_M; i < MAX_STATUS_N_SIZE - 1 - uUnrollExtra2; ++ i)
                    {
                        UIntType y = (m_arrStatus[i] & uUpperMask) | (m_arrStatus[i + 1] & uLowerMask);
                        m_arrStatus[i] = m_arrStatus[i - (MAX_STATUS_N_SIZE - RD_M)] ^ (y >> 1) ^ ((m_arrStatus[i + 1] & 1) * RD_A);
                    }
                    for(std::size_t i = MAX_STATUS_N_SIZE - 1 - uUnrollExtra2; i < MAX_STATUS_N_SIZE - 1; ++ i)
                    {
                        UIntType y = (m_arrStatus[i] & uUpperMask) | (m_arrStatus[i + 1] & uLowerMask);
                        m_arrStatus[i] = m_arrStatus[i - (MAX_STATUS_N_SIZE - RD_M)] ^ (y >> 1) ^ ((m_arrStatus[i + 1] & 1) * RD_A);
                    }
                    // last iteration
                    UIntType y = (m_arrStatus[MAX_STATUS_N_SIZE - 1] & uUpperMask) | (m_arrStatus[0] & uLowerMask);
                    m_arrStatus[MAX_STATUS_N_SIZE - 1] = m_arrStatus[RD_M - 1] ^ (y >> 1) ^ ((m_arrStatus[0] & 1) * RD_A);
                    m_uIndex = 0;
                }

            public:
                MersenneTwister()
                {
                }

                MersenneTwister(UIntType tSeed)
                {
                    InitSeed(tSeed);
                }

                /**
                 * 初始化随机数种子
                 * @param [in] tSeed 随机数种子
                 */
                void InitSeed(UIntType tSeed)
                {
                    // New seeding algorithm from
                    // http://www.math.sci.hiroshima-u.ac.jp/~m-mat/MT/MT2002/emt19937ar.html
                    // In the previous versions, MSBs of the seed affected only MSBs of the
                    // state m_arrStatus[].

                    // fuck vc
#ifdef max
#undef max
#endif
                    const UIntType mask = std::numeric_limits<UIntType>::max();

                    m_arrStatus[0] = tSeed & mask;
                    for (m_uIndex = 1; m_uIndex < MAX_STATUS_N_SIZE; ++ m_uIndex)
                    {
                        // See Knuth "The Art of Computer Programming"
                        // Vol. 2, 3rd ed., page 106
                        m_arrStatus[m_uIndex] = (INIT_SEED_F * (m_arrStatus[m_uIndex - 1] ^ (m_arrStatus[m_uIndex - 1] >> (sizeof(UIntType) * 8 - 2))) + m_uIndex) & mask;
                    }
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
                    It begin = first;
                    for(m_uIndex = 0; m_uIndex < MAX_STATUS_N_SIZE && begin != last; ++ m_uIndex, ++ begin)
                    {
                        m_arrStatus[m_uIndex] = static_cast<UIntType>(*begin);
                    }
                    m_uIndex = MAX_STATUS_N_SIZE;

                    // fix up the state if it's all zeroes.
                    if((m_arrStatus[0] & (~static_cast<UIntType>(0) << SPB_IDX)) == 0)
                    {
                        for(std::size_t i = 1; i < MAX_STATUS_N_SIZE; ++ i)
                        {
                            if(m_arrStatus[i] != 0)
                            {
                                return;
                            }
                            m_arrStatus[0] = static_cast<UIntType>(1) << (sizeof(UIntType) * 8 - 1);
                        }
                    }
                }

                /**
                 * 产生一个随机数
                 * @return 产生的随机数
                 */
                result_type Radom()
                {
                    // 每MAX_STATUS_N_SIZE次随机数重新生成缓存
                    if (m_uIndex >= MAX_STATUS_N_SIZE)
                    {
                        twist();
                    }

                    // 生成随机数
                    UIntType tRet = m_arrStatus[m_uIndex];
                    ++ m_uIndex;
                    tRet ^= ((tRet >> RD_U) & RD_D);
                    tRet ^= ((tRet << RD_S) & RD_B);
                    tRet ^= ((tRet << RD_T) & RD_C);
                    tRet ^= (tRet >> RD_L);

                    return tRet;
                }

                /**
                 * 产生一个随机数
                 * @return 产生的随机数
                 */
                result_type operator()()
                {
                    return Radom();
                }
            };
        }
    }
}


#endif /* RANDOMMTCORE_H_ */
