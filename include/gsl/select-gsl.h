// Copyright 2021 atframework
// @author OWenT, admin@owent.net
// @see https://github.com/gsl-lite/gsl-lite#features

#ifndef GSL_SELECT_GSL_H
#define GSL_SELECT_GSL_H

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <config/compiler_features.h>
#include <std/explicit_declare.h>

// 1.Lifetime safety
// not_null
// owner
// unique_ptr
// shared_ptr
// 2.Bounds safety
// zstring
// czstring
// span
// make_span
// as_bytes
// string_span    // deprecated -> string_view
// wstring_span   // deprecated -> wstring_view
// cstring_span   // deprecated
// cwstring_span  // deprecated
// zstring_span   // deprecated
// wzstring_span  // deprecated
// czstring_span  // deprecated
// cwzstring_span // deprecated
// ensure_z       // deprecated
// to_string
// at
// 3. Assertions
// Expects()
// Ensures()
// 4. Utilities
// byte
// final_action
// finally
// narrow_cast
// narrow
// ...

#if defined(LIBATFRAME_UTILS_GSL_TEST_STL_STRING_VIEW) && LIBATFRAME_UTILS_GSL_TEST_STL_STRING_VIEW
#  include <string_view>
#endif

#if defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_MS_GSL) && LIBATFRAME_UTILS_ENABLE_GSL_WITH_MS_GSL
#  include <gsl/gsl>
namespace gsl {
using std::make_shared;
#  if (defined(__cplusplus) && __cplusplus >= 201402L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201402L)
using std::make_unique;
#  else
template <class T, class... Args>
EXPLICIT_NODISCARD_ATTR unique_ptr<T> make_unique(Args &&...args) {
  return unique_ptr<T>(new T(std::forward<Args>(args)...));
}
#  endif
#  if defined(LIBATFRAME_UTILS_GSL_TEST_STL_STRING_VIEW) && LIBATFRAME_UTILS_GSL_TEST_STL_STRING_VIEW
using std::basic_string_view;
using std::string_view;
using std::wstring_view;
#  else
template <class CharT, class Traits = std::char_traits<CharT> >
using basic_string_view = span<CharT>;
using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
#  endif
}  // namespace gsl

#elif defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_GSL_LITE) && LIBATFRAME_UTILS_ENABLE_GSL_WITH_GSL_LITE
#  include <gsl-lite/gsl-lite.hpp>
namespace gsl {
#  if defined(LIBATFRAME_UTILS_GSL_TEST_STL_STRING_VIEW) && LIBATFRAME_UTILS_GSL_TEST_STL_STRING_VIEW
using std::basic_string_view;
using std::string_view;
using std::wstring_view;
#  else
template <class CharT, class Traits = std::char_traits<CharT> >
using basic_string_view = span<CharT>;
using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
#  endif
}  // namespace gsl
#elif defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL) && LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL

#  include <array>
#  include <cstddef>
#  include <memory>
#  include <type_traits>
#  include <utility>

#  if defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL_SPAN) && LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL_SPAN
#    include <span>
#  endif

