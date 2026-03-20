// Copyright 2026 atframework

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <climits>
#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

#if defined(_MSC_VER) && !defined(__clang__)
#  include <intrin.h>
#  pragma intrinsic(_BitScanReverse)
#  pragma intrinsic(_BitScanForward)
#  if defined(_M_X64) || defined(_M_ARM64)
#    pragma intrinsic(_BitScanReverse64)
#    pragma intrinsic(_BitScanForward64)
#  endif
#endif

#if (defined(__cplusplus) && __cplusplus >= 202002L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 202002L)
#  if defined(__cpp_lib_bitops) && __cpp_lib_bitops >= 201907L
#    include <bit>
#    define ATFW_UTIL_MACRO_HAVE_STD_BITOPS 1
#  endif
#  if defined(__cpp_lib_int_pow2) && __cpp_lib_int_pow2 >= 202002L
#    define ATFW_UTIL_MACRO_HAVE_STD_INT_POW2 1
#  endif
#  if defined(__cpp_lib_endian) && __cpp_lib_endian >= 201907L
#    if !(defined(ATFW_UTIL_MACRO_HAVE_STD_BITOPS) && ATFW_UTIL_MACRO_HAVE_STD_BITOPS)
#      include <bit>
#    endif
#    define ATFW_UTIL_MACRO_HAVE_STD_ENDIAN 1
#  endif
#endif

#if !defined(ATFW_UTIL_MACRO_HAVE_STD_ENDIAN)
#  if defined(__has_include)
#    if __has_include(<bit>)
#      include <bit>
#      if defined(__cpp_lib_endian) && __cpp_lib_endian >= 201907L
#        define ATFW_UTIL_MACRO_HAVE_STD_ENDIAN 1
#      endif
#    endif
#  endif
#endif

#if !defined(ATFW_UTIL_MACRO_HAVE_IF_CONSTEXPR)
#  if defined(__cpp_if_constexpr) && __cpp_if_constexpr >= 201606L
#    define ATFW_UTIL_MACRO_HAVE_IF_CONSTEXPR 1
#  endif
#endif

// Compile-time endian detection via platform macros
#if !defined(ATFW_UTIL_ENDIAN_COMPILETIME_LITTLE) && !defined(ATFW_UTIL_ENDIAN_COMPILETIME_BIG)
#  if defined(ATFW_UTIL_MACRO_HAVE_STD_ENDIAN) && ATFW_UTIL_MACRO_HAVE_STD_ENDIAN
// Will use std::endian at compile time
#  elif defined(_WIN32) || defined(__LITTLE_ENDIAN__) || \
      (defined(__BYTE_ORDER__) && defined(__ORDER_LITTLE_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__))
#    define ATFW_UTIL_ENDIAN_COMPILETIME_LITTLE 1
#  elif defined(__BIG_ENDIAN__) || \
      (defined(__BYTE_ORDER__) && defined(__ORDER_BIG_ENDIAN__) && (__BYTE_ORDER__ == __ORDER_BIG_ENDIAN__))
