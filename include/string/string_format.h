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
#  if FMT_VERSION >= 80000
#    define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE(...) fmt::format_string<__VA_ARGS__>
#  endif

// Compatiibility for old versions of atframe_utils
#  define LOG_WRAPPER_FWAPI_FMT_STRING(S) ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(S)
#  if FMT_VERSION >= 80000
#    define LOG_WRAPPER_FWAPI_USING_FORMAT_STRING(...) ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE(__VA_ARGS__)
#  endif
#endif

#if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
#  include <exception>
#endif

#include <iterator>
#include <string>
#include <type_traits>
#include <utility>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace string {
#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
namespace details {
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

  truncating_iterator_base(OutputIt out, size_type limit) : out_(out), limit_(limit), count_(0) {}

  OutputIt base() const noexcept { return out_; }
  size_type count() const noexcept { return count_; }

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
class ATFRAMEWORK_UTILS_API_HEAD_ONLY truncating_iterator<OutputIt, std::false_type>
    : public truncating_iterator_base<OutputIt> {
 public:
  using value_type = typename truncating_iterator_base<OutputIt>::value_type;
  using size_type = typename truncating_iterator_base<OutputIt>::size_type;

  truncating_iterator(OutputIt out, size_type limit) : truncating_iterator_base<OutputIt>(out, limit) {}

  inline truncating_iterator &operator++() noexcept {
    if (this->count_++ < this->limit_) {
      ++this->out_;
    }
    return *this;
  }

  inline truncating_iterator operator++(int) noexcept {
    auto it = *this;
    ++*this;
    return it;
  }

  template <typename T>
  inline void push_back(T &&val) {
    if (this->count_++ < this->limit_) {
      *this->out_++ = std::forward<T>(val);
    }
  }

  template <typename T>
  inline truncating_iterator &operator=(T &&val) {
    push_back(std::forward<T>(val));
    return *this;
  }

  inline truncating_iterator &operator*() noexcept { return *this; }
};

template <class OutputIt>
class ATFRAMEWORK_UTILS_API_HEAD_ONLY truncating_iterator<OutputIt, std::true_type>
    : public truncating_iterator_base<OutputIt> {
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

}  // namespace details
#endif

template <class OutputIt>
using format_to_n_result = ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_to_n_result<OutputIt>;

#if defined(ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI) && ATFRAMEWORK_UTILS_STRING_ENABLE_FWAPI
template <class TCONTEXT = ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_context, class... TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY auto make_format_args(TARGS &&...args) {
#  if defined(ATFRAMEWORK_UTILS_ENABLE_FMTLIB) && ATFRAMEWORK_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
      FMT_VERSION >= 100000
  return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args<TCONTEXT>(args...);
#  else
  return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args<TCONTEXT>(std::forward<TARGS>(args)...);
#  endif
}

#  ifdef ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE
template <class... TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY std::string format(ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE(TARGS...) fmt_text,
                                                   TARGS &&...args)
#  else
template <class TFMT, class... TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY std::string format(TFMT &&fmt_text, TARGS &&...args)
#  endif
{
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
#  ifdef ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format(fmt_text, std::forward<TARGS>(args)...);
#  elif defined(ATFRAMEWORK_UTILS_ENABLE_FORWARD_FMTTEXT) && ATFRAMEWORK_UTILS_ENABLE_FORWARD_FMTTEXT
  return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format(std::forward<TFMT>(fmt_text), std::forward<TARGS>(args)...);
#  else
  return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE vformat(
      std::forward<TFMT>(fmt_text),
#    if defined(ATFRAMEWORK_UTILS_ENABLE_FMTLIB) && ATFRAMEWORK_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
        FMT_VERSION >= 100000
      ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args(args...)
#    else
      ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args(std::forward<TARGS>(args)...)
#    endif
  );
#  endif
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_error &e) {
    return e.what();
  } catch (const std::runtime_error &e) {
    return e.what();
  } catch (...) {
    return "format got unknown exception";
  }
#  endif
}

#  ifdef ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE
template <class OutputIt, class... TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY auto format_to(OutputIt out,
                                               ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE(TARGS...) fmt_text,
                                               TARGS &&...args)
