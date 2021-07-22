// Copyright 2021 atframework
// Created by owent on 2021-07-22

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstring>
#include <iosfwd>
#include <iterator>
#include <limits>
#include <string>

#ifdef __cpp_impl_three_way_comparison
#  include <compare>
#endif

#include "config/atframe_utils_build_feature.h"

#ifdef max
#  undef max
#endif

#if (defined(__GNUC__) && !defined(__clang__))
#  define UTIL_NOSTD_INTERNAL_STRING_VIEW_MEMCMP __builtin_memcmp
#elif defined(__clang__)
#  if __has_builtin(__builtin_memcmp)
#    define UTIL_NOSTD_INTERNAL_STRING_VIEW_MEMCMP __builtin_memcmp
#  endif
#endif
#if !defined(UTIL_NOSTD_INTERNAL_STRING_VIEW_MEMCMP)
#  define UTIL_NOSTD_INTERNAL_STRING_VIEW_MEMCMP memcmp
#endif

#if defined(__cplusplus) && __cplusplus >= 201402L
#  define UTIL_NOSTD_STRING_VIEW_CXX14_CONSTEXPR constexpr
#elif defined(_MSVC_LANG) && _MSVC_LANG >= 201402L
#  define UTIL_NOSTD_STRING_VIEW_CXX14_CONSTEXPR constexpr
#else
#  define UTIL_NOSTD_STRING_VIEW_CXX14_CONSTEXPR
#endif

namespace util {
namespace nostd {

template <class CharT, class Traits = std::char_traits<CharT> >
class LIBATFRAME_UTILS_API_HEAD_ONLY basic_string_view {
 public:
  using traits_type = Traits;
  using value_type = CharT;
  using pointer = CharT*;
  using const_pointer = const CharT*;
  using reference = CharT&;
  using const_reference = const CharT&;
  using const_iterator = const CharT*;
  using iterator = const_iterator;
  using const_reverse_iterator = std::reverse_iterator<const_iterator>;
  using reverse_iterator = const_reverse_iterator;
  using size_type = size_t;
  using difference_type = std::ptrdiff_t;

  static constexpr size_type npos = static_cast<size_type>(-1);

 public:
  // Null `basic_string_view` constructor
  constexpr basic_string_view() noexcept : ptr_(nullptr), length_(0) {}

  template <typename Allocator>
  basic_string_view(  // NOLINT(runtime/explicit)
      const std::basic_string<CharT, std::char_traits<CharT>, Allocator>& str) noexcept
      // This is implemented in terms of `basic_string_view(p, n)` so `str.size()`
      // doesn't need to be reevaluated after `ptr_` is set.
      : basic_string_view(str.data(), str.size()) {}

  // Implicit constructor of a `basic_string_view` from NUL-terminated `str`. When
  // accepting possibly null strings.
  constexpr basic_string_view(const_pointer str)  // NOLINT(runtime/explicit)
      : ptr_(str), length_(str ? _strlen_internal(str) : 0) {}

  // Implicit constructor of a `basic_string_view` from a `const_pointer` and length.
  constexpr basic_string_view(const_pointer data, size_type len) : ptr_(data), length_(len) {}

  // NOTE: Harmlessly omitted to work around gdb bug.
  //   constexpr basic_string_view(const basic_string_view&) noexcept = default;
  //   basic_string_view& operator=(const basic_string_view&) noexcept = default;

  // Iterators

  // basic_string_view::begin()
  //
  // Returns an iterator pointing to the first character at the beginning of the
  // `basic_string_view`, or `end()` if the `basic_string_view` is empty.
  constexpr const_iterator begin() const noexcept { return ptr_; }

  // basic_string_view::end()
  //
  // Returns an iterator pointing just beyond the last character at the end of
  // the `basic_string_view`. This iterator acts as a placeholder; attempting to
  // access it results in undefined behavior.
  constexpr const_iterator end() const noexcept { return ptr_ + length_; }

  // basic_string_view::cbegin()
  //
  // Returns a const iterator pointing to the first character at the beginning
  // of the `basic_string_view`, or `end()` if the `basic_string_view` is empty.
  constexpr const_iterator cbegin() const noexcept { return begin(); }

