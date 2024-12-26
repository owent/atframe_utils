// Copyright 2024 atframework
// Created by owent on 2024-08-26.
//

#include "common/platform_compat.h"

#include <errno.h>
#include <cstdlib>
#include <cstring>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace platform {

ATFRAMEWORK_UTILS_API int32_t get_errno() noexcept { return errno; }

ATFRAMEWORK_UTILS_API gsl::string_view get_strerrno(int32_t result_from_get_errno, gsl::span<char> buffer) noexcept {
  if (buffer.size() <= 1) {
    return {};
  }
  buffer[buffer.size() - 1] = 0;
#ifdef _MSC_VER
  strerror_s(buffer.data(), buffer.size() - 1, result_from_get_errno);
  return buffer.data();
#elif defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
  strerror_r(result_from_get_errno, buffer.data(), buffer.size() - 1);
  return buffer.data();
#else
  return strerror(result_from_get_errno);
#endif
}

}  // namespace platform
ATFRAMEWORK_UTILS_NAMESPACE_END
