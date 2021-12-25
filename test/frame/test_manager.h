// Copyright 2021 atframework

#ifndef TEST_MANAGER_H_
#define TEST_MANAGER_H_

#pragma once

#include <stdint.h>
#include <ctime>
#include <map>
#include <string>
#include <type_traits>
#include <vector>

#ifdef __cpp_impl_three_way_comparison
#  include <compare>
#endif

#include "cli/shell_font.h"

#include "test_case_base.h"

#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1600)

#  include <unordered_map>
#  include <unordered_set>
#  define UTILS_TEST_ENV_AUTO_MAP(...) std::unordered_map<__VA_ARGS__>
#  define UTILS_TEST_ENV_AUTO_SET(...) std::unordered_set<__VA_ARGS__>
#  define UTILS_TEST_ENV_AUTO_UNORDERED 1
#else

#  include <map>
#  include <set>
#  define UTILS_TEST_ENV_AUTO_MAP(...) std::map<__VA_ARGS__>
#  define UTILS_TEST_ENV_AUTO_SET(...) std::set<__VA_ARGS__>

#endif

#include "nostd/string_view.h"

/**
 *
 */
class test_manager {
 public:
  using case_ptr_type = test_case_base *;
  using on_start_ptr_type = test_on_start_base *;
  using on_exit_ptr_type = test_on_exit_base *;
  using test_type = std::vector<std::pair<std::string, case_ptr_type> >;
  using event_on_start_type = std::vector<std::pair<std::string, on_start_ptr_type> >;
  using event_on_exit_type = std::vector<std::pair<std::string, on_exit_ptr_type> >;
  using test_data_type = UTILS_TEST_ENV_AUTO_MAP(std::string, test_type);

 public:
  test_manager();
  virtual ~test_manager();

  void append_test_case(const std::string &test_name, const std::string &case_name, case_ptr_type);
  void append_event_on_start(const std::string &event_name, on_start_ptr_type);
  void append_event_on_exit(const std::string &event_name, on_exit_ptr_type);

  int run_event_on_start();
  int run_event_on_exit();
  int run();

  void set_cases(const std::vector<std::string> &case_names);

  static test_manager &me();

  static std::string get_expire_time(clock_t begin, clock_t end);

#ifdef UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST
  static boost::unit_test::test_suite *&test_suit();
#endif

  template <class TL, class TR,
            bool has_pointer = std::is_pointer<typename std::decay<TL>::type>::value ||
                               std::is_pointer<typename std::decay<TR>::type>::value,
            bool has_number = std::is_arithmetic<typename std::decay<TL>::type>::value ||
                              std::is_arithmetic<typename std::decay<TR>::type>::value,
            bool all_number = std::is_arithmetic<typename std::decay<TL>::type>::value
                &&std::is_arithmetic<typename std::decay<TR>::type>::value>
  struct pick_param;

  // compare pointer with integer
  template <class TL, class TR>
  struct pick_param<TL, TR, true, true, false> {
    using value_type = uintptr_t;

    template <class T>
    value_type operator()(T &&t) {
      return (value_type)(std::forward<T>(t));
    }
  };

  // compare integer with integer, all converted to double/int64_t/uint64_t
  template <class TL, class TR>
  struct pick_param<TL, TR, false, true, true> {
    using value_type =
        typename std::conditional<std::is_floating_point<typename std::decay<TL>::type>::value ||
                                      std::is_floating_point<typename std::decay<TR>::type>::value,
                                  double,
                                  typename std::conditional<std::is_unsigned<typename std::decay<TL>::type>::value &&
                                                                std::is_unsigned<typename std::decay<TR>::type>::value,
                                                            uint64_t, int64_t>::type>::type;

    template <class T>
    value_type operator()(T &&t) {
      return static_cast<value_type>(std::forward<T>(t));
    }
  };

  template <class TVAL, bool>
  struct try_convert_to_string_view;

  template <class TVAL>
  struct try_convert_to_string_view<TVAL, true> {
    using value_type =
        typename std::conditional<std::is_same<std::nullptr_t, typename std::decay<TVAL>::type>::value, std::nullptr_t,
                                  LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view>::type;
    static inline value_type pick(TVAL v) { return value_type(v); }
  };

  template <class TVAL>
  struct try_convert_to_string_view<TVAL, false> {
    using value_type = TVAL;
    static inline value_type pick(TVAL v) { return v; }
  };

  template <class TL, class TR, bool has_pointer, bool has_integer, bool all_integer>
  struct pick_param {
    template <class T>
    typename try_convert_to_string_view<
        T, std::is_convertible<T, LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view>::value>::value_type
    operator()(T &&t) {
      return try_convert_to_string_view<
          T,
          std::is_convertible<T, LIBATFRAME_UTILS_NAMESPACE_ID::nostd::string_view>::value>::pick(std::forward<T>(t));
    }
  };

