// Licensed under the MIT licenses.

#include <cstring>
#include <algorithm>

#include "String/TQueryString.h"

namespace util
{

    namespace uri
    {
        typedef bool uri_map_type[256];
        static uri_map_type g_raw_url_map = { false };
        static uri_map_type g_uri_map = { false };
        static uri_map_type g_uri_component_map = { false };

        // RFC 3986
        static void _init_raw_url_map(uri_map_type& pUriMap)
        {
            if (pUriMap[static_cast<int>('0')])
                return;

            for(int i = 0; i < 26; ++ i)
            {
                pUriMap['a' + i] = pUriMap['A' + i] = true;
            }

            for(int i = 0; i < 10; ++ i)
            {
                pUriMap['0' + i] = true;
            }

            // -_.
            const char strSpec[] = "-_.";
            for (int i = 0; strSpec[i]; ++ i)
            {
                pUriMap[static_cast<int>(strSpec[i])] = true;
            }
        }

        static void _init_uri_component_map(uri_map_type& pUriMap)
        {
            if (pUriMap[static_cast<int>('0')])
                return;

            _init_raw_url_map(pUriMap);

            // -_.!~*'()
            const char strSpec[] = "!~*'()";
            for (int i = 0; strSpec[i]; ++ i)
            {
                pUriMap[static_cast<int>(strSpec[i])] = true;
            }
        }

        static void _init_uri_map(uri_map_type& pUriMap)
        {
            if (pUriMap[static_cast<int>('0')])
                return;

            _init_uri_component_map(pUriMap);
            // ;/?:@&=+$,#
            const char strSpec[] = ";/?:@&=+$,#";
            for (int i = 0; strSpec[i]; ++ i)
            {
                pUriMap[static_cast<int>(strSpec[i])] = true;
            }
        }


        static std::string _encode_uri(
            uri_map_type& pUriMap,
            const char* strData,
            std::size_t uSize,
            bool like_php
        )
        {
            std::string strRet;
            strRet.reserve(uSize);

            while (uSize --)
            {
                if (pUriMap[static_cast<int>(*strData)])
                {
                    strRet += *strData;
                }
                else if (like_php && ' ' == *strData)
                {
                    strRet += '+';
                }
                else
                {
                    strRet += '%';
                    static char strHexMap[16] = {0};
                    // 初始化16进制表
                    if (0 == strHexMap[0])
                    {
                        for (int i = 0; i < 10; i ++)
                        {
                            strHexMap[i] = '0' + i;
                        }

                        for (int i = 10; i < 16; i ++)
                        {
                            strHexMap[i] = 'A' - 10 + i;
                        }
                    }

                    // 转义前4位
                    strRet += strHexMap[*strData >> 4];
                    // 转义后4位
                    strRet += strHexMap[*strData & 0x0F];
                }

                ++strData;
            }

            return strRet;
        }

        static std::string _decode_uri(const char* strData, std::size_t uSize, bool like_php)
        {
            std::string strRet;

            uSize = uSize? uSize: strlen(strData);
            static char strHexMap[256] = {0};

            // 初始化字符表
            if (0 == strHexMap[static_cast<int>('A')])
            {
                for (int i = 0; i < 10; i ++)
                {
                    strHexMap['0' + i] = i;
                }

                for (int i = 10; i < 16; i ++)
                {
                    strHexMap['A' - 10 + i] = strHexMap['a' - 10 + i] = i;
                }
            }

            while (uSize --)
            {
                if (like_php && '+' == *strData)
                {
                    strRet += ' ';
                }
                else if (*strData != '%' || uSize < 2)
                {
                    strRet += *strData;
                }
                else
                {
                    const char &cHigh = strData[1], &cLow = strData[2];
                    strRet += (strHexMap[static_cast<int>(cHigh)]<< 4) + strHexMap[static_cast<int>(cLow)];
                    strData += 2;
                    uSize -= 2;
                }

                ++ strData;
            }

            return strRet;
        }


        std::string EncodeUri(const char* strContent, std::size_t uSize)
        {
            _init_uri_map(g_uri_map);
            uSize = uSize? uSize: strlen(strContent);

            return _encode_uri(g_uri_map, strContent, uSize, false);
        }

        std::string DecodeUri(const char* strUri, std::size_t uSize)
        {
            uSize = uSize? uSize: strlen(strUri);
            return _decode_uri(strUri, uSize, false);
        }

        std::string EncodeUriComponent(const char* strContent, std::size_t uSize)
        {
            _init_uri_component_map(g_uri_component_map);
            uSize = uSize? uSize: strlen(strContent);

            return _encode_uri(g_uri_component_map, strContent, uSize, false);
        }

        std::string DecodeUriComponent(const char* strUri, std::size_t uSize)
        {
            uSize = uSize? uSize: strlen(strUri);
            return _decode_uri(strUri, uSize, false);
        }

        // ==== RFC 3986 ====
        std::string RawEncodeUrl(const char* strContent, std::size_t uSize)
        {
            _init_raw_url_map(g_raw_url_map);
            uSize = uSize? uSize: strlen(strContent);

            return _encode_uri(g_raw_url_map, strContent, uSize, false);
        }

