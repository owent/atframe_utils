// Licensed under the MIT licenses.

#include <algorithm>
#include <cstring>

#include "string/tquerystring.h"

namespace util {
    namespace uri {
        typedef bool        uri_map_type[256];
        static uri_map_type g_raw_url_map       = {false};
        static uri_map_type g_uri_map           = {false};
        static uri_map_type g_uri_component_map = {false};

        // RFC 3986
        static void _init_raw_url_map(uri_map_type &uri_map) {
            if (uri_map[static_cast<unsigned char>('0')]) return;

            for (int i = 0; i < 26; ++i) {
                uri_map['a' + i] = uri_map['A' + i] = true;
            }

            for (int i = 0; i < 10; ++i) {
                uri_map['0' + i] = true;
            }

            // -_.
            const unsigned char spec_chars[] = "-_.";
            for (int i = 0; spec_chars[i]; ++i) {
                uri_map[spec_chars[i]] = true;
            }
        }

        static void _init_uri_component_map(uri_map_type &uri_map) {
            if (uri_map[static_cast<unsigned char>('0')]) return;

            _init_raw_url_map(uri_map);

            // -_.!~*'()
            const unsigned char spec_chars[] = "!~*'()";
            for (int i = 0; spec_chars[i]; ++i) {
                uri_map[spec_chars[i]] = true;
            }
        }

        static void _init_uri_map(uri_map_type &uri_map) {
            if (uri_map[static_cast<unsigned char>('0')]) return;

            _init_uri_component_map(uri_map);
            // ;/?:@&=+$,#
            const unsigned char spec_chars[] = ";/?:@&=+$,#";
            for (int i = 0; spec_chars[i]; ++i) {
                uri_map[spec_chars[i]] = true;
            }
        }


        static std::string _encode_uri(uri_map_type &uri_map, const char *data, size_t sz, bool like_php) {
            std::string ret;
            ret.reserve(sz);

            while (sz--) {
                unsigned char data_uc = static_cast<unsigned char>(*data);

                if (uri_map[data_uc]) {
                    ret += *data;
                } else if (like_php && ' ' == data_uc) {
                    ret += '+';
                } else {
                    ret += '%';
                    static char hex_char_map[16] = {0};
                    // 初始化16进制表
                    if (0 == hex_char_map[0]) {
                        for (int i = 0; i < 10; i++) {
                            hex_char_map[i] = static_cast<char>('0' + i);
                        }

                        for (int i = 10; i < 16; i++) {
                            hex_char_map[i] = static_cast<char>('A' - 10 + i);
                        }
                    }

                    // 转义前4位
                    ret += hex_char_map[data_uc >> 4];
                    // 转义后4位
                    ret += hex_char_map[data_uc & 0x0F];
                }

                ++data;
            }

            return ret;
        }

        static std::string _decode_uri(const char *data, size_t sz, bool like_php) {
            std::string ret;

            sz                                     = sz ? sz : strlen(data);
            static unsigned char hex_char_map[256] = {0};

            // 初始化字符表
            if (0 == hex_char_map[static_cast<unsigned char>('A')]) {
                for (int i = 0; i < 10; i++) {
                    hex_char_map['0' + i] = static_cast<unsigned char>(i);
                }

                for (int i = 10; i < 16; i++) {
                    hex_char_map['A' - 10 + i] = hex_char_map['a' - 10 + i] = static_cast<unsigned char>(i);
                }
            }

            while (sz--) {
                if (like_php && '+' == *data) {
                    ret += ' ';
                } else if (*data != '%' || sz < 2) {
                    ret += *data;
                } else {
                    const unsigned char high_c = static_cast<unsigned char>(data[1]);
                    const unsigned char low_c  = static_cast<unsigned char>(data[2]);
                    ret += static_cast<unsigned char>((hex_char_map[high_c] << 4) + hex_char_map[low_c]);
                    data += 2;
                    sz -= 2;
                }

                ++data;
            }

            return ret;
        }


