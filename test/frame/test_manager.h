// Copyright 2024 atframework

#pragma once

// Import the C++20 feature-test macros
#ifdef __has_include
#  if __has_include(<version>)
#    include <version>
#  endif
#elif defined(_MSC_VER) && \
    ((defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L))
#  if _MSC_VER >= 1922
#    include <version>
#  endif
#elif defined(__cplusplus) && __cplusplus < 201703L
#  include <ciso646>
#else
#  include <iso646.h>
#endif

#include <stdint.h>
#include <ctime>
#include <string>
#include <type_traits>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#ifdef __cpp_impl_three_way_comparison
#  include <compare>
#endif

#include "cli/shell_font.h"
#include "nostd/string_view.h"
#include "nostd/type_traits.h"
#include "nostd/utility_data_size.h"

#include "test_case_base.h"

#if (defined(__cplusplus) && __cplusplus >= 201103L) || (defined(_MSC_VER) && _MSC_VER >= 1600)

#  define UTILS_TEST_ENV_AUTO_MAP(...) std::unordered_map<__VA_ARGS__>
#  define UTILS_TEST_ENV_AUTO_SET(...) std::unordered_set<__VA_ARGS__>
#  define UTILS_TEST_ENV_AUTO_UNORDERED 1
#else

#  include <map>
#  include <set>
#  define UTILS_TEST_ENV_AUTO_MAP(...) std::map<__VA_ARGS__>
#  define UTILS_TEST_ENV_AUTO_SET(...) std::set<__VA_ARGS__>

#endif

/**
 *
 */
class test_manager {
 public:
  using case_ptr_type = test_case_base *;
  using on_start_ptr_type = test_on_start_base *;
  using on_exit_ptr_type = test_on_exit_base *;
  using test_type = std::vector<std::pair<std::string, case_ptr_type>>;
  using event_on_start_type = std::vector<std::pair<std::string, on_start_ptr_type>>;
  using event_on_exit_type = std::vector<std::pair<std::string, on_exit_ptr_type>>;
  using test_data_type = std::unordered_map<std::string, test_type>;

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

  const test_data_type &get_tests() const { return tests_; }

  static test_manager &me();

  static std::string get_expire_time(clock_t begin, clock_t end);

#ifdef UTILS_TEST_MACRO_TEST_ENABLE_BOOST_TEST
  static boost::unit_test::test_suite *&test_suit();
#endif

  template <class T>
  struct is_numberic : std::conditional<std::is_arithmetic<T>::value || std::is_enum<T>::value, std::true_type,
                                        std::false_type>::type {};

  template <class TL, class TR,
            bool has_pointer = std::is_pointer<typename std::decay<TL>::type>::value ||
                               std::is_pointer<typename std::decay<TR>::type>::value,
            bool has_number =
                is_numberic<typename std::decay<TL>::type>::value || is_numberic<typename std::decay<TR>::type>::value,
            bool all_number =
                is_numberic<typename std::decay<TL>::type>::value && is_numberic<typename std::decay<TR>::type>::value>
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
  struct try_pick_basic_string_view {
   private:
    template <class T>
    struct is_supported_string_view_char
        : ::std::integral_constant<
              bool,
              ::std::is_same<char, typename ::std::remove_cv<typename ::std::remove_reference<T>::type>::type>::value ||
                  ::std::is_same<wchar_t,
                                 typename ::std::remove_cv<typename ::std::remove_reference<T>::type>::type>::value
#ifdef __cpp_unicode_characters
                  || ::std::is_same<
                         char16_t, typename ::std::remove_cv<typename ::std::remove_reference<T>::type>::type>::value ||
                  ::std::is_same<char32_t,
                                 typename ::std::remove_cv<typename ::std::remove_reference<T>::type>::type>::value
#endif
#ifdef __cpp_char8_t
                  || ::std::is_same<char8_t,
                                    typename ::std::remove_cv<typename ::std::remove_reference<T>::type>::type>::value
#endif
              > {
    };

    template <class, class = void>
    struct try_deduce_char_type {
      using type = void;
    };

    template <class T>
    struct try_deduce_char_type<T, ::atfw::util::nostd::void_t<typename T::value_type>> {
      using value_type = ::atfw::util::nostd::remove_cv_t<typename T::value_type>;
      using type =
          typename ::std::conditional<is_supported_string_view_char<value_type>::value, value_type, void>::type;
    };

    template <class TChar, class TTraits>
    struct try_deduce_char_type<::atfw::util::nostd::basic_string_view<TChar, TTraits>> {
      using type = TChar;
    };

    template <class TChar, class TTraits, class TAlloc>
    struct try_deduce_char_type<::std::basic_string<TChar, TTraits, TAlloc>> {
      using type = TChar;
    };

