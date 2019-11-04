#include "solarus/net/net.h"
#include "solarus/lua/LuaContext.h"
#include "solarus/third_party/json/jsmin.h"

#include <string>

namespace Solarus {

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

/**
 * Name of the Lua table representing the map module.
 */
const std::string net_module_name = "sol.net";

// defines ...
void push_json_object(lua_State* l, char *json, jsmntok_t *tokens);
void push_json_array(lua_State* l, char *json, jsmntok_t *tokens);
void push_json_value(lua_State* l, char *json, jsmntok_t *tokens);
int net_api_http_post(lua_State* l);
int push_headers(lua_State* l, struct net_resp* resp);
int push_common_response(lua_State* l, char* headers, struct net_resp* resp, int ret);


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

void push_json_array(lua_State* l, char *json, jsmntok_t *tokens) {
  int size = tokens->size;
  int idx = 1;
  lua_newtable(l);
  while (size-- > 0) {
    // value ...
    tokens++;
    push_json_value(l, json, tokens);
    lua_rawseti(l, -2, idx++);
  }
}

void push_json_value(lua_State* l, char *json, jsmntok_t *tokens) {
  char *ptr = json + tokens->start;
  if ( tokens->type == JSMN_STRING ) {
    json[ tokens->end ] = '\0';
    // printf("string -> %s\n", ptr);
    lua_pushstring(l, ptr);
  }
  else if ( tokens->type == JSMN_PRIMITIVE ) {
    char c = json[ tokens->start ];
    if ( c >= '0' && c <= '9' ) {
      double value;
      json[ tokens->end ] = '\0';
      // strncpy( buffer, ptr, tokens[idx].end - tokens[idx].start );
      value = strtod(ptr, NULL);
      // printf("number -> %.2f\n", value);
      lua_pushnumber(l, value);
    } else {
      int boolvalue = (c == 't' || c == 'T');
      lua_pushboolean(l, boolvalue);
      // printf("bool -> %s\n", (boolvalue ? "true" : "false"));
    }
  }
  else if ( tokens->type == JSMN_OBJECT ) {
    push_json_object(l, json, tokens);
  }
  else if ( tokens->type == JSMN_ARRAY ) {
    push_json_array(l, json, tokens);
  }
  else {
    lua_pushstring(l, "#Json Unknown type#");
  }
}

void push_json_object(lua_State* l, char *json, jsmntok_t *tokens) {
  int size = tokens->size;
  char *ptr;
  lua_newtable(l);
  while (size-- > 0) {
    // Key (must be string)
    tokens++;
    ptr = json + tokens->start;
    json[ tokens->end ] = '\0';
    // printf("key -> %s\n", ptr);
    lua_pushstring(l, ptr);

    // value ...
    tokens++;
    push_json_value(l, json, tokens);

    lua_settable(l, -3);
  }
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

int push_json( lua_State* l, char *json ) {
    if ( json == NULL ) {
      lua_pushnil(l);
      return 1;
    }

    jsmn_parser parser;
    jsmn_init(&parser);
    // char json[] = "{\"hello\": 123, \"ahahah\": true, \"table\": [8, 2, \"aeiou\", false]}";
    jsmntok_t tokens[100];
    int r = jsmn_parse( &parser, json, strlen(json), tokens, 100 );

    if ( r == JSMN_ERROR_NOMEM ) return luaL_error(l, "Json contains too much nodes");
    if ( r == JSMN_ERROR_INVAL ) return luaL_error(l, "Json contains invalid characters");
    if ( r == JSMN_ERROR_PART ) return luaL_error(l, "incomplete or malformated Json");

    if (r < 1 || tokens[0].type != JSMN_OBJECT) {
      return luaL_error(l, "Body must be a json object");
    }
    
    push_json_object(l, json, tokens);
    return 1;
}



int push_headers(lua_State* l, struct net_resp* resp) {
  if ( ! resp->headers ) return 0;

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
    int i = index(name, ':') - name;
    if ( i > 0 ) name[i] = '\0';
    
    char *value = name + i + 2;
    

    if (!strcmp("Set-Cookie", name)) {
      nb_cookie++;
      name = value;
      i = index(name, '=') - name;
      name[i] = '\0';
      value = name + i + 1;
      i = index(value, ';') - value;
      if ( i > 0 ) value[i] = '\0';
      lua_pushstring(l, name);
      lua_pushstring(l, value);
      // printf("push cookie %s -> %s\n", name, value);
      lua_settable(l, -5);
    } else {
      nb_header++;
      lua_pushstring(l, name);
      lua_pushstring(l, value);
      // printf("push header %s -> %s\n", name, value);
      lua_settable(l, -3);
    }
  }

  if ( nb_header ) {
    lua_settable(l, -5);
  } else {
    lua_pop(l, 2);
  }

  if ( nb_cookie ) {
    lua_settable(l, -3);
  } else {
    lua_pop(l, 2);
  }

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
    headers += "Cookie: ";
    nb_cookie++;
    lua_pushnil(l);
    while (lua_next(l, -2))
    {
      if ( nb_cookie > 1 ) headers += "; ";
      // stackDump(l);
      // printf( "Cookie Next [%s] -> %s \n", lua_tostring(l, -2), lua_tostring(l, -1));
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
      // printf( "Header Next [%s] -> %s \n", lua_tostring(l, -2), lua_tostring(l, -1));
      headers += lua_tostring(l, -2);
      headers += ": ";
      headers += lua_tostring(l, -1);
      headers += "\r\n";
      lua_pop(l, 1);
    }
    lua_pop(l, 1);
  } 