        LIBATFRAME_UTILS_API std::string encode_uri(const char *content, size_t sz) {
            _init_uri_map(g_uri_map);
            sz = sz ? sz : strlen(content);

            return _encode_uri(g_uri_map, content, sz, false);
        }

        LIBATFRAME_UTILS_API std::string decode_uri(const char *uri, size_t sz) {
            sz = sz ? sz : strlen(uri);
            return _decode_uri(uri, sz, false);
        }

        LIBATFRAME_UTILS_API std::string encode_uri_component(const char *content, size_t sz) {
            _init_uri_component_map(g_uri_component_map);
            sz = sz ? sz : strlen(content);

            return _encode_uri(g_uri_component_map, content, sz, false);
        }

        LIBATFRAME_UTILS_API std::string decode_uri_component(const char *uri, size_t sz) {
            sz = sz ? sz : strlen(uri);
            return _decode_uri(uri, sz, false);
        }

        // ==== RFC 3986 ====
        LIBATFRAME_UTILS_API std::string raw_encode_url(const char *content, size_t sz) {
            _init_raw_url_map(g_raw_url_map);
            sz = sz ? sz : strlen(content);

            return _encode_uri(g_raw_url_map, content, sz, false);
        }

        LIBATFRAME_UTILS_API std::string raw_decode_url(const char *uri, size_t sz) {
            sz = sz ? sz : strlen(uri);
            return _decode_uri(uri, sz, false);
        }

        // ==== application/x-www-form-urlencoded ====
        LIBATFRAME_UTILS_API std::string encode_url(const char *content, size_t sz) {
            _init_raw_url_map(g_raw_url_map);
            sz = sz ? sz : strlen(content);

            return _encode_uri(g_raw_url_map, content, sz, true);
        }

        LIBATFRAME_UTILS_API std::string decode_url(const char *uri, size_t sz) {
            sz = sz ? sz : strlen(uri);
            return _decode_uri(uri, sz, true);
        }
    }  // namespace uri

    namespace types {
        LIBATFRAME_UTILS_API item_impl::~item_impl() {}

        LIBATFRAME_UTILS_API void item_impl::append_to(std::string &target, const std::string &key, const std::string &value) const {
            target +=
                uri::encode_uri_component(key.c_str(), key.size()) + "=" + uri::encode_uri_component(value.c_str(), value.size()) + "&";
        }

        // 字符串类型
        LIBATFRAME_UTILS_API item_string::item_string() {}

        LIBATFRAME_UTILS_API item_string::item_string(const std::string &data) : data_(data) {}

        LIBATFRAME_UTILS_API item_string::~item_string() {}

        LIBATFRAME_UTILS_API item_string::ptr_type item_string::create() { return std::make_shared<item_string>(); }

        LIBATFRAME_UTILS_API item_string::ptr_type item_string::create(const std::string &data) {
            return std::make_shared<item_string>(data);
        }

        LIBATFRAME_UTILS_API bool item_string::empty() const { return data_.empty(); }

        LIBATFRAME_UTILS_API size_t item_string::size() const { return data_.size(); }

        LIBATFRAME_UTILS_API ITEM_TYPE item_string::type() const { return ITEM_TYPE_STRING; }

        LIBATFRAME_UTILS_API std::string item_string::to_string(const char *) const { return data_; }

        LIBATFRAME_UTILS_API bool item_string::encode(std::string &output, const char *prefix) const {
            append_to(output, std::string(prefix), data_);
            return true;
        }

        LIBATFRAME_UTILS_API bool item_string::parse(const std::vector<std::string> &, size_t, const std::string &value) {
            data_ = value;
            return true;
        }

        LIBATFRAME_UTILS_API const std::string &item_string::data() const { return data_; }

        LIBATFRAME_UTILS_API item_string::operator std::string() { return get(); };

