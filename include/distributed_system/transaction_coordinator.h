// Copyright 2021 atframework
// Created by owent
// Stanards operations for Transaction coordinator

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

// TODO(owentou): 等稳定后移入atframe_utils
LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace distributed_system {}  // namespace distributed_system
LIBATFRAME_UTILS_NAMESPACE_END
