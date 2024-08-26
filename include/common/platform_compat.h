// Copyright 2024 atframework
// Created by owent on 2024-08-26.
//

#pragma once

#include <config/atframe_utils_build_feature.h>

#include <gsl/select-gsl.h>

#include <cstdint>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace platform {

LIBATFRAME_UTILS_API int32_t get_errno() noexcept;

LIBATFRAME_UTILS_API gsl::string_view get_strerrno(int32_t result_from_get_errno, gsl::span<char> buffer) noexcept;

}  // namespace platform
LIBATFRAME_UTILS_NAMESPACE_END
