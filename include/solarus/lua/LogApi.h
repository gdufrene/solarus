
#ifndef LUA_LOGAPI_H
#define LUA_LOGAPI_H

#include "solarus/lua/LuaContext.h"

namespace Solarus::Log {
    void register_log_module(lua_State* current_l);
}

#endif