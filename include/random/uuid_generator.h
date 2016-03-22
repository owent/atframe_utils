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

#ifndef _UTIL_UUID_GENERATOR_H_
#define _UTIL_UUID_GENERATOR_H_

#include <cstring>
#include <string>
#include <stdint.h>

#if defined(WIN32)
#include <rpc.h>
#include <rpcdce.h>
#else
#include <uuid/uuid.h>
#endif

namespace util {
    namespace random {
        typedef struct {
            uint32_t data1;
            uint16_t data2;
            uint16_t data3;
            uint8_t data4[8];
        } uuid;

        class uuid_generator {
        public:
            static std::string uuid_to_string(const uuid &uuid) {
                char str_buff[64] = {0};
#if defined(WIN32)
                _snprintf_s(str_buff, sizeof(str_buff), "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X", uuid.data1,
                            uuid.data2, uuid.data3, uuid.data4[0], uuid.data4[1], uuid.data4[2], uuid.data4[3],
                            uuid.data4[4], uuid.data4[5], uuid.data4[6], uuid.data4[7]);
#else
                uuid_t linux_uid;
                memcpy(linux_uid, &uuid, sizeof(uuid));
                uuid_unparse(linux_uid, str_buff);
#endif

                return std::string(str_buff);
            }

            static uuid generate() {
                uuid ret;

#if defined(WIN32)
                UuidCreate((uuid *)&ret);
#else
                uuid_t linux_uid;
                uuid_generate(linux_uid);
                memcpy(&ret, linux_uid, sizeof(ret));
#endif

                return ret;
            }

            static std::string generate_string() {
                uuid uuid = generate();

                return uuid_to_string(uuid);
            }

            static uuid generate_random() {
                uuid ret;

#if defined(WIN32)
                UuidCreate((uuid *)&ret);
#else
                uuid_t linux_uid;
                uuid_generate_random(linux_uid);
                memcpy(&ret, linux_uid, sizeof(ret));
#endif

                return ret;
            }

            static std::string generate_string_random() {
                uuid uuid = generate_string();

                return uuid_to_string(uuid);
            }

            static uuid generate_time() {
                uuid ret;

#if defined(WIN32)
                UuidCreate((uuid *)&ret);
#else
                uuid_t linux_uid;
                uuid_generate_time_safe(linux_uid);
                memcpy(&ret, linux_uid, sizeof(ret));
#endif

                return ret;
            }

            static std::string generate_string_time() {
                uuid uuid = generate_time();

                return uuid_to_string(uuid);
            }
        };
    }
}


#endif /* _UTIL_UUID_GENERATOR_H_ */
