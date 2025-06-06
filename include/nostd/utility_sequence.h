// Copyright 2021 atframework
// Created by owent on 2024-05-28

#pragma once

#include <config/atframe_utils_build_feature.h>
#include <config/compile_optimize.h>

#include <cstddef>
#include <utility>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace nostd {

#if __cplusplus >= 201402L

using ::std::index_sequence;
using ::std::index_sequence_for;
using ::std::integer_sequence;
using ::std::make_index_sequence;
using ::std::make_integer_sequence;

#else

// Stores a tuple of indices.  Used by tuple and pair, and by bind() to
// extract the elements in a tuple.
template <size_t... __Indexes>
struct __index_tuple {
  using __next = __index_tuple<__Indexes..., sizeof...(__Indexes)>;
};

// Builds an __index_tuple<0, 1, 2, ..., __Num-1>.
template <size_t __Num>
struct __build_index_tuple {
  using __type = typename __build_index_tuple<__Num - 1>::__type::__next;
};

template <>
struct __build_index_tuple<0> {
  using __type = __index_tuple<>;
};

/// Class template integer_sequence
template <class TSize, TSize... Idx>
struct integer_sequence {
  using value_type = TSize;
  static constexpr size_t size() noexcept { return sizeof...(Idx); }
};

template <class TSize, TSize Num, class ISeq = typename __build_index_tuple<Num>::__type>
struct __make_integer_sequence;

template <class TSize, TSize Num, size_t... Idx>
struct __make_integer_sequence<TSize, Num, __index_tuple<Idx...> > {
  static_assert(Num >= 0, "Cannot make integer sequence of negative length");

  using __type = integer_sequence<TSize, static_cast<TSize>(Idx)...>;
};

template <class TSize, TSize Num>
using make_integer_sequence = typename __make_integer_sequence<TSize, Num>::__type;

template <size_t... Idx>
using index_sequence = integer_sequence<size_t, Idx...>;

/// Alias template make_index_sequence
template <size_t Num>
using make_index_sequence = make_integer_sequence<size_t, Num>;

/// Alias template index_sequence_for
template <class... Types>
using index_sequence_for = make_index_sequence<sizeof...(Types)>;

#endif

}  // namespace nostd
ATFRAMEWORK_UTILS_NAMESPACE_END
