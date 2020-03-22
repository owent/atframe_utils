#include <config/compiler_features.h>

#include <random/uuid_generator.h>

#if defined(LIBATFRAME_UTILS_ENABLE_UUID) && LIBATFRAME_UTILS_ENABLE_UUID

#if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
#include <type_traits>
#endif

#if !defined(WIN32)
#if defined(UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT) && UTIL_CONFIG_COMPILER_CXX_STATIC_ASSERT
    #if (defined(__cplusplus) && __cplusplus >= 201402L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201402L))
        UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<uuid_t>::value);
        UTIL_CONFIG_STATIC_ASSERT(std::is_trivially_copyable<util::random::uuid>::value);
    #elif (defined(__cplusplus) && __cplusplus >= 201103L) || ((defined(_MSVC_LANG) && _MSVC_LANG >= 201103L))
        UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<uuid_t>::value);
        UTIL_CONFIG_STATIC_ASSERT(std::is_trivial<util::random::uuid>::value);
    #else
        UTIL_CONFIG_STATIC_ASSERT(std::is_pod<uuid_t>::value);
        UTIL_CONFIG_STATIC_ASSERT(std::is_pod<util::random::uuid>::value);
    #endif
    UTIL_CONFIG_STATIC_ASSERT(sizeof(uuid_t) == sizeof(util::random::uuid));
#endif
#endif

namespace util {
    namespace random {
        LIBATFRAME_UTILS_API std::string uuid_generator::uuid_to_string(const uuid &id) {
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

        LIBATFRAME_UTILS_API std::string uuid_generator::uuid_to_binary(const uuid &id) {
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

        LIBATFRAME_UTILS_API uuid uuid_generator::binary_to_uuid(const std::string &id_bin) {
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

        LIBATFRAME_UTILS_API uuid uuid_generator::generate() {
            uuid ret;

#if defined(WIN32)
            UUID res;
            UuidCreate(&res);
            ret.data1 = static_cast<uint32_t>(res.Data1);
            ret.data2 = res.Data2;
            ret.data3 = res.Data3;
            memcpy(ret.data4, res.Data4, sizeof(ret.data4));
#else
            uuid_t linux_uid;
            uuid_generate(linux_uid);
            memcpy(&ret, linux_uid, sizeof(ret));
#endif

            return ret;
        }

        LIBATFRAME_UTILS_API std::string uuid_generator::generate_string() { return uuid_to_string(generate()); }

        LIBATFRAME_UTILS_API uuid uuid_generator::generate_random() {
            uuid ret;

#if defined(WIN32)
            UUID res;
            UuidCreate(&res);
            ret.data1 = static_cast<uint32_t>(res.Data1);
            ret.data2 = res.Data2;
            ret.data3 = res.Data3;
            memcpy(ret.data4, res.Data4, sizeof(ret.data4));
#else
            uuid_t linux_uid;
            uuid_generate_random(linux_uid);
            memcpy(&ret, linux_uid, sizeof(ret));
#endif

            return ret;
        }

        LIBATFRAME_UTILS_API std::string uuid_generator::generate_string_random() { return generate_string(); }

        LIBATFRAME_UTILS_API uuid uuid_generator::generate_time() {
            uuid ret;

#if defined(WIN32)
            UUID res;
            UuidCreate(&res);
            ret.data1 = static_cast<uint32_t>(res.Data1);
            ret.data2 = res.Data2;
            ret.data3 = res.Data3;
            memcpy(ret.data4, res.Data4, sizeof(ret.data4));
#else
            uuid_t linux_uid;
            uuid_generate_time_safe(linux_uid);
            memcpy(&ret, linux_uid, sizeof(ret));
#endif

            return ret;
        }

        LIBATFRAME_UTILS_API std::string uuid_generator::generate_string_time() { return uuid_to_string(generate_time()); }
    } // namespace random
} // namespace util


#endif
