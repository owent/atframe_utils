// Copyright 2024 atframework
// Licensed under the MIT licenses.
// Created by owent on 2024-12-26
//
// This file is migrated from include/log/log_wrapper.h

#pragma once

#include <config/atframe_utils_build_feature.h>

#if defined(ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT) && ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT
#  include <format>
#  ifndef ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING
#    define ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(S) (S)
#  endif
#elif defined(ATFRAMEWORK_UTILS_ENABLE_FMTLIB) && ATFRAMEWORK_UTILS_ENABLE_FMTLIB

#  if defined(_MSC_VER)
#    pragma warning(push)
#    pragma warning(disable : 4702)
#  elif defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#    if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#      pragma GCC diagnostic push
#    endif
#    pragma GCC diagnostic ignored "-Warray-bounds"
#    if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 710
#      pragma GCC diagnostic ignored "-Wstringop-overflow"
#    endif
#  elif defined(__clang__) || defined(__apple_build_version__)
#    pragma clang diagnostic push
#    pragma GCC diagnostic ignored "-Warray-bounds"
#  endif

#  include <fmt/format.h>
#  include <fmt/xchar.h>

#  if defined(_MSC_VER)
#    pragma warning(pop)
#  elif defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)
#    if (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#      pragma GCC diagnostic pop
#    endif
#  elif defined(__clang__) || defined(__apple_build_version__)
#    pragma clang diagnostic pop
#  endif

#  ifndef ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING
#    define ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(S) FMT_STRING(S)
#  endif

static_assert(FMT_VERSION >= 100000, "Requires fmtlib 10.0 or upper");
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE(...) fmt::format_string<__VA_ARGS__>
#  define LOG_WRAPPER_FWAPI_USING_FORMAT_STRING(...) ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE(__VA_ARGS__)

// Compatiibility for old versions of atframe_utils
#  define LOG_WRAPPER_FWAPI_FMT_STRING(S) ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(S)
#endif

#if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
#  include <exception>
#endif

#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