#    define ATFW_UTIL_ENDIAN_COMPILETIME_BIG 1
#  endif
#endif

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace bit {

#if defined(ATFW_UTIL_MACRO_HAVE_STD_BITOPS) && ATFW_UTIL_MACRO_HAVE_STD_BITOPS

using std::countl_one;
using std::countl_zero;
using std::countr_one;
using std::countr_zero;
using std::popcount;
using std::rotl;
using std::rotr;

#else  // !ATFW_UTIL_MACRO_HAVE_STD_BITOPS

// ============================================================================
// Internal implementation details (C++14 compatible)
// ============================================================================
namespace detail {

// Type trait to check if T is an unsigned integer type
template <typename T>
struct is_unsigned_integer : std::integral_constant<bool, std::is_integral<T>::value && std::is_unsigned<T>::value &&
                                                              !std::is_same<T, bool>::value> {};

// ============================================================================
// countl_zero implementation - count leading zeros
// ============================================================================

// Tag dispatch for size selection
template <size_t N>
struct size_tag {};

// --- GCC/Clang implementation using __builtin_clz ---
#  if defined(__GNUC__) || defined(__clang__)

template <typename T>
ATFW_UTIL_FORCEINLINE int countl_zero_impl(T x, size_tag<1>) noexcept {
  return x == 0 ? 8 : (__builtin_clz(static_cast<unsigned int>(x)) - 24);
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countl_zero_impl(T x, size_tag<2>) noexcept {
  return x == 0 ? 16 : (__builtin_clz(static_cast<unsigned int>(x)) - 16);
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countl_zero_impl(T x, size_tag<4>) noexcept {
  return x == 0 ? 32 : __builtin_clz(static_cast<unsigned int>(x));
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countl_zero_impl(T x, size_tag<8>) noexcept {
  return x == 0 ? 64 : __builtin_clzll(static_cast<unsigned long long>(x));
}

// --- MSVC implementation using _BitScanReverse ---
#  elif defined(_MSC_VER)

template <typename T>
ATFW_UTIL_FORCEINLINE int countl_zero_impl(T x, size_tag<1>) noexcept {
  unsigned long index;
  return _BitScanReverse(&index, static_cast<unsigned long>(x)) ? static_cast<int>(7 - index) : 8;
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countl_zero_impl(T x, size_tag<2>) noexcept {
  unsigned long index;
  return _BitScanReverse(&index, static_cast<unsigned long>(x)) ? static_cast<int>(15 - index) : 16;
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countl_zero_impl(T x, size_tag<4>) noexcept {
  unsigned long index;
  return _BitScanReverse(&index, static_cast<unsigned long>(x)) ? static_cast<int>(31 - index) : 32;
}

#    if defined(_M_X64) || defined(_M_ARM64)
template <typename T>
ATFW_UTIL_FORCEINLINE int countl_zero_impl(T x, size_tag<8>) noexcept {
  unsigned long index;
  return _BitScanReverse64(&index, static_cast<unsigned long long>(x)) ? static_cast<int>(63 - index) : 64;
}

#    else   // 32-bit MSVC
template <typename T>
ATFW_UTIL_FORCEINLINE int countl_zero_impl(T x, size_tag<8>) noexcept {
  unsigned long index;
  uint32_t high = static_cast<uint32_t>(static_cast<unsigned long long>(x) >> 32);
  if (_BitScanReverse(&index, high)) {
    return static_cast<int>(31 - index);
  }
  uint32_t low = static_cast<uint32_t>(x);
  return _BitScanReverse(&index, low) ? static_cast<int>(63 - index) : 64;
}
#    endif  // _M_X64 || _M_ARM64

#  else  // Fallback for other compilers

template <typename T>
ATFW_UTIL_SYMBOL_VISIBLE inline int countl_zero_impl(T x, size_tag<1>) noexcept {
  if (x == 0) return 8;
  int n = 0;
  if (!(x & 0xF0)) {
    n += 4;
    x <<= 4;
  }
  if (!(x & 0xC0)) {
    n += 2;
    x <<= 2;
  }
  if (!(x & 0x80)) {
    n += 1;
  }
  return n;
}

template <typename T>
ATFW_UTIL_SYMBOL_VISIBLE inline int countl_zero_impl(T x, size_tag<2>) noexcept {
  if (x == 0) return 16;
  int n = 0;
  if (!(x & 0xFF00)) {
    n += 8;
    x <<= 8;
  }
  if (!(x & 0xF000)) {
    n += 4;
    x <<= 4;
  }
  if (!(x & 0xC000)) {
    n += 2;
    x <<= 2;
  }
  if (!(x & 0x8000)) {
    n += 1;
  }
  return n;
}

template <typename T>
ATFW_UTIL_SYMBOL_VISIBLE inline int countl_zero_impl(T x, size_tag<4>) noexcept {
  if (x == 0) return 32;
  int n = 0;
  if (!(x & 0xFFFF0000U)) {
    n += 16;
    x <<= 16;
  }
  if (!(x & 0xFF000000U)) {
    n += 8;
    x <<= 8;
  }
  if (!(x & 0xF0000000U)) {
    n += 4;
    x <<= 4;
  }
  if (!(x & 0xC0000000U)) {
    n += 2;
    x <<= 2;
  }
  if (!(x & 0x80000000U)) {
    n += 1;
  }
  return n;
}

template <typename T>
ATFW_UTIL_SYMBOL_VISIBLE inline int countl_zero_impl(T x, size_tag<8>) noexcept {
  if (x == 0) return 64;
  int n = 0;
  if (!(x & 0xFFFFFFFF00000000ULL)) {
    n += 32;
    x <<= 32;
  }
  if (!(x & 0xFFFF000000000000ULL)) {
    n += 16;
    x <<= 16;
  }
  if (!(x & 0xFF00000000000000ULL)) {
    n += 8;
    x <<= 8;
  }
  if (!(x & 0xF000000000000000ULL)) {
    n += 4;
    x <<= 4;
  }
  if (!(x & 0xC000000000000000ULL)) {
    n += 2;
    x <<= 2;
  }
  if (!(x & 0x8000000000000000ULL)) {
    n += 1;
  }
  return n;
}

#  endif  // Compiler selection

// ============================================================================
// countr_zero implementation - count trailing zeros
// ============================================================================

// --- GCC/Clang implementation using __builtin_ctz ---
#  if defined(__GNUC__) || defined(__clang__)

template <typename T>
ATFW_UTIL_FORCEINLINE int countr_zero_impl(T x, size_tag<1>) noexcept {
  // Set bit 8 to avoid returning 32 when x == 0
  return x == 0 ? 8 : __builtin_ctz(static_cast<unsigned int>(x) | 0x100U);
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countr_zero_impl(T x, size_tag<2>) noexcept {
  return x == 0 ? 16 : __builtin_ctz(static_cast<unsigned int>(x) | 0x10000U);
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countr_zero_impl(T x, size_tag<4>) noexcept {
  return x == 0 ? 32 : __builtin_ctz(static_cast<unsigned int>(x));
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countr_zero_impl(T x, size_tag<8>) noexcept {
  return x == 0 ? 64 : __builtin_ctzll(static_cast<unsigned long long>(x));
}

// --- MSVC implementation using _BitScanForward ---
#  elif defined(_MSC_VER)

template <typename T>
ATFW_UTIL_FORCEINLINE int countr_zero_impl(T x, size_tag<1>) noexcept {
  unsigned long index;
  return _BitScanForward(&index, static_cast<unsigned long>(x) | 0x100UL) ? static_cast<int>(index) : 8;
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countr_zero_impl(T x, size_tag<2>) noexcept {
  unsigned long index;
  return _BitScanForward(&index, static_cast<unsigned long>(x) | 0x10000UL) ? static_cast<int>(index) : 16;
}

template <typename T>
ATFW_UTIL_FORCEINLINE int countr_zero_impl(T x, size_tag<4>) noexcept {
  unsigned long index;
  return _BitScanForward(&index, static_cast<unsigned long>(x)) ? static_cast<int>(index) : 32;
}

#    if defined(_M_X64) || defined(_M_ARM64)
template <typename T>
ATFW_UTIL_FORCEINLINE int countr_zero_impl(T x, size_tag<8>) noexcept {
  unsigned long index;
  return _BitScanForward64(&index, static_cast<unsigned long long>(x)) ? static_cast<int>(index) : 64;
}
#    else   // 32-bit MSVC
template <typename T>
ATFW_UTIL_FORCEINLINE int countr_zero_impl(T x, size_tag<8>) noexcept {
  unsigned long index;
  uint32_t low = static_cast<uint32_t>(x);
  if (_BitScanForward(&index, low)) {
    return static_cast<int>(index);
  }
  uint32_t high = static_cast<uint32_t>(static_cast<unsigned long long>(x) >> 32);
  return _BitScanForward(&index, high) ? static_cast<int>(32 + index) : 64;
}
#    endif  // _M_X64 || _M_ARM64

#  else  // Fallback for other compilers

template <typename T>
ATFW_UTIL_SYMBOL_VISIBLE inline int countr_zero_impl(T x, size_tag<1>) noexcept {
  if (x == 0) return 8;
  int n = 0;
  if (!(x & 0x0F)) {
    n += 4;
    x >>= 4;
  }
  if (!(x & 0x03)) {
    n += 2;
    x >>= 2;
  }
  if (!(x & 0x01)) {
    n += 1;
  }
  return n;
}

template <typename T>
ATFW_UTIL_SYMBOL_VISIBLE inline int countr_zero_impl(T x, size_tag<2>) noexcept {
  if (x == 0) return 16;
  int n = 0;
  if (!(x & 0x00FF)) {
    n += 8;
    x >>= 8;
  }
  if (!(x & 0x000F)) {
    n += 4;
    x >>= 4;
  }
  if (!(x & 0x0003)) {
    n += 2;
    x >>= 2;
  }
  if (!(x & 0x0001)) {
    n += 1;
  }
  return n;
}

template <typename T>
ATFW_UTIL_SYMBOL_VISIBLE inline int countr_zero_impl(T x, size_tag<4>) noexcept {
  if (x == 0) return 32;
  int n = 0;
  if (!(x & 0x0000FFFFU)) {
    n += 16;
    x >>= 16;
  }
  if (!(x & 0x000000FFU)) {
    n += 8;
    x >>= 8;
  }
  if (!(x & 0x0000000FU)) {
    n += 4;
    x >>= 4;
  }
  if (!(x & 0x00000003U)) {
    n += 2;
    x >>= 2;
  }
  if (!(x & 0x00000001U)) {
    n += 1;
  }
  return n;
}

template <typename T>
ATFW_UTIL_SYMBOL_VISIBLE inline int countr_zero_impl(T x, size_tag<8>) noexcept {
  if (x == 0) return 64;
  int n = 0;
  if (!(x & 0x00000000FFFFFFFFULL)) {
    n += 32;
    x >>= 32;
  }
  if (!(x & 0x000000000000FFFFULL)) {
    n += 16;
    x >>= 16;
  }
  if (!(x & 0x00000000000000FFULL)) {
    n += 8;
    x >>= 8;
  }
  if (!(x & 0x000000000000000FULL)) {
    n += 4;
    x >>= 4;
  }
  if (!(x & 0x0000000000000003ULL)) {
    n += 2;
    x >>= 2;
  }
  if (!(x & 0x0000000000000001ULL)) {
    n += 1;
  }
  return n;
}

#  endif  // Compiler selection

// ============================================================================
// popcount implementation - count set bits
// ============================================================================

// Optimized popcount using parallel bit counting (SWAR algorithm)
ATFW_UTIL_FORCEINLINE int popcount_impl(uint8_t x, size_tag<1>) noexcept {
  x = static_cast<uint8_t>(x - ((x >> 1) & 0x55U));
  x = static_cast<uint8_t>((x & 0x33U) + ((x >> 2) & 0x33U));
  return static_cast<int>((x + (x >> 4)) & 0x0FU);
}

ATFW_UTIL_FORCEINLINE int popcount_impl(uint16_t x, size_tag<2>) noexcept {
  x = static_cast<uint16_t>(x - ((x >> 1) & 0x5555U));
  x = static_cast<uint16_t>((x & 0x3333U) + ((x >> 2) & 0x3333U));
  x = static_cast<uint16_t>((x + (x >> 4)) & 0x0F0FU);
  return static_cast<int>((x + (x >> 8)) & 0x1FU);
}

ATFW_UTIL_FORCEINLINE int popcount_impl(uint32_t x, size_tag<4>) noexcept {
#  if defined(__GNUC__) || defined(__clang__)
  return __builtin_popcount(x);
#  elif defined(_MSC_VER) && (defined(__AVX__) || defined(__POPCNT__))
  return static_cast<int>(__popcnt(x));
#  else
  x = x - ((x >> 1) & 0x55555555U);
  x = (x & 0x33333333U) + ((x >> 2) & 0x33333333U);
  x = (x + (x >> 4)) & 0x0F0F0F0FU;
  return static_cast<int>((x * 0x01010101U) >> 24);
#  endif
}

ATFW_UTIL_FORCEINLINE int popcount_impl(uint64_t x, size_tag<8>) noexcept {
#  if defined(__GNUC__) || defined(__clang__)
  return __builtin_popcountll(x);
#  elif defined(_MSC_VER) && (defined(__AVX__) || defined(__POPCNT__))
#    if defined(_M_X64)
  return static_cast<int>(__popcnt64(x));
#    else
  return static_cast<int>(__popcnt(static_cast<uint32_t>(x)) + __popcnt(static_cast<uint32_t>(x >> 32)));
#    endif
#  else
  x = x - ((x >> 1) & 0x5555555555555555ULL);
  x = (x & 0x3333333333333333ULL) + ((x >> 2) & 0x3333333333333333ULL);
  x = (x + (x >> 4)) & 0x0F0F0F0F0F0F0F0FULL;
  return static_cast<int>((x * 0x0101010101010101ULL) >> 56);
#  endif
}

}  // namespace detail

// ============================================================================
// Public API (C++14 compatible)
// ============================================================================

// countl_zero: Count leading zeros
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, int>::type countl_zero(
    T x) noexcept {
  return detail::countl_zero_impl(x, detail::size_tag<sizeof(T)>{});
}

// countl_one: Count leading ones
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, int>::type countl_one(
    T x) noexcept {
  return countl_zero(static_cast<T>(~x));
}

// countr_zero: Count trailing zeros
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, int>::type countr_zero(
    T x) noexcept {
  return detail::countr_zero_impl(x, detail::size_tag<sizeof(T)>{});
}

// countr_one: Count trailing ones
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, int>::type countr_one(
    T x) noexcept {
  return countr_zero(static_cast<T>(~x));
}

// popcount: Count set bits
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, int>::type popcount(T x) noexcept {
  using unsigned_type = typename std::conditional<
      sizeof(T) == 1, uint8_t,
      typename std::conditional<sizeof(T) == 2, uint16_t,
                                typename std::conditional<sizeof(T) <= 4, uint32_t, uint64_t>::type>::type>::type;
  return detail::popcount_impl(static_cast<unsigned_type>(x), detail::size_tag<sizeof(unsigned_type)>{});
}

// rotl: Rotate left
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, T>::type rotl(T x,
                                                                                                   int s) noexcept {
  constexpr int N = std::numeric_limits<T>::digits;
  const int r = s % N;
  if (r == 0) {
    return x;
  }
  if (r > 0) {
    return static_cast<T>((x << r) | (x >> (N - r)));
  }
  return static_cast<T>((x >> (-r)) | (x << (N + r)));
}

// rotr: Rotate right
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, T>::type rotr(T x,
                                                                                                   int s) noexcept {
  constexpr int N = std::numeric_limits<T>::digits;
  const int r = s % N;
  if (r == 0) {
    return x;
  }
  if (r > 0) {
    return static_cast<T>((x >> r) | (x << (N - r)));
  }
  return static_cast<T>((x << (-r)) | (x >> (N + r)));
}

#endif  // ATFW_UTIL_MACRO_HAVE_STD_BITOPS

// ============================================================================
// bit_width, bit_floor, bit_ceil
// ============================================================================

#if defined(ATFW_UTIL_MACRO_HAVE_STD_INT_POW2) && ATFW_UTIL_MACRO_HAVE_STD_INT_POW2

using std::bit_ceil;
using std::bit_floor;
using std::bit_width;
using std::has_single_bit;

#else  // !ATFW_UTIL_MACRO_HAVE_STD_INT_POW2

// bit_width: Returns the number of bits needed to represent x
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, int>::type bit_width(
    T x) noexcept {
  return std::numeric_limits<T>::digits - countl_zero(x);
}

// bit_floor: Returns the largest power of 2 not greater than x
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, T>::type bit_floor(T x) noexcept {
  if (x == 0) {
    return 0;
  }
  return static_cast<T>(static_cast<T>(1) << (bit_width(x) - 1));
}

// bit_ceil: Returns the smallest power of 2 not less than x
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, T>::type bit_ceil(T x) noexcept {
  if (x <= 1) {
    return 1;
  }
  return static_cast<T>(static_cast<T>(1) << bit_width(static_cast<T>(x - 1)));
}

// has_single_bit: Returns true if x is a power of 2
template <typename T>
ATFW_UTIL_FORCEINLINE typename std::enable_if<detail::is_unsigned_integer<T>::value, bool>::type has_single_bit(
    T x) noexcept {
  return x != 0 && (x & (x - 1)) == 0;
}

#endif  // ATFW_UTIL_MACRO_HAVE_STD_INT_POW2

// ============================================================================
// Endian detection and byte-order conversion utilities
// ============================================================================

#if defined(ATFW_UTIL_MACRO_HAVE_STD_ENDIAN) && ATFW_UTIL_MACRO_HAVE_STD_ENDIAN && \
    defined(ATFW_UTIL_MACRO_HAVE_IF_CONSTEXPR) && ATFW_UTIL_MACRO_HAVE_IF_CONSTEXPR
#  define ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR 1
#else
#  define ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR 0
#endif

namespace detail {

#if !ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
#  if defined(ATFW_UTIL_ENDIAN_COMPILETIME_LITTLE) && ATFW_UTIL_ENDIAN_COMPILETIME_LITTLE
ATFW_UTIL_FORCEINLINE constexpr bool is_little_endian_detect() noexcept { return true; }
#  elif defined(ATFW_UTIL_ENDIAN_COMPILETIME_BIG) && ATFW_UTIL_ENDIAN_COMPILETIME_BIG
ATFW_UTIL_FORCEINLINE constexpr bool is_little_endian_detect() noexcept { return false; }
#  else
// Unknown endianness at compile time, runtime detection required (not constexpr)
ATFW_UTIL_FORCEINLINE constexpr bool is_little_endian_detect() noexcept {
  const uint16_t x = 1;
  return *reinterpret_cast<const uint8_t *>(&x) == 1;
}
#  endif
#endif

}  // namespace detail

// is_little_endian: Returns true if the native byte order is little-endian
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
ATFW_UTIL_FORCEINLINE constexpr bool is_little_endian() noexcept { return std::endian::native == std::endian::little; }
#elif defined(ATFW_UTIL_ENDIAN_COMPILETIME_LITTLE) || defined(ATFW_UTIL_ENDIAN_COMPILETIME_BIG)
ATFW_UTIL_FORCEINLINE constexpr bool is_little_endian() noexcept { return detail::is_little_endian_detect(); }
#else
ATFW_UTIL_FORCEINLINE bool is_little_endian() noexcept { return detail::is_little_endian_detect(); }
#endif

#if defined(ATFW_UTIL_MACRO_HAVE_STD_ENDIAN) && ATFW_UTIL_MACRO_HAVE_STD_ENDIAN
using std::endian;
#elif defined(ATFW_UTIL_ENDIAN_COMPILETIME_LITTLE) && ATFW_UTIL_ENDIAN_COMPILETIME_LITTLE
enum class endian { little = 0, big = 1, native = 0 };
#elif defined(ATFW_UTIL_ENDIAN_COMPILETIME_BIG) && ATFW_UTIL_ENDIAN_COMPILETIME_BIG
enum class endian { little = 0, big = 1, native = 1 };
#else
// Mixed or unknown endianness — native differs from both little and big
enum class endian { little = 0, big = 1, native = 2 };
#endif

// --- Read big-endian ---

ATFW_UTIL_FORCEINLINE uint16_t read_be_uint16(const unsigned char *buf) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::big) {
    uint16_t value;  // NOLINT(cppcoreguidelines-init-variables)
    std::memcpy(&value, buf, sizeof(value));
    return value;
  } else {
    return static_cast<uint16_t>((static_cast<uint16_t>(buf[0]) << 8) | static_cast<uint16_t>(buf[1]));
  }
#else
  if (!detail::is_little_endian_detect()) {
    uint16_t value;
    std::memcpy(&value, buf, sizeof(value));
    return value;
  }
  return static_cast<uint16_t>((static_cast<uint16_t>(buf[0]) << 8) | static_cast<uint16_t>(buf[1]));
#endif
}

ATFW_UTIL_FORCEINLINE uint32_t read_be_uint32(const unsigned char *buf) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::big) {
    uint32_t value;  // NOLINT(cppcoreguidelines-init-variables)
    std::memcpy(&value, buf, sizeof(value));
    return value;
  } else {
    return (static_cast<uint32_t>(buf[0]) << 24) | (static_cast<uint32_t>(buf[1]) << 16) |
           (static_cast<uint32_t>(buf[2]) << 8) | static_cast<uint32_t>(buf[3]);
  }
#else
  if (!detail::is_little_endian_detect()) {
    uint32_t value;
    std::memcpy(&value, buf, sizeof(value));
    return value;
  }
  return (static_cast<uint32_t>(buf[0]) << 24) | (static_cast<uint32_t>(buf[1]) << 16) |
         (static_cast<uint32_t>(buf[2]) << 8) | static_cast<uint32_t>(buf[3]);
#endif
}

ATFW_UTIL_FORCEINLINE uint64_t read_be_uint64(const unsigned char *buf) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::big) {
    uint64_t value;  // NOLINT(cppcoreguidelines-init-variables)
    std::memcpy(&value, buf, sizeof(value));
    return value;
  } else {
    return (static_cast<uint64_t>(buf[0]) << 56) | (static_cast<uint64_t>(buf[1]) << 48) |
           (static_cast<uint64_t>(buf[2]) << 40) | (static_cast<uint64_t>(buf[3]) << 32) |
           (static_cast<uint64_t>(buf[4]) << 24) | (static_cast<uint64_t>(buf[5]) << 16) |
           (static_cast<uint64_t>(buf[6]) << 8) | static_cast<uint64_t>(buf[7]);
  }
#else
  if (!detail::is_little_endian_detect()) {
    uint64_t value;
    std::memcpy(&value, buf, sizeof(value));
    return value;
  }
  return (static_cast<uint64_t>(buf[0]) << 56) | (static_cast<uint64_t>(buf[1]) << 48) |
         (static_cast<uint64_t>(buf[2]) << 40) | (static_cast<uint64_t>(buf[3]) << 32) |
         (static_cast<uint64_t>(buf[4]) << 24) | (static_cast<uint64_t>(buf[5]) << 16) |
         (static_cast<uint64_t>(buf[6]) << 8) | static_cast<uint64_t>(buf[7]);
#endif
}

// --- Write big-endian ---

ATFW_UTIL_FORCEINLINE void write_be_uint16(unsigned char *buf, uint16_t value) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::big) {
    std::memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = static_cast<unsigned char>((value >> 8) & 0xFF);
    buf[1] = static_cast<unsigned char>(value & 0xFF);
  }
#else
  if (!detail::is_little_endian_detect()) {
    std::memcpy(buf, &value, sizeof(value));
    return;
  }
  buf[0] = static_cast<unsigned char>((value >> 8) & 0xFF);
  buf[1] = static_cast<unsigned char>(value & 0xFF);
#endif
}

ATFW_UTIL_FORCEINLINE void write_be_uint32(unsigned char *buf, uint32_t value) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::big) {
    std::memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = static_cast<unsigned char>((value >> 24) & 0xFF);
    buf[1] = static_cast<unsigned char>((value >> 16) & 0xFF);
    buf[2] = static_cast<unsigned char>((value >> 8) & 0xFF);
    buf[3] = static_cast<unsigned char>(value & 0xFF);
  }
#else
  if (!detail::is_little_endian_detect()) {
    std::memcpy(buf, &value, sizeof(value));
    return;
  }
  buf[0] = static_cast<unsigned char>((value >> 24) & 0xFF);
  buf[1] = static_cast<unsigned char>((value >> 16) & 0xFF);
  buf[2] = static_cast<unsigned char>((value >> 8) & 0xFF);
  buf[3] = static_cast<unsigned char>(value & 0xFF);
#endif
}

