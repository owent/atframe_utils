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

#ifndef UTIL_STRING_AC_AUTOMATION_H
#define UTIL_STRING_AC_AUTOMATION_H

#pragma once

#include <assert.h>
#include <algorithm>
#include <list>
#include <map>
#include <set>
#include <vector>

#include "string/utf8_char_t.h"

#include <config/atframe_utils_build_feature.h>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace string {
namespace detail {
template <typename TC, typename TCTT>
LIBATFRAME_UTILS_API_HEAD_ONLY void write_varint(std::basic_ostream<TC, TCTT> &os, uint32_t i) {
  unsigned char c;
  if (i < (1U << 6)) {
    c = static_cast<unsigned char>(i & 0x3F);
    os << c;
  } else if (i < (1U << 14)) {
    c = static_cast<unsigned char>((i >> 8) & 0x3F) | 0x40;
    os << c;
    c = static_cast<unsigned char>(i & 0xFF);
    os << c;
  } else if (i < (1U << 22)) {
    c = static_cast<unsigned char>((i >> 16) & 0x3F) | 0x80;
    os << c;
    c = static_cast<unsigned char>((i >> 8) & 0xFF);
    os << c;
    c = static_cast<unsigned char>(i & 0xFF);
    os << c;
  } else if (i < (1U << 30)) {
    c = static_cast<unsigned char>((i >> 24) & 0x3F) | 0xC0;
    os << c;
    c = static_cast<unsigned char>((i >> 16) & 0xFF);
    os << c;
    c = static_cast<unsigned char>((i >> 8) & 0xFF);
    os << c;
    c = static_cast<unsigned char>(i & 0xFF);
    os << c;
  } else {
    // too many nodes
    abort();
  }
}

template <typename TC, typename TCTT>
LIBATFRAME_UTILS_API_HEAD_ONLY uint32_t read_varint(std::basic_istream<TC, TCTT> &is) {
  unsigned char c;
  if (!is >> c) {
    return 0;
  }

  uint32_t ret = 0;
  ret = c & 0x3F;
  for (unsigned char n = (c >> 6) & 0x03; n > 0; --n) {
    ret <<= 8;
    if (is >> c) {
      ret |= c;
    }
  }

  return ret;
}

template <typename CH, size_t CHSZ>
class LIBATFRAME_UTILS_API_HEAD_ONLY actrie_skip_charset;

template <typename CH>
class LIBATFRAME_UTILS_API_HEAD_ONLY actrie_skip_charset<CH, 1> {
 public:
  using self_t = actrie_skip_charset<CH, 1>;
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

  template <typename OCH, typename OTCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY friend std::basic_ostream<OCH, OTCTT> &operator<<(std::basic_ostream<OCH, OTCTT> &os,
                                                                                   const self_t &self) {
    os.write((CH *)self.skip_code_, sizeof(self.skip_code_));
    return os;
  }

  template <typename OCH, typename OTCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY friend std::basic_istream<OCH, OTCTT> &operator>>(std::basic_istream<OCH, OTCTT> &is,
                                                                                   self_t &self) {
    is.read((CH *)self.skip_code_, sizeof(self.skip_code_));
    return is;
  }

 private:
  uint8_t skip_code_[(static_cast<size_t>(1) << (sizeof(CH) * 8)) / 8];
};

template <typename CH, size_t CHSZ>
class LIBATFRAME_UTILS_API_HEAD_ONLY actrie_skip_charset {
 public:
  using self_t = actrie_skip_charset<CH, CHSZ>;
  actrie_skip_charset() {}

  void set(CH c) { skip_code_.insert(c); }

  void unset(CH c) { skip_code_.erase(c); }

  bool test(CH c) const { return skip_code_.end() != skip_code_.find(c); }

  inline bool operator[](CH c) const { return test(c); }

  template <typename OCH, typename OTCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY friend std::basic_ostream<OCH, OTCTT> &operator<<(std::basic_ostream<OCH, OTCTT> &os,
                                                                                   const self_t &self) {
    write_varint(os, static_cast<uint32_t>(self.skip_code_.size()));
    for (typename std::set<CH>::iterator iter = self.skip_code_.begin(); iter != self.skip_code_.end(); ++iter) {
      os << *iter;
    }
    return os;
  }

