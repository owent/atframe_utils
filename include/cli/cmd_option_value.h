#ifndef UTIL_CLI_CMDOPTIONVALUE_H
#define UTIL_CLI_CMDOPTIONVALUE_H

#pragma once

/*
 * cmd_option_value.h
 *
 *  Created on: 2011-12-29
 *      Author: OWenT
 *
 * 应用程序命令处理
 *
 */

#include <cstring>
#include <sstream>
#include <stdint.h>
#include <string>
#include <type_traits>
#include <vector>


#include "std/smart_ptr.h"

#include <common/string_oprs.h>

namespace util {
    namespace cli {
        class cmd_option_value {
        protected:
            std::string data_;

            struct string2any {

                template <typename Tt>
                static UTIL_FORCEINLINE Tt conv(const std::string &s, Tt *) {
                    Tt ret;
                    std::stringstream ss;
                    ss.str(s);
                    ss >> ret;
                    return ret;
                }

                static UTIL_FORCEINLINE const std::string &conv(const std::string &s, std::string *) { return s; }

                // static const char *conv(const std::string &s, const char **) { return s.c_str(); }

                static UTIL_FORCEINLINE char conv(const std::string &s, char *) { return s.empty() ? 0 : s[0]; }

                static UTIL_FORCEINLINE unsigned char conv(const std::string &s, unsigned char *) {
                    return static_cast<unsigned char>(s.empty() ? 0 : s[0]);
                }

                static UTIL_FORCEINLINE int16_t conv(const std::string &s, int16_t *) { return util::string::to_int<int16_t>(s.c_str()); }

                static UTIL_FORCEINLINE uint16_t conv(const std::string &s, uint16_t *) { return util::string::to_int<int16_t>(s.c_str()); }

                static UTIL_FORCEINLINE int32_t conv(const std::string &s, int32_t *) { return util::string::to_int<int32_t>(s.c_str()); }

                static UTIL_FORCEINLINE uint32_t conv(const std::string &s, uint32_t *) { return util::string::to_int<uint32_t>(s.c_str()); }

                static UTIL_FORCEINLINE int64_t conv(const std::string &s, int64_t *) { return util::string::to_int<int64_t>(s.c_str()); }
                static UTIL_FORCEINLINE uint64_t conv(const std::string &s, uint64_t *) { return util::string::to_int<uint64_t>(s.c_str()); }

                static UTIL_FORCEINLINE bool conv(const std::string &s, bool *) { return !s.empty() && "0" != s; }
            };

        public:
            LIBATFRAME_UTILS_API cmd_option_value(const char *str_data);
            LIBATFRAME_UTILS_API cmd_option_value(const char *begin, const char *end);
            LIBATFRAME_UTILS_API cmd_option_value(const std::string &str_data);

            template <typename Tr>
            LIBATFRAME_UTILS_API_HEAD_ONLY Tr to() const {
                typedef typename ::std::remove_cv<Tr>::type cv_type;
                return string2any::conv(data_, reinterpret_cast<cv_type *>(NULL));
            }

            // 获取存储对象的字符串
            LIBATFRAME_UTILS_API const std::string &to_cpp_string() const;

            LIBATFRAME_UTILS_API bool to_bool() const;

            LIBATFRAME_UTILS_API char to_char() const;

            LIBATFRAME_UTILS_API short to_short() const;

            LIBATFRAME_UTILS_API int to_int() const;

            LIBATFRAME_UTILS_API long to_long() const;

            LIBATFRAME_UTILS_API long long to_longlong() const;

            LIBATFRAME_UTILS_API double to_double() const;

            LIBATFRAME_UTILS_API float to_float() const;

            LIBATFRAME_UTILS_API const char *to_string() const;

            LIBATFRAME_UTILS_API unsigned char to_uchar() const;

            LIBATFRAME_UTILS_API unsigned short to_ushort() const;

            LIBATFRAME_UTILS_API unsigned int to_uint() const;

            LIBATFRAME_UTILS_API unsigned long to_ulong() const;

            LIBATFRAME_UTILS_API unsigned long long to_ulonglong() const;

            LIBATFRAME_UTILS_API int8_t to_int8() const;

            LIBATFRAME_UTILS_API uint8_t to_uint8() const;

            LIBATFRAME_UTILS_API int16_t to_int16() const;

            LIBATFRAME_UTILS_API uint16_t to_uint16() const;

            LIBATFRAME_UTILS_API int32_t to_int32() const;

            LIBATFRAME_UTILS_API uint32_t to_uint32() const;

            LIBATFRAME_UTILS_API int64_t to_int64() const;

            LIBATFRAME_UTILS_API uint64_t to_uint64() const;

            // ============ logic operation ============
            LIBATFRAME_UTILS_API bool to_logic_bool() const;

            LIBATFRAME_UTILS_API void split(char delim, std::vector<cmd_option_value> &out);
        };
    } // namespace cli
} // namespace util

#endif /* _CMDOPTIONVALUE_H_ */