ATFW_UTIL_FORCEINLINE void write_be_uint64(unsigned char *buf, uint64_t value) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::big) {
    std::memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = static_cast<unsigned char>((value >> 56) & 0xFF);
    buf[1] = static_cast<unsigned char>((value >> 48) & 0xFF);
    buf[2] = static_cast<unsigned char>((value >> 40) & 0xFF);
    buf[3] = static_cast<unsigned char>((value >> 32) & 0xFF);
    buf[4] = static_cast<unsigned char>((value >> 24) & 0xFF);
    buf[5] = static_cast<unsigned char>((value >> 16) & 0xFF);
    buf[6] = static_cast<unsigned char>((value >> 8) & 0xFF);
    buf[7] = static_cast<unsigned char>(value & 0xFF);
  }
#else
  if (!detail::is_little_endian_detect()) {
    std::memcpy(buf, &value, sizeof(value));
    return;
  }
  buf[0] = static_cast<unsigned char>((value >> 56) & 0xFF);
  buf[1] = static_cast<unsigned char>((value >> 48) & 0xFF);
  buf[2] = static_cast<unsigned char>((value >> 40) & 0xFF);
  buf[3] = static_cast<unsigned char>((value >> 32) & 0xFF);
  buf[4] = static_cast<unsigned char>((value >> 24) & 0xFF);
  buf[5] = static_cast<unsigned char>((value >> 16) & 0xFF);
  buf[6] = static_cast<unsigned char>((value >> 8) & 0xFF);
  buf[7] = static_cast<unsigned char>(value & 0xFF);
