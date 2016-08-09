/**
 * @file ac_automation.h
 * @brief AC 自动机算法实现
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2016.04.22
 *
 * @history
 *
 */

#ifndef _UTIL_STRING_AC_AUTOMATION_H_
#define _UTIL_STRING_AC_AUTOMATION_H_

#pragma once

#include "std/smart_ptr.h"
#include <algorithm>
#include <assert.h>
#include <cstddef>
#include <cstring>
#include <list>
#include <map>
#include <stdint.h>
#include <string>
#include <vector>

namespace util {
    namespace string {
        struct utf8_char_t {
            utf8_char_t(const char* str) {
                size_t len = length(str);
                for (size_t i = 0; i < len; ++i) {
                    data[i] = str[i];
                }
            };
            utf8_char_t(const std::string& str) {
                size_t len = length(str.c_str());
                for (size_t i = 0; i < len; ++i) {
                    data[i] = str[i];
                }
            };

            char data[8];

            inline size_t length(const char* s) const {
                size_t ret = 1;
                char c = (*s);

                if (!(c & 0x80)) {
                    return ret;
                }
                c <<= 1;
                
                for (; ret < 6; ++ret) {
                    if (!(c & 0x80)) {
                        break;
                    }
                }

                return ret;
            }

            inline size_t length() const {
                return length(&data[0]);
            }

            friend bool operator==(const utf8_char_t& l, const utf8_char_t& r) {
                size_t len = l.length();

                if (l.length() != r.length()) {
                    return false;
                }
                
                for (size_t i = 0; i < len; ++i) {
                    if (l.data[i] != r.data[i]) {
                        return false;
                    }
                }

                return true;
            }

            friend bool operator!=(const utf8_char_t& l, const utf8_char_t& r) {
                return !(l == r);
            }

            friend bool operator<(const utf8_char_t& l, const utf8_char_t& r) {
                size_t len = l.length();

                if (l.length() != r.length()) {
                    return l.length() < r.length();
                }

                for (size_t i = 0; i < len; ++i) {
                    if (l.data[i] != r.data[i]) {
                        return l.data[i] < r.data[i];
                    }
                }

                return false;
            }

            friend bool operator<=(const utf8_char_t& l, const utf8_char_t& r) {
                size_t len = l.length();

                if (l.length() != r.length()) {
                    return l.length() < r.length();
                }

                for (size_t i = 0; i < len; ++i) {
                    if (l.data[i] != r.data[i]) {
                        return l.data[i] <= r.data[i];
                    }
                }

                return true;
            }

            friend bool operator>(const utf8_char_t& l, const utf8_char_t& r) {
                return !(l <= r);
            }

            friend bool operator>=(const utf8_char_t& l, const utf8_char_t& r) {
                return !(l < r);
            }
        };

        namespace detail {
            template <typename CH>
            class actrie_skip_charset {
            public:
                actrie_skip_charset() { memset(skip_code_, 0, sizeof(skip_code_)); }

                void set(CH c) {
                    size_t index = c / 8;
                    size_t offset = c % 8;
                    skip_code_[index] |= 1 << offset;
                }

                void unset(CH c) {
                    size_t index = c / 8;
                    size_t offset = c % 8;
                    skip_code_[index] &= ~(1 << offset);
                }

                bool test(CH c) const {
                    size_t index = c / 8;
                    size_t offset = c % 8;
                    return 0 != (skip_code_[index] & (1 << offset));
                }

                inline bool operator[](CH c) const { return test(c); }

            private:
                uint8_t skip_code_[(static_cast<size_t>(1) << (sizeof(CH) * 8)) / 8];
            };

            template <typename CH = char>
            class actrie : public std::enable_shared_from_this<actrie<CH> > {
            public:
                typedef CH char_t;
                typedef std::basic_string<char_t> string_t;
                typedef actrie<char_t> self_t;
                typedef std::shared_ptr<self_t> ptr_type;

                struct match_result_t {
                    size_t start;
                    size_t length;
                    const self_t *keyword;
                    bool has_skip;
                };

            private:
                /**
                 * 关联的匹配字符串<br />
                 * size不为0表示该节点有关联的字符串并且是最后一个节点
                 */
                string_t matched_string_;

                /**
                 * 失败转向节点
                 */
                ptr_type failed_;

                /**
                 * 下一个查找项
                 */
                std::map<char_t, ptr_type> next_;

                /**
                 * 初始化自身和子节点的失败指针
                 * @param pre_failed 初始搜索的指针（一般为父节点的失败指针）
                 * @param ch 搜索的字符
                 */
                void _init_failed(ptr_type pre_failed, const char_t &ch) {
                    typedef typename std::map<char_t, ptr_type>::iterator iter_type;

                    // 设置自身的失败指针
                    iter_type iter;
                    for (;; pre_failed = pre_failed->failed_) {
                        iter = pre_failed->next_.find(ch);
                        if (iter != pre_failed->next_.end()) {
                            failed_ = iter->second;
                            break;
                        }

                        if (NULL == pre_failed->failed_.get()) {
                            failed_ = pre_failed;
                            break;
                        }
                    }
                }