  template <typename OCH, typename OTCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY friend std::basic_istream<OCH, OTCTT> &operator>>(std::basic_istream<OCH, OTCTT> &is,
                                                                                   self_t &self) {
    uint32_t sz = read_varint(is);
    self.skip_code_.clear();
    for (uint32_t i = 0; i < sz; ++i) {
      CH c;
      if (is >> c) {
        self.skip_code_.insert(c);
      } else {
        break;
      }
    }
    return is;
  }

 private:
  std::set<CH> skip_code_;
};

template <typename CH>
size_t LIBATFRAME_UTILS_API_HEAD_ONLY actrie_get_length(const CH &c) {
  return sizeof(c);
}

LIBATFRAME_UTILS_API_HEAD_ONLY size_t actrie_get_length(const utf8_char_t &c) { return c.length(); }

template <typename CH = char>
class LIBATFRAME_UTILS_API_HEAD_ONLY actrie {
 public:
  using char_t = CH;
  using string_t = std::string;
  using self_t = actrie<char_t>;
  using ptr_type = std::shared_ptr<self_t>;
  using storage_t = std::vector<ptr_type>;

  struct match_result_t {
    size_t start;
    size_t length;
    const self_t *keyword;
    bool has_skip;
  };

 private:
  /**
   * id
   */
  uint32_t idx_;

  /**
   * 失败转向节点
   */
  uint32_t failed_;

  /**
   * 关联的匹配字符串<br />
   * size不为0表示该节点有关联的字符串并且是最后一个节点
   */
  string_t matched_string_;

  /**
   * 下一个查找项
   */
  std::map<char_t, uint32_t> next_;

  /**
   * 初始化自身和子节点的失败节点ID
   * @param pre_failed 初始搜索的节点ID（一般为父节点的失败节点ID）
   * @param ch 搜索的字符
   */
  void _init_failed(storage_t &storage, uint32_t pre_failed, const char_t &ch) {
    assert(pre_failed < storage.size());
    using iter_type = typename std::map<char_t, uint32_t>::iterator;

    // 设置自身的失败节点ID
    iter_type iter;
    for (;; pre_failed = storage[pre_failed]->failed_) {
      iter = storage[pre_failed]->next_.find(ch);
      if (iter != storage[pre_failed]->next_.end()) {
        failed_ = iter->second;
        break;
      }

      if (0 == storage[pre_failed]->failed_) {
        failed_ = pre_failed;
        break;
      }
    }
  }

  /**
   * 把子节点填充到链表中（用于BFS）<br />
   * 调用此函数时，当前节点的失败节点ID必须已经设置好
   * @param ls 填充目标
   */
  void _fill_children(storage_t &storage, std::list<std::pair<char_t, uint32_t> > &ls) {
    using iter_type = typename std::map<char_t, uint32_t>::iterator;
    for (iter_type iter = next_.begin(); iter != next_.end(); ++iter) {
      assert(iter->second < storage.size());
      storage[iter->second]->failed_ = failed_;  // 临时用于记录父节点的失败节点ID
      ls.push_back(std::make_pair(iter->first, iter->second));
    }
  }

  static inline bool equal_char(char l, char r) { return l == r; }

  template <typename OTC, typename OTCTT>
  static LIBATFRAME_UTILS_API_HEAD_ONLY inline void output_char(char l, std::basic_ostream<OTC, OTCTT> &os) {
    if (l > 0x21 && l <= 0x7f) {
      os << l;
    } else {
      char hex_val[3] = {0};
      LIBATFRAME_UTILS_NAMESPACE_ID::string::hex(hex_val, l);
      os << "0x" << hex_val;
    }
  }

  static LIBATFRAME_UTILS_API_HEAD_ONLY inline bool equal_char(unsigned char l, char r) {
    return l == static_cast<unsigned char>(r);
  }

  template <typename OTC, typename OTCTT>
  static LIBATFRAME_UTILS_API_HEAD_ONLY inline void output_char(unsigned char l, std::basic_ostream<OTC, OTCTT> &os) {
    if (l > 0x21 && l <= 0x7f) {
      os << l;
    } else {
      char hex_val[3] = {0};
      LIBATFRAME_UTILS_NAMESPACE_ID::string::hex(hex_val, l);
      os << "0x" << hex_val;
    }
  }

  static inline bool equal_char(utf8_char_t l, char r) { return l == r; }

  template <typename OTC, typename OTCTT>
  static LIBATFRAME_UTILS_API_HEAD_ONLY inline void output_char(utf8_char_t l, std::basic_ostream<OTC, OTCTT> &os) {
    os << l;
  }

