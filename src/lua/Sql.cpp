#include "solarus/lua/LuaContext.h"
#include "solarus/lua/SqlModule.h"
#include "solarus/core/Logger.h"

#include <string>
#include "sqlite3.h"


namespace Solarus::Sql {
  
  int added = 0;
  lua_State* current_lua;

  static void stackDump (lua_State *L) {
  int i;
  int top = lua_gettop(L);
  printf("----- bottom -----\n");
  for (i = 1; i <= top; i++) {  /* repeat for each level */
    int t = lua_type(L, i);
    char format[] = "[%d] %s -> %s\n";
    switch (t) {
      
      case LUA_TSTRING:  /* strings */
        printf(format, i, "STRING", lua_tostring(L, i));
        break;

      case LUA_TBOOLEAN:  /* booleans */
        printf(format, i, "BOOL", lua_toboolean(L, i) ? "true" : "false");
        break;

      case LUA_TNUMBER:  /* numbers */
        printf("[%d] %s -> %g\n", i, "NUMBER", lua_tonumber(L, i));
        break;

      default:  /* other values */
        printf("[%d] %s\n", i, lua_typename(L, t));
        break;

    }
    // printf("  ");  /* put a separator */
  }
  printf("----- top -----\n");
}


  static int callback(void *NotUsed, int argc, char **argv, char **azColName) {
    if (added == 0) {
      lua_newtable(current_lua); // global list
    }
    lua_newtable(current_lua); // current row
    for(int i = 0; i < argc; i++) {
      lua_pushstring(current_lua, azColName[i]);
      lua_pushstring(current_lua, argv[i] ? argv[i] : "NULL");
      lua_settable(current_lua, -3); // push col
      //DEBUG printf("%s = %s\n", azColName[i], argv[i] ? argv[i] : "NULL");
    }
    added++;
    lua_rawseti(current_lua, -2, added);
    // lua_settable(current_lua, -3); // push row
    //DEBUG printf("\n");
    return 0;
  }

  int query(lua_State* l) {
    const char* sql = lua_tostring(l, 1);
    sqlite3 *db;
    char *zErrMsg = 0;
    int rc;
    rc = sqlite3_open("data.db", &db);

    if( rc ) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      // return "error";
      lua_pushstring(l, "Error");
      return 1;
    }
    
    //DEBUG printf("Opened database successfully\n");
    
    /* Execute SQL statement */
    current_lua = l;
    added = 0;
    rc = sqlite3_exec(db, sql, callback, 0, &zErrMsg);
    
    if( rc != SQLITE_OK ){
      fprintf(stderr, "SQL error: %s\n", zErrMsg);
      lua_pushstring(l, (std::string("Error: ") + zErrMsg).c_str() );
      sqlite3_free(zErrMsg);
    } else {
      if ( added > 0 ) {
        // lua_settable(current_lua, -3);
        printf("added %d rows.\n", added);
        stackDump(current_lua);
      } else {
        lua_pushstring(current_lua, "Ok");
      }
      //DEBUG printf("Query successfully executed !\n");
    }
    sqlite3_close(db);
    return 1;
  }

  int query_l(lua_State* l) {
    return LuaContext::state_boundary_handle(l, [&] {
      return query(l);
    });
  }

  void register_sql_module(lua_State* current_l) {

    const std::vector<luaL_Reg> methods = {
        { "query", Solarus::Sql::query_l },
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
    luaL_register(current_l, "sol.sql", methods.data());
    lua_pop(current_l, 1);

    lua_getglobal(current_l, "sol"); // ... sol
    lua_getfield(current_l, -1, "sql"); // ... sol net
    lua_setfield(current_l, LUA_REGISTRYINDEX, "sol.sql"); // ... sol
    lua_pop(current_l, 1); // ...

  }

}