    template <class T>
    struct try_deduce_char_type<
        T, typename ::std::enable_if<::std::is_pointer<T>::value &&
                                     is_supported_string_view_char<typename ::std::remove_cv<
                                         typename ::std::remove_pointer<T>::type>::type>::value>::type> {
      using type = typename ::std::remove_cv<typename ::std::remove_pointer<T>::type>::type;
    };

    template <class T>
    struct try_deduce_char_type<
        T, typename ::std::enable_if<::std::is_array<T>::value &&
                                     is_supported_string_view_char<typename ::std::remove_cv<
                                         typename ::std::remove_extent<T>::type>::type>::value>::type> {
      using type = typename ::std::remove_cv<typename ::std::remove_extent<T>::type>::type;
    };

    using raw_value_type = ::atfw::util::nostd::remove_cvref_t<TVAL>;
    using deduced_char_type = typename try_deduce_char_type<raw_value_type>::type;

    template <class TChar>
    struct make_basic_string_view {
      using type = ::atfw::util::nostd::basic_string_view<TChar>;
    };

    template <class TChar, class TInput, class = void>
    struct can_construct_from_data_size : ::std::false_type {};

    template <class TChar, class TInput>
    struct can_construct_from_data_size<
        TChar, TInput,
        ::atfw::util::nostd::void_t<decltype(::atfw::util::nostd::data(
                                        ::std::declval<typename ::std::add_lvalue_reference<TInput>::type>())),
                                    decltype(::atfw::util::nostd::size(
                                        ::std::declval<typename ::std::add_lvalue_reference<TInput>::type>()))>> {
      using data_type =
          decltype(::atfw::util::nostd::data(::std::declval<typename ::std::add_lvalue_reference<TInput>::type>()));
      using size_type =
          decltype(::atfw::util::nostd::size(::std::declval<typename ::std::add_lvalue_reference<TInput>::type>()));

      static constexpr const bool value =
          ::std::is_convertible<data_type, const TChar *>::value && ::std::is_convertible<size_type, size_t>::value;
    };

    template <class TChar, bool>
    struct try_use_deduced_char;

    template <class TChar>
    struct try_use_deduced_char<TChar, true> {
      using type = void;
      static constexpr const bool value = false;
    };

    template <class TChar>
    struct try_use_deduced_char<TChar, false> {
      using type = typename make_basic_string_view<TChar>::type;
      static constexpr const bool value =
          ::std::is_convertible<TVAL, type>::value || can_construct_from_data_size<TChar, TVAL>::value;
    };

    using deduced_try = try_use_deduced_char<deduced_char_type, ::std::is_void<deduced_char_type>::value>;
    static constexpr const bool use_deduced_type = deduced_try::value;

    template <class TChar, class TFallback>
    struct try_use_fallback_char {
      using type = typename ::std::conditional<
          (::std::is_convertible<TVAL, typename make_basic_string_view<TChar>::type>::value ||
           can_construct_from_data_size<TChar, TVAL>::value),
          typename make_basic_string_view<TChar>::type, TFallback>::type;
    };

    using fallback_step0 = void;
#ifdef __cpp_unicode_characters
    using fallback_step1 = typename try_use_fallback_char<char32_t, fallback_step0>::type;
    using fallback_step2 = typename try_use_fallback_char<char16_t, fallback_step1>::type;
#else
    using fallback_step2 = fallback_step0;
#endif
    using fallback_step3 = typename try_use_fallback_char<wchar_t, fallback_step2>::type;
    using fallback_step4 = typename try_use_fallback_char<char, fallback_step3>::type;
#ifdef __cpp_char8_t
    using fallback_type = typename try_use_fallback_char<char8_t, fallback_step4>::type;
#else
    using fallback_type = fallback_step4;
#endif

   public:
    using type = typename ::std::conditional<use_deduced_type, typename deduced_try::type, fallback_type>::type;
    static constexpr const bool value = !::std::is_void<type>::value;
  };

  template <class TVAL>
  struct try_convert_to_string_view<TVAL, true> {
    using picked_string_view_type = typename try_pick_basic_string_view<TVAL>::type;
    static constexpr const bool is_nullptr = std::is_same<std::nullptr_t, typename std::decay<TVAL>::type>::value;

    using value_type = typename std::conditional<is_nullptr, std::nullptr_t, picked_string_view_type>::type;

   private:
    static inline value_type pick_impl(TVAL v, ::std::true_type) { return value_type(v); }

    static inline value_type pick_impl(TVAL v, ::std::false_type) {
      return pick_view(v, ::std::integral_constant<bool, ::std::is_convertible<TVAL, value_type>::value>());
    }

    static inline value_type pick_view(TVAL v, ::std::true_type) { return value_type(v); }

    static inline value_type pick_view(TVAL v, ::std::false_type) {
      return value_type(::atfw::util::nostd::data(v), ::atfw::util::nostd::size(v));
    }

   public:
    static inline value_type pick(TVAL v) { return pick_impl(v, ::std::integral_constant<bool, is_nullptr>()); }
  };

