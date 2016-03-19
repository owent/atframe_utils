/**
 * @file UUIDGenerator.h
 * @brief UUID 生成器
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

namespace util
{
    namespace random
    {
        typedef struct {
            uint32_t  Data1;
            uint16_t  Data2;
            uint16_t  Data3;
            uint8_t   Data4[8];
        } UUID;
    
        class UUIDGenerator
        {
        public:
            static std::string UUIDToString(const UUID& uuid)
            {
                char strBuff[64] = {0};
                #if defined(WIN32)
                _snprintf_s(
                    strBuff,
                    sizeof(strBuff),
                     "%08X-%04X-%04X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                    uuid.Data1, uuid.Data2, uuid.Data3,
                    uuid.Data4[0], uuid.Data4[1],
                    uuid.Data4[2], uuid.Data4[3],
                    uuid.Data4[4], uuid.Data4[5],
                    uuid.Data4[6], uuid.Data4[7]);
                #else
                uuid_t linux_uid;
                memcpy(linux_uid, &uuid, sizeof(uuid));
                uuid_unparse(linux_uid, strBuff);
                #endif
    
                return std::string(strBuff);
            }
    
            static UUID Generate()
            {
                UUID ret;
    
                #if defined(WIN32)
                UuidCreate((UUID*)&ret);
                #else
                uuid_t linux_uid;
                uuid_generate(linux_uid);
                memcpy(&ret, linux_uid, sizeof(ret));
                #endif
    
                return ret;
            }
    
            static std::string GenerateString()
            {
                UUID uuid = Generate();
    
                return UUIDToString(uuid);
            }
    
            static UUID GenerateRandom()
            {
                UUID ret;
    
                #if defined(WIN32)
                UuidCreate((UUID*)&ret);
                #else
                uuid_t linux_uid;
                uuid_generate_random(linux_uid);
                memcpy(&ret, linux_uid, sizeof(ret));
                #endif
    
                return ret;
            }
    
            static std::string GenerateStringRandom()
            {
                UUID uuid = GenerateRandom();
    
                return UUIDToString(uuid);
            }
    
            static UUID GenerateTime()
            {
                UUID ret;
    
                #if defined(WIN32)
                UuidCreate((UUID*)&ret);
                #else
                uuid_t linux_uid;
                uuid_generate_time_safe(linux_uid);
                memcpy(&ret, linux_uid, sizeof(ret));
                #endif
    
                return ret;
            }
    
            static std::string GenerateStringTime()
            {
                UUID uuid = GenerateTime();
    
                return UUIDToString(uuid);
            }
        };
    }
}


#endif /* _UTIL_UUID_GENERATOR_H_ */
