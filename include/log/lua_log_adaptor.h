/**
 * @file lua_log_adaptor.h
 * @brief 日志lua封装
 * Licensed under the MIT licenses.
 *
 * @version 1.0
 * @author owent
 * @date 2015-06-29
 * @history
 */

#ifndef UTIL_LOG_LUA_LOG_ADAPTER_H
#define UTIL_LOG_LUA_LOG_ADAPTER_H

#pragma once

#include "config/atframe_utils_build_feature.h"

#if defined(LOG_WRAPPER_ENABLE_LUA_SUPPORT) && LOG_WRAPPER_ENABLE_LUA_SUPPORT

#  ifdef __cplusplus
extern "C" {
#  endif

#  include "lauxlib.h"
#  include "lua.h"

LIBATFRAME_UTILS_API int lua_log_adaptor_openLib(lua_State *L);

#  ifdef __cplusplus
}
#  endif

#endif

#endif
