// Licensed under the MIT licenses.

#include <cstring>
#include <algorithm>

#include "string/tquerystring.h"

namespace util {
    namespace uri {
        typedef bool uri_map_type[256];
        static uri_map_type g_raw_url_map = {false};
        static uri_map_type g_uri_map = {false};
        static uri_map_type g_uri_component_map = {false};

        // RFC 3986
        static void _init_raw_url_map(uri_map_type &uri_map) {
            if (uri_map[static_cast<int>('0')]) return;

            for (int i = 0; i < 26; ++i) {
                uri_map['a' + i] = uri_map['A' + i] = true;
            }

            for (int i = 0; i < 10; ++i) {
                uri_map['0' + i] = true;
            }

            // -_.
            const char spec_chars[] = "-_.";
            for (int i = 0; spec_chars[i]; ++i) {
                uri_map[static_cast<int>(spec_chars[i])] = true;
            }
        }

        static void _init_uri_component_map(uri_map_type &uri_map) {
            if (uri_map[static_cast<int>('0')]) return;

            _init_raw_url_map(uri_map);

            // -_.!~*'()
            const char spec_chars[] = "!~*'()";
            for (int i = 0; spec_chars[i]; ++i) {
                uri_map[static_cast<int>(spec_chars[i])] = true;
            }
        }

        static void _init_uri_map(uri_map_type &uri_map) {
            if (uri_map[static_cast<int>('0')]) return;

            _init_uri_component_map(uri_map);
            // ;/?:@&=+$,#
            const char spec_chars[] = ";/?:@&=+$,#";
            for (int i = 0; spec_chars[i]; ++i) {
                uri_map[static_cast<int>(spec_chars[i])] = true;
            }
        }


        static std::string _encode_uri(uri_map_type &uri_map, const char *data, std::size_t sz, bool like_php) {
            std::string ret;
            ret.reserve(sz);

            while (sz--) {
                if (uri_map[static_cast<int>(*data)]) {
                    ret += *data;
                } else if (like_php && ' ' == *data) {
                    ret += '+';
                } else {
                    ret += '%';
                    static char hex_char_map[16] = {0};
                    // 初始化16进制表
                    if (0 == hex_char_map[0]) {
                        for (int i = 0; i < 10; i++) {
                            hex_char_map[i] = '0' + i;
                        }

                        for (int i = 10; i < 16; i++) {
                            hex_char_map[i] = 'A' - 10 + i;
                        }
                    }

                    // 转义前4位
                    ret += hex_char_map[*data >> 4];
                    // 转义后4位
                    ret += hex_char_map[*data & 0x0F];
                }

                ++data;
            }

            return ret;
        }

        static std::string _decode_uri(const char *data, std::size_t sz, bool like_php) {
            std::string ret;

            sz = sz ? sz : strlen(data);
            static char hex_char_map[256] = {0};

            // 初始化字符表
            if (0 == hex_char_map[static_cast<int>('A')]) {
                for (int i = 0; i < 10; i++) {
                    hex_char_map['0' + i] = i;
                }

                for (int i = 10; i < 16; i++) {
                    hex_char_map['A' - 10 + i] = hex_char_map['a' - 10 + i] = i;
                }
            }

            while (sz--) {
                if (like_php && '+' == *data) {
                    ret += ' ';
                } else if (*data != '%' || sz < 2) {
                    ret += *data;
                } else {
                    const char &high_c = data[1], &low_c = data[2];
                    ret += (hex_char_map[static_cast<int>(high_c)] << 4) + hex_char_map[static_cast<int>(low_c)];
                    data += 2;
                    sz -= 2;
                }

                ++data;
            }

            return ret;
        }


        std::string encode_uri(const char *content, std::size_t sz) {
            _init_uri_map(g_uri_map);
            sz = sz ? sz : strlen(content);

            return _encode_uri(g_uri_map, content, sz, false);
        }

        std::string decode_uri(const char *uri, std::size_t sz) {
            sz = sz ? sz : strlen(uri);
            return _decode_uri(uri, sz, false);
        }

        std::string encode_uri_component(const char *content, std::size_t sz) {
            _init_uri_component_map(g_uri_component_map);
            sz = sz ? sz : strlen(content);

            return _encode_uri(g_uri_component_map, content, sz, false);
        }

