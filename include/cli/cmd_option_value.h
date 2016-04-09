#ifndef _UTIL_CLI_CMDOPTIONVALUE_H_
#define _UTIL_CLI_CMDOPTIONVALUE_H_

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

#include <sstream>
#include <stdint.h>
#include <string>

#include "std/smart_ptr.h"

namespace util {
    namespace cli {
        class cmd_option_value {
        protected:
            std::string data_;

            template <typename _Tt>
            inline _Tt string2any(const char *strData) const {
                _Tt ret;
                std::stringstream ss;
                ss.str(strData);
                ss >> ret;
                return ret;
            }

        public:
            cmd_option_value(const char *strData);

            template <typename _Tt>
            inline _Tt to() const {
                return string2any<_Tt>(data_.c_str());
            }

            // 获取存储对象的字符串
            const std::string &to_cpp_string() const;

            bool to_bool() const;

            char to_char() const;

            short to_short() const;

            int to_int() const;

            long to_long() const;

            long long to_longlong() const;

            double to_double() const;

            float to_float() const;

            const char *to_string() const;

            unsigned char to_uchar() const;

            unsigned short to_ushort() const;

            unsigned int to_uint() const;

            unsigned long to_ulong() const;

            unsigned long long to_ulonglong() const;

            int8_t to_int8() const;

            uint8_t to_uint8() const;

            int16_t to_int16() const;

            uint16_t to_uint16() const;

            int32_t to_int32() const;

            uint32_t to_uint32() const;

            int64_t to_int64() const;

            uint64_t to_uint64() const;

            // ============ logic operation ============
            bool to_logic_bool() const;
        };
    }
}

#endif /* _CMDOPTIONVALUE_H_ */
