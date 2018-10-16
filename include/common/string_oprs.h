/**
 * @file string_oprs.h
 * @brief 字符串相关操作
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2015.11.24
 *
 * @history
 *
 *
 */

#ifndef UTIL_COMMON_STRING_OPRS_H
#define UTIL_COMMON_STRING_OPRS_H

// 目测主流编译器都支持且有优化， gcc 3.4 and upper, vc, clang, c++ builder xe3, intel c++ and etc.
#pragma once

#include <stdint.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <string>
#include <utility>

#include <type_traits>


#if defined(_MSC_VER) && _MSC_VER >= 1600
#define UTIL_STRFUNC_STRCASE_CMP(l, r) _stricmp(l, r)
#define UTIL_STRFUNC_STRNCASE_CMP(l, r, s) _strnicmp(l, r, s)
#define UTIL_STRFUNC_STRCMP(l, r) strcmp(l, r)
#define UTIL_STRFUNC_STRNCMP(l, r, s) strncmp(l, r, s)
#else
#define UTIL_STRFUNC_STRCASE_CMP(l, r) strcasecmp(l, r)
#define UTIL_STRFUNC_STRNCASE_CMP(l, r, s) strncasecmp(l, r, s)
#define UTIL_STRFUNC_STRCMP(l, r) strcmp(l, r)
#define UTIL_STRFUNC_STRNCMP(l, r, s) strncmp(l, r, s)
#endif

#if (defined(_MSC_VER) && _MSC_VER >= 1600) || (defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L) || defined(__STDC_LIB_EXT1__)
#define UTIL_STRFUNC_SSCANF(...) sscanf_s(__VA_ARGS__)

#ifdef _MSC_VER
#define UTIL_STRFUNC_VSNPRINTF(buffer, bufsz, fmt, arg) vsnprintf_s(buffer, static_cast<size_t>(bufsz), _TRUNCATE, fmt, arg)
#define UTIL_STRFUNC_SNPRINTF(...) sprintf_s(__VA_ARGS__)
#else
#define UTIL_STRFUNC_VSNPRINTF(buffer, bufsz, fmt, arg) vsnprintf_s(buffer, static_cast<size_t>(bufsz), fmt, arg)
#define UTIL_STRFUNC_SNPRINTF(...) snprintf_s(__VA_ARGS__)
#endif

#define UTIL_STRFUNC_C11_SUPPORT 1
#else
#define UTIL_STRFUNC_SSCANF(...) sscanf(__VA_ARGS__)
#define UTIL_STRFUNC_SNPRINTF(...) snprintf(__VA_ARGS__)
#define UTIL_STRFUNC_VSNPRINTF(buffer, bufsz, fmt, arg) vsnprintf(buffer, static_cast<int>(bufsz), fmt, arg)
#endif

namespace util {
    namespace string {
        /**
         * @brief 字符转小写
         * @param c 字符
         * @return str 如果是大写字符输出响应的小写字符，否则原样返回
         * @note 用于替换标准函数里参数是int类型导致的某些编译器warning问题
         */
        template <typename TCH = char>
        static TCH tolower(TCH c) {
            if (c >= 'A' && c <= 'Z') {
                return static_cast<TCH>(c - 'A' + 'a');
            }

            return c;
        }

        /**
         * @brief 字符转大写
         * @param c 字符
         * @return str 如果是小写字符输出响应的大写字符，否则原样返回
         * @note 用于替换标准函数里参数是int类型导致的某些编译器warning问题
         */
        template <typename TCH = char>
        static TCH toupper(TCH c) {
            if (c >= 'a' && c <= 'z') {
                return static_cast<TCH>(c - 'a' + 'A');
            }

            return c;
        }

        /**
         * @brief 是否是空白字符
         * @param c 字符
         * @return 如果是空白字符，返回true，否则返回false
         */
        template <typename TCH>
        inline bool is_space(const TCH &c) {
            return ' ' == c || '\t' == c || '\r' == c || '\n' == c;
        }

        /**
         * @brief 移除两边或一边的空白字符
         * @param str_begin 字符串起始地址
         * @param sz 字符串长度，填0则会自动判定长度
         * @param trim_left 是否移除左边的空白字符
         * @param trim_right 是否移除右边的空白字符
         * @return 返回子串的起始地址和长度
         * @note 注意，返回的字符串是源的子串，共享地址。并且不保证以0结尾，需要用返回的长度来判定子串长度
         */
        template <typename TCH>
        std::pair<const TCH *, size_t> trim(const TCH *str_begin, size_t sz, bool trim_left = true, bool trim_right = true) {
            if (0 == sz) {
                const TCH *str_end = str_begin;
                while (str_end && *str_end) {
                    ++str_end;
                }

                sz = str_end - str_begin;
            }

            if (trim_left && str_begin) {
                while (*str_begin && sz > 0) {
                    if (!is_space(*str_begin)) {
                        break;
                    }

                    --sz;
                    ++str_begin;
                }
            }

            size_t sub_str_sz = sz;
            if (trim_right && str_begin) {
                while (sub_str_sz > 0) {
                    if (is_space(str_begin[sub_str_sz - 1])) {
                        --sub_str_sz;
                    } else {
                        break;
                    }
                }
            }

            return std::make_pair(str_begin, sub_str_sz);
        }

