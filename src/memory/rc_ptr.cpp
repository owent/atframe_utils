// Copyright 2024 atframework
// Licenses under the MIT License

#include "memory/rc_ptr.h"

#include <cstdlib>

LIBATFRAME_UTILS_NAMESPACE_BEGIN
namespace memory {

LIBATFRAME_UTILS_API __rc_ptr_counted_data_base::~__rc_ptr_counted_data_base() noexcept {}

LIBATFRAME_UTILS_API void __rc_ptr_counted_data_base::throw_bad_weak_ptr() {
#if defined(LIBATFRAME_UTILS_ENABLE_EXCEPTION) && LIBATFRAME_UTILS_ENABLE_EXCEPTION
  throw std::bad_weak_ptr();
#else
  abort();
#endif
}

}  // namespace memory
LIBATFRAME_UTILS_NAMESPACE_END
