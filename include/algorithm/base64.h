// Copyright 2022 atframework
// @file base64.h
// @brief base64算法
// Licensed under the MIT licenses.
//
// @version 1.0
// @author OWenT
// @date 2017.11.17
//
// @see https://en.wikipedia.org/wiki/Base64

#pragma once

#include <stddef.h>

#if defined(__FreeBSD__) && __FreeBSD__ < 5
/* FreeBSD 4 doesn't have stdint.h file */
#  include <inttypes.h>
#else
#  include <stdint.h>
#endif

#include <cstring>
#include <string>

#include <config/atframe_utils_build_feature.h>

LIBATFRAME_UTILS_NAMESPACE_BEGIN

// @see https://en.wikipedia.org/wiki/Base64
struct base64_mode_t {
  enum type {
    // RFC 1421, RFC 2045, [RFC 3548](https://en.wikipedia.org/wiki/Base64#RFC_3548)
    // [RFC 4648](https://en.wikipedia.org/wiki/Base64#RFC_4648)
    EN_BMT_STANDARD = 0,

    // RFC 1642
    EN_BMT_UTF7 = 1,

    // RFC 3501(https://tools.ietf.org/html/rfc3501)
    EN_BMT_IMAP_MAILBOX_NAME = 2,

    // [RFC 4648](https://en.wikipedia.org/wiki/Base64#RFC_4648)
    EN_BMT_URL_FILENAME_SAFE = 3,
  };
};

/**
 * @brief          Encode a buffer into base64 format
 *
 * @param dst      destination buffer
 * @param dlen     size of the destination buffer, pass 0 to get output length into olen.
 * @param olen     number of bytes written, not include the last \0 for string terminate
 * @param src      source buffer
 * @param slen     amount of data to be encoded
 * @param mode     base64 mode @see base64_mode_t
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
LIBATFRAME_UTILS_API int base64_encode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src,
                                       size_t slen, base64_mode_t::type mode = base64_mode_t::EN_BMT_STANDARD);

/**
 * @brief          Encode a buffer into base64 format
 *
 * @param dst      destination buffer
 * @param src      source buffer
 * @param slen     amount of data to be encoded
 * @param mode     base64 mode @see base64_mode_t
 *
 * @return         0 if successful
 */
LIBATFRAME_UTILS_API int base64_encode(std::string &dst, const unsigned char *src, size_t slen,
                                       base64_mode_t::type mode = base64_mode_t::EN_BMT_STANDARD);

/**
 * @brief          Encode a buffer into base64 format
 *
 * @param dst      destination buffer
 * @param in       source buffer
 * @param mode     base64 mode @see base64_mode_t
 *
 * @return         0 if successful
 */
LIBATFRAME_UTILS_API int base64_encode(std::string &dst, const std::string &in,
                                       base64_mode_t::type mode = base64_mode_t::EN_BMT_STANDARD);

/**
 * @brief          Decode a base64-formatted buffer, support no padding
 *
 * @param dst      destination buffer (can be nullptr for checking size)
 * @param dlen     size of the destination buffer
 * @param olen     number of bytes written
 * @param src      source buffer
 * @param slen     amount of data to be decoded
 * @param mode     base64 mode @see base64_mode_t
 *
 * @return         0 if successful, -1 for too small dlen, or -2  if the input data is
 *                 not correct. *olen is always updated to reflect the amount
 *                 of data that has (or would have) been written.
 *
 * @note           Call this function with *dst = nullptr or dlen = 0 to obtain
 *                 the required buffer size in *olen
 */
LIBATFRAME_UTILS_API int base64_decode(unsigned char *dst, size_t dlen, size_t *olen, const unsigned char *src,
                                       size_t slen, base64_mode_t::type mode = base64_mode_t::EN_BMT_STANDARD);

/**
 * @brief          Decode a base64-formatted buffer, support no padding
 *
 * @param dst      destination buffer
 * @param src      source buffer
 * @param slen     amount of data to be decoded
 * @param mode     base64 mode @see base64_mode_t
 *
 * @return         0 if successful
 */
LIBATFRAME_UTILS_API int base64_decode(std::string &dst, const unsigned char *src, size_t slen,
                                       base64_mode_t::type mode = base64_mode_t::EN_BMT_STANDARD);

/**
 * @brief          Decode a base64-formatted buffer, support no padding
 *
 * @param dst      destination buffer
 * @param in       source buffer
 * @param mode     base64 mode @see base64_mode_t
 *
 * @return         0 if successful
 */
LIBATFRAME_UTILS_API int base64_decode(std::string &dst, const std::string &in,
                                       base64_mode_t::type mode = base64_mode_t::EN_BMT_STANDARD);
LIBATFRAME_UTILS_NAMESPACE_END