#endif
}

// --- Read little-endian ---

ATFW_UTIL_FORCEINLINE uint16_t read_le_uint16(const unsigned char *buf) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::little) {
    uint16_t value;  // NOLINT(cppcoreguidelines-init-variables)
    std::memcpy(&value, buf, sizeof(value));
    return value;
  } else {
    return static_cast<uint16_t>(static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8));
  }
#else
  if (detail::is_little_endian_detect()) {
    uint16_t value;
    std::memcpy(&value, buf, sizeof(value));
    return value;
  }
  return static_cast<uint16_t>(static_cast<uint16_t>(buf[0]) | (static_cast<uint16_t>(buf[1]) << 8));
#endif
}

ATFW_UTIL_FORCEINLINE uint32_t read_le_uint32(const unsigned char *buf) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::little) {
    uint32_t value;  // NOLINT(cppcoreguidelines-init-variables)
    std::memcpy(&value, buf, sizeof(value));
    return value;
  } else {
    return static_cast<uint32_t>(buf[0]) | (static_cast<uint32_t>(buf[1]) << 8) |
           (static_cast<uint32_t>(buf[2]) << 16) | (static_cast<uint32_t>(buf[3]) << 24);
  }
#else
  if (detail::is_little_endian_detect()) {
    uint32_t value;
    std::memcpy(&value, buf, sizeof(value));
    return value;
  }
  return static_cast<uint32_t>(buf[0]) | (static_cast<uint32_t>(buf[1]) << 8) | (static_cast<uint32_t>(buf[2]) << 16) |
         (static_cast<uint32_t>(buf[3]) << 24);