#  else
template <class OutputIt, class TFMT, class... TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY auto format_to(OutputIt out, TFMT &&fmt_text, TARGS &&...args)
#  endif
{
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
#  ifdef ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_to(out, fmt_text, std::forward<TARGS>(args)...);
#  elif defined(ATFRAMEWORK_UTILS_ENABLE_FORWARD_FMTTEXT) && ATFRAMEWORK_UTILS_ENABLE_FORWARD_FMTTEXT
  return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_to(out, std::forward<TFMT>(fmt_text),
                                                            std::forward<TARGS>(args)...);
#  else
  return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE vformat_to(
      out, std::forward<TFMT>(fmt_text),
#    if defined(ATFRAMEWORK_UTILS_ENABLE_FMTLIB) && ATFRAMEWORK_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
        FMT_VERSION >= 100000
      ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args(args...)
#    else
      ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args(std::forward<TARGS>(args)...)
#    endif
  );
#  endif
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_error &e) {
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
ATFRAMEWORK_UTILS_API_HEAD_ONLY ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_to_n_result<OutputIt> format_to_n(
    OutputIt out, size_t n, TFMT &&fmt_text, TARGS &&...args) {
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
#  ifdef ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_STRING_TYPE
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE vformat_to_n(
        out, static_cast<typename details::truncating_iterator<OutputIt>::size_type>(n),
#    if defined(FMT_VERSION) && FMT_VERSION >= 100000
        ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE string_view{std::forward<TFMT>(fmt_text)},
#    else
        std::forward<TFMT>(fmt_text),
#    endif
#    if defined(ATFRAMEWORK_UTILS_ENABLE_FMTLIB) && ATFRAMEWORK_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
        FMT_VERSION >= 100000
        ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args(args...)
#    else
        ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args(std::forward<TARGS>(args)...)
#    endif
    );  // NOLINT: whitespace/parens
#  elif defined(ATFRAMEWORK_UTILS_ENABLE_FORWARD_FMTTEXT) && ATFRAMEWORK_UTILS_ENABLE_FORWARD_FMTTEXT
  return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_to_n(
      out, static_cast<typename details::truncating_iterator<OutputIt>::size_type>(n), std::forward<TFMT>(fmt_text),
      std::forward<TARGS>(args)...);
#  else
  typename details::truncating_iterator<OutputIt> buf(std::move(out), n);
  ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE vformat_to(std::back_inserter(buf), std::forward<TFMT>(fmt_text),
#    if defined(ATFRAMEWORK_UTILS_ENABLE_FMTLIB) && ATFRAMEWORK_UTILS_ENABLE_FMTLIB && defined(FMT_VERSION) && \
        FMT_VERSION >= 100000
                                                      ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args(args...)
#    else
                                                      ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE make_format_args(
                                                          std::forward<TARGS>(args)...)
#    endif
  );  // NOLINT: whitespace/parens
  ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_to_n_result<OutputIt> ret;
  ret.out = buf.base();
  ret.size = static_cast<decltype(ret.size)>(buf.count());
  return ret;
#  endif
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_error &e) {
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
ATFRAMEWORK_UTILS_API_HEAD_ONLY std::string vformat(TFMT &&fmt_text, TARGS &&args) {
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE vformat(std::forward<TFMT>(fmt_text), std::forward<TARGS>(args));
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_error &e) {
    return e.what();
  } catch (const std::runtime_error &e) {
    return e.what();
  } catch (...) {
    return "format got unknown exception";
  }
#  endif
}

template <class OutputIt, class TFMT, class TARGS>
ATFRAMEWORK_UTILS_API_HEAD_ONLY OutputIt vformat_to(OutputIt out, TFMT &&fmt_text, TARGS &&args) {
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  try {
#  endif
    return ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE vformat_to(out, std::forward<TFMT>(fmt_text),
                                                               std::forward<TARGS>(args));
#  if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  } catch (const ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE format_error &e) {
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
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT(...) ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::format(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO(...) ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::format_to(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO_N(...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::format_to_n(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT(...) ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::vformat(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT_TO(...) ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::vformat_to(__VA_ARGS__)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_MAKE_FORMAT_ARGS(...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::make_format_args(__VA_ARGS__)
#else
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT(FMT, args...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::format(ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO(OUT, FMT, args...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::format_to(OUT, ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_FORMAT_TO_N(OUT, N, FMT, args...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::format_to_n(OUT, N, ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT(FMT, args) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::vformat(ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_VFORMAT_TO(OUT, FMT, args...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::vformat_to(OUT, ATFRAMEWORK_UTILS_STRING_FWAPI_FMT_STRING(FMT), ##args)
#  define ATFRAMEWORK_UTILS_STRING_FWAPI_MAKE_FORMAT_ARGS(...) \
    ::ATFRAMEWORK_UTILS_NAMESPACE_ID::log::make_format_args(__VA_ARGS__)
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
      : formatter<ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE basic_string_view<CharT>, CharT> {
    template <class FormatContext>
    auto format(ATFRAMEWORK_UTILS_NAMESPACE_ID::nostd::basic_string_view<CharT, Traits> const &val,
                FormatContext &ctx) const -> decltype(ctx.out()) {
      return formatter<ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE basic_string_view<CharT>, CharT>::format(
          ATFRAMEWORK_UTILS_STRING_FWAPI_NAMESPACE basic_string_view<CharT>{val.data(), val.size()}, ctx);
    }
  };
}

#endif