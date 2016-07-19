/**
 * @file tquerystring.h
 * @brief 支持嵌套结构的Web Querystring类，生成和解析方式类似PHP <br />
 *        依赖智能指针库 <br />
 *        注: 解析参数字符串的时候，除最后*[]=...外的所有符合结构将全部解析为Object类型 <br />
 *            数据和key中不允许出现[和]
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author OWenT
 * @date 2012.08.01
 *
 * @history
 *     2014.05.20 增加类似php的rawurlencode和urlencode函数
 *
 */

#ifndef _UTIL_URI_TQUERYSTRING_H_
#define _UTIL_URI_TQUERYSTRING_H_

#include <cstddef>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "std/smart_ptr.h"

namespace util {
    namespace uri {
        /**
         * @brief 编码URI，类似Javascript的encodeURI函数
         * @param [in] content 待编码内容指针
         * @param [in] sz      编码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string encode_uri(const char *content, std::size_t sz = 0);

        /**
         * @brief 解码URI，类似Javascript的decodeURI函数
         * @param [in] uri     待解码内容指针
         * @param [in] sz      解码内容大小（默认当作字符串）
         * @return 解码后的字符串
         */
        std::string decode_uri(const char *uri, std::size_t sz = 0);

        /**
         * @brief 编码并转义URI，类似Javascript的decodeURIComponent函数
         * @param [in] content 待编码内容指针
         * @param [in] sz      编码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string encode_uri_component(const char *content, std::size_t sz = 0);

        /**
         * @brief 解码转义的URI，类似Javascript的encodeURIComponent函数
         * @param [in] uri     待解码内容指针
         * @param [in] sz      解码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string decode_uri_component(const char *uri, std::size_t sz = 0);

        /**
         * @brief 编码并转义URL，类似php的rawurlencode函数
         * 根据RFC 3986 编码字符串
         * @param [in] content 待编码内容指针
         * @param [in] sz      编码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string raw_encode_url(const char *content, std::size_t sz = 0);

        /**
         * @brief 解码转义的URL，类似php的rawurldecode函数
         * @param [in] uri     待解码内容指针
         * @param [in] sz      解码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string raw_decode_url(const char *uri, std::size_t sz = 0);

        /**
         * @brief 编码并转义URL，类似php的urlencode函数
         * 根据 application/x-www-form-urlencoded 编码字符串
         * @param [in] content 待编码内容指针
         * @param [in] sz      编码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string encode_url(const char *content, std::size_t sz = 0);

        /**
         * @brief 解码转义的URL，类似php的urldecode函数
         * @param [in] uri     待解码内容指针
         * @param [in] sz      解码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string decode_url(const char *uri, std::size_t sz = 0);

        /**
         * @brief 字符串转换为任意类型
         * @param [in] str     字符串表示的数据内容
         * @return 任意类型
         */
        template <typename T>
        T query_string_to_any(const char *str) {
            T ret;
            std::stringstream ss;
            ss << str;
            ss >> ret;
            return ret;
        }

        /**
         * @brief 任意类型转换为字符串
         * @param [in] val     任意类型数据
         * @return 对应的字符串
         */
        template <typename T>
        std::string any_to_query_string(const T &val) {
            std::stringstream ss;
            ss << val;
            return ss.str();
        }
    }


    namespace types {
        /**
         * @brief 数据类型枚举，小于ITEM_TYPE_QUERYSTRING的为元类型
         */
        enum ITEM_TYPE { ITEM_TYPE_STRING = 0, ITEM_TYPE_QUERYSTRING = 1, ITEM_TYPE_ARRAY = 2, ITEM_TYPE_OBJECT = 3 };

        /**
         * @brief 数据类型抽象接口
         */
        class item_impl {
        protected:
            item_impl() {}

            /**
             * @brief 添加到字符串
             * @param [in] target   填充目标
             * @param [in] key      key值
             * @param [in] value    value值
             */
            void append_to(std::string &target, const std::string &key, const std::string &value) const;

        public:
            /**
             * @brief 对应数据类型的智能指针类型
             */
            typedef std::shared_ptr<item_impl> ptr_type;

            virtual ~item_impl() {}

            /**
            * @brief 获取是否为空内容
            * @return 是否为空
            */
            virtual bool empty() const = 0;

            /**
             * @brief 获取数据数量
             * @return 数据长度
             */
            virtual std::size_t size() const = 0;

            /**
             * @brief 获取数据类型
             * @return 表示类型的枚举类型
             */
            virtual ITEM_TYPE type() const = 0;

            /**
             * @brief 把数据转为字符串
             * @param [in] prefix 数据项前缀
             * @return 字符串表示
             */
            virtual std::string to_string(const char *prefix = "") const = 0;

            /**
             * @brief 编码并追加目标字符串尾部
             * @param [out] output 输出内容
             * @param [in] prefix 编码内容前缀
             * @return 如果成功，返回true，否则返回false
             */
            virtual bool encode(std::string &output, const char *prefix = "") const = 0;