        /**
         * @brief 翻转字符串
         * @param begin 字符串起始地址
         * @param end 字符串结束地址,填入NULL，则从begin开是找到\0结束
         */
        template <typename TCH>
        void reverse(TCH *begin, TCH *end) {
            if (NULL == begin) {
                return;
            }

            if (NULL == end) {
                end = begin;
                while (*end) {
                    ++end;
                }
            }

            if (begin >= end) {
                return;
            }

            --end;
            while (begin < end) {
                TCH c  = *end;
                *end   = *begin;
                *begin = c;
                ++begin;
                --end;
            }
        }

        template <typename T>
        size_t int2str_unsigned(const char *str, size_t strsz, typename std::remove_cv<T>::type in) {
            if (0 == strsz) {
                return 0;
            }

            if (0 == in) {
                *str = '0';
                return 1;
            }

            size_t ret = 0;
            while (ret < strsz && in > 0) {
                str[ret] = (in % 10) + '0';

                in /= 10;
                ++ret;
            }

            if (in > 0 && ret >= strsz) {
                return 0;
            }

            reverse(str, str + ret);
            return ret;
        }

        template <typename T>
        size_t int2str_signed(const char *str, size_t strsz, typename std::remove_cv<T>::type in) {
            if (0 == strsz) {
                return 0;
            }

            if (in < 0) {
                *str       = '-';
                size_t ret = int2str_unsigned(str + 1, strsz - 1, -in);
                if (0 == ret) {
                    return 0;
                }

                return ret + 1;
            } else {
                return int2str_unsigned(str, strsz, in);
            }
        }

        template <typename T>
        struct int2str_helper {
            typedef T                                    value_type_s;
            typedef typename std::make_unsigned<T>::type value_type_u;

            static inline size_t call(const char *str, size_t strsz, const value_type_s &in) { return int2str_signed(str, strsz, in); }

            static inline size_t call(const char *str, size_t strsz, const value_type_u &in) { return int2str_unsigned(str, strsz, in); }
        };


        /**
         * @brief 整数转字符串
         * @param str 输出的字符串缓冲区
         * @param strsz 字符串缓冲区长度
         * @param in 输入的数字
         * @return 返回输出的数据长度，失败返回0
         */
        template <typename T>
        inline size_t int2str(const char *str, size_t strsz, const typename std::remove_cv<T>::type &in) {
            return int2str_helper<typename std::make_signed<typename std::remove_cv<T>::type>::type>::call(str, strsz, in);
        }

        /**
         * @brief 字符串转整数
         * @param out 输出的整数
         * @param str 被转换的字符串
         * @note 性能肯定比sscanf系，和iostream系高。strtol系就不知道了
         */
        template <typename T>
        void str2int(T &out, const char *str) {
            out = static_cast<T>(0);
            if (NULL == str || !(*str)) {
                return;
            }

            // negative
            bool is_negative = false;
            while (*str && *str == '-') {
                is_negative = !is_negative;
                ++str;
            }

            if (!(*str)) {
                return;
            }

            if ('0' == str[0] && 'x' == tolower(str[1])) { // hex
                for (size_t i = 2; str[i]; ++i) {
                    char c = tolower(str[i]);
                    if (c >= '0' && c <= '9') {
                        out <<= 4;
                        out += c - '0';
                    } else if (c >= 'a' && c <= 'f') {
                        out <<= 4;
                        out += c - 'a' + 10;
                    } else {
                        break;
                    }
                }
            } else if ('\\' == str[0]) { // oct
                for (size_t i = 1; str[i] >= '0' && str[i] < '8'; ++i) {
                    out <<= 3;
                    out += str[i] - '0';
                }
            } else { // dec
                for (size_t i = 0; str[i] >= '0' && str[i] <= '9'; ++i) {
                    out *= 10;
                    out += str[i] - '0';
                }
            }

            if (is_negative) {
                out = (~out) + 1;
            }
        }

        /**
         * @brief 字符串转整数
         * @param str 被转换的字符串
         * @return 输出的整数
         */
        template <typename T>
        inline T to_int(const char *str) {
            T ret = 0;
            str2int(ret, str);
            return ret;
        }

        /**
         * @brief 字符转十六进制表示
         * @param out 输出的字符串(缓冲区长度至少为2)
         * @param c 被转换的字符
         * @param upper_case 输出大写字符？
         */
        template <typename TStr, typename TCh>
        void hex(TStr *out, TCh c, bool upper_case = false) {
            out[0] = static_cast<TStr>((c >> 4) & 0x0F);
            out[1] = static_cast<TStr>(c & 0x0F);

            for (int i = 0; i < 2; ++i) {
                if (out[i] > 9) {
                    out[i] += (upper_case ? 'A' : 'a') - 10;
                } else {
                    out[i] += '0';
                }
            }
        }

