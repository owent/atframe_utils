/**
 * @file random_xor_combine_core.h
 * @brief 伪随机数生成器  - 线性回归核心
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
#pragma once
#endif

namespace util {
    namespace random {
        namespace core {
            template <class UIntType, int w, int k, int q, int s>
            class linear_feedback_shift_engine {
            public:
                typedef UIntType result_type;

            private:
                /// \endcond
                result_type xor_c_seed_;

                /// \cond show_private
                static result_type wordmask() { return (~((~(static_cast<uint64_t>(0))) << (sizeof(result_type) * 8))); }

            public:
                linear_feedback_shift_engine() {}
                linear_feedback_shift_engine(result_type xor_c_seed) { init_seed(xor_c_seed); }

                void init_seed(result_type xor_c_seed) {
                    xor_c_seed_ = xor_c_seed & wordmask();
                    if (xor_c_seed_ < (1 << (w - k))) {
                        xor_c_seed_ += 1 << (w - k);
                    }
                }

                result_type random() {
                    const result_type b = (((xor_c_seed_ << q) ^ xor_c_seed_) & wordmask()) >> (k - s);
                    const result_type mask = (wordmask() << (w - k)) & wordmask();
                    xor_c_seed_ = ((xor_c_seed_ & mask) << s) ^ b;
                    return xor_c_seed_;
                }

                result_type operator()() { return random(); }
            };

            template <class URNG1, int s1, class URNG2, int s2>
            class xor_combine_engine {
            public:
                typedef URNG1 base_left_type;
                typedef URNG2 base_right_type;
                typedef typename base_left_type::result_type result_type;

            private:
                base_left_type xor_c_rng_left_;
                base_right_type xor_c_rng_right_;

            public:
                xor_combine_engine() {}
                xor_combine_engine(result_type xor_c_seed) { init_seed(xor_c_seed); }

                void init_seed(result_type xor_c_seed) {
                    xor_c_rng_left_.init_seed(xor_c_seed);
                    xor_c_rng_right_.init_seed(xor_c_seed);
                }

                template <class It>
                void init_seed(It &first, It last) {
                    It begin = first;
                    if (begin != last) {
                        xor_c_rng_left_.init_seed(*begin);
                        ++begin;
                    }

                    if (begin != last) {
                        xor_c_rng_right_.init_seed(*begin);
                        ++begin;
                    }
                }

                /** Returns the first base generator. */
                const base_left_type &GetLeftBase() const { return xor_c_rng_left_; }

                /** Returns the second base generator. */
                const base_right_type &GetRightBase() const { return xor_c_rng_right_; }

                result_type random() { return (xor_c_rng_left_() << s1) ^ (xor_c_rng_right_() << s2); }

                result_type operator()() { return random(); }
            };
        }
    }
}


#endif /* _UTIL_RANDOM_RANDOMXORCOMBINECORE_H_ */