            /**
             * @brief 数据解码
             * @param [in] keys   key列表
             * @param [in] index    当前解码位置下标
             * @param [in] value 目标值
             * @return 如果成功，返回true，否则返回false
             */
            virtual bool parse(const std::vector<std::string> &keys, std::size_t index, const std::string &value) = 0;
        };

        /**
         * @brief 普通字符串类型
         */
        class item_string : public item_impl {
        protected:
            std::string data_;

        public:
            /**
             * @brief 对应数据类型的智能指针类型
             */
            typedef std::shared_ptr<item_string> ptr_type;

            item_string();

            item_string(const std::string &data);

            virtual ~item_string();

            /**
             * @brief 创建自身类型的实例
             * @return 新实例的智能指针
             */
            static inline ptr_type create() { return std::make_shared<item_string>(); }

            /**
             * @brief 创建自身类型的实例
             * @param [in] data 初始数据
             * @return 新实例的智能指针
             */
            static inline ptr_type create(const std::string &data) { return std::make_shared<item_string>(data); }

            virtual bool empty() const;

            virtual std::size_t size() const;

            virtual types::ITEM_TYPE type() const;

            virtual std::string to_string(const char *prefix = "") const;

            virtual bool encode(std::string &output, const char *prefix = "") const;

            virtual bool parse(const std::vector<std::string> &keys, std::size_t index, const std::string &value);

            inline const std::string& data() const { return data_; }

            /**
             * @breif 类型转换操作
             */
            inline operator std::string() { return get(); };

            /**
             * @breif 兼容赋值操作
             * @param [in] data 原始数据
             * @return 依据等号操作符规则返回自身引用
             */
            inline item_string &operator=(const std::string &data) {
                set(data);
                return (*this);
            };

            /**
             * @breif 设置数据
             * @param [in] data 原始数据
             */
            inline void set(const std::string &data) { data_ = data; };

            /**
             * @breif 获取数据
             * @return 数据内容
             */
            inline std::string &get() { return data_; };
        };

        /**
         * @brief 数组类型
         */
        class item_array : public item_impl {
        protected:
            std::vector<std::shared_ptr<item_impl> > data_;

        public:
            /**
             * @brief 对应数据类型的智能指针类型
             */
            typedef std::shared_ptr<item_array> ptr_type;

            item_array();

            virtual ~item_array();

            /**
             * @brief 创建自身类型的实例
             * @param [in] data 初始数据
             * @return 新实例的智能指针
             */
            static inline ptr_type create() { return std::make_shared<item_array>(); }

            virtual bool empty() const;

            virtual std::size_t size() const;

            virtual types::ITEM_TYPE type() const;

            virtual std::string to_string(const char *prefix = "") const;

            virtual bool encode(std::string &output, const char *prefix = "") const;

            virtual bool parse(const std::vector<std::string> &keys, std::size_t index, const std::string &value);

            /**
             * @breif 依据下标获取数据
             * @param [in] uIndex 下标
             * @return 数据内容的智能指针
             */
            inline std::shared_ptr<item_impl> get(std::size_t uIndex) { return data_[uIndex]; };

            /**
             * @breif 依据下标获取数据的字符串值
             * @param [in] uIndex 下标
             * @return 数据内容的字符串表示
             */
            inline std::string get_string(std::size_t uIndex) const { return data_[uIndex]->to_string(); };

            /**
             * @breif 设置值
             * @param [in] uIndex 下标
             * @param [in] value 值的智能指针
             */
            inline void set(std::size_t uIndex, const std::shared_ptr<item_impl> &value) { data_[uIndex] = value; };

            /**
             * @breif 设置字符串值
             * @param [in] uIndex 下标
             * @param [in] value 字符串值
             */
            inline void set(std::size_t uIndex, const std::string &value) { data_[uIndex] = std::make_shared<item_string>(value); };

            /**
             * @breif 添加值
             * @param [in] uIndex 下标
             * @param [in] value 值的智能指针
             */
            inline void append(const std::shared_ptr<item_impl> &value) { data_.push_back(value); };

            /**
             * @breif 添加字符串值
             * @param [in] uIndex 下标
             * @param [in] value 字符串值
             */
            inline void append(const std::string &value) { data_.push_back(std::make_shared<item_string>(value)); };

            /**
             * @breif 清除最后一项数据
             */
            inline void pop_back() { data_.pop_back(); };

            /**
             * @breif 清空数据
             */
            inline void clear() { data_.clear(); };
        };

        /**
         * @brief 映射类型
         */
        class item_object : public item_impl {
        public:
            typedef std::map<std::string, std::shared_ptr<item_impl> > data_map_t;
            data_map_t data_;
            typedef data_map_t::iterator data_iterator;
            typedef data_map_t::const_iterator data_const_iterator;