        /**
         * @brief 字符转8进制表示
         * @param out 输出的字符串(缓冲区长度至少为3)
         * @param c 被转换的字符
         * @param upper_case 输出大写字符？
         */
        template <typename TStr, typename TCh>
        void oct(TStr *out, TCh c) {
            out[0] = static_cast<TStr>(((c >> 6) & 0x07) + '0');
            out[1] = static_cast<TStr>(((c >> 3) & 0x07) + '0');
            out[2] = static_cast<TStr>((c & 0x07) + '0');
        }

        /**
         * @brief 字符转8进制表示
         * @param src 输入的buffer
         * @param ss 输入的buffer长度
         * @param out 输出buffer
         * @param os 输出buffer长度，回传输出缓冲区使用的长度
         */
        template <typename TCh>
        void serialization(const void *src, size_t ss, TCh *out, size_t &os) {
            const TCh *cs = reinterpret_cast<const TCh *>(src);
            size_t     i, j;
            for (i = 0, j = 0; i < ss && j < os; ++i) {
                if (cs[i] >= 32 && cs[i] < 127) {
                    out[j] = cs[i];
                    ++j;
                } else if (j + 4 <= os) {
                    out[j++] = '\\';
                    oct(&out[j], cs[i]);
                    j += 3;
                } else {
                    break;
                }
            }

            os = j;
        }

        /**
         * @brief 字符转8进制表示
         * @param src 输入的buffer
         * @param ss 输入的buffer长度
         * @param out 输出缓冲区
         */
        template <typename Elem, typename Traits>
        void serialization(const void *src, size_t ss, std::basic_ostream<Elem, Traits> &out) {
            const Elem *cs = reinterpret_cast<const Elem *>(src);
            size_t      i;
            for (i = 0; i < ss; ++i) {
                if (cs[i] >= 32 && cs[i] < 127) {
                    out.put(cs[i]);
                } else {
                    Elem tmp[4] = {'\\', 0, 0, 0};
                    oct(&tmp[1], cs[i]);
                    out.write(tmp, 4);
                }
            }
        }

        /**
         * @brief 字符转16进制表示
         * @param src 输入的buffer
         * @param ss 输入的buffer长度
         * @param out 输出buffer
         * @param upper_case 是否大写
         */
        template <typename TCh>
        void dumphex(const void *src, size_t ss, TCh *out, bool upper_case = false) {
            const unsigned char *cs = reinterpret_cast<const unsigned char *>(src);
            size_t               i;
            for (i = 0; i < ss; ++i) {
                hex<TCh, unsigned char>(&out[i << 1], cs[i], upper_case);
            }
        }

        /**
         * @brief 字符转16进制表示
         * @param src 输入的buffer
         * @param ss 输入的buffer长度
         * @param out 输出缓冲区
         * @param upper_case 是否大写
         */
        template <typename Elem, typename Traits>
        void dumphex(const void *src, size_t ss, std::basic_ostream<Elem, Traits> &out, bool upper_case = false) {
            const unsigned char *cs = reinterpret_cast<const unsigned char *>(src);
            size_t               i;
            Elem                 tmp[2];
            for (i = 0; i < ss; ++i) {
                hex<Elem, unsigned char>(tmp, cs[i], upper_case);
                out.write(tmp, 2);
            }
        }

        /**
         * @brief 提取版本号
         * @param v 版本号字符串(a.b.c.d...)
         * @note 版本号字符串可以是十进制数字或0x开头的十六进制或\开头的八进制,且每个数字必须在int64_t以内
         * @return 返回剩余字符串地址
         */
        const char *version_tok(const char *v, int64_t &out);

        /**
         * @brief 版本比较函数
         * @param l 版本号字符串(a.b.c.d...)
         * @param r 版本号字符串(a.b.c.d...)
         * @note 版本号字符串可以是十进制数字或0x开头的十六进制或\开头的八进制,且每个数字必须在int64_t以内
         * @return 如果l<r则返回-1，如果l>r则返回1，如果l==r则返回0
         */
        int version_compare(const char *l, const char *r);

        /**
         * @brief 版本号字符串标准化，把版本号字符串处理为以十进制表示并以.分隔的形式。移除所有无效字符
         * @param v 版本号字符串(a.b.c.d...)
         * @note 版本号字符串可以是十进制数字或0x开头的十六进制或\开头的八进制,且每个数字必须在int64_t以内
         * @note 返回的版本号字符串会移除末尾的.0，即 1.2.0.0和1.2...都会输出1.2
         * @note 空的版本号字符串会返回0
         * @return 返回标准化的版本号字符
         */
        std::string version_normalize(const char *v);
    } // namespace string
} // namespace util

#endif /* _UTIL_COMMON_COMPILER_MESSAGE_H_ */