#endif
}

ATFW_UTIL_FORCEINLINE uint64_t read_le_uint64(const unsigned char *buf) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::little) {
    uint64_t value;  // NOLINT(cppcoreguidelines-init-variables)
    std::memcpy(&value, buf, sizeof(value));
    return value;
  } else {
    return static_cast<uint64_t>(buf[0]) | (static_cast<uint64_t>(buf[1]) << 8) |
           (static_cast<uint64_t>(buf[2]) << 16) | (static_cast<uint64_t>(buf[3]) << 24) |
           (static_cast<uint64_t>(buf[4]) << 32) | (static_cast<uint64_t>(buf[5]) << 40) |
           (static_cast<uint64_t>(buf[6]) << 48) | (static_cast<uint64_t>(buf[7]) << 56);
  }
#else
  if (detail::is_little_endian_detect()) {
    uint64_t value;
    std::memcpy(&value, buf, sizeof(value));
    return value;
  }
  return static_cast<uint64_t>(buf[0]) | (static_cast<uint64_t>(buf[1]) << 8) | (static_cast<uint64_t>(buf[2]) << 16) |
         (static_cast<uint64_t>(buf[3]) << 24) | (static_cast<uint64_t>(buf[4]) << 32) |
         (static_cast<uint64_t>(buf[5]) << 40) | (static_cast<uint64_t>(buf[6]) << 48) |
         (static_cast<uint64_t>(buf[7]) << 56);
