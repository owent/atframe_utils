/**
 * @file Hash.h
 * @brief 常用Hash算法
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2013.05.07
 *
 * @history
 *
 *
 */

#ifndef _UTIL_HASH_HASH_H_
#define _UTIL_HASH_HASH_H_

#include <stddef.h>
#include <limits>
#include <cstring>
#include <stdint.h>

namespace util
{
    namespace hash
    {
        namespace core
        {
            template<typename Ty, size_t s>
            struct fnv_magic_prime_number
            {
                static const Ty value = 0x01000193U;
            };

            template<typename Ty>
            struct fnv_magic_prime_number<Ty, 8>
            {
                static const Ty value = 0x100000001b3ULL;
            };

            template<typename Ty, size_t s>
            struct fnv_magic_offset_basis
            {
                static const Ty value = 0x811C9DC5U;

                static Ty fix(Ty hval)
                {
                    return hval;
                }
            };

            template<typename Ty>
            struct fnv_magic_offset_basis<Ty, 8>
            {
                static const Ty value = 0xCBF29CE484222325ULL;

                static Ty fix(Ty hval)
                {
                    return hval ^ (hval>>32);
                }
            };

            /**
            * @brief fnv-1 算法 （二进制）
            * @param [in] buf 二进制数据
            * @param [in] len 二进制长度
            * @param [in] hval 初始值
            * @return 返回的指定类型的值
            */
            template<typename Ty>
            Ty fnv_n_buf(const void *buf, size_t len, Ty hval = fnv_magic_offset_basis<Ty, sizeof(Ty)>::value)
            {
                unsigned char *bp = (unsigned char *) buf;
                unsigned char *be = bp + len;
                Ty mn = fnv_magic_prime_number<Ty, sizeof(Ty)>::value;

                while (bp < be)
                {
                    hval *= mn;
                    hval ^= (Ty) *bp++;
                }

                return fnv_magic_offset_basis<Ty, sizeof(Ty)>::fix(hval);
            }

            /**
            * @brief fnv-1a 算法 （二进制）
            * @param [in] buf 二进制数据
            * @param [in] len 二进制长度
            * @param [in] hval 初始值
            * @return 返回的指定类型的值
            */
            template<typename Ty>
            Ty fnv_n_buf_a(const void *buf, size_t len, Ty hval = fnv_magic_offset_basis<Ty, sizeof(Ty)>::value)
            {
                unsigned char *bp = (unsigned char *) buf;
                unsigned char *be = bp + len;
                Ty mn = fnv_magic_prime_number<Ty, sizeof(Ty)>::value;

                while (bp < be)
                {
                    hval ^= (Ty) *bp++;
                    hval *= mn;
                }

                return fnv_magic_offset_basis<Ty, sizeof(Ty)>::fix(hval);
            }
        }

        /**
        * fnv-1算法hash函数 （二进制）
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashFNV1(const void* bin, size_t len, THVal hval = core::fnv_magic_offset_basis<THVal, sizeof(THVal)>::value)
        {
            return core::fnv_n_buf(bin, len, hval);
        }


        /**
        * fnv-1a算法hash函数 （二进制）
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashFNV1A(const void* bin, size_t len, THVal hval = core::fnv_magic_offset_basis<THVal, sizeof(THVal)>::value)
        {
            return core::fnv_n_buf_a(bin, len, hval);
        }

        /**
        * SDBM Hash函数
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashSDBM(const void* bin, size_t len, THVal hval = 0)
        {
            unsigned char* strBuff = (unsigned char*) bin;
            size_t iIndex = 0;
            while (iIndex < len)
            {
                // equivalent to: hval = 65599 * hval + strBuff[iIndex ++]);
                hval = strBuff[iIndex++] + (hval << 6) + (hval << 16) - hval;
            }

            return hval;
        }

        /**
        * RS Hash函数
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashRS(const void* bin, size_t len, THVal hval = 0)
        {
            unsigned int b = 378551;
            unsigned int a = 63689;
            size_t iIndex = 0;
            unsigned char* strBuff = (unsigned char*) bin;

            while (iIndex < len)
            {
                hval = hval * a + strBuff[iIndex++];
                a *= b;
            }

            return hval;
        }

        /**
        * JS Hash函数
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashJS(const void* bin, size_t len, THVal hval = 1315423911)
        {
            size_t iIndex = 0;
            unsigned char* strBuff = (unsigned char*) bin;

            while (iIndex < len)
            {
                hval ^= ((hval << 5) + strBuff[iIndex++] + (hval >> 2));
            }

            return hval;
        }

        /**
        * P. J. Weinberger Hash函数
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashPJW(const void* bin, size_t len, THVal hval = 0)
        {
            size_t iIndex = 0;
            unsigned char* strBuff = (unsigned char*) bin;

            THVal BitsInUnignedInt = (THVal) (sizeof(THVal) * 8);
            THVal ThreeQuarters = (THVal) ((BitsInUnignedInt * 3) / 4);
            THVal OneEighth = (THVal) (BitsInUnignedInt / 8);
            THVal HighBits = (THVal) (-1) << (BitsInUnignedInt - OneEighth);
            THVal test = 0;
            while (iIndex < len)
            {
                hval = (hval << OneEighth) + strBuff[iIndex++];
                if ((test = hval & HighBits) != 0)
                {
                    hval = ((hval ^ (test >> ThreeQuarters)) & (~HighBits));
                }
            }

            return hval;
        }

        /**
        * ELF Hash函数
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashELF(const void* bin, size_t len, THVal hval = 0)
        {
            size_t iIndex = 0;
            unsigned char* strBuff = (unsigned char*) bin;

            THVal x = 0;
            while (iIndex < len)
            {
                hval = (hval << 4) + strBuff[iIndex++];
                if ((x = hval & 0xF0000000L) != 0)
                {
                    hval ^= (x >> 24);
                    hval &= ~x;
                }
            }

            return hval;
        }

        /**
        * BKDR Hash函数
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashBKDR(const void* bin, size_t len, THVal hval = 0)
        {
            size_t iIndex = 0;
            unsigned char* strBuff = (unsigned char*) bin;

            THVal seed = 131; // 31 131 1313 13131 131313 etc..
            while (iIndex < len)
            {
                hval = hval * seed + strBuff[iIndex++];
            }

            return hval;
        }

        /**
        * DJB Hash函数
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashDJB(const void* bin, size_t len, THVal hval = 5381)
        {
            size_t iIndex = 0;
            unsigned char* strBuff = (unsigned char*) bin;

            while (iIndex < len)
            {
                hval += (hval << 5) + strBuff[iIndex++];
            }

            return hval;
        }

        /**
        * AP Hash函数
        * @param [in] bin 二进制数据
        * @param [in] len 二进制长度
        * @param [in] hval 初始散列值
        * @return 散列值
        */
        template<typename THVal>
        THVal HashAP(const void* bin, size_t len, THVal hval = 0)
        {
            size_t iIndex = 0;
            unsigned char* strBuff = (unsigned char*) bin;

            for (int i = 0; iIndex < len; i++)
            {
                if ((i & 1) == 0)
                {
                    hval ^= ((hval << 7) ^ strBuff[iIndex++] ^ (hval >> 3));
                }
                else
                {
                    hval ^= (~((hval << 11) ^ strBuff[iIndex++] ^ (hval >> 5)));
                }
            }

            return hval;
        }
    }
}

#endif /* HASH_H_ */
