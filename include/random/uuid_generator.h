/**
 * @file uuid_generator.h
 * @brief uuid 生成器
 *
 *
 * Licensed under the MIT licenses.
 * @note Windows 下需要链接Rpcrt4.dll或Rpcrt4.lib
 * @note Linux 下需要安装libuuid-devel包并链接libuuid.a 或 libuuid.so
 *
 * @version 1.0
 * @author OWenT
 * @date 2013年9月29日
 *
 * @history
 *
 */

#ifndef UTIL_UUID_GENERATOR_H
#define UTIL_UUID_GENERATOR_H

#pragma once

#include <stdint.h>
#include <cstring>
#include <string>

#include <common/string_oprs.h>
#include <config/atframe_utils_build_feature.h>

#if defined(ATFRAMEWORK_UTILS_ENABLE_LIBUUID) && ATFRAMEWORK_UTILS_ENABLE_LIBUUID
#  include <uuid/uuid.h>

#elif defined(ATFRAMEWORK_UTILS_ENABLE_UUID_WINRPC) && ATFRAMEWORK_UTILS_ENABLE_UUID_WINRPC
#  include <rpc.h>
#  include <rpcdce.h>

#endif

#if (defined(ATFRAMEWORK_UTILS_ENABLE_LIBUUID) && ATFRAMEWORK_UTILS_ENABLE_LIBUUID) ||         \
    (defined(ATFRAMEWORK_UTILS_ENABLE_UUID_WINRPC) && ATFRAMEWORK_UTILS_ENABLE_UUID_WINRPC) || \
    (defined(ATFRAMEWORK_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT) && ATFRAMEWORK_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT)

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace random {
/**
 * @brief UUID generator
 * @note
 *      Using libuuid if found on Linux.
 *      Using system uuid/uuid.h on OSX/BSD
 *      Using Rpcrt on Windows
 *      Using internal implement which is just like libuuid when none of above found.
 * @see https://en.wikipedia.org/wiki/Universally_unique_identifier
 * @see https://tools.ietf.org/html/rfc4122
 */
struct ATFRAMEWORK_UTILS_API_HEAD_ONLY uuid {
  uint32_t time_low;            /* time_low */
  uint16_t time_mid;            /* time_mid */
  uint16_t time_hi_and_version; /* time_hi_and_version */
  uint16_t clock_seq;           /* clock_seq_hi_and_res clock_seq_low */
  uint8_t node[6];              /* node: the 48-bit node id */
};

class uuid_generator {
 public:
  static ATFRAMEWORK_UTILS_API std::string uuid_to_string(const uuid &id, bool remove_minus = false);

  static ATFRAMEWORK_UTILS_API std::string uuid_to_binary(const uuid &id);

  static ATFRAMEWORK_UTILS_API uuid binary_to_uuid(const std::string &id_bin);

  static ATFRAMEWORK_UTILS_API uuid generate();

  /**
   * @brief prefer to use generate_string_random(...) on linux and windows/generate_string_time(...) on macOS
   * @see https://tools.ietf.org/html/rfc4122
   * @see https://en.wikipedia.org/wiki/Universally_unique_identifier
   */
  static ATFRAMEWORK_UTILS_API std::string generate_string(bool remove_minus = false);

  /**
   * @brief generate a uuid of Version 4
   * @note QPS:
   *      1851k => inner implement(-O2)
   *      210k  => libuuid on linux
   *      570k  => uuid/uuid.h on macOS
   *      370k  => MSVC with Rpcrt on Windows
   *      650k  => Mingw64-GCC with Rpcrt on Windows
   * @see https://tools.ietf.org/html/rfc4122
   * @see https://en.wikipedia.org/wiki/Universally_unique_identifier#Version_4_(random)
   */
  static ATFRAMEWORK_UTILS_API uuid generate_random();

  /**
   * @brief generate a uuid of Version 4
   * @note QPS:
   *      1851k => inner implement(-O2)
   *      210k  => libuuid on linux
   *      570k  => uuid/uuid.h on macOS
   *      370k  => MSVC with Rpcrt on Windows
   *      650k  => Mingw64-GCC with Rpcrt on Windows
   * @see https://tools.ietf.org/html/rfc4122
   * @see https://en.wikipedia.org/wiki/Universally_unique_identifier#Version_4_(random)
   */
  static ATFRAMEWORK_UTILS_API std::string generate_string_random(bool remove_minus = false);

  /**
   * @brief generate a uuid of Version 1
   * @note This API will generate a uuid with MAC address and time, which is not suggested for security reasons
   * @note QPS:
   *      1580k => inner implement(-O2)
   *      1800k => libuuid on linux
   *      5340k => uuid/uuid.h on macOS
   *      720k  => MSVC with Rpcrt on Windows
   *      2550k => Mingw64-GCC with Rpcrt on Windows
   * @see https://tools.ietf.org/html/rfc4122
   * @see https://en.wikipedia.org/wiki/Universally_unique_identifier#Version_1_(date-time_and_MAC_address)
   */
  static ATFRAMEWORK_UTILS_API uuid generate_time();

  /**
   * @brief generate a uuid of Version 1
   * @note This API will generate a uuid with MAC address and time, which is not suggested for security reasons
   * @note QPS:
   *      1580k => inner implement(-O2)
   *      1800k => libuuid on linux
   *      5340k => uuid/uuid.h on macOS
   *      720k  => MSVC with Rpcrt on Windows
   *      2550k => Mingw64-GCC with Rpcrt on Windows
   * @see https://tools.ietf.org/html/rfc4122
   * @see https://en.wikipedia.org/wiki/Universally_unique_identifier#Version_1_(date-time_and_MAC_address)
   */
  static ATFRAMEWORK_UTILS_API std::string generate_string_time(bool remove_minus = false);
};
}  // namespace random
ATFRAMEWORK_UTILS_NAMESPACE_END

#endif

#endif /* UTIL_UUID_GENERATOR_H */