#include "nostd/string_view.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace string {
namespace details {

#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
template <class OutputIt>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY truncating_iterator_base {
 public:
#  if defined(ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT) && ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT
  using size_type = typename std::iter_difference_t<OutputIt>;
#  else
  using size_type = size_t;
#  endif
  using iterator_category = std::output_iterator_tag;
  using value_type = typename std::iterator_traits<OutputIt>::value_type;
  using difference_type = typename std::iterator_traits<OutputIt>::difference_type;
  using pointer = typename std::iterator_traits<OutputIt>::pointer;
  using reference = typename std::iterator_traits<OutputIt>::reference;

  ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR truncating_iterator_base(OutputIt out, size_type limit)
      : out_(out), limit_(limit), count_(0) {}

  ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR OutputIt base() const noexcept { return out_; }
  ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR size_type count() const noexcept { return count_; }

 protected:
  OutputIt out_;
  size_type limit_;
  size_type count_;
};

// An output iterator that truncates the output and counts the number of objects
// written to it.
template <class OutputIt,
          class Enable = typename std::is_void<typename std::iterator_traits<OutputIt>::value_type>::type>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY truncating_iterator;

template <class OutputIt>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY
truncating_iterator<OutputIt, std::false_type> : public truncating_iterator_base<OutputIt> {
 public:
  using value_type = typename truncating_iterator_base<OutputIt>::value_type;
  using size_type = typename truncating_iterator_base<OutputIt>::size_type;

  ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR truncating_iterator(OutputIt out, size_type limit)
      : truncating_iterator_base<OutputIt>(out, limit) {}

  ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR inline truncating_iterator &operator++() noexcept {
    if (this->count_++ < this->limit_) {
      ++this->out_;
    }
    return *this;
  }

  ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR inline truncating_iterator operator++(int) noexcept {
    auto it = *this;
    ++*this;
    return it;
  }

  template <typename T>
  ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR inline void push_back(T &&val) {
    if (this->count_++ < this->limit_) {
      *this->out_++ = std::forward<T>(val);
    }
  }

  template <typename T>
  ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR inline truncating_iterator &operator=(T &&val) {
    push_back(std::forward<T>(val));
    return *this;
  }

  ATFW_UTIL_ATTRIBUTE_CXX20_CONSTEXPR inline truncating_iterator &operator*() noexcept { return *this; }
};

template <class OutputIt>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY
truncating_iterator<OutputIt, std::true_type> : public truncating_iterator_base<OutputIt> {
 public:
  using value_type = typename truncating_iterator_base<OutputIt>::value_type;
  using size_type = typename truncating_iterator_base<OutputIt>::size_type;

  truncating_iterator(OutputIt out, size_type limit) : truncating_iterator_base<OutputIt>(out, limit) {}

  template <typename T>
  inline void push_back(T &&val) {
    if (this->count_++ < this->limit_) {
      using assign_target_type = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
      assign_target_type *out = reinterpret_cast<assign_target_type *>(this->out_);
      *out++ = std::forward<T>(val);
      this->out_ = reinterpret_cast<void *>(out);
    }
  }

  template <typename T>
  truncating_iterator &operator=(T &&val) {
    push_back(std::forward<T>(val));
    return *this;
  }

  inline truncating_iterator &operator++() noexcept { return *this; }
  inline truncating_iterator &operator++(int) noexcept { return *this; }
  inline truncating_iterator &operator*() noexcept { return *this; }
};
#endif

template <class>
struct fmtapi_is_char : std::false_type {};

template <>
struct fmtapi_is_char<char> : std::true_type {};
template <>
struct fmtapi_is_char<wchar_t> : std::true_type {};
#ifdef __cpp_unicode_characters
template <>
struct fmtapi_is_char<char16_t> : std::true_type {};
template <>
struct fmtapi_is_char<char32_t> : std::true_type {};
#endif
#ifdef __cpp_char8_t
template <>
struct fmtapi_is_char<char8_t> : std::true_type {};
#endif

template <typename T,
          bool = std::is_convertible<decltype(nostd::data(std::declval<T>())), const typename T::value_type *>::value>
struct fmtapi_is_std_string_like;

template <typename T>
struct fmtapi_is_std_string_like<T, true> : std::true_type {};

template <typename T>
struct fmtapi_is_std_string_like<T, false> : std::false_type {};

template <class CharT,
          class = typename std::enable_if<fmtapi_is_char<typename std::remove_cvref<CharT>::type>::value>::type>
constexpr auto fmtapi_to_string_view(const CharT *s)
    -> ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT> {
  return s;
}
template <class T, class = typename std::enable_if<fmtapi_is_std_string_like<T>::value>::type>
constexpr auto fmtapi_to_string_view(const T &s)
    -> ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<typename T::value_type> {
  return s;
}
template <class CharT>
constexpr auto fmtapi_to_string_view(ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT> s)
    -> ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT> {
  return s;
}

template <class CharT, class Traits>
constexpr auto fmtapi_to_string_view(nostd::basic_string_view<CharT, Traits> s)
    -> ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT> {
  return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT>{s.data(), s.size()};
}

#if defined(ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW) && ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW
template <class CharT, class Traits>
constexpr auto fmtapi_to_string_view(std::basic_string_view<CharT, Traits> s)
    -> ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT> {
  return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT>{s.data(), s.size()};
}
#endif

template <class TFMT>
struct fmtapi_detect_char_t_from_fmt_base;

template <class CharT, size_t N>
struct fmtapi_detect_char_t_from_fmt_base<CharT[N]> {
  using value_type = CharT;
};

template <class CharT>
struct fmtapi_detect_char_t_from_fmt_base<CharT *> {
  using value_type = CharT;
};

template <class CharT>
struct fmtapi_detect_char_t_from_fmt_base<ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT>> {
  using value_type = CharT;
};

template <class CharT, class Traits>
struct fmtapi_detect_char_t_from_fmt_base<nostd::basic_string_view<CharT, Traits>> {
  using value_type = CharT;
};

#if defined(ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW) && ATFRAMEWORK_UTILS_GSL_TEST_STL_STRING_VIEW
template <class CharT, class Traits>
struct fmtapi_detect_char_t_from_fmt_base<std::basic_string_view<CharT, Traits>> {
  using value_type = CharT;
};
#endif

template <class T>
struct fmtapi_detect_char_t_from_fmt_base {
  using value_type = typename T::value_type;
};

template <class TFMT>
struct fmtapi_detect_char_t_from_fmt
    : public fmtapi_detect_char_t_from_fmt_base<typename std::remove_cvref<TFMT>::type> {};

template <class CharT>
struct make_format_args_helper_base;

template <>
struct make_format_args_helper_base<wchar_t> {
  template <class... TARGS>
  ATFW_UTIL_FORCEINLINE static auto make(TARGS &...args) {
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::make_wformat_args(args...);
  }
};

template <>
struct make_format_args_helper_base<char> {
  template <class... TARGS>
  ATFW_UTIL_FORCEINLINE static auto make(TARGS &...args) {
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::make_format_args(args...);
  }
};

template <class CharT>
struct make_format_args_helper_base {
  template <class... TARGS>
  ATFW_UTIL_FORCEINLINE static auto make(TARGS &...args) {
#if defined(ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT) && ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT
    return std::make_format_args<std::basic_format_context<std::back_insert_iterator<std::basic_string<CharT>>, CharT>>(
        args...);
#else
#  if FMT_VERSION >= 110000
    return fmt::make_format_args<fmt::buffered_context<CharT>>(args...);
#  else
    return fmt::make_format_args<fmt::buffer_context<CharT>>(args...);
#  endif
#endif
  }
};

template <class CharT>
struct make_format_args_helper : public make_format_args_helper_base<typename std::remove_cvref<CharT>::type> {};

}  // namespace details