        std::string RawDecodeUrl(const char* strUri, std::size_t uSize)
        {
            uSize = uSize? uSize: strlen(strUri);
            return _decode_uri(strUri, uSize, false);
        }

        // ==== application/x-www-form-urlencoded ====
        std::string EncodeUrl(const char* strContent, std::size_t uSize)
        {
            _init_raw_url_map(g_raw_url_map);
            uSize = uSize? uSize: strlen(strContent);

            return _encode_uri(g_raw_url_map, strContent, uSize, true);
        }

        std::string DecodeUrl(const char* strUri, std::size_t uSize)
        {
            uSize = uSize? uSize: strlen(strUri);
            return _decode_uri(strUri, uSize, true);
        }
    }

    namespace types
    {
        void ItemImpl::appendTo(std::string& strTar, const std::string& key, const std::string& value) const
        {
            strTar += uri::EncodeUriComponent(key.c_str(), key.size()) + "=" + uri::EncodeUriComponent(value.c_str(), value.size()) + "&";
        }

        // 字符串类型
        ItemString::ItemString(){}

        ItemString::ItemString(const std::string& strData): m_strData(strData){}

        ItemString::~ItemString(){}

        std::size_t ItemString::GetSize() const
        {
            return m_strData.size();
        }

        ITEM_TYPE ItemString::GetType() const
        {
            return ITEM_TYPE_STRING;
        }

        std::string ItemString::ToString(const char* strPrefix) const
        {
            return m_strData;
        }

        bool ItemString::parse(const std::vector<std::string>& stKeys, std::size_t index, const std::string& strValue)
        {
            m_strData = strValue;
            return true;
        }

        bool ItemString::Encode(std::string& strOutput, const char* strPrefix) const
        {
            appendTo(strOutput, std::string(strPrefix), m_strData);
            return true;
        }

        // 数组类型
        ItemArray::ItemArray(){}

        ItemArray::~ItemArray() {}

        std::size_t ItemArray::GetSize() const
        {
            return m_stData.size();
        }

        ITEM_TYPE ItemArray::GetType() const
        {
            return ITEM_TYPE_ARRAY;
        }

        std::string ItemArray::ToString(const char* strPrefix) const
        {
            std::string ret = "[";
            for(std::size_t i = 0; i < m_stData.size(); ++ i)
            {
                if (i)
                {
                    ret += ", ";
                }

                ret += m_stData[i]->GetType() < ITEM_TYPE_QUERYSTRING? '\"' + m_stData[i]->ToString(strPrefix) + '\"': m_stData[i]->ToString(strPrefix);
            }

            ret += "]";
            return ret;
        }

        bool ItemArray::parse(const std::vector<std::string>& stKeys, std::size_t index, const std::string& strValue)
        {
            if(index + 1 != stKeys.size() || stKeys[index].size())
            {
                return false;
            }

            Append(strValue);
            return true;
        }

        bool ItemArray::Encode(std::string& strOutput, const char* strPrefix) const
        {
            bool ret = true;
            std::size_t uIndex = 0;
            std::string strNewPrefix, strPrePrefix = strPrefix;
            for(; uIndex < m_stData.size(); ++ uIndex)
            {
                if (m_stData[uIndex]->GetType() >= ITEM_TYPE_QUERYSTRING)
                {
                    break;
                }
            }

            // 不带下标编码
            if (uIndex >= m_stData.size())
            {
                strNewPrefix = strPrePrefix + "[]";
                for(uIndex = 0; uIndex < m_stData.size(); ++ uIndex)
                {
                    ret = m_stData[uIndex]->Encode(strOutput, strNewPrefix.c_str()) && ret;
                }
            }
            else // 带下标编码
            {
                for(uIndex = 0; uIndex < m_stData.size(); ++ uIndex)
                {
                    strNewPrefix = strPrePrefix + '[' + uri::AnyToQueryString(uIndex) + ']';
                    ret = m_stData[uIndex]->Encode(strOutput, strNewPrefix.c_str()) && ret;
                }
            }

            return ret;
        }

        // 映射类型
        ItemObject::ItemObject(){}

        ItemObject::~ItemObject() {}

        std::size_t ItemObject::GetSize() const
        {
            return m_stData.size();
        }

        ITEM_TYPE ItemObject::GetType() const
        {
            return ITEM_TYPE_ARRAY;
        }

        std::string ItemObject::ToString(const char* strPrefix) const
        {
            std::string ret = "{";
            for(data_const_iterator iter = m_stData.begin(); iter != m_stData.end(); ++ iter)
            {
                if (iter != m_stData.begin())
                {
                    ret += ", ";
                }
                ret += '\"' + iter->first + "\": " + 
                    (iter->second->GetType() < ITEM_TYPE_QUERYSTRING? '\"' + iter->second->ToString(strPrefix) + '\"': iter->second->ToString(strPrefix));
            }

            ret += "}";
            return ret;
        }