  template <typename TC>
  static LIBATFRAME_UTILS_API_HEAD_ONLY inline bool equal_char(TC, char) {
    return false;
  }

  template <typename TC, typename OTC, typename OTCTT>
  static LIBATFRAME_UTILS_API_HEAD_ONLY inline void output_char(TC l, std::basic_ostream<OTC, OTCTT> &os) {
    os << l;
  }

 private:
  actrie(storage_t &, uint32_t failed_idx = 0) : idx_(0), failed_(failed_idx) {}

  struct protect_constructor_helper {};

 public:
  actrie(protect_constructor_helper, storage_t &, uint32_t failed_idx = 0) : idx_(0), failed_(failed_idx) {}

  inline uint32_t get_idx() const { return idx_; }

  /**
   * 创建actrie树节点
   * @param storage 失败节点ID
   * @param failed_idx 失败节点ID
   */
  static ptr_type create(storage_t &storage, uint32_t failed_idx = 0) {
    ptr_type ret = std::make_shared<self_t>(protect_constructor_helper(), storage, failed_idx);
    if (!ret) {
      return ret;
    }

    ret->idx_ = static_cast<uint32_t>(storage.size());
    storage.push_back(ret);
    return ret;
  }

  template <typename TC, typename TCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump(std::basic_ostream<TC, TCTT> &os) const {
    write_varint(os, idx_);
    write_varint(os, failed_);
    write_varint(os, static_cast<uint32_t>(matched_string_.size()));
    if (!matched_string_.empty()) {
      os.write(matched_string_.data(), matched_string_.size());
    }

    write_varint(os, static_cast<uint32_t>(next_.size()));
    using iter_type = typename std::map<char_t, uint32_t>::const_iterator;
    for (iter_type iter = next_.begin(); iter != next_.end(); ++iter) {
      os << iter->first;
      write_varint(os, iter->second);
    }
  }

  template <typename TC, typename TCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY bool load(std::basic_istream<TC, TCTT> &is) {
    idx_ = read_varint(is);
    failed_ = read_varint(is);
    uint32_t mstr_sz = read_varint(is);
    if (mstr_sz > 0) {
      matched_string_.resize(mstr_sz);
      is.readsome(&matched_string_[0], mstr_sz);
    }

    next_.clear();
    uint32_t next_sz = read_varint(is);
    for (uint32_t i = 0; i < next_sz; ++i) {
      std::pair<char_t, uint32_t> p;
      is >> p.first;
      p.second = (is);
    }

    return true;
  }

  template <typename TC, typename TCTT>
  static LIBATFRAME_UTILS_API_HEAD_ONLY inline std::basic_ostream<TC, TCTT> &dump_dot_node_name(
      std::basic_ostream<TC, TCTT> &os, uint32_t idx) {
    return os << "char_" << idx;
  }

  template <typename TC, typename TCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump_dot_node(std::basic_ostream<TC, TCTT> &os) const {
    dump_dot_node_name(os, get_idx());
    if (!matched_string_.empty()) {
      os << " [label=\"";
      for (size_t i = 0; i < matched_string_.size(); ++i) {
        if (equal_char(matched_string_[i], '"')) {
          os << "\\\"";
        } else {
          os << matched_string_[i];
        }
      }
      os << "\"];" << std::endl;
    } else {
      os << " [label=\"" << get_idx() << "\"];" << std::endl;
    }
  }

  template <typename TC, typename TCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump_dot_relationship(std::basic_ostream<TC, TCTT> &os) const {
    // fail node
    if (0 != failed_) {
      dump_dot_node_name(os, get_idx());
      os << " -> ";
      dump_dot_node_name(os, failed_);
      os << " [color=red];" << std::endl;
    }

    using iter_type = typename std::map<char_t, uint32_t>::const_iterator;
    for (iter_type iter = next_.begin(); iter != next_.end(); ++iter) {
      if (iter->second > 0) {
        dump_dot_node_name(os, get_idx());
        os << " -> ";
        dump_dot_node_name(os, iter->second);
        os << " [style=bold,label=\"";
        if (equal_char(iter->first, '"')) {
          os << "\\\"";
        } else {
          output_char(iter->first, os);
        }

        os << "\"];" << std::endl;
      }
    }
  }

  /**
   * 设置失败节点ID
   * @param pFailed 失败节点ID
   */
  void set_failed(uint32_t idx) { failed_ = idx; }

