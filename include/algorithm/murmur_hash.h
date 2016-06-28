/**
 * @file murmur_hash.h
 * @brief MurmurHash算法
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2016.06.28
 *
 * @history
 *
 *
 */

#ifndef _UTIL_HASH_MURMUR_HASH_H_
#define _UTIL_HASH_MURMUR_HASH_H_

#include <stddef.h>
#include <stdint.h>


namespace util {
    namespace hash {
        uint32_t murmur_hash2(const void *key, int len, uint32_t seed);
        uint64_t murmur_hash2_64a(const void *key, int len, uint64_t seed);
        uint64_t murmur_hash2_64b(const void *key, int len, uint64_t seed);

        uint32_t murmur_hash3_x86_32(const void *key, int len, uint32_t seed);
        void murmur_hash3_x86_128(const void *key, const int len, uint32_t seed, uint32_t out[4]);
        void murmur_hash3_x64_128(const void *key, const int len, const uint32_t seed, uint64_t out[2]);
    }
}