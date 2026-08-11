// Copyright 2026 atframework

#pragma once

#include <string>
#include <unordered_map>  // IWYU pragma: keep
#include <unordered_set>  // IWYU pragma: keep
#include <utility>

#ifdef ATFW_UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST
#  include <boost/test/unit_test.hpp>
#endif

#include <config/atframe_utils_build_feature.h>
#include <config/compiler_features.h>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace testing {

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
  using after_set_t = std::unordered_set<std::string>;
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
  using before_set_t = std::unordered_set<std::string>;
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

}  // namespace testing
ATFRAMEWORK_UTILS_NAMESPACE_END