                /**
                 * 把子节点填充到链表中（用于BFS）<br />
                 * 调用此函数时，当前节点的失败指针必须已经设置好
                 * @param ls 填充目标
                 */
                void _fill_children(std::list<std::pair<char_t, ptr_type> > &ls) {
                    typedef typename std::map<char_t, ptr_type>::iterator iter_type;
                    for (iter_type iter = next_.begin(); iter != next_.end(); ++iter) {
                        iter->second->failed_ = failed_; // 临时用于记录父节点的失败指针
                        ls.push_back(std::make_pair(iter->first, iter->second));
                    }
                }

                /**
                 * 获取当前指针
                 * @return 当前对象的智能指针
                 */
                ptr_type _get_ptr() { return this->shared_from_this(); }

            public:
                actrie(ptr_type pRoot) : failed_(pRoot) {}

                /**
                 * 设置失败指针
                 * @param pFailed 失败指针
                 */
                void set_failed(ptr_type pFailed) { failed_ = pFailed; }

                /**
                 * 初始化根节点中，子节点的失败指针<br />
                 * 当前节点会被视为根节点
                 */
                void init_failed() {
                    failed_.reset();
                    std::list<std::pair<char_t, ptr_type> > ls;

                    typedef typename std::map<char_t, ptr_type>::iterator iter_type;

                    // 第一层节点
                    for (iter_type iter = next_.begin(); iter != next_.end(); ++iter) {
                        iter->second->failed_ = _get_ptr();
                        iter->second->_fill_children(ls);
                    }

                    // 后续节点 BFS 建树
                    while (ls.size() > 0) {
                        std::pair<char_t, ptr_type> node = ls.front();
                        ls.pop_front();
                        node.second->_init_failed(node.second->failed_, node.first);
                        node.second->_fill_children(ls);
                    }
                }

                /**
                 * 清空后续分支
                 */
                void reset() { next_.clear(); }

                /**
                 * 当前节点是否是一个关键字的最后一个节点(叶子节点)
                 * @return 如果是返回true
                 */
                bool is_leaf() const { return matched_string_.size() > 0; }

                /**
                 * 获取叶子节点的字符串
                 * @return 叶子节点的字符串
                 */
                const string_t &get_leaf() const { return matched_string_; }

                /**
                 * 构建关键字的字典树节点
                 * @param str          当前字符指针
                 * @param left_sz      关键字剩余字符数
                 * @param origin_val   关键字原始内容
                 */
                void insert(const char_t *str, size_t left_sz, const string_t &origin_val) {
                    // 最后一个节点
                    if (0 >= left_sz) {
                        matched_string_.assign(origin_val.data(), origin_val.size());
                        return;
                    }

                    --left_sz;

                    typedef typename std::map<char_t, ptr_type>::iterator iter_type;
                    iter_type iter = next_.find(*str);
                    if (iter != next_.end()) {
                        iter->second->insert(str + 1, left_sz, origin_val);
                        return;
                    }

                    std::pair<iter_type, bool> iter_new = next_.insert(std::make_pair(*str, ptr_type(new actrie<char_t>(failed_))));
                    assert(iter_new.second);

                    iter_new.first->second->insert(str + 1, left_sz, origin_val);
                }

                /**
                 * 匹配目标字符
                 * @param out 输出匹配数据
                 * @param chp 目标字符指针
                 * @param left_sz 剩余字符数
                 * @param skip 可跳过字符集
                 * @return 匹配完成后剩余字节数
                 */
                template <typename TSkipSet>
                size_t match(match_result_t &out, const char_t *chp, size_t left_sz, const TSkipSet *skip) const {
                    // 成功匹配
                    if (is_leaf()) {
                        out.keyword = this;
                        return left_sz;
                    }

                    // 已到目标串目末尾，无匹配
                    if (left_sz <= 0) {
                        out.keyword = NULL;
                        return left_sz;
                    }

                    // 匹配下一项
                    typedef typename std::map<char_t, ptr_type>::const_iterator iter_type;
                    iter_type iter = next_.find(*chp);
                    if (iter != next_.end()) {
                        return iter->second->match(out, chp + 1, left_sz - 1, skip);
                    }

                    // 忽略字符，直接往后匹配
                    if (NULL != skip && (*skip)[*chp]) {
                        out.has_skip = true;
                        return match(out, chp + 1, left_sz - 1, skip);
                    }

                    // 如果是root节点，放弃匹配
                    if (!failed_) {
                        return left_sz - 1;
                        // return match(out, chp + 1, left_sz - 1, skip);
                    }

                    // 如果失败节点是根节点，放弃匹配，退栈
                    // 这里只是个优化，尽快退栈，防止调用栈过深
                    // 走到这里则至少已经消费了一个节点，left_sz不会再和前面相同
                    if (!failed_->failed_) {
                        return left_sz;
                    }

                    // 否则, failed节点进行匹配
                    return failed_->match(out, chp, left_sz, skip);
                }