namespace gsl {
using std::make_shared;
using std::shared_ptr;
using std::unique_ptr;

template <class T, size_t N>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline UTIL_CONFIG_CONSTEXPR T& at(T (&arr)[N], size_t pos) {
  return arr[pos];
}

template <class Container>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline UTIL_CONFIG_CONSTEXPR typename Container::value_type& at(
    Container& cont, size_t pos) {
  return cont[pos];
}

template <class Container>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline UTIL_CONFIG_CONSTEXPR
    typename Container::value_type const&
    at(Container const& cont, size_t pos) {
  return cont[pos];
}

#  if gsl_HAVE(INITIALIZER_LIST)

template <class T>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline const UTIL_CONFIG_CONSTEXPR T
at(std::initializer_list<T> cont, size_t pos) {
  return *(cont.begin() + pos);
}
#  endif

template <class T>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline UTIL_CONFIG_CONSTEXPR T& at(span<T> s, size_t pos) {
  return s[pos];
}

#  if defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL_SPAN) && LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL_SPAN
using std::span;
template <class T>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline UTIL_CONFIG_CONSTEXPR span<T> make_span(
    T* ptr, typename span<T>::index_type count) {
  return span<T>(ptr, count);
}

template <class T>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline UTIL_CONFIG_CONSTEXPR span<T> make_span(T* first,
                                                                                                      T* last) {
  return span<T>(first, last);
}

template <class T, size_t N>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline UTIL_CONFIG_CONSTEXPR span<T> make_span(T (&arr)[N]) {
  return span<T>(&arr[0], N);
}

template <class T, size_t N>
EXPLICIT_NODISCARD_ATTR inline UTIL_CONFIG_CONSTEXPR span<T> make_span(std::array<T, N>& arr) {
  return span<T>(arr);
}

template <class T, size_t N>
EXPLICIT_NODISCARD_ATTR inline UTIL_CONFIG_CONSTEXPR span<const T> make_span(std::array<T, N> const& arr) {
  return span<const T>(arr);
}

template <class Container, class EP = decltype(std17::data(std::declval<Container&>()))>
EXPLICIT_NODISCARD_ATTR inline UTIL_CONFIG_CONSTEXPR auto make_span(Container& cont)
    -> span<typename std::remove_pointer<EP>::type> {
  return span<typename std::remove_pointer<EP>::type>(cont);
}

template <class Container, class EP = decltype(std17::data(std::declval<Container&>()))>
EXPLICIT_NODISCARD_ATTR inline UTIL_CONFIG_CONSTEXPR auto make_span(Container const& cont)
    -> span<const typename std::remove_pointer<EP>::type> {
  return span<const typename std::remove_pointer<EP>::type>(cont);
}

template <class Container>
EXPLICIT_NODISCARD_ATTR inline UTIL_CONFIG_CONSTEXPR span<typename Container::value_type> make_span(
    with_container_t, Container& cont) UTIL_CONFIG_NOEXCEPT {
  return span<typename Container::value_type>(with_container, cont);
}

template <class Container>
EXPLICIT_NODISCARD_ATTR inline UTIL_CONFIG_CONSTEXPR span<const typename Container::value_type> make_span(
    with_container_t, Container const& cont) UTIL_CONFIG_NOEXCEPT {
  return span<const typename Container::value_type>(with_container, cont);
}

#  endif

#  if defined(LIBATFRAME_UTILS_GSL_TEST_STL_STRING_VIEW) && LIBATFRAME_UTILS_GSL_TEST_STL_STRING_VIEW
using std::basic_string_view;
using std::string_view;
using std::wstring_view;
#  elif defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL_SPAN) && \
      LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL_SPAN
template <class CharT, class Traits = std::char_traits<CharT> >
using basic_string_view = span<CharT>;
using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
#  endif

using zstring = char*;
using czstring = const char*;
using wzstring = wchar_t*;
using cwzstring = const wchar_t*;
using std::to_string;

template <class T, typename = typename std::enable_if<std::is_pointer<T>::value>::type>
using owner = T;

// template< class T >
// class not_null;

#  if defined(LIBATFRAME_UTILS_GSL_TEST_FALLBACK_STL_BYTE) && LIBATFRAME_UTILS_GSL_TEST_FALLBACK_STL_BYTE
using std::byte;
#    if defined(LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL_SPAN) && \
        LIBATFRAME_UTILS_ENABLE_GSL_WITH_FALLBACK_STL_SPAN
template <class T>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline span<const byte> as_bytes(span<T> spn)
    UTIL_CONFIG_NOEXCEPT {
  return span<const byte>(reinterpret_cast<const byte*>(spn.data()), spn.size_bytes());  // NOLINT
}
#    endif
#  endif

template <class F>
class LIBATFRAME_UTILS_API_HEAD_ONLY final_action {
 public:
  explicit final_action(F action) UTIL_CONFIG_NOEXCEPT : action_(std::move(action)), invoke_(true) {}

  final_action(final_action&& other) UTIL_CONFIG_NOEXCEPT : action_(std::move(other.action_)), invoke_(other.invoke_) {
    other.invoke_ = false;
  }

  virtual ~final_action() UTIL_CONFIG_NOEXCEPT {
    if (invoke_) action_();
  }

 private:
  final_action(final_action const&) UTIL_CONFIG_DELETED_FUNCTION;
  final_action& operator=(final_action const&) UTIL_CONFIG_DELETED_FUNCTION;
  final_action& operator=(final_action&&) UTIL_CONFIG_DELETED_FUNCTION;

 protected:
  void dismiss() UTIL_CONFIG_NOEXCEPT { invoke_ = false; }

 private:
  F action_;
  bool invoke_;
};

template <class F>
EXPLICIT_NODISCARD_ATTR inline final_action<F> finally(F const& action) UTIL_CONFIG_NOEXCEPT {
  return final_action<F>(action);
}

template <class F>
EXPLICIT_NODISCARD_ATTR inline final_action<F> finally(F&& action) UTIL_CONFIG_NOEXCEPT {
  return final_action<F>(std::forward<F>(action));
}

template <class T, class U>
EXPLICIT_NODISCARD_ATTR LIBATFRAME_UTILS_API_HEAD_ONLY inline UTIL_CONFIG_CONSTEXPR T narrow_cast(U&& u)
    UTIL_CONFIG_NOEXCEPT {
  return static_cast<T>(std::forward<U>(u));
}

}  // namespace gsl
#else
#  if defined(_MSC_VER)
#    error GSL Unsupported
#  else
#    error "GSL Unsupported"
#  endif
#endif

#endif  // GSL_SELECT_GSL_H