  template <class TVAL, bool>
  struct try_convert_to_string_view_underlying;

  template <class TVAL>
  struct try_convert_to_string_view_underlying<TVAL, false> {
    using type = TVAL;
  };

  template <class TVAL>
  struct try_convert_to_string_view_underlying<TVAL, true> {
    using type = typename std::underlying_type<::atfw::util::nostd::remove_cvref_t<TVAL>>::type;
  };

  template <class TVAL>
  struct try_convert_to_string_view<TVAL, false> {
    using value_type = typename try_convert_to_string_view_underlying<
        TVAL, std::is_enum<::atfw::util::nostd::remove_cvref_t<TVAL>>::value>::type;
    static inline value_type pick(TVAL v) { return static_cast<value_type>(v); }
  };

  template <class TL, class TR, bool has_pointer, bool has_integer, bool all_integer>
  struct pick_param {
    template <class T>
    typename try_convert_to_string_view<T, try_pick_basic_string_view<T>::value>::value_type operator()(T &&t) {
      return try_convert_to_string_view<T, try_pick_basic_string_view<T>::value>::pick(std::forward<T>(t));
    }
  };

  template <class TL, bool CONVERT_TO_VOID_P = std::is_pointer<typename std::decay<TL>::type>::value ||
                                               std::is_function<typename std::decay<TL>::type>::value>
  struct convert_param;

  template <class TL>
  struct convert_param<TL, true> {
    using value_type = void;
    using type = const void *;
    template <class TINPUT>
    static inline const void *pick(TINPUT &&v) {
      return reinterpret_cast<const void *>(v);
    }
  };

  template <class TL>
  struct convert_param<TL, false> {
    using value_type = typename std::decay<TL>::type;
    using type = const value_type &;
    template <class TINPUT>
    static inline const value_type &pick(TINPUT &&v) {
      return v;
    }
  };

  template <class TL>
  typename convert_param<TL>::type pick_convert_value(TL &&v) {
    return convert_param<TL>::pick(std::forward<TL>(v));
  }

  // expect functions
  template <class TL, class TR>
  bool expect_eq(TL &&l, TR &&r, const char *lexpr, const char *rexpr, const char *file, size_t line) {
    pick_param<TL, TR> pp;
    if (pp(l) == pp(r)) {
      inc_success_counter();
      return true;
    } else {
      inc_failed_counter();
      atfw::util::cli::shell_stream ss(std::cout);
      ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":" << line
           << std::endl
           << "Expected: " << lexpr << " == " << rexpr << std::endl
           << lexpr << ": " << pick_convert_value(l) << std::endl
           << rexpr << ": " << pick_convert_value(r) << std::endl;

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
      atfw::util::cli::shell_stream ss(std::cout);
      ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":" << line
           << std::endl
           << "Expected: " << lexpr << " != " << rexpr << std::endl
           << lexpr << ": " << pick_convert_value(l) << std::endl
           << rexpr << ": " << pick_convert_value(r) << std::endl;

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
      atfw::util::cli::shell_stream ss(std::cout);
      ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":" << line
           << std::endl
           << "Expected: " << lexpr << " < " << rexpr << std::endl
           << lexpr << ": " << pick_convert_value(l) << std::endl
           << rexpr << ": " << pick_convert_value(r) << std::endl;

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
      atfw::util::cli::shell_stream ss(std::cout);
      ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":" << line
           << std::endl
           << "Expected: " << lexpr << " <= " << rexpr << std::endl
           << lexpr << ": " << pick_convert_value(l) << std::endl
           << rexpr << ": " << pick_convert_value(r) << std::endl;

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
      atfw::util::cli::shell_stream ss(std::cout);
      ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":" << line
           << std::endl
           << "Expected: " << lexpr << " > " << rexpr << std::endl
           << lexpr << ": " << pick_convert_value(l) << std::endl
           << rexpr << ": " << pick_convert_value(r) << std::endl;

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
      atfw::util::cli::shell_stream ss(std::cout);
      ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":" << line
           << std::endl
           << "Expected: " << lexpr << " >= " << rexpr << std::endl
           << lexpr << ": " << pick_convert_value(l) << std::endl
           << rexpr << ": " << pick_convert_value(r) << std::endl;

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
      atfw::util::cli::shell_stream ss(std::cout);
      ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":" << line
           << std::endl
           << "Expected true: " << expr << std::endl
           << expr << ": " << pick_convert_value(l) << std::endl;

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
      atfw::util::cli::shell_stream ss(std::cout);
      ss() << atfw::util::cli::shell_font_style::SHELL_FONT_COLOR_RED << "FAILED => " << file << ":" << line
           << std::endl
           << "Expected false: " << expr << std::endl
           << expr << ": " << pick_convert_value(l) << std::endl;

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
  std::unordered_set<std::string> run_cases_;
  std::unordered_set<std::string> run_groups_;
};

int run_event_on_start();
int run_event_on_exit();
int run_tests(int argc, char *argv[]);
