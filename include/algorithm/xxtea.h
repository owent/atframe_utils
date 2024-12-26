/**
 * @file xxtea.h
 * @brief XXTEA加密算法
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2016.08.09
 *
 * @see https://en.wikipedia.org/wiki/XXTEA
 * @history
 *
 *
 */

#ifndef UTIL_ALGORITHM_XXTEA_H
#define UTIL_ALGORITHM_XXTEA_H

#pragma once

#include <stddef.h>

#if defined(__FreeBSD__) && __FreeBSD__ < 5
/* FreeBSD 4 doesn't have stdint.h file */
#  include <inttypes.h>
#else
#  include <stdint.h>
#endif

#include <config/atframe_utils_build_feature.h>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
struct xxtea_key {
  uint32_t data[4];
};

ATFRAMEWORK_UTILS_API void xxtea_setup(xxtea_key *k, const unsigned char filled[4 * sizeof(uint32_t)]);

/**
 * @brief encrypt data use xxtea
 * @param key           xxtea key, should be initialized by xxtea_setup
 * @param buffer        buffer address, must padding to uint32_t
 * @param len           buffer size, must padding to uint32_t, can not be greater than 2^34
 */
ATFRAMEWORK_UTILS_API void xxtea_encrypt(const xxtea_key *key, void *buffer, size_t len);

/**
 * @brief encrypt data use xxtea
 * @param key           xxtea key, should be initialized by xxtea_setup
 * @param input         buffer holding the input data
 * @param ilen          length of the input data
 * @param output        buffer for the output data. Should be able to hold at
 *                      least ((ilen - 1) | 0x03) + 1. Cannot be the same buffer as
 *                      input!
 * @param olen          length of the output data, will be filled with the
 *                      actual number of bytes written.
 * @note if passed invalid parameter, olen will be set to 0
 */
ATFRAMEWORK_UTILS_API void xxtea_encrypt(const xxtea_key *key, const void *input, size_t ilen, void *output,
                                         size_t *olen);

/**
 * @brief decrypt data use xxtea
 * @param key           xxtea key, should be initialized by xxtea_setup
 * @param buffer        buffer address, must padding to uint32_t
 * @param len           buffer size, must padding to uint32_t, can not be greater than 2^34
 */
ATFRAMEWORK_UTILS_API void xxtea_decrypt(const xxtea_key *key, void *buffer, size_t len);

/**
 * @brief decrypt data use xxtea
 * @param key           xxtea key, should be initialized by xxtea_setup
 * @param input         buffer holding the input data
 * @param ilen          length of the input data
 * @param output        buffer for the output data. Should be able to hold at
 *                      least ((ilen - 1) | 0x03) + 1. Cannot be the same buffer as
 *                      input!
 * @param olen          length of the output data, will be filled with the
 *                      actual number of bytes written.
 * @note if passed invalid parameter, olen will be set to 0
 */
ATFRAMEWORK_UTILS_API void xxtea_decrypt(const xxtea_key *key, const void *input, size_t ilen, void *output,
                                         size_t *olen);
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif