// Copyright 2021 atframework
// Created by owent
// Stanards operations for Write Ahead Log client

#pragma once

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

#include "distributed_system/wal_common_defs.h"

namespace util {
namespace distributed_system {
/*
class LIBATFRAME_UTILS_API_HEAD_ONLY wal_client {
  // TODO(owentou): send subscribe request

 private:
  // TODO(owentou): load snapshot
  // TODO(owentou): load logs
  // TODO(owentou): dump
};
*/
}  // namespace distributed_system
}  // namespace util