        LIBATFRAME_UTILS_API item_string &item_string::operator=(const std::string &data) {
            set(data);
            return (*this);
        };

        LIBATFRAME_UTILS_API void item_string::set(const std::string &data) { data_ = data; };

        LIBATFRAME_UTILS_API std::string &item_string::get() { return data_; };

        // 数组类型
        LIBATFRAME_UTILS_API item_array::item_array() {}

        LIBATFRAME_UTILS_API item_array::~item_array() {}

        LIBATFRAME_UTILS_API item_array::ptr_type item_array::create() { return std::make_shared<item_array>(); }

        LIBATFRAME_UTILS_API bool item_array::empty() const { return data_.empty(); }

        LIBATFRAME_UTILS_API size_t item_array::size() const { return data_.size(); }

        LIBATFRAME_UTILS_API ITEM_TYPE item_array::type() const { return ITEM_TYPE_ARRAY; }

        LIBATFRAME_UTILS_API std::string item_array::to_string(const char *prefix) const {
            std::string ret = "[";
            for (size_t i = 0; i < data_.size(); ++i) {
                if (i) {
                    ret += ", ";
                }

                ret += data_[i]->type() < ITEM_TYPE_QUERYSTRING ? '\"' + data_[i]->to_string(prefix) + '\"' : data_[i]->to_string(prefix);
            }

            ret += "]";
            return ret;
        }

        LIBATFRAME_UTILS_API bool item_array::parse(const std::vector<std::string> &keys, size_t index, const std::string &value) {
            if (index + 1 != keys.size() || keys[index].size()) {
                return false;
            }

            append(value);
            return true;
        }

        LIBATFRAME_UTILS_API bool item_array::encode(std::string &output, const char *prefix) const {
            bool        ret   = true;
            size_t      index = 0;
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
            } else  // 带下标编码
            {
                for (index = 0; index < data_.size(); ++index) {
                    new_prefix = pre_prefix + '[' + uri::any_to_query_string(index) + ']';
                    ret        = data_[index]->encode(output, new_prefix.c_str()) && ret;
                }
            }

            return ret;
        }

        LIBATFRAME_UTILS_API std::shared_ptr<item_impl> item_array::get(std::size_t uIndex) { return data_[uIndex]; };

        LIBATFRAME_UTILS_API std::string item_array::get_string(std::size_t uIndex) const { return data_[uIndex]->to_string(); };

        LIBATFRAME_UTILS_API void item_array::set(std::size_t uIndex, const std::shared_ptr<item_impl> &value) { data_[uIndex] = value; };

        LIBATFRAME_UTILS_API void item_array::set(std::size_t uIndex, const std::string &value) {
            data_[uIndex] = std::make_shared<item_string>(value);
        };

        LIBATFRAME_UTILS_API void item_array::append(const std::shared_ptr<item_impl> &value) { data_.push_back(value); };

        LIBATFRAME_UTILS_API void item_array::append(const std::string &value) { data_.push_back(std::make_shared<item_string>(value)); };

        LIBATFRAME_UTILS_API void item_array::pop_back() { data_.pop_back(); };

        LIBATFRAME_UTILS_API void item_array::clear() { data_.clear(); };

        // 映射类型
        LIBATFRAME_UTILS_API item_object::ptr_type item_object::create() { return std::make_shared<item_object>(); }

        LIBATFRAME_UTILS_API item_object::item_object() {}

        LIBATFRAME_UTILS_API item_object::~item_object() {}

        LIBATFRAME_UTILS_API bool item_object::empty() const { return data_.empty(); }

        LIBATFRAME_UTILS_API size_t item_object::size() const { return data_.size(); }

        LIBATFRAME_UTILS_API ITEM_TYPE item_object::type() const { return ITEM_TYPE_ARRAY; }