  // stackDump(l);
  return headers;
} 

int net_api_json_post(lua_State* l) {
  return LuaContext::state_boundary_handle(l, [&] {
    struct net_resp resp = { .code = -1, .content_length = -1, .body = NULL };
    const char* headers = lua_istable(l, 3) ? table_to_headers(l).c_str() : NULL;
    int ret = net_http_post( 
      lua_tostring(l, 1), 
      lua_istable(l, 2) ? table_to_json(l).c_str() : "{}",
      headers,
      &resp 
    );
    if ( ret < 0 ) return 0;
    lua_pushnumber( l, resp.code );
    // if ( headers ) free( headers );
    return 1 + push_json(l, resp.body) + push_headers(l, &resp);
  });
}

int net_api_json_get(lua_State* l) {
  return LuaContext::state_boundary_handle(l, [&] {
    struct net_resp resp = { .code = -1, .content_length = -1, .body = NULL };
    const char* headers = lua_istable(l, 2) ? table_to_headers(l).c_str() : NULL;
    int ret = net_http_get( lua_tostring(l, 1), headers, &resp );
    if ( ret < 0 ) return 0;
    lua_pushnumber( l, resp.code );
    return 1 + push_json(l, resp.body) + push_headers(l, &resp);
  });
}




int LuaContext::net_api_test(lua_State* l) {
  return state_boundary_handle(l, [&] {
    struct net_resp resp = { .code = -1, .content_length = -1, .body = NULL };
    int ret = net_http_get( lua_tostring(l, 1), NULL, &resp );
    return push_common_response(l, NULL, &resp, ret);
  });
}

char* init_qry(lua_State* l, struct net_resp* resp, int headerPos) {
  resp->code = -1;
  resp->content_length = -1;
  resp->body = NULL;

  int args = lua_gettop(l);
  if ( args < 1 ) { luaL_error(l, "query URL required"); return NULL; }
  if ( args >= headerPos && !lua_istable(l, headerPos) ) {
    printf("'headers' should be a table\n");
    luaL_error(l, "'headers' should be a table");
    return NULL;
  }

  char* headers = NULL;

  if ( args >= headerPos && lua_istable(l, headerPos) ) {
    // printf("create headers\n");
    const char* h = table_to_headers(l).c_str();
    // printf("%s\n", h);
    headers = (char*) malloc( strlen(h) );
    strcpy(headers, h);
  }

  return headers;
}

int push_common_response(lua_State* l, char* headers, struct net_resp* resp, int ret) {
    if ( ret < 0 ) {
      lua_pushnumber(l, ret);
      ret = 1;
      goto end;
    }
    lua_pushnumber(l, resp->code);
    lua_pushstring(l, resp->body ? resp->body : "" );
    ret = 2 + push_headers(l, resp);
  end:
    if ( headers != NULL ) free(headers);
    if ( resp->headers != NULL ) free(resp->headers);
    return ret;
}

int LuaContext::net_api_http_get(lua_State* l) {
  return state_boundary_handle(l, [&] {
    struct net_resp resp;
    char* headers = init_qry(l, &resp, 2);
    int ret = net_http_get( lua_tostring(l, 1), headers, &resp );
    return push_common_response(l, headers, &resp, ret);
  });
}

int net_api_http_post(lua_State* l) {
  return LuaContext::state_boundary_handle(l, [&] {
    struct net_resp resp;
    // printf("URL:%s BODY:%s\n", lua_tostring(l, 1), lua_tostring(l, 2));
    char* headers = init_qry(l, &resp, 3);
    const char* body = lua_gettop(l) < 2 ? "\0" : lua_tostring(l, 2);
    int ret = net_http_post( lua_tostring(l, 1), body, headers, &resp );
    return push_common_response(l, headers, &resp, ret);
  });
}

int run_test(lua_State* l) {
   int r = system("java -version");
   lua_pushnumber(l, r);
   return 1;
}



/**
 * \brief Initializes the map features provided to Lua.
 */
void LuaContext::register_net_module() {

  const std::vector<luaL_Reg> methods = {
      { "test", net_api_test },
      { "http_get", net_api_http_get },
      { "http_post", net_api_http_post },
      { "json_get", Solarus::net_api_json_get },
      { "json_post", Solarus::net_api_json_post },
      { "run", run_test},
  };

/*
  const std::vector<luaL_Reg> metamethods = {
      { "__gc", userdata_meta_gc },
      { "__newindex", userdata_meta_newindex_as_table },
      { "__index", userdata_meta_index_as_table },
  };
*/

  register_functions(net_module_name, methods);
  lua_getglobal(current_l, "sol"); // ... sol
  lua_getfield(current_l, -1, "net"); // ... sol net
  lua_setfield(current_l, LUA_REGISTRYINDEX, net_module_name.c_str()); // ... sol
  lua_pop(current_l, 1); // ...

}

}