#endif
}

// --- Write little-endian ---

ATFW_UTIL_FORCEINLINE void write_le_uint16(unsigned char *buf, uint16_t value) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::little) {
    std::memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = static_cast<unsigned char>(value & 0xFF);
    buf[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
  }
#else
  if (detail::is_little_endian_detect()) {
    std::memcpy(buf, &value, sizeof(value));
    return;
  }
  buf[0] = static_cast<unsigned char>(value & 0xFF);
  buf[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
#endif
}

ATFW_UTIL_FORCEINLINE void write_le_uint32(unsigned char *buf, uint32_t value) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::little) {
    std::memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = static_cast<unsigned char>(value & 0xFF);
    buf[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
    buf[2] = static_cast<unsigned char>((value >> 16) & 0xFF);
    buf[3] = static_cast<unsigned char>((value >> 24) & 0xFF);
  }
#else
  if (detail::is_little_endian_detect()) {
    std::memcpy(buf, &value, sizeof(value));
    return;
  }
  buf[0] = static_cast<unsigned char>(value & 0xFF);
  buf[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
  buf[2] = static_cast<unsigned char>((value >> 16) & 0xFF);
  buf[3] = static_cast<unsigned char>((value >> 24) & 0xFF);
#endif
}

ATFW_UTIL_FORCEINLINE void write_le_uint64(unsigned char *buf, uint64_t value) noexcept {
#if ATFW_UTIL_ENDIAN_CAN_USE_IF_CONSTEXPR
  if constexpr (std::endian::native == std::endian::little) {
    std::memcpy(buf, &value, sizeof(value));
  } else {
    buf[0] = static_cast<unsigned char>(value & 0xFF);
    buf[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
    buf[2] = static_cast<unsigned char>((value >> 16) & 0xFF);
    buf[3] = static_cast<unsigned char>((value >> 24) & 0xFF);
    buf[4] = static_cast<unsigned char>((value >> 32) & 0xFF);
    buf[5] = static_cast<unsigned char>((value >> 40) & 0xFF);
    buf[6] = static_cast<unsigned char>((value >> 48) & 0xFF);
    buf[7] = static_cast<unsigned char>((value >> 56) & 0xFF);
  }
#else
  if (detail::is_little_endian_detect()) {
    std::memcpy(buf, &value, sizeof(value));
    return;
  }
  buf[0] = static_cast<unsigned char>(value & 0xFF);
  buf[1] = static_cast<unsigned char>((value >> 8) & 0xFF);
  buf[2] = static_cast<unsigned char>((value >> 16) & 0xFF);
  buf[3] = static_cast<unsigned char>((value >> 24) & 0xFF);
  buf[4] = static_cast<unsigned char>((value >> 32) & 0xFF);
  buf[5] = static_cast<unsigned char>((value >> 40) & 0xFF);
  buf[6] = static_cast<unsigned char>((value >> 48) & 0xFF);
  buf[7] = static_cast<unsigned char>((value >> 56) & 0xFF);
#endif
}

}  // namespace bit
ATFRAMEWORK_UTILS_NAMESPACE_END
