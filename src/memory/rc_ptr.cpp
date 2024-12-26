// Copyright 2024 atframework
// Licenses under the MIT License

#include "memory/rc_ptr.h"

#include <cstdlib>

ATFRAMEWORK_UTILS_NAMESPACE_BEGIN
namespace memory {

ATFRAMEWORK_UTILS_API __rc_ptr_counted_data_base::~__rc_ptr_counted_data_base() noexcept {}

ATFRAMEWORK_UTILS_API void __rc_ptr_counted_data_base::throw_bad_weak_ptr() {
#if defined(ATFRAMEWORK_UTILS_ENABLE_EXCEPTION) && ATFRAMEWORK_UTILS_ENABLE_EXCEPTION
  throw std::bad_weak_ptr();
#else
  abort();
#endif
}

}  // namespace memory
ATFRAMEWORK_UTILS_NAMESPACE_END
