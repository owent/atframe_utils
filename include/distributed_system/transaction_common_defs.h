// Copyright 2021 atframework
// Created by owent
// Common definitions for Transaction

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <design_pattern/result_type.h>

#include <stdint.h>
#include <atomic>
#include <chrono>
#include <deque>
#include <functional>
#include <memory>
#include <unordered_map>
#include <utility>

// TODO(owentou): 等稳定后移入atframe_utils
namespace util {
namespace distributed_system {

enum class transaction_status : int32_t {
  kCreated = 0,  // (Initial) Created and not prepare yet
  kPraparing,    // Running the prepare process
  kApproved,     // Accepted but not commited yet
  kRejected,     // (Final) Rejected
  kCommiting,    // Accepted and run commiting
  kCommited,     // (Final) Commited
  kAborting,     // Accepted but run aborting
  kAborted,      // (Final) Aborted
};

}  // namespace distributed_system
}  // namespace util