  // basic_string_view::cend()
  //
  // Returns a const iterator pointing just beyond the last character at the end
  // of the `basic_string_view`. This pointer acts as a placeholder; attempting to
  // access its element results in undefined behavior.
  constexpr const_iterator cend() const noexcept { return end(); }

  // basic_string_view::rbegin()
  //
  // Returns a reverse iterator pointing to the last character at the end of the
  // `basic_string_view`, or `rend()` if the `basic_string_view` is empty.
  const_reverse_iterator rbegin() const noexcept { return const_reverse_iterator(end()); }

  // basic_string_view::rend()
  //
  // Returns a reverse iterator pointing just before the first character at the
  // beginning of the `basic_string_view`. This pointer acts as a placeholder;
  // attempting to access its element results in undefined behavior.
  const_reverse_iterator rend() const noexcept { return const_reverse_iterator(begin()); }

  // basic_string_view::crbegin()
  //
  // Returns a const reverse iterator pointing to the last character at the end
  // of the `basic_string_view`, or `crend()` if the `basic_string_view` is empty.
  const_reverse_iterator crbegin() const noexcept { return rbegin(); }

  // basic_string_view::crend()
  //
  // Returns a const reverse iterator pointing just before the first character
  // at the beginning of the `basic_string_view`. This pointer acts as a placeholder;
  // attempting to access its element results in undefined behavior.
  const_reverse_iterator crend() const noexcept { return rend(); }

  // Capacity Utilities

  // basic_string_view::size()
  //
  // Returns the number of characters in the `basic_string_view`.
  constexpr size_type size() const noexcept { return length_; }

  // basic_string_view::length()
  //
  // Returns the number of characters in the `basic_string_view`. Alias for `size()`.
  constexpr size_type length() const noexcept { return size(); }

  // basic_string_view::max_size()
  //
  // Returns the maximum number of characters the `basic_string_view` can hold.
  constexpr size_type max_size() const noexcept { return kMaxSize; }

  // basic_string_view::empty()
  //
  // Checks if the `basic_string_view` is empty (refers to no characters).
  constexpr bool empty() const noexcept { return length_ == 0; }

  // basic_string_view::operator[]
  //
  // Returns the ith element of the `basic_string_view` using the array operator.
  // Note that this operator does not perform any bounds checking.
  constexpr const_reference operator[](size_type i) const { return ptr_[i]; }

  // basic_string_view::at()
  //
  // Returns the ith element of the `basic_string_view`. Bounds checking is performed,
  // and an exception of type `std::out_of_range` will be thrown on invalid
  // access.
  constexpr const_reference at(size_type i) const { return ptr_[i]; }

  // basic_string_view::front()
  //
  // Returns the first element of a `basic_string_view`.
  constexpr const_reference front() const { return ptr_[0]; }

  // basic_string_view::back()
  //
  // Returns the last element of a `basic_string_view`.
  constexpr const_reference back() const { return ptr_[size() - 1]; }

  // basic_string_view::data()
  //
  // Returns a pointer to the underlying character array (which is of course
  // stored elsewhere). Note that `basic_string_view::data()` may contain embedded nul
  // characters, but the returned buffer may or may not be NUL-terminated;
  // therefore, do not pass `data()` to a routine that expects a NUL-terminated
  // string.
  constexpr const_pointer data() const noexcept { return ptr_; }

  // Modifiers

  // basic_string_view::remove_prefix()
  //
  // Removes the first `n` characters from the `basic_string_view`. Note that the
  // underlying string is not changed, only the view.
  UTIL_NOSTD_STRING_VIEW_CXX14_CONSTEXPR void remove_prefix(size_type n) {
    if (n <= length_) {
      ptr_ += n;
      length_ -= n;
    } else {
      ptr_ += length_;
      length_ = 0;
    }
  }

  // basic_string_view::remove_suffix()
  //
  // Removes the last `n` characters from the `basic_string_view`. Note that the
  // underlying string is not changed, only the view.
  UTIL_NOSTD_STRING_VIEW_CXX14_CONSTEXPR void remove_suffix(size_type n) {
    if (n <= length_) {
      length_ -= n;
    } else {
      length_ = 0;
    }
  }

