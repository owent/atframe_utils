// Copyright 2021 atframework
// Created by owent
// Stanards operations for Transaction node

#pragma once

#include <design_pattern/result_type.h>
#include <mem_pool/lru_map.h>

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include "distributed_system/transaction_common_defs.h"

namespace util {
namespace distributed_system {
// Event: prepare(check conditions)
// Event: merge(check running[Wound Wait: lock])
// Event: commit
// Event: cancle
// Event: redo
}  // namespace distributed_system
}  // namespace util
