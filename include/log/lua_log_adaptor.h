// Copyright 2021 atframework
// Licensed under the MIT licenses.
// Created by owent on 2015-06-29

#ifndef UTIL_LOG_LUA_LOG_ADAPTER_H
#define UTIL_LOG_LUA_LOG_ADAPTER_H

#pragma once

#include "config/atframe_utils_build_feature.h"

#if defined(ATFRAMEWORK_UTILS_LOG_ENABLE_LUA_SUPPORT) && ATFRAMEWORK_UTILS_LOG_ENABLE_LUA_SUPPORT

#  ifdef __cplusplus
extern "C" {
#  endif

#  include "lauxlib.h"
#  include "lua.h"

ATFRAMEWORK_UTILS_API int lua_log_adaptor_openLib(lua_State *L);

#  ifdef __cplusplus
}
#  endif

#endif

#endif
