/**
 * @file crc.h
 * @brief mapping方法实现的crc16/crc32/crc64算法
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2017.11.30
 *
 * @history
 *
 *
 */

#ifndef UTIL_ALGORITHM_CRC_H
#define UTIL_ALGORITHM_CRC_H

#pragma once

#include <stddef.h>

#if defined(__FreeBSD__) && __FreeBSD__ < 5
/* FreeBSD 4 doesn't have stdint.h file */
#include <inttypes.h>
#else
#include <stdint.h>
#endif

#include <config/atframe_utils_build_feature.h>

namespace util {
    /**
     * @brief          Calculate crc32
     *
     * @param init_val initialize value
     * @param s        buffer address
     * @param l        buffer length
     *
     * @return         crc32 result
     */
    LIBATFRAME_UTILS_API_C(uint16_t) crc16(const unsigned char *s, size_t l, uint16_t init_val = 0);

    /**
     * @brief          Calculate crc32
     *
     * @param init_val initialize value
     * @param s        buffer address
     * @param l        buffer length
     *
     * @return         crc32 result
     */
    LIBATFRAME_UTILS_API_C(uint32_t) crc32(const unsigned char *s, size_t l, uint32_t init_val = 0);

    /**
     * @brief          Calculate crc32
     *
     * @param init_val initialize value
     * @param s        buffer address
     * @param l        buffer length
     *
     * @return         crc32 result
     */
    LIBATFRAME_UTILS_API_C(uint64_t) crc64(const unsigned char *s, size_t l, uint64_t init_val = 0);
} // namespace util

#endif