                /**
                 * 查找匹配项的开始位置
                 * @param str 字符串起始地址
                 * @param end_sz 字符串结束位置
                 */
                size_t find_start(const char_t *str, size_t end_sz) const {
                    if (end_sz <= 0) {
                        return 0;
                    }

                    size_t ret = end_sz - 1;
                    // 匹配错误，直接返回
                    if (matched_string_.empty() || str[ret] != *matched_string_.rbegin()) {
                        return end_sz;
                    }

                    size_t midx = matched_string_.size() - 1;
                    for (; ret > 0; --ret) {
                        if (matched_string_[midx] == str[ret]) {
                            // 修正匹配完成
                            if (0 == midx) {
                                break;
                            }

                            --midx;
                        }
                    };

                    return ret;
                }
            };
        }


        template <typename CH = char, typename TSKIP = detail::actrie_skip_charset<CH> >
        class ac_automation {
        public:
            typedef CH char_t;
            typedef typename detail::actrie<char_t> trie_type;
            typedef typename trie_type::string_t string_t;
            typedef TSKIP skip_set_t;

            struct match_t {
                size_t start;
                size_t length;
                const string_t *keyword;
            };
            typedef std::vector<match_t> value_type;

        private:
            /**
             * 根节点(空节点)
             */
            std::shared_ptr<detail::actrie<char_t> > root_;

            /**
             * 忽略的特殊字符
             */
            skip_set_t skip_charset_;

            bool is_inited_;
            bool is_no_case_;

            /**
             * 初始化字典树的失败指针
             */
            void init() {
                if (is_inited_) return;

                root_->init_failed();

                is_inited_ = true;
            }

        public:
            ac_automation()
                : root_(new detail::actrie<char_t>(std::shared_ptr<detail::actrie<char_t> >())), is_inited_(false), is_no_case_(false) {
                // 临时的自环
                root_->set_failed(root_);
            }

            ~ac_automation() {
                // 解除自环，防止内存泄漏
                root_->set_failed(std::shared_ptr<detail::actrie<char_t> >());
            }

            /**
             * @brief 获取是否已经初始化过（已建立索引）
             * @return 是否已初始化过
             */
            inline bool is_inited() const { return is_inited_; }

            /**
             * 增加关键字
             * @param keyword 关键字字符串
             */
            void insert_keyword(const string_t &keyword) {
                if (keyword.empty()) {
                    return;
                }

                is_inited_ = false;

                if (is_no_case_) {
                    string_t res = keyword;
                    std::transform(res.begin(), res.end(), res.begin(), ::tolower);
                    root_->insert(res.c_str(), res.size(), keyword);
                } else {
                    root_->insert(keyword.c_str(), keyword.size(), keyword);
                }
            }

            /**
             * 匹配目标串，返回匹配结果
             * @param content 目标字符串
             * @return 返回的结果列表,返回结果的first为开始位置，second为匹配的关键字
             */
            value_type match(const string_t &content) {
                value_type ret;
                if (content.empty()) {
                    return ret;
                }
                const string_t *conv_content = &content;
                string_t nocase;
                if (is_no_case_) {
                    nocase = content;
                    std::transform(nocase.begin(), nocase.end(), nocase.begin(), ::tolower);
                    conv_content = &nocase;
                }

                init();
                size_t sz = conv_content->size();
                size_t left = sz;
                const char_t *end = conv_content->data() + sz;

                while (left > 0) {
                    typename trie_type::match_result_t mres;
                    mres.start = 0;
                    mres.length = 0;
                    mres.keyword = NULL;
                    mres.has_skip = false;

                    size_t res = root_->match(mres, end - left, left, &skip_charset_);
                    if (NULL != mres.keyword && mres.keyword->is_leaf()) {
                        ret.push_back(match_t());
                        match_t &item = ret.back();
                        item.keyword = &mres.keyword->get_leaf();
                        if (mres.has_skip) {
                            item.start = mres.keyword->find_start(conv_content->data(), sz - res);
                        } else {
                            item.start = sz - res - mres.keyword->get_leaf().size();
                        }
                        item.length = sz - item.start - res;
                    }

                    assert(0 == left || left > res);
                    left = res;
                }

                return ret;
            }

            /**
             * 清空关键字列表
             */
            void reset() { root_->reset(); }

            /**
             * 设置忽略字符
             */
            void set_skip(char_t c) { skip_charset_.set(c); }

            /**
             * 取消设置忽略字符
             */
            void unset_skip(char_t c) { skip_charset_.unset(c); }

            /**
             * 设置是否忽视大小写
             * @note 必须在insert_keyword前调用
             */
            void set_nocase(bool v) { is_no_case_ = v; }

            /**
             * 获取是否忽视大小写
             */
            bool is_nocase() const { return is_no_case_; }
        };
    }
}

#endif