        LIBATFRAME_UTILS_API std::string item_object::to_string(const char *prefix) const {
            std::string ret = "{";
            for (data_const_iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                if (iter != data_.begin()) {
                    ret += ", ";
                }
                ret += '\"' + iter->first + "\": " +
                       (iter->second->type() < ITEM_TYPE_QUERYSTRING ? '\"' + iter->second->to_string(prefix) + '\"'
                                                                     : iter->second->to_string(prefix));
            }

            ret += "}";
            return ret;
        }

        LIBATFRAME_UTILS_API bool item_object::parse(const std::vector<std::string> &keys, size_t index, const std::string &value) {
            if (index >= keys.size()) {
                return false;
            }

            data_iterator iter = data_.find(keys[index]);
            if (iter == data_.end()) {
                types::item_impl::ptr_type ptr;
                // 最后一级，字符串类型
                if (index + 1 == keys.size()) {
                    ptr = std::static_pointer_cast<types::item_impl>(item_string::create());
                }
                // 倒数第二级，且最后一级key为空，数组类型
                else if (index + 2 == keys.size() && keys.back().size() == 0) {
                    ptr = std::static_pointer_cast<types::item_impl>(item_array::create());
                }
                // Object类型
                else {
                    ptr = std::static_pointer_cast<types::item_impl>(item_object::create());
                }

                data_.insert(std::make_pair(keys[index], ptr));
                return ptr->parse(keys, index + 1, value);
            } else {
                return iter->second->parse(keys, index + 1, value);
            }

            // return false;
        }

        LIBATFRAME_UTILS_API bool item_object::encode(std::string &output, const char *prefix) const {
            bool        ret = true;
            std::string new_prefix, pre_prefix = prefix;

            for (data_const_iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                new_prefix = pre_prefix + "[" + iter->first + "]";
                ret        = iter->second->encode(output, new_prefix.c_str()) && ret;
            }

            return ret;
        }

        LIBATFRAME_UTILS_API std::vector<std::string> item_object::keys() const {
            std::vector<std::string> ret;

            for (data_const_iterator iter = data_.begin(); iter != data_.end(); ++iter) {
                ret.push_back(iter->first);
            }

            return ret;
        }

        LIBATFRAME_UTILS_API const LIBATFRAME_UTILS_AUTO_SELETC_MAP(std::string, std::shared_ptr<item_impl>) & item_object::data() const {
            return data_;
        }

        LIBATFRAME_UTILS_API std::shared_ptr<item_impl> item_object::get(const std::string &key) {
            data_iterator iter = data_.find(key);
            return iter == data_.end() ? std::shared_ptr<item_impl>() : iter->second;
        };

        LIBATFRAME_UTILS_API std::string item_object::get_string(const std::string &key) const {
            data_const_iterator iter = data_.find(key);
            return iter == data_.end() ? "" : iter->second->to_string();
        };

        LIBATFRAME_UTILS_API void item_object::set(const std::string &key, const std::shared_ptr<item_impl> &value) {
            data_iterator iter = data_.find(key);
            if (iter == data_.end()) {
                data_.insert(std::make_pair(key, value));
            } else {
                iter->second = value;
            }
        };

        LIBATFRAME_UTILS_API void item_object::set(const std::string &key, const std::string &value) {
            set(key, std::make_shared<item_string>(value));
        };

        LIBATFRAME_UTILS_API void item_object::remove(const std::string &key) { data_.erase(key); };

