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

#include <cstring>
#include <stdint.h>
#include <string>

#if defined(WIN32)
#include <rpc.h>
#include <rpcdce.h>
#else
#include <uuid/uuid.h>
#endif

#include <common/string_oprs.h>
#include <config/atframe_utils_build_feature.h>

#if defined(LIBATFRAME_UTILS_ENABLE_UUID) && LIBATFRAME_UTILS_ENABLE_UUID

namespace util {
    namespace random {
        struct uuid {
            uint32_t data1;
            uint16_t data2;
            uint16_t data3;
            uint8_t  data4[8];
        };

        class uuid_generator {
        public:
            static LIBATFRAME_UTILS_API std::string uuid_to_string(const uuid &id);

            static LIBATFRAME_UTILS_API std::string uuid_to_binary(const uuid &id);

            static LIBATFRAME_UTILS_API uuid binary_to_uuid(const std::string &id_bin);

            static LIBATFRAME_UTILS_API uuid generate();

            static LIBATFRAME_UTILS_API std::string generate_string();

            static LIBATFRAME_UTILS_API uuid generate_random();

            static LIBATFRAME_UTILS_API std::string generate_string_random();

            static LIBATFRAME_UTILS_API uuid generate_time();

            static LIBATFRAME_UTILS_API std::string generate_string_time();
        };
    } // namespace random
} // namespace util

#endif

#endif /* UTIL_UUID_GENERATOR_H */