template <class OutputIt>
using format_to_n_result = ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::format_to_n_result<OutputIt>;

using ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::make_format_args;
using ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::make_wformat_args;

#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI

template <class TFMT, class... TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY auto format(TFMT &&fmt_text, TARGS &&...args)
    -> std::basic_string<typename details::fmtapi_detect_char_t_from_fmt<TFMT>::value_type> {
  using return_type = std::basic_string<typename details::fmtapi_detect_char_t_from_fmt<TFMT>::value_type>;
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::vformat(
        details::fmtapi_to_string_view(fmt_text),
        details::make_format_args_helper<typename details::fmtapi_detect_char_t_from_fmt<TFMT>::value_type>::make(
            args...));
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::format_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    return_type ret;
    ret.resize(static_cast<return_type::size_type>(input_end - input_begin));
    std::copy(input_begin, input_end, ret.data());
    return ret;
  } catch (const std::runtime_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    return_type ret;
    ret.resize(static_cast<return_type::size_type>(input_end - input_begin));
    std::copy(input_begin, input_end, ret.data());
    return ret;
  } catch (...) {
    const char *input_begin = "format got unknown exception";
    const char *input_end = input_begin + strlen(input_begin);
    return_type ret;
    ret.resize(static_cast<return_type::size_type>(input_end - input_begin));
    std::copy(input_begin, input_end, ret.data());
    return ret;
  }
#  endif
}

template <class OutputIt, class TFMT, class... TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY auto format_to(OutputIt out, TFMT &&fmt_text, TARGS &&...args) -> OutputIt {
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::vformat_to(
        out, details::fmtapi_to_string_view(std::forward<TFMT>(fmt_text)),
        details::make_format_args_helper<typename std::iterator_traits<OutputIt>::value_type>::make(args...));
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::format_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  } catch (const std::runtime_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  } catch (...) {
    const char *input_begin = "format got unknown exception";
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  }
#  endif
}

template <class OutputIt, class TFMT, class... TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::format_to_n_result<OutputIt> format_to_n(
    OutputIt out, size_t n, TFMT &&fmt_text, TARGS &&...args) {
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
    details::truncating_iterator<OutputIt> res(out, n);
    ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::vformat_to(
        res, details::fmtapi_to_string_view(std::forward<TFMT>(fmt_text)),
        details::make_format_args_helper<typename std::iterator_traits<OutputIt>::value_type>::make(args...));
    return {res.base(), res.count()};
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::format_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    details::truncating_iterator<OutputIt> res =
        std::copy(input_begin, input_end, details::truncating_iterator<OutputIt>(out, n));
    return {res.base(), res.count()};
  } catch (const std::runtime_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    details::truncating_iterator<OutputIt> res =
        std::copy(input_begin, input_end, details::truncating_iterator<OutputIt>(out, n));
    return {res.base(), res.count()};
  } catch (...) {
    const char *input_begin = "format got unknown exception";
    const char *input_end = input_begin + strlen(input_begin);
    details::truncating_iterator<OutputIt> res =
        std::copy(input_begin, input_end, details::truncating_iterator<OutputIt>(out, n));
    return {res.base(), res.count()};
  }
#  endif
}

