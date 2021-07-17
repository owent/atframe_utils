/**
 * @brief 静态无锁队列(数组)
 * @note 固定最大长度
 * @note 使用了 c++11的atomic
 *       不支持的编译器就自求多福吧
 *
 * @version 1.0
 * @author OWenT
 * @date 2015-01-09
 *
 */
#pragma once

#include <array>
#include <atomic>

#include "config/compiler_features.h"
#include "std/explicit_declare.h"

#include <config/atframe_utils_build_feature.h>

namespace util {
namespace ds {

template <typename T, size_t SIZE, typename TContainer = std::array<T, SIZE + 1> >
class LIBATFRAME_UTILS_API_HEAD_ONLY lock_free_array {
 public:
  typedef T value_type;
  typedef value_type *pointer_type;
  typedef value_type &reference_type;
  typedef TContainer container_type;

 private:
  lock_free_array(const lock_free_array &) = delete;
  lock_free_array &operator=(const lock_free_array &) = delete;

 public:
  typedef pointer_type iterator;
  typedef const iterator const_iterator;

 public:
  lock_free_array() {
    start_.store(0);
    end_.store(0);
  }

  iterator begin() { return &data_[start_.load()]; }

  iterator begin() const { return &data_[start_.load()]; }

  iterator end() { return &data_[end_.load()]; }

  iterator end() const { return &data_[end_.load()]; }

  /**
   * @brief 获取原始数据
   */
  iterator at(size_t index) { return &data_[index]; }

  /**
   * @brief 获取原始数据
   */
  iterator at(size_t index) const { return &data_[index]; }

  iterator push_back() {
    while (true) {
      size_t end_pos = end_.load();
      size_t start_pos = start_.load();

      if ((end_pos + 1) % data_.size() == start_pos) {
        return nullptr;
      }

      size_t new_end_pos = (end_pos + 1) % data_.size();
      if (end_.compare_exchange_weak(end_pos, new_end_pos)) {
        return at(end_pos);
      }
    }

    return nullptr;
  }

  iterator pop_back() {
    while (true) {
      size_t end_pos = end_.load();
      size_t start_pos = start_.load();

      if (end_pos == start_pos) {
        return nullptr;
      }

      size_t new_end_pos = (end_pos + data_.size() - 1) % data_.size();
      if (end_.compare_exchange_weak(end_pos, new_end_pos)) {
        return at(new_end_pos);
      }
    }

    return nullptr;
  }

  iterator push_front() {
    while (true) {
      size_t end_pos = end_.load();
      size_t start_pos = start_.load();

      if ((end_pos + 1) % data_.size() == start_pos) {
        return nullptr;
      }

      size_t new_start_pos = (start_pos + data_.size() - 1) % data_.size();
      if (start_.compare_exchange_weak(start_pos, new_start_pos)) {
        return at(new_start_pos);
      }
    }

    return nullptr;
  }

  iterator pop_front() {
    while (true) {
      size_t end_pos = end_.load();
      size_t start_pos = start_.load();

      if (end_pos == start_pos) {
        return nullptr;
      }

      size_t new_start_pos = (start_pos + 1) % data_.size();
      if (start_.compare_exchange_weak(start_pos, new_start_pos)) {
        return at(start_pos);
      }
    }

    return nullptr;
  }

  bool empty() const { return start_ != end_; }

  size_t size() const { return (data_.size() + end_ - start_) % data_.size(); }

  size_t capacity() const { return data_.capacity(); }

 private:
  std::atomic<size_t> start_;
  std::atomic<size_t> end_;
  container_type data_;
};
}  // namespace ds
}  // namespace util