        std::string decode_uri_component(const char *uri, std::size_t sz) {
            sz = sz ? sz : strlen(uri);
            return _decode_uri(uri, sz, false);
        }

        // ==== RFC 3986 ====
        std::string raw_encode_url(const char *content, std::size_t sz) {
            _init_raw_url_map(g_raw_url_map);
            sz = sz ? sz : strlen(content);

            return _encode_uri(g_raw_url_map, content, sz, false);
        }

        std::string raw_decode_url(const char *uri, std::size_t sz) {
            sz = sz ? sz : strlen(uri);
            return _decode_uri(uri, sz, false);
        }

        // ==== application/x-www-form-urlencoded ====
        std::string encode_url(const char *content, std::size_t sz) {
            _init_raw_url_map(g_raw_url_map);
            sz = sz ? sz : strlen(content);

            return _encode_uri(g_raw_url_map, content, sz, true);
        }

        std::string decode_url(const char *uri, std::size_t sz) {
            sz = sz ? sz : strlen(uri);
            return _decode_uri(uri, sz, true);
        }
    }

    namespace types {
        void item_impl::append_to(std::string &target, const std::string &key, const std::string &value) const {
            target +=
                uri::encode_uri_component(key.c_str(), key.size()) + "=" + uri::encode_uri_component(value.c_str(), value.size()) + "&";
        }

        // 字符串类型
        item_string::item_string() {}

        item_string::item_string(const std::string &data) : data_(data) {}

        item_string::~item_string() {}

        std::size_t item_string::size() const { return data_.size(); }

        ITEM_TYPE item_string::type() const { return ITEM_TYPE_STRING; }

        std::string item_string::to_string(const char *prefix) const { return data_; }

        bool item_string::parse(const std::vector<std::string> &keys, std::size_t index, const std::string &value) {
            data_ = value;
            return true;
        }

        bool item_string::encode(std::string &output, const char *prefix) const {
            append_to(output, std::string(prefix), data_);
            return true;
        }

        // 数组类型
        item_array::item_array() {}

        item_array::~item_array() {}

        std::size_t item_array::size() const { return data_.size(); }

        ITEM_TYPE item_array::type() const { return ITEM_TYPE_ARRAY; }

        std::string item_array::to_string(const char *prefix) const {
            std::string ret = "[";
            for (std::size_t i = 0; i < data_.size(); ++i) {
                if (i) {
                    ret += ", ";
                }

                ret += data_[i]->type() < ITEM_TYPE_QUERYSTRING ? '\"' + data_[i]->to_string(prefix) + '\"' : data_[i]->to_string(prefix);
            }

            ret += "]";
            return ret;
        }

        bool item_array::parse(const std::vector<std::string> &keys, std::size_t index, const std::string &value) {
            if (index + 1 != keys.size() || keys[index].size()) {
                return false;
            }

            append(value);
            return true;
        }

        bool item_array::encode(std::string &output, const char *prefix) const {
            bool ret = true;
            std::size_t index = 0;
            std::string new_prefix, pre_prefix = prefix;
            for (; index < data_.size(); ++index) {
                if (data_[index]->type() >= ITEM_TYPE_QUERYSTRING) {
                    break;
                }
            }

            // 不带下标编码
            if (index >= data_.size()) {
                new_prefix = pre_prefix + "[]";
                for (index = 0; index < data_.size(); ++index) {
                    ret = data_[index]->encode(output, new_prefix.c_str()) && ret;
                }
            } else // 带下标编码
            {
                for (index = 0; index < data_.size(); ++index) {
                    new_prefix = pre_prefix + '[' + uri::any_to_query_string(index) + ']';
                    ret = data_[index]->encode(output, new_prefix.c_str()) && ret;
                }
            }

            return ret;
        }

        // 映射类型
        item_object::item_object() {}

        item_object::~item_object() {}

        std::size_t item_object::size() const { return data_.size(); }

        ITEM_TYPE item_object::type() const { return ITEM_TYPE_ARRAY; }

        std::string item_object::to_string(const char *prefix) const {
            std::string ret = "{";
            for (data_const_iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                if (iter != data_.begin()) {
                    ret += ", ";
                }
                ret += '\"' + iter->first + "\": " + (iter->second->type() < ITEM_TYPE_QUERYSTRING
                                                          ? '\"' + iter->second->to_string(prefix) + '\"'
                                                          : iter->second->to_string(prefix));
            }

            ret += "}";
            return ret;
        }