  // basic_string_view::swap()
  //
  // Swaps this `basic_string_view` with another `basic_string_view`.
  UTIL_NOSTD_STRING_VIEW_CXX14_CONSTEXPR void swap(basic_string_view& s) noexcept {
    auto t = *this;
    *this = s;
    s = t;
  }

  // Converts to `std::basic_string`.
  template <typename A>
  explicit operator std::basic_string<value_type, traits_type, A>() const {
    if (!data()) return {};
    return std::basic_string<value_type, traits_type, A>(data(), size());
  }

  // basic_string_view::copy()
  //
  // Copies the contents of the `basic_string_view` at offset `pos` and length `n`
  // into `buf`.
  size_type copy(value_type* buf, size_type n, size_type pos = 0) const {
    if (pos > length_) {
      return 0;
    }
    size_type rlen = Min(length_ - pos, n);
    if (rlen > 0) {
      const CharT* start = ptr_ + pos;
      traits_type::copy(buf, start, rlen);
    }
    return rlen;
  }

  // basic_string_view::substr()
  //
  // Returns a "substring" of the `basic_string_view` (at offset `pos` and length
  // `n`) as another basic_string_view. This function throws `std::out_of_bounds` if
  // `pos > size`.
  constexpr basic_string_view substr(size_type pos = 0, size_type n = npos) const {
    if (pos > length_) {
      return basic_string_view();
    }

    return basic_string_view(ptr_ + pos, Min(n, length_ - pos));
  }

  // basic_string_view::compare()
  //
  // Performs a lexicographical comparison between this `basic_string_view` and
  // another `basic_string_view` `x`, returning a negative value if `*this` is less
  // than `x`, 0 if `*this` is equal to `x`, and a positive value if `*this`
  // is greater than `x`.
  constexpr int compare(basic_string_view x) const noexcept {
    return _compare_impl(length_, x.length_,
                         Min(length_, x.length_) == 0
                             ? 0
                             : UTIL_NOSTD_INTERNAL_STRING_VIEW_MEMCMP(ptr_, x.ptr_, Min(length_, x.length_)));
  }

  // Overload of `basic_string_view::compare()` for comparing a substring of the
  // 'basic_string_view` and another `basic_string_view`.
  constexpr int compare(size_type pos1, size_type count1, basic_string_view v) const {
    return substr(pos1, count1).compare(v);
  }

  // Overload of `basic_string_view::compare()` for comparing a substring of the
  // `basic_string_view` and a substring of another `Basic_string_view`.
  constexpr int compare(size_type pos1, size_type count1, basic_string_view v, size_type pos2, size_type count2) const {
    return substr(pos1, count1).compare(v.substr(pos2, count2));
  }

  // Overload of `basic_string_view::compare()` for comparing a `basic_string_view` and a
  // a different C-style string `s`.
  constexpr int compare(const CharT* s) const { return compare(basic_string_view(s)); }

  // Overload of `basic_string_view::compare()` for comparing a substring of the
  // `basic_string_view` and a different string C-style string `s`.
  constexpr int compare(size_type pos1, size_type count1, const CharT* s) const {
    return substr(pos1, count1).compare(basic_string_view(s));
  }

  // Overload of `basic_string_view::compare()` for comparing a substring of the
  // `basic_string_view` and a substring of a different C-style string `s`.
  constexpr int compare(size_type pos1, size_type count1, const CharT* s, size_type count2) const {
    return substr(pos1, count1).compare(basic_string_view(s, count2));
  }

  // Find Utilities

  // basic_string_view::find()
  //
  // Finds the first occurrence of the substring `s` within the `basic_string_view`,
  // returning the position of the first character's match, or `npos` if no
  // match was found.
  size_type find(basic_string_view s, size_type pos = 0) const noexcept {
    if (empty() || pos > length_) {
      if (empty() && pos == 0 && s.empty()) return 0;
      return npos;
    }
    const CharT* result = reinterpret_cast<const CharT*>(
        _memmatch(reinterpret_cast<const unsigned char*>(ptr_ + pos), (length_ - pos) * sizeof(CharT),
                  reinterpret_cast<const unsigned char*>(s.ptr_), s.length_ * sizeof(CharT)));
    return result ? result - ptr_ : npos;
  }

