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

#include <common/string_oprs.h>
#include <config/atframe_utils_build_feature.h>

#if defined(LIBATFRAME_UTILS_ENABLE_LIBUUID) && LIBATFRAME_UTILS_ENABLE_LIBUUID
#include <uuid/uuid.h>

#elif defined(LIBATFRAME_UTILS_ENABLE_UUID_WINRPC) && LIBATFRAME_UTILS_ENABLE_UUID_WINRPC
#include <rpc.h>
#include <rpcdce.h>

#endif

#if (defined(LIBATFRAME_UTILS_ENABLE_LIBUUID) && LIBATFRAME_UTILS_ENABLE_LIBUUID) ||            \
    (defined(LIBATFRAME_UTILS_ENABLE_UUID_WINRPC) && LIBATFRAME_UTILS_ENABLE_UUID_WINRPC) ||    \
    (defined(LIBATFRAME_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT) && LIBATFRAME_UTILS_ENABLE_UUID_INTERNAL_IMPLEMENT)

namespace util {
    namespace random {
        /**
         * @brief https://en.wikipedia.org/wiki/Universally_unique_identifier
         */
        struct LIBATFRAME_UTILS_API_HEAD_ONLY uuid {
            uint32_t time_low;              /* time_low */
            uint16_t time_mid;              /* time_mid */
            uint16_t time_hi_and_version;   /* time_hi_and_version */
            uint16_t clock_seq;             /* clock_seq_hi_and_res clock_seq_low */
            uint8_t	 node[6];               /* node: the 48-bit node id */            
        };

        class uuid_generator {
        public:
            static LIBATFRAME_UTILS_API std::string uuid_to_string(const uuid &id, bool remove_minus = false);

            static LIBATFRAME_UTILS_API std::string uuid_to_binary(const uuid &id);

            static LIBATFRAME_UTILS_API uuid binary_to_uuid(const std::string &id_bin);

            static LIBATFRAME_UTILS_API uuid generate();

            /*
             * @brief prefer to use generate_string_random(...) on linux and windows/generate_string_time(...) on macOS
             */
            static LIBATFRAME_UTILS_API std::string generate_string(bool remove_minus = false);

            /**
             * @brief generate a uuid of Version 4
             */
            static LIBATFRAME_UTILS_API uuid generate_random();

            /**
             * @brief generate a uuid of Version 4
             * @note QPS:
             *      150k => inner implement(-O2)
             *      150k => libuuid on linux
             *      600k => uuid/uuid.h on macOS
             */
            static LIBATFRAME_UTILS_API std::string generate_string_random(bool remove_minus = false);

            /**
             * @brief generate a uuid of Version 1
             * @note This API will generate a uuid with MAC address, which is not suggested for security reasons
             */
            static LIBATFRAME_UTILS_API uuid generate_time();

            /**
             * @brief generate a uuid of Version 1
             * @note This API will generate a uuid with MAC address, which is not suggested for security reasons
             * @note QPS:
             *      540k => inner implement(-O2)
             *      245k => libuuid on linux
             *      711k => uuid/uuid.h on macOS
             */
            static LIBATFRAME_UTILS_API std::string generate_string_time(bool remove_minus = false);
        };
    } // namespace random
} // namespace util

#endif

#endif /* UTIL_UUID_GENERATOR_H */
