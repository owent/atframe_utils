/**
 * @file RandomXorCombineCore.h
 * @brief 伪随机数生成器  - 梅森旋转算法
 *
 *
 * Licensed under the MIT licenses.
 * @version 1.0
 * @author OWenT
 * @date 2013年8月6日
 *
 * @history
 *
 */
 

#ifndef _UTIL_RANDOM_RANDOMXORCOMBINECORE_H_
#define _UTIL_RANDOM_RANDOMXORCOMBINECORE_H_

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
# pragma once
#endif

namespace util
{
  namespace random
	{
		namespace core
			{
				template<class UIntType, int w, int k, int q, int s>
				class LinearFeedbackShiftEngine
				{
				public:
					typedef UIntType result_type;

				private:
					/// \endcond
					result_type m_uSeed;

					/// \cond show_private
					static result_type wordmask()
					{
						return (~( (~(static_cast<uint64_t>(0))) << (sizeof(result_type) * 8) ));
					}
				public:
					LinearFeedbackShiftEngine(){}
					LinearFeedbackShiftEngine(result_type tSeed)
					{
						InitSeed(tSeed);
					}

					void InitSeed(result_type tSeed)
					{
						m_uSeed = tSeed & wordmask();
						if(m_uSeed < (1 << (w-k)))
						{
							m_uSeed += 1 << (w-k);
						}
					}

					result_type Random()
					{
						const result_type b = (((m_uSeed << q) ^ m_uSeed) & wordmask()) >> (k - s);
						const result_type mask = (wordmask() << (w - k)) & wordmask();
						m_uSeed = ((m_uSeed & mask) << s) ^ b;
						return m_uSeed;
					}

					result_type operator()()
					{
						return Random();
					}
				};

				template<class URNG1, int s1, class URNG2, int s2>
				class XorCombineEngine
				{
				public:
					typedef URNG1 base_left_type;
					typedef URNG2 base_right_type;
					typedef typename base_left_type::result_type result_type;

				private:
					base_left_type m_stRngLeft;
					base_right_type m_stRngRight;

				public:
					XorCombineEngine(){}
					XorCombineEngine(result_type tSeed)
					{
						InitSeed(tSeed);
					}

					void InitSeed(result_type tSeed)
					{
						m_stRngLeft.InitSeed(tSeed);
						m_stRngRight.InitSeed(tSeed);
					}

					template<class It>
					void InitSeed(It& first, It last)
					{
						It begin = first;
						if (begin != last)
						{
							m_stRngLeft.InitSeed(*begin);
							++ begin;
						}

						if (begin != last)
						{
							m_stRngRight.InitSeed(*begin);
							++ begin;
						}
					}

					/** Returns the first base generator. */
					const base_left_type& GetLeftBase() const { return m_stRngLeft; }

					/** Returns the second base generator. */
					const base_right_type& GetRightBase() const { return m_stRngRight; }

					result_type Random()
					{
						return (m_stRngLeft() << s1) ^ (m_stRngRight() << s2);
					}

					result_type operator()()
					{
						return Random();
					}
				};

			}
	}
}


#endif /* _UTIL_RANDOM_RANDOMXORCOMBINECORE_H_ */
