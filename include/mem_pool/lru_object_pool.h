// Copyright 2024 atframework
// Licenses under the MIT License

#pragma once

#include "memory/lru_object_pool.h"

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace mempool {
using memory::lru_pool_base;
using memory::lru_pool_manager;

template <class TObj>
using lru_default_action = memory::lru_default_action<TObj>;

template <class TKey, class TObj, class TAction = lru_default_action<TObj> >
using lru_pool = memory::lru_pool<TKey, TObj, TAction>;

}  // namespace mempool
ATFRAMEWORK_UTILS_NAMESPACE_END
