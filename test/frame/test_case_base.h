/*
 * test_case_base.h
 *
 *  Created on: 2014年3月11日
 *      Author: owent
 *
 *  Released under the MIT license
 */

#ifndef TEST_CASE_BASE_H_
#define TEST_CASE_BASE_H_

#pragma once

#include <string>

#ifdef UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST
#  include <boost/test/unit_test.hpp>
#endif

#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1600)

#  include <unordered_map>
#  include <unordered_set>
#  define UTIL_UNIT_TEST_MACRO_AUTO_MAP(...) std::unordered_map<__VA_ARGS__>
#  define UTIL_UNIT_TEST_MACRO_AUTO_SET(...) std::unordered_set<__VA_ARGS__>
#  define UTIL_UNIT_TEST_MACRO_AUTO_UNORDERED 1
#else

#  include <map>
#  include <set>
#  define UTIL_UNIT_TEST_MACRO_AUTO_MAP(...) std::map<__VA_ARGS__>
#  define UTIL_UNIT_TEST_MACRO_AUTO_SET(...) std::set<__VA_ARGS__>

#endif

#include <config/compiler_features.h>

class test_case_base {
 public:
  using test_func = void (*)();

 public:
  test_case_base(const std::string& test_name, const std::string& case_name, test_func func);
  virtual ~test_case_base();

  virtual int run();

  int success_;
  int failed_;

  test_func func_;
};

class test_on_start_base {
 public:
  using after_set_t = UTIL_UNIT_TEST_MACRO_AUTO_SET(std::string);
  using on_start_func = void (*)();

 public:
  template <typename... T>
  test_on_start_base(const std::string& n, on_start_func func, T&&... deps) : name(n), func_(func) {
    after.reserve(sizeof...(T));
    expand(after.insert(after.end(), std::forward<T>(deps))...);
    register_self();
  }

  template <typename... T>
  void expand(T&&...) {}
  virtual ~test_on_start_base();

  virtual int run();

  std::string name;
  on_start_func func_;
  after_set_t after;

 private:
  void register_self();
};

class test_on_exit_base {
 public:
  using before_set_t = UTIL_UNIT_TEST_MACRO_AUTO_SET(std::string);
  using on_exit_func = void (*)();

 public:
  template <typename... T>
  test_on_exit_base(const std::string& n, on_exit_func func, T&&... deps) : name(n), func_(func) {
    before.reserve(sizeof...(T));
    expand(before.insert(before.end(), std::forward<T>(deps))...);
    register_self();
  }

  template <typename... T>
  void expand(T&&...) {}
  virtual ~test_on_exit_base();

  virtual int run();

  std::string name;
  on_exit_func func_;
  before_set_t before;

 private:
  void register_self();
};

#endif /* TEST_CASE_BASE_H_ */
