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

#ifndef _UTIL_LOG_LUA_LOG_ADAPTER_H_
#define _UTIL_LOG_LUA_LOG_ADAPTER_H_

#pragma once

#ifndef LOG_WRAPPER_DISABLE_LUA_SUPPORT

#ifdef __cplusplus
extern "C" {
#endif

#include "lua.h"
#include "lauxlib.h"


int lua_log_adaptor_openLib(lua_State *L);

#ifdef __cplusplus
}
#endif

#endif

#endif
