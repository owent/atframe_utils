// Copyright 2024 atframework
// Licenses under the MIT License

#pragma once

#include "memory/lru_map.h"

#include <functional>
#include <memory>
#include <utility>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace mempool {

template <class TKEY, class TVALUE, class TOption = memory::lru_map_option<memory::compat_strong_ptr_mode::kStl>>
using lru_map_type_traits = memory::lru_map_type_traits<TKEY, TVALUE, TOption>;

template <class TKEY, class TVALUE, class THasher = std::hash<TKEY>, class TKeyEQ = std::equal_to<TKEY>,
          class TOption = memory::lru_map_option<memory::compat_strong_ptr_mode::kStl>,
          class TAlloc =
              std::allocator<std::pair<const TKEY, typename memory::lru_map_type_traits<TKEY, TVALUE>::iterator>>>
using lru_map = memory::lru_map<TKEY, TVALUE, THasher, TKeyEQ, TOption, TAlloc>;

}  // namespace mempool
LIBATFRAME_UTILS_NAMESPACE_END