  // Overload of `basic_string_view::find()` for finding the given character `c`
  // within the `basic_string_view`.
  size_type find(CharT c, size_type pos = 0) const noexcept {
    if (empty() || pos >= length_) {
      return npos;
    }
    auto result = std::find(ptr_ + pos, ptr_ + length_, c);
    if (result == ptr_ + length_) {
      return npos;
    }
    return result - ptr_;
  }

  // Overload of `basic_string_view::find()` for finding a substring of a different
  // C-style string `s` within the `basic_string_view`.
  size_type find(const CharT* s, size_type pos, size_type count) const {
    return find(basic_string_view(s, count), pos);
  }

  // Overload of `basic_string_view::find()` for finding a different C-style string
  // `s` within the `basic_string_view`.
  size_type find(const CharT* s, size_type pos = 0) const { return find(basic_string_view(s), pos); }

  // basic_string_view::rfind()
  //
  // Finds the last occurrence of a substring `s` within the `basic_string_view`,
  // returning the position of the first character's match, or `npos` if no
  // match was found.
  size_type rfind(basic_string_view s, size_type pos = npos) const noexcept {
    if (length_ < s.length_) return npos;
    if (s.empty()) return Min(length_, pos);
    const CharT* last = ptr_ + Min(length_ - s.length_, pos) + s.length_;
    const CharT* result = std::find_end(ptr_, last, s.ptr_, s.ptr_ + s.length_);
    return result != last ? result - ptr_ : npos;
  }

  // Overload of `basic_string_view::rfind()` for finding the last given character `c`
  // within the `basic_string_view`.
  size_type rfind(CharT c, size_type pos = npos) const noexcept {
    if (empty()) return npos;
    for (size_type i = Min(pos, length_ - 1);; --i) {
      if (ptr_[i] == c) {
        return i;
      }
      if (i == 0) break;
    }
    return npos;
  }

  // Overload of `basic_string_view::rfind()` for finding a substring of a different
  // C-style string `s` within the `basic_string_view`.
  size_type rfind(const CharT* s, size_type pos, size_type count) const {
    return rfind(basic_string_view(s, count), pos);
  }

  // Overload of `basic_string_view::rfind()` for finding a different C-style string
  // `s` within the `basic_string_view`.
  size_type rfind(const CharT* s, size_type pos = npos) const { return rfind(basic_string_view(s), pos); }

 private:
  static constexpr size_type kMaxSize = (std::numeric_limits<difference_type>::max)();

  static constexpr size_type _strlen_internal(const CharT* str) {
#if defined(_MSC_VER) && _MSC_VER >= 1910 && !defined(__clang__)
    // MSVC 2017+ can evaluate this at compile-time.
    const CharT* begin = str;
    while (*str != '\0') ++str;
    return str - begin;
#elif (defined(__GNUC__) && !defined(__clang__))
    // GCC has __builtin_strlen according to
    // https://gcc.gnu.org/onlinedocs/gcc-4.7.0/gcc/Other-Builtins.html
    return __builtin_strlen(str);
#elif defined(__clang__)
#  if __has_builtin(__builtin_strlen)
    return __builtin_strlen(str);
#  else
    return str ? strlen(str) : 0;
#  endif
#else
    return str ? strlen(str) : 0;
#endif
  }

  static constexpr size_t Min(size_type length_a, size_type length_b) {
    return length_a < length_b ? length_a : length_b;
  }

  static constexpr int _compare_impl(size_type length_a, size_type length_b, int compare_result) {
    return compare_result == 0 ? static_cast<int>(length_a > length_b) - static_cast<int>(length_a < length_b)
                               : (compare_result < 0 ? -1 : 1);
  }

