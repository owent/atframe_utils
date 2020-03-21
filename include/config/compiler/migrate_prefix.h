#ifndef UTIL_CONFIG_COMPILER_MIGRATE_PREFIX_H
#define UTIL_CONFIG_COMPILER_MIGRATE_PREFIX_H

// https://docs.microsoft.com/en-us/cpp/error-messages/compiler-warnings/compiler-warnings-by-compiler-version

#if defined(_MSC_VER)
#pragma warning(push)
#if _MSC_VER >= 1922 && ((defined(__cplusplus) && __cplusplus >= 201704L) || (defined(_MSVC_LANG) && _MSVC_LANG >= 201704L))
#pragma warning(disable : 5054)
#endif
#endif

#endif