        bool ItemObject::parse(const std::vector<std::string>& stKeys, std::size_t index, const std::string& strValue)
        {
            data_iterator iter = m_stData.find(stKeys[index]);
            if (iter == m_stData.end())
            {
                types::ItemImpl::ptr_type ptr;
                // 最后一级，字符串类型
                if (index + 1 == stKeys.size())
                {
                    ptr = ItemString::Create();
                }
                // 倒数第二级，且最后一级key为空，数组类型
                else if (index + 2 == stKeys.size() && stKeys.back().size() == 0)
                {
                    ptr = ItemArray::Create();
                }
                // Object类型
                else
                {
                    ptr = ItemObject::Create();
                }

                m_stData.insert(std::make_pair(stKeys[index], ptr));
                return ptr->parse(stKeys, index + 1, strValue);
            }
            else
            {
                return iter->second->parse(stKeys, index + 1, strValue);
            }

            return false;
        }

        bool ItemObject::Encode(std::string& strOutput, const char* strPrefix) const
        {
            bool ret = true;
            std::string strNewPrefix, strPrePrefix = strPrefix;

            for(data_const_iterator iter = m_stData.begin(); iter != m_stData.end(); ++ iter)
            {
                strNewPrefix = strPrePrefix + "[" + iter->first + "]";
                ret = iter->second->Encode(strOutput, strNewPrefix.c_str()) && ret;
            }

            return ret;
        }

        std::vector<std::string> ItemObject::GetKeys() const
        {
            std::vector<std::string> ret;
        
            for(data_const_iterator iter = m_stData.begin(); 
                iter != m_stData.end();
                ++ iter
                )
            {
                ret.push_back(iter->first);
            }

            return ret;
        }

    }

    TQueryString::TQueryString(): m_strSpliter("?#&"){}

    TQueryString::TQueryString(const std::string& strSpliter): m_strSpliter(strSpliter){}

    TQueryString::~TQueryString(){}

    std::size_t TQueryString::GetSize() const
    {
        return m_stData.size();
    }

    types::ITEM_TYPE TQueryString::GetType() const
    {
        return types::ITEM_TYPE_QUERYSTRING;
    }

    std::string TQueryString::ToString(const char* strPrefix) const
    {
        std::string strRet;

        Encode(strRet, strPrefix);

        return strRet;
    }

    bool TQueryString::Decode(const char* strContent, std::size_t uSize)
    {
        bool pDeclMap[256] = { false }, ret = true;
        std::size_t uLen = 0, isDecl;
        uSize = uSize? uSize: strlen(strContent);

        for (std::size_t i = 0; i < m_strSpliter.size(); ++ i)
        {
            pDeclMap[static_cast<int>(m_strSpliter[i])] = true;
        }

        while(uSize)
        {
            for (isDecl = 0, uLen = 0; uLen < uSize; ++ uLen)
            {
                if( pDeclMap[static_cast<int>(*(strContent + uLen))] )
                {
                    isDecl = 1;
                    break;
                }
            }

            ret = decodeRecord(strContent, uLen);

            strContent += uLen + isDecl;
            uSize -= uLen + isDecl;
        }

        return ret;
    }

    bool TQueryString::decodeRecord(const char* strContent, std::size_t uSize)
    {
        std::string strSeg, strValue, strOrigin;
        std::vector<std::string> keys;
        strOrigin.assign(strContent, uSize);

        // 计算值
        std::size_t uValStart = strOrigin.find_last_of('=');
        if (uValStart != strOrigin.npos)
        {
            strValue = strOrigin.substr(uValStart + 1);
            strValue = uri::DecodeUriComponent(strValue.data(), strValue.size());
            strOrigin = strOrigin.substr(0, uValStart);
        }

        strOrigin = uri::DecodeUriComponent(strOrigin.data(), strOrigin.size());

        // 计算key列表
        for(uSize = 0; uSize < strOrigin.size(); ++ uSize)
        {
            while (uSize < strOrigin.size() && strOrigin[uSize] == ']')
            {
                ++ uSize;
            }

            while(uSize < strOrigin.size() && strOrigin[uSize] != '[')
            {
                strSeg += strOrigin[uSize];
                ++ uSize;
            }

            keys.push_back(strSeg);
            strSeg.clear();

            if (uSize >= strOrigin.size())
            {
                break;
            }
            for (++ uSize; uSize < strOrigin.size() && strOrigin[uSize] != ']'; ++ uSize)
            {
                strSeg += strOrigin[uSize];
            }
        }

        if (strSeg.size() > 0)
        {
            keys.push_back(strSeg);
        }

        return parse(keys, 0, strValue);
    }

    bool TQueryString::Encode(std::string& strOutput, const char* strPrefix) const
    {
        data_const_iterator iter = m_stData.begin();

        while(iter != m_stData.end() && 
            iter->second->Encode(strOutput, iter->first.c_str())
            )
        {
            ++ iter;
        }

        if (strOutput.size() > 0 && strOutput[strOutput.size() - 1] == '&')
        {
            strOutput.resize(strOutput.size() - 1);
        }

        return iter == m_stData.end();
    }

    std::shared_ptr<types::ItemImpl> TQueryString::operator[](const std::string& key)
    {
        return Get(key);
    }
}