  static const unsigned char* _memmatch(const unsigned char* phaystack, size_t haylen, const unsigned char* pneedle,
                                        size_t neelen) {
    if (0 == neelen) {
      return phaystack;  // even if haylen is 0
    }
    if (haylen < neelen) return nullptr;

    const unsigned char* match;
    const unsigned char* hayend = phaystack + haylen - neelen + 1;
    // A static cast is used here to work around the fact that memchr returns
    // a void* on Posix-compliant systems and const void* on Windows.
    while ((match = static_cast<const unsigned char*>(memchr(phaystack, pneedle[0], hayend - phaystack)))) {
      if (UTIL_NOSTD_INTERNAL_STRING_VIEW_MEMCMP(match, pneedle, neelen) == 0)
        return match;
      else
        phaystack = match + 1;
    }
    return nullptr;
  }

 private:
  const_pointer ptr_;
  size_type length_;
};

// This large function is defined inline so that in a fairly common case where
// one of the arguments is a literal, the compiler can elide a lot of the
// following comparisons.
template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr bool operator==(basic_string_view<CharT, Traits> x,
                                                         basic_string_view<CharT, Traits> y) noexcept {
  return x.size() == y.size() &&
         (x.empty() || UTIL_NOSTD_INTERNAL_STRING_VIEW_MEMCMP(x.data(), y.data(), x.size()) == 0);
}

template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr bool operator==(std::nullptr_t, basic_string_view<CharT, Traits> y) noexcept {
  return y.empty();
}

template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr bool operator==(basic_string_view<CharT, Traits> x, std::nullptr_t) noexcept {
  return x.empty();
}

#ifdef __cpp_impl_three_way_comparison
template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr std::strong_ordering operator<=>(basic_string_view<CharT, Traits> x,
                                                                          basic_string_view<CharT, Traits> y) noexcept {
  auto result = x.compare(y);
  if (result < 0) {
    return std::strong_ordering::less;
  } else if (result > 0) {
    return std::strong_ordering::greater;
  }
  return std::strong_ordering::equal;
}

template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr std::strong_ordering operator<=>(std::nullptr_t,
                                                                          basic_string_view<CharT, Traits> y) noexcept {
  return basic_string_view<CharT, Traits>(nullptr) <=> y;
}

template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr std::strong_ordering operator<=>(basic_string_view<CharT, Traits> x,
                                                                          std::nullptr_t) noexcept {
  return x <=> basic_string_view<CharT, Traits>(nullptr);
}
#else
template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr bool operator!=(basic_string_view<CharT, Traits> x,
                                                         basic_string_view<CharT, Traits> y) noexcept {
  return !(x == y);
}

template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr bool operator<(basic_string_view<CharT, Traits> x,
                                                        basic_string_view<CharT, Traits> y) noexcept {
  return x.compare(y) < 0;
}

template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr bool operator>(basic_string_view<CharT, Traits> x,
                                                        basic_string_view<CharT, Traits> y) noexcept {
  return y < x;
}

template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr bool operator<=(basic_string_view<CharT, Traits> x,
                                                         basic_string_view<CharT, Traits> y) noexcept {
  return !(y < x);
}

template <class CharT, class Traits>
LIBATFRAME_UTILS_API_HEAD_ONLY constexpr bool operator>=(basic_string_view<CharT, Traits> x,
                                                         basic_string_view<CharT, Traits> y) noexcept {
  return !(x < y);
}
#endif

// IO Insertion Operator
template <class CharT, class Traits = std::char_traits<CharT> >
LIBATFRAME_UTILS_API_HEAD_ONLY std::basic_ostream<CharT, Traits>& operator<<(
    std::basic_ostream<CharT, Traits>& os, basic_string_view<CharT, Traits> const& spn) {
  typename std::basic_ostream<CharT, Traits>::sentry sentry(os);

  if (!os) return os;

  const std::streamsize length = static_cast<std::streamsize>(spn.size());

  // Write span characters
  os.rdbuf()->sputn(spn.begin(), length);

  return os;
}

// Alias
using string_view = basic_string_view<char>;
using wstring_view = basic_string_view<wchar_t>;
}  // namespace nostd
}  // namespace util

#undef UTIL_NOSTD_STRING_VIEW_CXX14_CONSTEXPR
#undef UTIL_NOSTD_INTERNAL_STRING_VIEW_MEMCMP