  /**
   * 初始化根节点中，子节点的失败节点ID<br />
   * 当前节点会被视为根节点
   */
  void init_failed(storage_t &storage) {
    failed_ = 0;
    std::list<std::pair<char_t, uint32_t> > ls;

    using iter_type = typename std::map<char_t, uint32_t>::iterator;

    // 第一层节点
    for (iter_type iter = next_.begin(); iter != next_.end(); ++iter) {
      assert(iter->second < storage.size());
      if (iter->second < storage.size()) {
        storage[iter->second]->failed_ = get_idx();
        storage[iter->second]->_fill_children(storage, ls);
      }
    }

    // 后续节点 BFS 建树
    while (!ls.empty()) {
      std::pair<char_t, uint32_t> node = ls.front();
      ls.pop_front();

      assert(node.second < storage.size());
      if (node.second < storage.size()) {
        storage[node.second]->_init_failed(storage, storage[node.second]->failed_, node.first);
        storage[node.second]->_fill_children(storage, ls);
      }
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
   * @param storage      actrie节点存储区
   * @param str          当前字符指针
   * @param left_sz      关键字剩余字符数
   * @param origin_val   关键字原始内容
   */
  void insert(storage_t &storage, const char *str, size_t left_sz, const string_t &origin_val) {
    // 最后一个节点
    if (0 >= left_sz) {
      matched_string_.assign(origin_val.data(), origin_val.size());
      return;
    }

    char_t c(*str);
    size_t csz = actrie_get_length(c);
    if (csz > left_sz) {
      csz = left_sz;
    }
    left_sz -= csz;

    using iter_type = typename std::map<char_t, uint32_t>::iterator;
    iter_type iter = next_.find(c);
    if (iter != next_.end() && iter->second < storage.size()) {
      storage[iter->second]->insert(storage, str + csz, left_sz, origin_val);
      return;
    }

    ptr_type node = create(storage, failed_);
    assert(!!node);

    std::pair<iter_type, bool> iter_new(next_.insert(std::make_pair(c, node->get_idx())));
    assert(iter_new.second);

    storage[iter_new.first->second]->insert(storage, str + csz, left_sz, origin_val);
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
  LIBATFRAME_UTILS_API_HEAD_ONLY size_t match(storage_t &storage, match_result_t &out, const char *chp, size_t left_sz,
                                              const TSkipSet *skip) const {
    // 成功匹配
    if (is_leaf()) {
      out.keyword = this;
      return left_sz;
    }

    // 已到目标串目末尾，无匹配
    if (left_sz <= 0) {
      out.keyword = nullptr;
      return left_sz;
    }

    char_t c(*chp);
    size_t csz = actrie_get_length(c);
    if (csz > left_sz) {
      csz = left_sz;
    }

    // 匹配下一项
    using iter_type = typename std::map<char_t, uint32_t>::const_iterator;
    iter_type iter = next_.find(c);
    // 如果是root节点或者无效节点，放弃匹配
    if (iter != next_.end() && 0 != iter->second && iter->second < storage.size()) {
      return storage[iter->second]->match(storage, out, chp + csz, left_sz - csz, skip);
    }

    // 忽略字符，直接往后匹配
    if (nullptr != skip && (*skip)[c]) {
      out.has_skip = true;
      return match(storage, out, chp + csz, left_sz - csz, skip);
    }

    // 如果是root节点，放弃匹配
    if (0 == get_idx()) {
      return left_sz - csz;
      // return match(storage, out, chp + csz, left_sz - csz, skip);
    }

    // 如果失败节点是根节点，放弃匹配，退栈
    // 这里只是个优化，尽快退栈，防止调用栈过深
    // 走到这里则至少已经消费了一个节点，left_sz不会再和前面相同
    if (0 == storage[failed_]->failed_) {
      return left_sz;
    }

    // 否则, failed节点进行匹配
    return storage[failed_]->match(storage, out, chp, left_sz, skip);
  }

  /**
   * 查找匹配项的开始位置
   * @param str 字符串起始地址
   * @param end_sz 字符串结束位置
   */
  size_t find_start(const char *str, size_t end_sz) const {
    if (end_sz <= 0 || nullptr == str) {
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
}  // namespace detail

template <typename CH = char, typename TSKIP = detail::actrie_skip_charset<CH, sizeof(CH)> >
class LIBATFRAME_UTILS_API_HEAD_ONLY ac_automation {
 public:
  using char_t = CH;
  using trie_type = typename detail::actrie<char_t>;
  using string_t = typename trie_type::string_t;
  using storage_t = typename trie_type::storage_t;
  using skip_set_t = TSKIP;

  struct match_t {
    size_t start;
    size_t length;
    const string_t *keyword;
  };
  using value_type = std::vector<match_t>;

 private:
  /**
   * 根节点(空节点)
   */
  storage_t storage_;

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

    storage_[0]->init_failed(storage_);

    is_inited_ = true;
  }

 public:
  ac_automation() : is_inited_(false), is_no_case_(false) { trie_type::create(storage_); }
  ~ac_automation() {}

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
      std::transform(res.begin(), res.end(), res.begin(), LIBATFRAME_UTILS_NAMESPACE_ID::string::tolower<char>);
      storage_[0]->insert(storage_, res.c_str(), res.size(), keyword);
    } else {
      storage_[0]->insert(storage_, keyword.c_str(), keyword.size(), keyword);
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
      std::transform(nocase.begin(), nocase.end(), nocase.begin(),
                     LIBATFRAME_UTILS_NAMESPACE_ID::string::tolower<char>);
      conv_content = &nocase;
    }

    init();
    size_t sz = conv_content->size();
    size_t left = sz;
    const char *end = conv_content->data() + sz;

    while (left > 0) {
      typename trie_type::match_result_t mres;
      mres.start = 0;
      mres.length = 0;
      mres.keyword = nullptr;
      mres.has_skip = false;

      size_t res = storage_[0]->match(storage_, mres, end - left, left, &skip_charset_);
      if (nullptr != mres.keyword && mres.keyword->is_leaf()) {
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
  void reset() {
    storage_.clear();
    trie_type::create(storage_);
  }

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

  /**
   * 导出AC自动机的关系图（dot格式）
   * @param os 输出流
   * @param options digraph选项(最后跟一个NULL表示结束)
   * @param node_options 节点绘制选项(最后跟一个NULL表示结束)
   * @param edge_options 边绘制选项(最后跟一个NULL表示结束)
   */
  template <typename TC, typename TCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump_dot(std::basic_ostream<TC, TCTT> &os, const char *options[] = nullptr,
                                               const char *node_options[] = nullptr,
                                               const char *edge_options[] = nullptr) {
    init();

    os << "digraph \"ac_automation";
    if (is_nocase()) {
      os << "(no case)";
    }

    os << "\" {" << std::endl;

    while (options && *options) {
      os << *options << std::endl;
      ++options;
    }

    if (node_options && *node_options) {
      os << "node [" << *node_options;
      ++node_options;
      while (node_options && *node_options) {
        os << ", " << *node_options;
        ++node_options;
      }
      os << "];" << std::endl;
    }

    if (edge_options && *edge_options) {
      os << "edge [" << *edge_options;
      ++edge_options;
      while (edge_options && *edge_options) {
        os << ", " << *edge_options;
        ++edge_options;
      }
      os << "];" << std::endl;
    }

    os << std::endl;
    for (size_t i = 0; i < storage_.size(); ++i) {
      storage_[i]->dump_dot_node(os);
    }
    os << std::endl;
    for (size_t i = 0; i < storage_.size(); ++i) {
      storage_[i]->dump_dot_relationship(os);
    }
    os << "}" << std::endl;
  }

  /**
   * 导出AC自动机数据
   * @param os 输出流
   * @param options digraph选项(最后跟一个NULL表示结束)
   * @param node_options 节点绘制选项(最后跟一个NULL表示结束)
   * @param edge_options 边绘制选项(最后跟一个NULL表示结束)
   */
  template <typename TC, typename TCTT>
  LIBATFRAME_UTILS_API_HEAD_ONLY void dump(std::basic_ostream<TC, TCTT> &os) {
    init();

    os << "ACAUTOMATION " << sizeof(char_t) << "\r\n";
    if (is_nocase()) {
      os << "nocase 1\r\n";
    }
    os << "node " << storage_.size() << "\r\n";
    os << "\r\n";

    // skip datas
    os << skip_charset_;

    for (size_t i = 0; i < storage_.size(); ++i) {
      storage_[i]->dump(os);
    }
  }
};
}  // namespace string
LIBATFRAME_UTILS_NAMESPACE_END

#endif