        LIBATFRAME_UTILS_API void item_object::clear() { data_.clear(); };

    }  // namespace types

    LIBATFRAME_UTILS_API tquerystring::tquerystring() : spliter_("?#&") {}

    LIBATFRAME_UTILS_API tquerystring::tquerystring(const std::string &spliter) : spliter_(spliter) {}

    LIBATFRAME_UTILS_API tquerystring::~tquerystring() {}

    LIBATFRAME_UTILS_API tquerystring::ptr_type tquerystring::create() { return std::make_shared<tquerystring>(); }

    LIBATFRAME_UTILS_API tquerystring::ptr_type tquerystring::create(const std::string &spliter) {
        return std::make_shared<tquerystring>(spliter);
    }

    LIBATFRAME_UTILS_API bool tquerystring::empty() const { return data_.empty(); }

    LIBATFRAME_UTILS_API size_t tquerystring::size() const { return data_.size(); }

    LIBATFRAME_UTILS_API types::ITEM_TYPE tquerystring::type() const { return types::ITEM_TYPE_QUERYSTRING; }

    LIBATFRAME_UTILS_API std::string tquerystring::to_string(const char *prefix) const {
        std::string ret;

        encode(ret, prefix);

        return ret;
    }

    LIBATFRAME_UTILS_API bool tquerystring::decode(const char *content, size_t sz) {
        bool   decl_map[256] = {false}, ret = true;
        size_t len = 0, is_decl;
        sz         = sz ? sz : strlen(content);

        for (size_t i = 0; i < spliter_.size(); ++i) {
            decl_map[static_cast<unsigned char>(spliter_[i])] = true;
        }

        while (sz) {
            for (is_decl = 0, len = 0; len < sz; ++len) {
                if (decl_map[static_cast<unsigned char>(*(content + len))]) {
                    is_decl = 1;
                    break;
                }
            }

            ret = decode_record(content, len);

            content += len + is_decl;
            sz -= len + is_decl;
        }

        return ret;
    }

    LIBATFRAME_UTILS_API bool tquerystring::decode_record(const char *content, size_t sz) {
        std::string              seg, value, origin_val;
        std::vector<std::string> keys;
        origin_val.assign(content, sz);
        seg.reserve(sz);

        // 计算值
        size_t val_start = origin_val.find_last_of('=');
        if (val_start != origin_val.npos) {
            value      = origin_val.substr(val_start + 1);
            value      = uri::decode_uri_component(value.data(), value.size());
            origin_val = origin_val.substr(0, val_start);
        }

        origin_val = uri::decode_uri_component(origin_val.data(), origin_val.size());

        // 计算key列表
        for (sz = 0; sz < origin_val.size(); ++sz) {
            while (sz < origin_val.size() && origin_val[sz] == ']') {
                ++sz;
            }

            while (sz < origin_val.size() && origin_val[sz] != '[') {
                seg += origin_val[sz];
                ++sz;
            }

            keys.push_back(seg);
            seg.clear();

            if (sz >= origin_val.size()) {
                break;
            }
            for (++sz; sz < origin_val.size() && origin_val[sz] != ']'; ++sz) {
                seg += origin_val[sz];
            }
        }

        if (seg.size() > 0) {
            keys.push_back(seg);
            return parse(keys, 0, value);
        } else {
            return false;
        }
    }

    LIBATFRAME_UTILS_API bool tquerystring::encode(std::string &output, const char *) const {
        data_const_iterator iter = data_.begin();

        while (iter != data_.end() && iter->second->encode(output, iter->first.c_str())) {
            ++iter;
        }

        if (output.size() > 0 && output[output.size() - 1] == '&') {
            output.resize(output.size() - 1);
        }

        return iter == data_.end();
    }

    LIBATFRAME_UTILS_API std::shared_ptr<types::item_impl> tquerystring::operator[](const std::string &key) { return get(key); }

    LIBATFRAME_UTILS_API void tquerystring::set_spliter(const std::string &spliter) { spliter_ = spliter; };

    LIBATFRAME_UTILS_API types::item_string::ptr_type tquerystring::create_string() { return types::item_string::create(); };

    LIBATFRAME_UTILS_API types::item_string::ptr_type tquerystring::create_string(const std::string &val) {
        return types::item_string::create(val);
    };

    LIBATFRAME_UTILS_API types::item_array::ptr_type tquerystring::create_array() { return types::item_array::create(); };

    LIBATFRAME_UTILS_API types::item_object::ptr_type tquerystring::create_object() { return types::item_object::create(); };
}  // namespace util
