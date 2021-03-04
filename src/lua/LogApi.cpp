
#include "solarus/core/Logger.h"
#include "solarus/lua/LogApi.h"

namespace Solarus::Log {

    int handle_lua_log_call(lua_State* l, void(*logFunction)(const std::string&)) {
        return LuaContext::state_boundary_handle(l, [&] {
            if ( lua_gettop(l) < 1 ) {
                luaL_error(l, "log require a string as parameter");
                return 0;
            }
            std::string message = lua_tostring(l, 1);
            logFunction(message);
            return 0;
        });
    }

    int debug(lua_State* l) {
        return handle_lua_log_call(l, Solarus::Logger::debug);
    }

    int info(lua_State* l) {
        return handle_lua_log_call(l, Solarus::Logger::info);
    }

    int warn(lua_State* l) {
        return handle_lua_log_call(l, Solarus::Logger::warning);
    }

    int error(lua_State* l) {
        return handle_lua_log_call(l, Solarus::Logger::error);
    }

    void register_log_module(lua_State* current_l) {
        const std::vector<luaL_Reg> methods = {
            { "debug", Solarus::Log::debug },
            { "info", Solarus::Log::info },
            { "warn", Solarus::Log::warn },
            { "error", Solarus::Log::error },
            { nullptr, nullptr }
        };

        /*
        const std::vector<luaL_Reg> metamethods = {
            { "__gc", userdata_meta_gc },
            { "__newindex", userdata_meta_newindex_as_table },
            { "__index", userdata_meta_index_as_table },
        };
        */

        // methods.push_back({ nullptr, nullptr });
        luaL_register(current_l, "sol.log", methods.data());
        lua_pop(current_l, 1);

        lua_getglobal(current_l, "sol"); // ... sol
        lua_getfield(current_l, -1, "log"); // ... sol net
        lua_setfield(current_l, LUA_REGISTRYINDEX, "sol.log"); // ... sol
        lua_pop(current_l, 1); // ...
    }
}