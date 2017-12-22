/**
 * @file base64.h
 * @brief base64算法
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2017.11.17
 *
 * @see https://en.wikipedia.org/wiki/Base64
 * @history
 *
 *
 */

#ifndef UTIL_ALGORITHM_BASE64_H
#define UTIL_ALGORITHM_BASE64_H

#pragma once

#include <stddef.h>

#if defined(__FreeBSD__) && __FreeBSD__ < 5
/* FreeBSD 4 doesn't have stdint.h file */
#include <inttypes.h>
#else
#include <stdint.h>
#endif

#include <string>

namespace util {
    /**
     * @brief          Encode a buffer into base64 format
     *
     * @param dst      destination buffer
     * @param dlen     size of the destination buffer, pass 0 to get output length into olen.
     * @param olen     number of bytes written, not include the last \0 for string terminate
     * @param src      source buffer
     * @param slen     amount of data to be encoded
     *
     * @return         0 if successful, or -1 if dlen is too small.
     *                 *olen is always updated to reflect the amount
     *                 of data that has (or would have) been written.
     *                 If that length cannot be represented, then no data is
     *                 written to the buffer and *olen is set to the maximum
     *                 length representable as a size_t.
     *
     * @note           Call this function with dlen = 0 to obtain the
     *                 required buffer size in *olen
     */
    int base64_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen);

    int base64_encode(std::string &dst, const unsigned char *src, size_t slen);
    int base64_encode(std::string &dst, const std::string &in);

    /**
     * @brief          Decode a base64-formatted buffer
     *
     * @param dst      destination buffer (can be NULL for checking size)
     * @param dlen     size of the destination buffer
     * @param olen     number of bytes written
     * @param src      source buffer
     * @param slen     amount of data to be decoded
     *
     * @return         0 if successful, -1 for too small dlen, or -2  if the input data is
     *                 not correct. *olen is always updated to reflect the amount
     *                 of data that has (or would have) been written.
     *
     * @note           Call this function with *dst = NULL or dlen = 0 to obtain
     *                 the required buffer size in *olen
     */
    int base64_decode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src, size_t slen);

    int base64_decode(std::string &dst, const unsigned char *src, size_t slen);
    int base64_decode(std::string &dst, const std::string &in);
} // namespace util

#endif