/*
 * cmd_option_value.cpp
 *
 *  Created on: 2011-12-29
 *      Author: OWenT
 *
 * 应用程序命令处理
 *
 */

#include "cli/cmd_option_value.h"
#include <algorithm>

namespace util {
    namespace cli {
        namespace detail {
            static char tolower(char c) {
                if (c >= 'A' && c <= 'Z') {
                    return c - 'A' + 'a';
                }

                return c;
            }
        } // namespace detail

        cmd_option_value::cmd_option_value(const char *strData) : data_(strData) {}

        const std::string &cmd_option_value::to_cpp_string() const { return data_; }

        bool cmd_option_value::to_bool() const { return to<bool>(); }

        char cmd_option_value::to_char() const { return to<char>(); }

        short cmd_option_value::to_short() const { return to<short>(); }

        int cmd_option_value::to_int() const { return to<int>(); }

        long cmd_option_value::to_long() const { return to<long>(); }

        long long cmd_option_value::to_longlong() const { return to<long long>(); }

        double cmd_option_value::to_double() const { return to<double>(); }

        float cmd_option_value::to_float() const { return to<float>(); }

        const char *cmd_option_value::to_string() const { return data_.c_str(); }

        // ============ unsigned ============
        unsigned char cmd_option_value::to_uchar() const { return to<unsigned char>(); }

        unsigned short cmd_option_value::to_ushort() const { return to<unsigned short>(); }

        unsigned int cmd_option_value::to_uint() const { return to<unsigned int>(); }

        unsigned long cmd_option_value::to_ulong() const { return to<unsigned long>(); }

        unsigned long long cmd_option_value::to_ulonglong() const { return to<unsigned long long>(); }

        int8_t cmd_option_value::to_int8() const { return static_cast<int8_t>(to_int()); }

        uint8_t cmd_option_value::to_uint8() const { return static_cast<uint8_t>(to_uint()); }

        int16_t cmd_option_value::to_int16() const { return to<int16_t>(); }

        uint16_t cmd_option_value::to_uint16() const { return to<uint16_t>(); }

        int32_t cmd_option_value::to_int32() const { return to<int32_t>(); }

        uint32_t cmd_option_value::to_uint32() const { return to<uint32_t>(); }

        int64_t cmd_option_value::to_int64() const { return to<int64_t>(); }

        uint64_t cmd_option_value::to_uint64() const { return to<uint64_t>(); }

        bool cmd_option_value::to_logic_bool() const {
            std::string lowercase_content = data_;
            std::transform(lowercase_content.begin(), lowercase_content.end(), lowercase_content.begin(), detail::tolower);

            if (lowercase_content.empty()) {
                return false;
            }

            if ("no" == lowercase_content || "false" == lowercase_content || "disabled" == lowercase_content ||
                "disable" == lowercase_content || "0" == lowercase_content) {
                return false;
            }

            return true;
        }
    } // namespace cli
} // namespace util
