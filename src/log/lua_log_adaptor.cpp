#include "log/log_wrapper.h"

#include "log/lua_log_adaptor.h"

#if defined(LOG_WRAPPER_ENABLE_LUA_SUPPORT) && LOG_WRAPPER_ENABLE_LUA_SUPPORT

static int lua_log_adaptor_fn_lua_log(lua_State *L) {
  int top = lua_gettop(L);
  if (top < 2) {
    WLOGERROR("call lua function: lua_log without log level.");
    return 0;
  }

  // log 分类
  uint32_t cat = static_cast<uint32_t>(luaL_checkinteger(L, 1));

  util::log::log_wrapper::level_t::type level = WLOG_LEVELID(luaL_checkinteger(L, 2));

  util::log::log_wrapper *logger = WDTLOGGETCAT(cat);
  if (NULL != logger && logger->check_level(level)) {
    // TODO: 是否填充lua文件名和行号？但是那个操作比较耗性能
    util::log::log_wrapper::caller_info_t caller(level, "Lua", NULL, 0, NULL);

    for (int i = 3; i <= top; ++i) {
      const char *content = lua_tostring(L, i);
      if (NULL != content) {
        logger->log(caller, "%s", content);
      }
    }
  }

  return 0;
}

#  ifdef __cplusplus
extern "C" {
#  endif

LIBATFRAME_UTILS_API int lua_log_adaptor_openLib(lua_State *L) {
  lua_newtable(L);

  lua_pushinteger(L, static_cast<lua_Integer>(util::log::log_wrapper::level_t::LOG_LW_DISABLED));
  lua_setfield(L, -2, "DISABLED");

  lua_pushinteger(L, static_cast<lua_Integer>(util::log::log_wrapper::level_t::LOG_LW_FATAL));
  lua_setfield(L, -2, "FATAL");

  lua_pushinteger(L, static_cast<lua_Integer>(util::log::log_wrapper::level_t::LOG_LW_ERROR));
  lua_setfield(L, -2, "ERROR");

  lua_pushinteger(L, static_cast<lua_Integer>(util::log::log_wrapper::level_t::LOG_LW_WARNING));
  lua_setfield(L, -2, "WARNING");

  lua_pushinteger(L, static_cast<lua_Integer>(util::log::log_wrapper::level_t::LOG_LW_INFO));
  lua_setfield(L, -2, "INFO");

  lua_pushinteger(L, static_cast<lua_Integer>(util::log::log_wrapper::level_t::LOG_LW_NOTICE));
  lua_setfield(L, -2, "NOTICE");

  lua_pushinteger(L, static_cast<lua_Integer>(util::log::log_wrapper::level_t::LOG_LW_DEBUG));
  lua_setfield(L, -2, "DEBUG");

  lua_setglobal(L, "lua_log_level_t");

  lua_pushcfunction(L, lua_log_adaptor_fn_lua_log);
  lua_setglobal(L, "lua_log");

  return 0;
}

#  ifdef __cplusplus
}
#  endif

#endif