template <class TFMT, class TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY auto vformat(TFMT &&fmt_text, TARGS &&args)
    -> std::basic_string<typename details::fmtapi_detect_char_t_from_fmt<TFMT>::value_type> {
  using return_type = std::basic_string<typename details::fmtapi_detect_char_t_from_fmt<TFMT>::value_type>;
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::vformat(
        details::fmtapi_to_string_view(std::forward<TFMT>(fmt_text)), std::forward<TARGS>(args));
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::format_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    return_type ret;
    ret.resize(static_cast<return_type::size_type>(input_end - input_begin));
    std::copy(input_begin, input_end, ret.data());
    return ret;
  } catch (const std::runtime_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    return_type ret;
    ret.resize(static_cast<return_type::size_type>(input_end - input_begin));
    std::copy(input_begin, input_end, ret.data());
    return ret;
  } catch (...) {
    const char *input_begin = "format got unknown exception";
    const char *input_end = input_begin + strlen(input_begin);
    return_type ret;
    ret.resize(static_cast<return_type::size_type>(input_end - input_begin));
    std::copy(input_begin, input_end, ret.data());
    return ret;
  }
#  endif
}

template <class OutputIt, class TFMT, class TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY OutputIt vformat_to(OutputIt out, TFMT &&fmt_text, TARGS &&args) {
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::vformat_to(
        out, details::fmtapi_to_string_view(std::forward<TFMT>(fmt_text)), std::forward<TARGS>(args));
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::format_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  } catch (const std::runtime_error &e) {
    const char *input_begin = e.what();
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  } catch (...) {
    const char *input_begin = "format got unknown exception";
    const char *input_end = input_begin + strlen(input_begin);
    while (input_begin && *input_begin && input_begin < input_end) {
      *out = *input_begin;
      ++out;
      ++input_begin;
    }
    return out;
  }
#  endif
}
#endif

}  // namespace string
ATFRAMEWORK_UTILS_NAMESPACE_END

#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT(...) ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO(...) ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format_to(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO_N(...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format_to_n(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT(...) ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::vformat(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT_TO(...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::vformat_to(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_MAKE_FORMAT_ARGS(...) \
    ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::make_format_args(__VA_ARGS__)
#else
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT(FMT, args...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format(ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO(OUT, FMT, args...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format_to(OUT, ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO_N(OUT, N, FMT, args...)                                        \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::format_to_n(OUT, N, ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), \
                                                          ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT(FMT, args) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::vformat(ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT_TO(OUT, FMT, args...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::string::vformat_to(OUT, ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_MAKE_FORMAT_ARGS(...) \
    ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::make_format_args(__VA_ARGS__)
#endif

#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
#  if defined(ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT) && ATFRAMEWORK_UTILS_ENABLE_STD_FORMAT
#    define ATFRAMEWORK_UTILS_STRING_FWAPI_DECL_NAMESPACE() namespace std
#    define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_AS(Type, Base)                          \
      ATFRAMEWORK_UTILS_STRING_FWAPI_DECL_NAMESPACE() {                                   \
        template <class CharT>                                                            \
        struct formatter<Type, CharT> : formatter<Base, CharT> {                          \
          template <class FormatContext>                                                  \
          auto format(Type const &val, FormatContext &ctx) -> decltype(ctx.out()) const { \
            return formatter<Base, CharT>::format(val, ctx);                              \
          }                                                                               \
        };                                                                                \
      }
#  else
#    define ATFRAMEWORK_UTILS_STRING_FWAPI_DECL_NAMESPACE() namespace fmt
#    define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_AS(Type, Base) \
      ATFRAMEWORK_UTILS_STRING_FWAPI_DECL_NAMESPACE() { FMT_FORMAT_AS(Type, Base); }
#  endif

ATFRAMEWORK_UTILS_STRING_FWAPI_DECL_NAMESPACE() {
  template <class CharT, class Traits>
  struct formatter<ATFRAMEWORK_UTILS_NAMESPACE_ID::nostd::basic_string_view<CharT, Traits>, CharT>
      : formatter<ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT>, CharT> {
    template <class FormatContext>
    auto format(ATFRAMEWORK_UTILS_NAMESPACE_ID::nostd::basic_string_view<CharT, Traits> const &val,
                FormatContext &ctx) const -> decltype(ctx.out()) {
      return formatter<ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT>, CharT>::format(
          ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE_ID::basic_string_view<CharT>{val.data(), val.size()}, ctx);
    }
  };
}

#endif
