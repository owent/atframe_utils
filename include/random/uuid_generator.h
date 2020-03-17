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

#if defined(WIN32)
#include <rpc.h>
#include <rpcdce.h>
#else
#include <uuid/uuid.h>
#endif

namespace util {
    namespace random {
        struct uuid {
            uint32_t data1;
            uint16_t data2;
            uint16_t data3;
            uint8_t  data4[8];
        } uuid;

        class uuid_generator {
        public:
            static std::string uuid_to_string(const uuid &id) {
                char str_buff[64] = {0};
#if defined(WIN32)
                UTIL_STRFUNC_SNPRINTF(str_buff, sizeof(str_buff), "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", id.data1, id.data2,
                                      id.data3, id.data4[0], id.data4[1], id.data4[2], id.data4[3], id.data4[4], id.data4[5], id.data4[6],
                                      id.data4[7]);
#else
                uuid_t linux_uid;
                memcpy(linux_uid, &id, sizeof(uuid));
                uuid_unparse(linux_uid, str_buff);
#endif

                return std::string(str_buff);
            }

            static std::string uuid_to_binary(const uuid &id) {
                std::string ret;
                ret.resize(sizeof(uuid));
                ret[0] = static_cast<char>((id.data1 >> 24) | 0xFF);
                ret[1] = static_cast<char>((id.data1 >> 16) | 0xFF);
                ret[2] = static_cast<char>((id.data1 >> 8) | 0xFF);
                ret[3] = static_cast<char>(id.data1 | 0xFF);
                ret[4] = static_cast<char>((id.data2 >> 8) | 0xFF);
                ret[5] = static_cast<char>(id.data2 | 0xFF);
                ret[6] = static_cast<char>((id.data3 >> 8) | 0xFF);
                ret[7] = static_cast<char>(id.data3 | 0xFF);
                memcpy(&ret[8], id.data4, sizeof(id.data4));

                return ret;
            }

            static uuid binary_to_uuid(const std::string &id_bin) {
                uuid ret;
                if (sizeof(uuid) > id_bin.size()) {
                    memset(&ret, 0, sizeof(ret));
                    return ret;
                }

                ret.data1 = (static_cast<uint32_t>(id_bin[0]) << 24) | static_cast<uint32_t>(id_bin[1] << 16) |
                            static_cast<uint32_t>(id_bin[2] << 8) | static_cast<uint32_t>(id_bin[3]);
                ret.data2 = (static_cast<uint16_t>(id_bin[4]) << 8) | static_cast<uint16_t>(id_bin[5]);
                ret.data3 = (static_cast<uint16_t>(id_bin[6]) << 8) | static_cast<uint16_t>(id_bin[7]);
                memcpy(ret.data4, &id_bin[8], sizeof(ret.data4));
                return ret;
            }

            static uuid generate() {
                uuid ret;

#if defined(WIN32)
                UuidCreate((UUID *)&ret);
#else
                uuid_t linux_uid;
                uuid_generate(linux_uid);
                memcpy(&ret, linux_uid, sizeof(ret));
#endif

                return ret;
            }

            static inline std::string generate_string() { return uuid_to_string(generate()); }

            static uuid generate_random() {
                uuid ret;

#if defined(WIN32)
                UuidCreate((UUID *)&ret);
#else
                uuid_t linux_uid;
                uuid_generate_random(linux_uid);
                memcpy(&ret, linux_uid, sizeof(ret));
#endif

                return ret;
            }

            static inline std::string generate_string_random() { return generate_string(); }

            static uuid generate_time() {
                uuid ret;

#if defined(WIN32)
                UuidCreate((UUID *)&ret);
#else
                uuid_t linux_uid;
                uuid_generate_time_safe(linux_uid);
                memcpy(&ret, linux_uid, sizeof(ret));
#endif

                return ret;
            }

            static inline std::string generate_string_time() { return uuid_to_string(generate_time()); }
        };
    } // namespace random
} // namespace util


#endif /* _UTIL_UUID_GENERATOR_H_ */
