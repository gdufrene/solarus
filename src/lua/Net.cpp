#include "solarus/net/Http.h"
#include "solarus/net/Json.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/core/Logger.h"

#include <string>

#include "sqlite3.h"


#define index(str, c) (strchr(str, c) - str)


namespace Solarus {

namespace Net {

/**
 * Name of the Lua table representing the map module.
 */
const std::string net_module_name = "sol.net";

// defines ...
int net_api_http_post(lua_State* l);
int push_headers(lua_State* l, Solarus::Http::Response *resp);
int push_common_response(lua_State* l, Solarus::Http::Response *resp);
// ---- ---- ----


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





int inc = 0;

void print_table(lua_State *L)
{
 //   if ((lua_type(L, -2) == LUA_TSTRING))
 //       printf("%s", lua_tostring(L, -2));

    lua_pushnil(L);
    printf("{\n");
    while(lua_next(L, -2) != 0) {
        if(lua_isstring(L, -1))
            printf("%s => %s\n", lua_tostring(L, -2), lua_tostring(L, -1));
        else if(lua_isnumber(L, -1))
            printf("%s => %f\n", lua_tostring(L, -2), lua_tonumber(L, -1));
        else if(lua_istable(L, -1)) {
            printf("%s =>\n", lua_tostring(L, -2));
            print_table(L);
        }
        lua_pop(L, 1);
    }
    printf("}\n");
}

std::string table_to_json(lua_State* l) {
  std::string json = "";
  bool is_array = false;
  bool first = true;
  lua_pushnil(l);
  while (lua_next(l, 2))
  {
      // printf("NEXT\n");
      // stack now contains: -1 => value; -2 => key; -3 => table
      bool current_numeric = lua_isnumber(l, -2);
      if ( first ) {
        is_array = current_numeric;
        json.append( is_array ? "[" : "{" );
        first = false;
      } else {
        json += ",";
      }

      // -- copy key
      if ( !is_array ) {
        // copy the key so that lua_tostring does not modify the original
        // stack now contains: -1 => key; -2 => value; -3 => key; -4 => table
        lua_pushvalue(l, -2);
        json += "\"";
        json += lua_tostring(l, -1);
        json += "\": ";
        lua_pop(l, 1);
      }

      // -- copy value
      if ( lua_istable(l, -1) ) {
        json += table_to_json(l);
      } else {
        if ( lua_isnumber(l, -1) ) {
          // sprintf(buffer, "%g", lua_tonumber(l, -1));
          json += lua_tostring(l, -1);
        } 
        else if ( lua_isboolean(l, -1) ) {
          json += lua_toboolean(l, -1) ? "true" : "false";
        }
        else {
          json += "\"";
          json += lua_tostring(l, -1);
          json += "\"";
        }
      }
      lua_pop(l, 1);
  }
  json.append( is_array ? "]" : "}" );
  return json;
}

int push_json( lua_State* l, const char *json ) {
    if ( json == NULL ) {
      lua_pushnil(l);
      return 1;
    }

    Solarus::Json::Parser *parser = Solarus::Json::createParser();
    //DEBUG printf("Created parser. %#010x\n", parser);

    auto new_table = [&] (int) { lua_newtable(l); };

    parser
      ->onString( [&] (string val) { lua_pushstring(l, val.c_str()); } )
      ->onNumber( [&] (double val) { lua_pushnumber(l, val); })
      ->onBoolean( [&] (bool val) { lua_pushboolean(l, val); })
      ->onObject( new_table,
        [&] () { lua_settable(l, -3); }
      )
      ->onArray( new_table,
        [&] (int idx) { lua_rawseti(l, -2, idx); }
      )
      ->onError( [&] (string error) { luaL_error(l, error.c_str()); } )
      ;
    parser->parse( json );

    delete parser; // fail

    return 1;
}



int push_headers(lua_State* l, Solarus::Http::Response *resp) {
  if ( resp->headers[0] == NULL ) {
    lua_pushnil(l);
    return 1;
  }

  lua_newtable(l); 

  int nb_cookie = 0;
  int nb_header = 0;
  char* header;

  lua_pushstring(l, "cookies"); 
  lua_newtable(l); 

  lua_pushstring(l, "headers"); 
  lua_newtable(l); 


  for ( int k = 0; k < 32; k++) {
    header = resp->headers[k];
    if ( header == NULL ) continue;

    char *name = header;
    int i = index(name, ':');
    //DEBUG printf("Header %s. %d.\n", name, i);
    if ( i > 0 ) name[i] = 0;
    
    char *value = name + i + 2;
    

    if (!strcmp("Set-Cookie", name)) {
      nb_cookie++;
      name = value;
      i = index(name, '=');
      name[i] = 0;
      value = name + i + 1;
      i = index(value, ';');
      if ( i > 0 ) value[i] = 0;
      lua_pushstring(l, name);
      lua_pushstring(l, value);
      //DEBUG printf("push cookie %s -> %s\n", name, value);
      lua_settable(l, -5);
    } else {
      nb_header++;
      lua_pushstring(l, name);
      lua_pushstring(l, value);
      //DEBUG printf("push header %s -> %s\n", name, value);
      lua_settable(l, -3);
    }
  }

  
    lua_settable(l, -5); // set headers
    lua_settable(l, -3); // set cookies

  // stackDump(l);
  // print_table(l);
  
  return 1;
}

std::string table_to_headers(lua_State* l) {
  std::string headers = "";
  // stackDump(l);
  lua_pushstring(l, "cookies");
  lua_gettable(l, -2);
  int nb_cookie = 0;
  if ( !lua_isnil(l, -1) ) {
    nb_cookie++;
    lua_pushnil(l);
    while (lua_next(l, -2))
    {
      if ( nb_cookie == 1 ) headers += "Cookie: ";
      if ( nb_cookie > 1 ) headers += "; ";
      // stackDump(l);
      //DEBUG printf( "Cookie Next [%s] -> %s \n", lua_tostring(l, -2), lua_tostring(l, -1));
      headers += lua_tostring(l, -2);
      headers += "=";
      headers += lua_tostring(l, -1);
      lua_pop(l, 1);
    }
    headers += "\r\n";
  } 
  lua_pop(l, 1);
  lua_pushstring(l, "headers");
  lua_gettable(l, -2);
  if ( !lua_isnil(l, -1) ) {
    lua_pushnil(l);
    while (lua_next(l, -2))
    {
      // stackDump(l);
      //DEBUG printf( "Header Next [%s] -> %s \n", lua_tostring(l, -2), lua_tostring(l, -1));
      headers += lua_tostring(l, -2);
      headers += ": ";
      headers += lua_tostring(l, -1);
      headers += "\r\n";
      lua_pop(l, 1);
    }
    lua_pop(l, 1);
  } 

  //DEBUG printf("table to headers ->\n%s\n", headers.c_str());
  // stackDump(l);
  return headers;
}

int push_common_response(lua_State* l, Solarus::Http::Response *resp) {
    //DEBUG printf("Code: %d\n", resp->code);
    lua_pushnumber(l, resp->code);
    //DEBUG printf("Body (size): %lu\n", resp->body().size() );
    lua_pushstring(l, resp->body().c_str() );
    //DEBUG printf("Push headers ...\n" );
    push_headers(l, resp);

    resp->~Response(); // after pushing everything response is no more needed.

    //DEBUG printf("destroyed\n" );

    return 3;
}

int push_common_json_response(lua_State* l, Solarus::Http::Response *resp) {
    //DEBUG printf("Code: %d\n", resp->code);
    lua_pushnumber(l, resp->code);
    //DEBUG printf("Body (size): %d\n", resp->body().size() );
    // lua_pushstring(l, resp->body().c_str() );
    push_json(l, (resp->body()).c_str());
    //DEBUG printf("Push headers ...\n" );
    push_headers(l, resp);

    resp->~Response(); // after pushing everything response is no more needed.

    return 3;
}


int do_cpp_net_call( lua_State* l, std::function<int()> function ) {
  return LuaContext::state_boundary_handle(l, [&] {
    try {
      return function();
    } catch (NetException e) {
      Solarus::Logger::error( e.what() );
      // luaL_error(l, e.what());
      lua_pushnumber(l, -1);
      lua_pushstring(l, e.what());
      lua_pushnil(l);
      return 3;
    }
  });
}


int net_api_http_get(lua_State* l) {
  return do_cpp_net_call(l, [&] {
    Solarus::Http::Response *resp = Solarus::Http::query(
      "GET", 
      lua_tostring(l, 1),
      lua_gettop(l) < 2 ? "" : table_to_headers(l),
      ""
    );
    return push_common_response(l, resp);
  });
}

int net_api_http_post(lua_State* l) {
  return do_cpp_net_call(l, [&] {
    Solarus::Http::Response *resp = Solarus::Http::query(
      "POST",
      lua_tostring(l, 1),
      lua_gettop(l) < 3 ? "" : table_to_headers(l),
      lua_gettop(l) < 2 ? "" : lua_tostring(l, 2)
    );
    return push_common_response(l, resp);
  });
}

int net_api_json_post(lua_State* l) {
  return do_cpp_net_call(l, [&] {
    Solarus::Http::Response *resp = Solarus::Http::query(
      "POST",
      lua_tostring(l, 1),
      lua_gettop(l) < 3 ? "" : table_to_headers(l),
      lua_istable(l, 2) ? table_to_json(l) : "{}"
    );
    return push_common_response(l, resp);
  });
}

int net_api_json_get(lua_State* l) {
  return do_cpp_net_call(l, [&] {
    Solarus::Http::Response *resp = Solarus::Http::query(
      "GET",
      lua_tostring(l, 1),
      lua_gettop(l) < 3 ? "" : table_to_headers(l),
      ""
    );
    return push_common_json_response(l, resp);
  });
}

int net_api_test(lua_State* l) {
  return do_cpp_net_call(l, [&] {
    //DEBUG printf("net_api_test\n");
    Solarus::Http::Response *resp = Solarus::Http::get("www.google.fr");
    //DEBUG printf("Response received. %p\n", resp);
    return push_common_json_response(l, resp);
  });
}

}; // ns net

}; // ns solarus

/**
 * \brief Initializes the map features provided to Lua.
 */
void Solarus::LuaContext::register_net_module() {

  const std::vector<luaL_Reg> methods = {
      { "test", Solarus::Net::net_api_test },
      { "http_get", Solarus::Net::net_api_http_get },
      { "http_post", Solarus::Net::net_api_http_post },
      { "json_get", Solarus::Net::net_api_json_get },
      { "json_post", Solarus::Net::net_api_json_post }
  };

/*
  const std::vector<luaL_Reg> metamethods = {
      { "__gc", userdata_meta_gc },
      { "__newindex", userdata_meta_newindex_as_table },
      { "__index", userdata_meta_index_as_table },
  };
*/

  register_functions(Solarus::Net::net_module_name, methods);
  lua_getglobal(current_l, "sol"); // ... sol
  lua_getfield(current_l, -1, "net"); // ... sol net
  lua_setfield(current_l, LUA_REGISTRYINDEX, Solarus::Net::net_module_name.c_str()); // ... sol
  lua_pop(current_l, 1); // ...

}



