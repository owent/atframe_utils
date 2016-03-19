/**
 * @file TQueryString.h
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
#include <vector>
#include <string>
#include <sstream>

#include "std/smart_ptr.h"

namespace util
{
    namespace uri
    {
        /**
         * @brief 编码URI，类似Javascript的encodeURI函数
         * @param [in] strContent 待编码内容指针
         * @param [in] uSize      编码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string EncodeUri(const char* strContent, std::size_t uSize = 0);

        /**
         * @brief 解码URI，类似Javascript的decodeURI函数
         * @param [in] strUri     待解码内容指针
         * @param [in] uSize      解码内容大小（默认当作字符串）
         * @return 解码后的字符串
         */
        std::string DecodeUri(const char* strUri, std::size_t uSize = 0);

        /**
         * @brief 编码并转义URI，类似Javascript的decodeURIComponent函数
         * @param [in] strContent 待编码内容指针
         * @param [in] uSize      编码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string EncodeUriComponent(const char* strContent, std::size_t uSize = 0);

        /**
         * @brief 解码转义的URI，类似Javascript的encodeURIComponent函数
         * @param [in] strUri     待解码内容指针
         * @param [in] uSize      解码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string DecodeUriComponent(const char* strUri, std::size_t uSize = 0);
        
        /**
         * @brief 编码并转义URL，类似php的rawurlencode函数
         * 根据RFC 3986 编码字符串
         * @param [in] strContent 待编码内容指针
         * @param [in] uSize      编码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string RawEncodeUrl(const char* strContent, std::size_t uSize = 0);

        /**
         * @brief 解码转义的URL，类似php的rawurldecode函数
         * @param [in] strUri     待解码内容指针
         * @param [in] uSize      解码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string RawDecodeUrl(const char* strUri, std::size_t uSize = 0);

        /**
         * @brief 编码并转义URL，类似php的urlencode函数
         * 根据 application/x-www-form-urlencoded 编码字符串
         * @param [in] strContent 待编码内容指针
         * @param [in] uSize      编码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string EncodeUrl(const char* strContent, std::size_t uSize = 0);

        /**
         * @brief 解码转义的URL，类似php的urldecode函数
         * @param [in] strUri     待解码内容指针
         * @param [in] uSize      解码内容大小（默认当作字符串）
         * @return 编码后的字符串
         */
        std::string DecodeUrl(const char* strUri, std::size_t uSize = 0);

        /**
         * @brief 字符串转换为任意类型
         * @param [in] str     字符串表示的数据内容
         * @return 任意类型
         */
        template<typename T>
        T QueryStringToAny(const char* str)
        {
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
        template<typename T>
        std::string AnyToQueryString(const T& val)
        {
            std::stringstream ss;
            ss << val;
            return ss.str();
        }
    }


    namespace types
    {
        /**
         * @brief 数据类型枚举，小于ITEM_TYPE_QUERYSTRING的为元类型
         */
        enum ITEM_TYPE
        {
            ITEM_TYPE_STRING        = 0,
            ITEM_TYPE_QUERYSTRING   = 1,
            ITEM_TYPE_ARRAY         = 2,
            ITEM_TYPE_OBJECT        = 3
        };

        /**
         * @brief 数据类型抽象接口
         */
        class ItemImpl
        {
        protected:
            ItemImpl(){}

            /**
             * @brief 添加到字符串
             * @param [in] strTar   填充目标
             * @param [in] key      key值
             * @param [in] value    value值
             */
            void appendTo(std::string& strTar, const std::string& key, const std::string& value) const;

        public:
            /**
             * @brief 对应数据类型的智能指针类型
             */
            typedef std::shared_ptr<ItemImpl> ptr_type;

            virtual ~ItemImpl(){}

            /**
             * @brief 获取数据数量
             * @return 数据长度
             */
            virtual std::size_t GetSize() const = 0;

            /**
             * @brief 获取数据类型
             * @return 表示类型的枚举类型
             */
            virtual ITEM_TYPE GetType() const = 0;

            /**
             * @brief 把数据转为字符串
             * @param [in] strPrefix 数据项前缀
             * @return 字符串表示
             */
            virtual std::string ToString(const char* strPrefix = "") const = 0;

            /**
             * @brief 编码并追加目标字符串尾部
             * @param [out] strOutput 输出内容
             * @param [in] strPrefix 编码内容前缀
             * @return 如果成功，返回true，否则返回false
             */
            virtual bool Encode(std::string& strOutput, const char* strPrefix = "") const = 0;

            /**
             * @brief 数据解码
             * @param [in] stKeys   key列表
             * @param [in] index    当前解码位置下标
             * @param [in] strValue 目标值
             * @return 如果成功，返回true，否则返回false
             */
            virtual bool parse(const std::vector<std::string>& stKeys, std::size_t index, const std::string& strValue) = 0;
        };

        /**
         * @brief 普通字符串类型
         */
        class ItemString: public ItemImpl
        {
        protected:
            std::string m_strData;

        public:
            /**
             * @brief 对应数据类型的智能指针类型
             */
            typedef std::shared_ptr<ItemString> ptr_type;

            ItemString();
            
            ItemString(const std::string& strData);

            virtual ~ItemString();

            /**
             * @brief 创建自身类型的实例
             * @return 新实例的智能指针
             */
            static inline ptr_type Create() { return std::shared_ptr<ItemString>(new ItemString()); }

            /**
             * @brief 创建自身类型的实例
             * @param [in] strData 初始数据
             * @return 新实例的智能指针
             */
            static inline ptr_type Create(const std::string& strData) { return std::shared_ptr<ItemString>(new ItemString(strData)); }

            virtual std::size_t GetSize() const;

            virtual types::ITEM_TYPE GetType() const;

            virtual std::string ToString(const char* strPrefix = "") const;

            virtual bool Encode(std::string& strOutput, const char* strPrefix = "") const;

            virtual bool parse(const std::vector<std::string>& stKeys, std::size_t index, const std::string& strValue);

            /**
             * @breif 类型转换操作
             */
            inline operator std::string() { return Get(); };

            /**
             * @breif 兼容赋值操作
             * @param [in] strData 原始数据
             * @return 依据等号操作符规则返回自身引用
             */
            inline ItemString& operator=(const std::string& strData) { Set(strData); return (*this); };

            /**
             * @breif 设置数据
             * @param [in] strData 原始数据
             */
            inline void Set(const std::string& strData) { m_strData = strData; };

            /**
             * @breif 获取数据
             * @return 数据内容
             */
            inline std::string& Get() { return m_strData; };
        };

        /**
         * @brief 数组类型
         */
        class ItemArray: public ItemImpl
        {
        protected:
            std::vector<std::shared_ptr<ItemImpl> > m_stData;

        public:
            /**
             * @brief 对应数据类型的智能指针类型
             */
            typedef std::shared_ptr<ItemArray> ptr_type;

            ItemArray();

            virtual ~ItemArray();

            /**
             * @brief 创建自身类型的实例
             * @param [in] strData 初始数据
             * @return 新实例的智能指针
             */
            static inline ptr_type Create() { return std::shared_ptr<ItemArray>(new ItemArray()); }

            virtual std::size_t GetSize() const;

            virtual types::ITEM_TYPE GetType() const;

            virtual std::string ToString(const char* strPrefix = "") const;

            virtual bool Encode(std::string& strOutput, const char* strPrefix = "") const;

            virtual bool parse(const std::vector<std::string>& stKeys, std::size_t index, const std::string& strValue);

            /**
             * @breif 依据下标获取数据
             * @param [in] uIndex 下标
             * @return 数据内容的智能指针
             */
            inline std::shared_ptr<ItemImpl> Get(std::size_t uIndex) { return m_stData[uIndex]; };

            /**
             * @breif 依据下标获取数据的字符串值
             * @param [in] uIndex 下标
             * @return 数据内容的字符串表示
             */
            inline std::string GetString(std::size_t uIndex) const { return m_stData[uIndex]->ToString(); };

            /**
             * @breif 设置值
             * @param [in] uIndex 下标
             * @param [in] value 值的智能指针
             */
            inline void Set(std::size_t uIndex, const std::shared_ptr<ItemImpl>& value) { m_stData[uIndex] = value; };

            /**
             * @breif 设置字符串值
             * @param [in] uIndex 下标
             * @param [in] value 字符串值
             */
            inline void Set(std::size_t uIndex, const std::string& value) { m_stData[uIndex] = std::shared_ptr<ItemString>(new ItemString(value)); };

            /**
             * @breif 添加值
             * @param [in] uIndex 下标
             * @param [in] value 值的智能指针
             */
            inline void Append(const std::shared_ptr<ItemImpl>& value) { m_stData.push_back(value); };

            /**
             * @breif 添加字符串值
             * @param [in] uIndex 下标
             * @param [in] value 字符串值
             */
            inline void Append(const std::string& value) { m_stData.push_back(std::shared_ptr<ItemString>(new ItemString(value))); };

            /**
             * @breif 清除最后一项数据
             */
            inline void PopBack() { m_stData.pop_back(); };

            /**
             * @breif 清空数据
             */
            inline void Clear() { m_stData.clear(); };
        };

        /**
         * @brief 映射类型
         */
        class ItemObject: public ItemImpl
        {
        protected:
            std::map<std::string, std::shared_ptr<ItemImpl> > m_stData;
            typedef std::map<std::string, std::shared_ptr<ItemImpl> >::iterator data_iterator;
            typedef std::map<std::string, std::shared_ptr<ItemImpl> >::const_iterator data_const_iterator;

        public:
            /**
             * @brief 对应数据类型的智能指针类型
             */
            typedef std::shared_ptr<ItemObject> ptr_type;

            /**
             * @brief 创建自身类型的实例
             * @param [in] strData 初始数据
             * @return 新实例的智能指针
             */
            static inline ptr_type Create() { return std::shared_ptr<ItemObject>(new ItemObject()); }

            ItemObject();

            virtual ~ItemObject();

            virtual std::size_t GetSize() const;

            virtual types::ITEM_TYPE GetType() const;

            virtual std::string ToString(const char* strPrefix = "") const;

            virtual bool Encode(std::string& strOutput, const char* strPrefix = "") const;

            virtual bool parse(const std::vector<std::string>& stKeys, std::size_t index, const std::string& strValue);

            std::vector<std::string> GetKeys() const;

            /**
             * @breif 依据Key获取数据
             * @param [in] key Key
             * @return 数据内容的智能指针
             */
            inline std::shared_ptr<ItemImpl> Get(const std::string& key) 
            { 
                data_iterator iter = m_stData.find(key);
                return iter == m_stData.end()? std::shared_ptr<ItemImpl>(): iter->second; 
            };

            /**
             * @breif 依据Key获取数据的字符串值
             * @param [in] key Key
             * @return 数据内容的字符串表示
             */
            inline std::string GetString(const std::string& key) const 
            { 
                data_const_iterator iter = m_stData.find(key);
                return iter == m_stData.end()? "": iter->second->ToString(); 
            };

            /**
             * @breif 添加或设置值
             * @param [in] key Key
             * @param [in] value 值的智能指针
             */
            inline void Set(const std::string& key, const std::shared_ptr<ItemImpl>& value) 
            {
                data_iterator iter = m_stData.find(key);
                if (iter == m_stData.end())
                {
                    m_stData.insert(std::make_pair(key, value));
                }
                else
                {
                    iter->second = value;
                }
            };

            /**
             * @breif 添加或设置字符串值
             * @param [in] key Key
             * @param [in] value 字符串值
             */
            inline void Set(const std::string& key, const std::string& value) { Set(key, std::shared_ptr<ItemString>(new ItemString(value))); };

            /**
             * @breif 删除数据
             * @param [in] key Key
             */
            inline void Del(const std::string& key) { m_stData.erase(key); };

            /**
             * @breif 清空数据
             */
            inline void Clear() { m_stData.clear(); };
        };
    }

    class TQueryString: public types::ItemObject
    {
    protected:
        typedef std::map<std::string, std::shared_ptr<types::ItemImpl> >::iterator data_iterator;
        typedef std::map<std::string, std::shared_ptr<types::ItemImpl> >::const_iterator data_const_iterator;

        std::string m_strSpliter;

        bool decodeRecord(const char* strContent, std::size_t uSize);

    public:
        /**
         * @brief 对应数据类型的智能指针类型
         */
        typedef std::shared_ptr<TQueryString> ptr_type;

        TQueryString();

        TQueryString(const std::string& strSpliter);

        virtual ~TQueryString();

        /**
         * @brief 创建自身类型的实例
         * @return 新实例的智能指针
         */
        static inline ptr_type Create() { return std::shared_ptr<TQueryString>(new TQueryString()); }

        /**
         * @brief 创建自身类型的实例
         * @param [in] strSpliter 默认分隔符
         * @return 新实例的智能指针
         */
        static inline ptr_type Create(const std::string& strSpliter) { return std::shared_ptr<TQueryString>(new TQueryString(strSpliter)); }

        virtual std::size_t GetSize() const;
        
        virtual types::ITEM_TYPE GetType() const;

        virtual std::string ToString(const char* strPrefix = "") const;

        virtual bool Encode(std::string& strOutput, const char* strPrefix = "") const;

        /**
         * @breif 解码数据
         * @param [in] strContent 数据指针
         * @param [in] uSize      数据长度
         * @return 成功返回true
         */
        bool Decode(const char* strContent, std::size_t uSize = 0);

        /**
         * @breif 根据ID获取数据
         * @param [in] key Key
         * @return 存在返回对应的智能指针，否则返回空智能指针
         */
        std::shared_ptr<types::ItemImpl> operator[](const std::string& key);

        /**
         * @breif 设置数据分隔符
         * @param [in] strSpliter 分割符，每个字符都是单独的分隔符
         */
        inline void SetSpliter(const std::string& strSpliter) { m_strSpliter = strSpliter; };

        /**
         * @breif 创建字符串实例
         * @return 新实例指针
         */
        static inline types::ItemString::ptr_type CreateString() { return types::ItemString::Create(); };

        /**
         * @breif 创建字符串实例
         * @param [in] val 初始值
         * @return 新实例指针
         */
        static inline types::ItemString::ptr_type CreateString(const std::string& val) { return types::ItemString::Create(val); };

        /**
         * @breif 创建数组实例
         * @return 新实例指针
         */
        static inline types::ItemArray::ptr_type CreateArray() { return types::ItemArray::Create(); };

        /**
         * @breif 创建Object实例
         * @return 新实例指针
         */
        static inline types::ItemObject::ptr_type CreateObject() { return types::ItemObject::Create(); };
    };

}

#endif
