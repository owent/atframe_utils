﻿#include "template_suffix.h"

#if defined(__GNUC__) && !defined(__clang__) && !defined(__apple_build_version__)  // && (__GNUC__ * 100 + __GNUC_MINOR__ * 10) >= 460
#pragma GCC diagnostic pop
#elif defined(__clang__) || defined(__apple_build_version__)
#pragma clang diagnostic pop
#endif

#if defined(_MSC_VER) && ((defined(__cplusplus) && __cplusplus >= 201703L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L))
#pragma warning(pop)
#endif


#undef UTIL_CONFIG_COMPILER_PROTOBUF_PREFIX_H
