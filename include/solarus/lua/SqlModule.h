
#ifndef LUA_SQLMODULE_H
#define LUA_SQLMODULE_H

#include "solarus/lua/LuaContext.h"

namespace Solarus::Sql {
    void register_sql_module(lua_State* current_l);
}

#endif