        bool item_object::parse(const std::vector<std::string> &keys, std::size_t index, const std::string &value) {
            data_iterator iter = data_.find(keys[index]);
            if (iter == data_.end()) {
                types::item_impl::ptr_type ptr;
                // 最后一级，字符串类型
                if (index + 1 == keys.size()) {
                    ptr = item_string::create();
                }
                // 倒数第二级，且最后一级key为空，数组类型
                else if (index + 2 == keys.size() && keys.back().size() == 0) {
                    ptr = item_array::create();
                }
                // Object类型
                else {
                    ptr = item_object::create();
                }

                data_.insert(std::make_pair(keys[index], ptr));
                return ptr->parse(keys, index + 1, value);
            } else {
                return iter->second->parse(keys, index + 1, value);
            }

            return false;
        }

        bool item_object::encode(std::string &output, const char *prefix) const {
            bool ret = true;
            std::string new_prefix, pre_prefix = prefix;

            for (data_const_iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                new_prefix = pre_prefix + "[" + iter->first + "]";
                ret = iter->second->encode(output, new_prefix.c_str()) && ret;
            }

            return ret;
        }

        std::vector<std::string> item_object::keys() const {
            std::vector<std::string> ret;

            for (data_const_iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                ret.push_back(iter->first);
            }

            return ret;
        }
    }

    tquerystring::tquerystring() : spliter_("?#&") {}

    tquerystring::tquerystring(const std::string &spliter) : spliter_(spliter) {}

    tquerystring::~tquerystring() {}

    std::size_t tquerystring::size() const { return data_.size(); }

    types::ITEM_TYPE tquerystring::type() const { return types::ITEM_TYPE_QUERYSTRING; }

    std::string tquerystring::to_string(const char *prefix) const {
        std::string ret;

        encode(ret, prefix);

        return ret;
    }

    bool tquerystring::decode(const char *content, std::size_t sz) {
        bool pDeclMap[256] = {false}, ret = true;
        std::size_t uLen = 0, isDecl;
        sz = sz ? sz : strlen(content);

        for (std::size_t i = 0; i < spliter_.size(); ++i) {
            pDeclMap[static_cast<int>(spliter_[i])] = true;
        }

        while (sz) {
            for (isDecl = 0, uLen = 0; uLen < sz; ++uLen) {
                if (pDeclMap[static_cast<int>(*(content + uLen))]) {
                    isDecl = 1;
                    break;
                }
            }

            ret = decode_record(content, uLen);

            content += uLen + isDecl;
            sz -= uLen + isDecl;
        }

        return ret;
    }

    bool tquerystring::decode_record(const char *content, std::size_t sz) {
        std::string seg, value, strOrigin;
        std::vector<std::string> keys;
        strOrigin.assign(content, sz);

        // 计算值
        std::size_t uValStart = strOrigin.find_last_of('=');
        if (uValStart != strOrigin.npos) {
            value = strOrigin.substr(uValStart + 1);
            value = uri::decode_uri_component(value.data(), value.size());
            strOrigin = strOrigin.substr(0, uValStart);
        }

        strOrigin = uri::decode_uri_component(strOrigin.data(), strOrigin.size());

        // 计算key列表
        for (sz = 0; sz < strOrigin.size(); ++sz) {
            while (sz < strOrigin.size() && strOrigin[sz] == ']') {
                ++sz;
            }

            while (sz < strOrigin.size() && strOrigin[sz] != '[') {
                seg += strOrigin[sz];
                ++sz;
            }

            keys.push_back(seg);
            seg.clear();

            if (sz >= strOrigin.size()) {
                break;
            }
            for (++sz; sz < strOrigin.size() && strOrigin[sz] != ']'; ++sz) {
                seg += strOrigin[sz];
            }
        }

        if (seg.size() > 0) {
            keys.push_back(seg);
        }

        return parse(keys, 0, value);
    }

    bool tquerystring::encode(std::string &output, const char *prefix) const {
        data_const_iterator iter = data_.begin();

        while (iter != data_.end() && iter->second->encode(output, iter->first.c_str())) {
            ++iter;
        }

        if (output.size() > 0 && output[output.size() - 1] == '&') {
            output.resize(output.size() - 1);
        }

        return iter == data_.end();
    }

    std::shared_ptr<types::item_impl> tquerystring::operator[](const std::string &key) { return get(key); }
}