        public:
            /**
             * @brief 对应数据类型的智能指针类型
             */
            typedef std::shared_ptr<item_object> ptr_type;

            /**
             * @brief 创建自身类型的实例
             * @param [in] data 初始数据
             * @return 新实例的智能指针
             */
            static inline ptr_type create() { return std::make_shared<item_object>(); }

            item_object();

            virtual ~item_object();

            virtual bool empty() const;

            virtual std::size_t size() const;

            virtual types::ITEM_TYPE type() const;

            virtual std::string to_string(const char *prefix = "") const;

            virtual bool encode(std::string &output, const char *prefix = "") const;

            virtual bool parse(const std::vector<std::string> &keys, std::size_t index, const std::string &value);

            std::vector<std::string> keys() const;

            inline const std::map<std::string, std::shared_ptr<item_impl> >& data() const { return data_; }
            /**
             * @breif 依据Key获取数据
             * @param [in] key Key
             * @return 数据内容的智能指针
             */
            inline std::shared_ptr<item_impl> get(const std::string &key) {
                data_iterator iter = data_.find(key);
                return iter == data_.end() ? std::shared_ptr<item_impl>() : iter->second;
            };

            /**
             * @breif 依据Key获取数据的字符串值
             * @param [in] key Key
             * @return 数据内容的字符串表示
             */
            inline std::string get_string(const std::string &key) const {
                data_const_iterator iter = data_.find(key);
                return iter == data_.end() ? "" : iter->second->to_string();
            };

            /**
             * @breif 添加或设置值
             * @param [in] key Key
             * @param [in] value 值的智能指针
             */
            inline void set(const std::string &key, const std::shared_ptr<item_impl> &value) {
                data_iterator iter = data_.find(key);
                if (iter == data_.end()) {
                    data_.insert(std::make_pair(key, value));
                } else {
                    iter->second = value;
                }
            };

            /**
             * @breif 添加或设置字符串值
             * @param [in] key Key
             * @param [in] value 字符串值
             */
            inline void set(const std::string &key, const std::string &value) { set(key, std::make_shared<item_string>(value)); };

            /**
             * @breif 删除数据
             * @param [in] key Key
             */
            inline void remove(const std::string &key) { data_.erase(key); };

            /**
             * @breif 清空数据
             */
            inline void clear() { data_.clear(); };
        };
    }

    class tquerystring : public types::item_object {
    public:
        typedef types::item_object::data_map_t data_map_t;
        typedef types::item_object::data_iterator data_iterator;
        typedef types::item_object::data_const_iterator data_const_iterator;

    protected:
        std::string spliter_;

        bool decode_record(const char *content, std::size_t sz);

    public:
        /**
         * @brief 对应数据类型的智能指针类型
         */
        typedef std::shared_ptr<tquerystring> ptr_type;

        tquerystring();

        tquerystring(const std::string &spliter);

        virtual ~tquerystring();

        /**
         * @brief 创建自身类型的实例
         * @return 新实例的智能指针
         */
        static inline ptr_type create() { return std::make_shared<tquerystring>(); }

        /**
         * @brief 创建自身类型的实例
         * @param [in] spliter 默认分隔符
         * @return 新实例的智能指针
         */
        static inline ptr_type create(const std::string &spliter) { return std::make_shared<tquerystring>(spliter); }

        virtual bool empty() const;

        virtual std::size_t size() const;

        virtual types::ITEM_TYPE type() const;

        virtual std::string to_string(const char *prefix = "") const;

        virtual bool encode(std::string &output, const char *prefix = "") const;

        /**
         * @breif 解码数据
         * @param [in] content 数据指针
         * @param [in] sz      数据长度
         * @return 成功返回true
         */
        bool decode(const char *content, std::size_t sz = 0);

        /**
         * @breif 根据ID获取数据
         * @param [in] key Key
         * @return 存在返回对应的智能指针，否则返回空智能指针
         */
        std::shared_ptr<types::item_impl> operator[](const std::string &key);

        /**
         * @breif 设置数据分隔符
         * @param [in] spliter 分割符，每个字符都是单独的分隔符
         */
        inline void set_spliter(const std::string &spliter) { spliter_ = spliter; };

        /**
         * @breif 创建字符串实例
         * @return 新实例指针
         */
        static inline types::item_string::ptr_type create_string() { return types::item_string::create(); };

        /**
         * @breif 创建字符串实例
         * @param [in] val 初始值
         * @return 新实例指针
         */
        static inline types::item_string::ptr_type create_string(const std::string &val) { return types::item_string::create(val); };

        /**
         * @breif 创建数组实例
         * @return 新实例指针
         */
        static inline types::item_array::ptr_type create_array() { return types::item_array::create(); };

        /**
         * @breif 创建Object实例
         * @return 新实例指针
         */
        static inline types::item_object::ptr_type create_object() { return types::item_object::create(); };
    };
}

#endif
