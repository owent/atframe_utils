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
 *
 */


#ifndef UTIL_RANDOM_XOSHIRO_CORE_H
#define UTIL_RANDOM_XOSHIRO_CORE_H

#pragma once

#include <config/atframe_utils_build_feature.h>

namespace util {
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
            class LIBATFRAME_UTILS_API_HEAD_ONLY xoshinro_engine {
            public:
                typedef UIntType    result_type;
                typedef result_type seed_type[4];

            private:
                /// \endcond
                seed_type xoshinro_seed_;

                static inline result_type rotl(const result_type x, int k) {
                    return (x << k) | (x >> ((sizeof(result_type) * 8) - static_cast<result_type>(k)));
                }

                template <class, bool>
                struct LIBATFRAME_UTILS_API_HEAD_ONLY next_init;

                template <class T>
                struct LIBATFRAME_UTILS_API_HEAD_ONLY next_init<T, true> {
                    static inline result_type call(seed_type &s) { return s[0] + s[3]; }
                };

                template <class T>
                struct LIBATFRAME_UTILS_API_HEAD_ONLY next_init<T, false> {
                    static inline result_type call(seed_type &s) { return rotl(s[iidx] * 5, 7) * 9; }
                };

            protected:
                result_type next() {
                    const result_type ret = next_init<UIntType, is_plus>::call(xoshinro_seed_);
                    const result_type t   = xoshinro_seed_[1] << n1;

                    xoshinro_seed_[2] ^= xoshinro_seed_[0];
                    xoshinro_seed_[3] ^= xoshinro_seed_[1];
                    xoshinro_seed_[1] ^= xoshinro_seed_[2];
                    xoshinro_seed_[0] ^= xoshinro_seed_[3];

                    xoshinro_seed_[2] ^= t;
                    xoshinro_seed_[3] = rotl(xoshinro_seed_[3], n2);

                    return ret;
                }

                void jump(const seed_type &JUMP) {
                    result_type s0 = 0;
                    result_type s1 = 0;
                    result_type s2 = 0;
                    result_type s3 = 0;
                    for (size_t i = 0; i < sizeof(JUMP) / sizeof(JUMP[0]); i++) {
                        for (size_t b = 0; b < sizeof(result_type) * 8; b++) {
                            if (JUMP[i] & result_type(1) << b) {
                                s0 ^= xoshinro_seed_[0];
                                s1 ^= xoshinro_seed_[1];
                                s2 ^= xoshinro_seed_[2];
                                s3 ^= xoshinro_seed_[3];
                            }
                            next();
                        }
                    }

                    xoshinro_seed_[0] = s0;
                    xoshinro_seed_[1] = s1;
                    xoshinro_seed_[2] = s2;
                    xoshinro_seed_[3] = s3;
                }

            public:
                xoshinro_engine() {
                    xoshinro_seed_[0] = 0;
                    xoshinro_seed_[1] = 0;
                    xoshinro_seed_[2] = 0;
                    xoshinro_seed_[3] = 0;
                }
                xoshinro_engine(result_type s) {
                    xoshinro_seed_[0] = 0;
                    xoshinro_seed_[1] = 0;
                    xoshinro_seed_[2] = 0;
                    xoshinro_seed_[3] = 0;
                    init_seed(s);
                }

                void init_seed(result_type s) {
                    xoshinro_seed_[0] = s;
                    xoshinro_seed_[1] = 0xff;
                    xoshinro_seed_[2] = 0;
                    xoshinro_seed_[3] = 0;

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
                            xoshinro_seed_[i] = *begin;
                            ++begin;
                        } else {
                            xoshinro_seed_[i] = 0;
                        }
                    }

                    // just like in lua 5.4
                    for (int i = 0; i < 16; ++i) {
                        next();
                    }
                }

                result_type random() { return next(); }

                result_type operator()() { return random(); }

                inline const seed_type &get_seed() const { return xoshinro_seed_; }
            };

            template <bool is_plus>
            class LIBATFRAME_UTILS_API_HEAD_ONLY xoshinro_engine_128 : public xoshinro_engine<uint32_t, is_plus, 0, 9, 11> {
            public:
                typedef xoshinro_engine<uint32_t, is_plus, 0, 9, 11> base_type;
                typedef typename base_type::result_type              result_type;
                typedef typename base_type::seed_type                seed_type;

            public:
                xoshinro_engine_128() {}
                xoshinro_engine_128(result_type s) : base_type(s) {}

                using base_type::jump;

                /**
                 * @brief just like call next() for 2^64 times
                 */
                void jump() {
                    static const result_type jump_params[4] = {0x8764000b, 0xf542d2d3, 0x6fa035c3, 0x77f2db5b};
                    jump(jump_params);
                }
            };

            template <bool is_plus>
            class LIBATFRAME_UTILS_API_HEAD_ONLY xoshinro_engine_256 : public xoshinro_engine<uint64_t, is_plus, 1, 17, 45> {
            public:
                typedef xoshinro_engine<uint64_t, is_plus, 1, 17, 45> base_type;
                typedef typename base_type::result_type               result_type;
                typedef typename base_type::seed_type                 seed_type;

            public:
                xoshinro_engine_256() {}
                xoshinro_engine_256(result_type s) : base_type(s) {}

                using base_type::jump;

                /**
                 * @brief just like call next() for 2^128 times
                 */
                void jump() {
                    static const result_type jump_params[4] = {0x180ec6d33cfd0aba, 0xd5a61266f0c9392c, 0xa9582618e03fc9aa,
                                                               0x39abdc4529b1661c};
                    jump(jump_params);
                }

                /**
                 * @brief just like call next() for 2^192 times
                 */
                void long_jump() {
                    static const result_type jump_params[4] = {0x76e15d3efefdcbbf, 0xc5004e441c522fb3, 0x77710069854ee241,
                                                               0x39109bb02acbe635};
                    jump(jump_params);
                }
            };
        } // namespace core
    }     // namespace random
} // namespace util


#endif /* UTIL_RANDOM_XOSHIRO_CORE_H */