  // expect functions
  template <class TL, class TR>
  bool expect_eq(TL &&l, TR &&r, const char *lexpr, const char *rexpr, const char *file, size_t line) {
    pick_param<TL, TR> pp;
    if (pp(l) == pp(r)) {
      inc_success_counter();
      return true;
    } else {
      inc_failed_counter();
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream ss(std::cout);
      ss() << LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":"
           << line << std::endl
           << "Expected: " << lexpr << " == " << rexpr << std::endl
           << lexpr << ": " << l << std::endl
           << rexpr << ": " << r << std::endl;

      return false;
    }
  }

  template <class TL, class TR>
  bool expect_ne(TL &&l, TR &&r, const char *lexpr, const char *rexpr, const char *file, size_t line) {
    pick_param<TL, TR> pp;

    if (pp(l) != pp(r)) {
      inc_success_counter();
      return true;
    } else {
      inc_failed_counter();
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream ss(std::cout);
      ss() << LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":"
           << line << std::endl
           << "Expected: " << lexpr << " ！= " << rexpr << std::endl
           << lexpr << ": " << l << std::endl
           << rexpr << ": " << r << std::endl;

      return false;
    }
  }

  template <class TL, class TR>
  bool expect_lt(TL &&l, TR &&r, const char *lexpr, const char *rexpr, const char *file, size_t line) {
    pick_param<TL, TR> pp;

    if (pp(l) < pp(r)) {
      inc_success_counter();
      return true;
    } else {
      inc_failed_counter();
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream ss(std::cout);
      ss() << LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":"
           << line << std::endl
           << "Expected: " << lexpr << " < " << rexpr << std::endl
           << lexpr << ": " << l << std::endl
           << rexpr << ": " << r << std::endl;

      return false;
    }
  }

  template <class TL, class TR>
  bool expect_le(TL &&l, TR &&r, const char *lexpr, const char *rexpr, const char *file, size_t line) {
    pick_param<TL, TR> pp;

    if (pp(l) <= pp(r)) {
      inc_success_counter();
      return true;
    } else {
      inc_failed_counter();
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream ss(std::cout);
      ss() << LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":"
           << line << std::endl
           << "Expected: " << lexpr << " <= " << rexpr << std::endl
           << lexpr << ": " << l << std::endl
           << rexpr << ": " << r << std::endl;

      return false;
    }
  }

  template <class TL, class TR>
  bool expect_gt(TL &&l, TR &&r, const char *lexpr, const char *rexpr, const char *file, size_t line) {
    pick_param<TL, TR> pp;

    if (pp(l) > pp(r)) {
      inc_success_counter();
      return true;
    } else {
      inc_failed_counter();
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream ss(std::cout);
      ss() << LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":"
           << line << std::endl
           << "Expected: " << lexpr << " > " << rexpr << std::endl
           << lexpr << ": " << l << std::endl
           << rexpr << ": " << r << std::endl;

      return false;
    }
  }

  template <class TL, class TR>
  bool expect_ge(TL &&l, TR &&r, const char *lexpr, const char *rexpr, const char *file, size_t line) {
    pick_param<TL, TR> pp;

    if (pp(l) >= pp(r)) {
      inc_success_counter();
      return true;
    } else {
      inc_failed_counter();
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream ss(std::cout);
      ss() << LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":"
           << line << std::endl
           << "Expected: " << lexpr << " >= " << rexpr << std::endl
           << lexpr << ": " << l << std::endl
           << rexpr << ": " << r << std::endl;

      return false;
    }
  }

  template <class TL>
  bool expect_true(TL &&l, const char *expr, const char *file, size_t line) {
    if (!!(l)) {
      inc_success_counter();
      return true;
    } else {
      inc_failed_counter();
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream ss(std::cout);
      ss() << LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":"
           << line << std::endl
           << "Expected true: " << expr << std::endl
           << expr << ": " << l << std::endl;

      return false;
    }
  }

  template <class TL>
  bool expect_false(TL &&l, const char *expr, const char *file, size_t line) {
    if (!(l)) {
      inc_success_counter();
      return true;
    } else {
      inc_failed_counter();
      LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_stream ss(std::cout);
      ss() << LIBATFRAME_UTILS_NAMESPACE_ID::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":"
           << line << std::endl
           << "Expected false: " << expr << std::endl
           << expr << ": " << l << std::endl;

      return false;
    }
  }

  static void set_counter_ptr(int *success_counter_ptr, int *failed_counter_ptr);
  static void inc_success_counter();
  static void inc_failed_counter();

 private:
  test_data_type tests_;
  event_on_start_type evt_on_starts_;
  event_on_exit_type evt_on_exits_;
  int success_;
  int failed_;
  UTILS_TEST_ENV_AUTO_SET(std::string) run_cases_;
  UTILS_TEST_ENV_AUTO_SET(std::string) run_groups_;
};

int run_event_on_start();
int run_event_on_exit();
int run_tests(int argc, char *argv[]);

#endif /* TEST_MANAGER_H_ */
