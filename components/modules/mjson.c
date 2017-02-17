// Module for mjson

#include <stdlib.h>

#include "module.h"
#include "lauxlib.h"
#include "platform.h"

#include "cJSON.h"
extern void (*cJSON_free)(void *ptr);

typedef struct {
  cJSON *data;
} mjson_data;

static int mjson_value( lua_State *L, cJSON *data )
{
  if(data == NULL || data->type == cJSON_NULL) {
    lua_pushnil(L);
  } else if(data->type == cJSON_True) {
    lua_pushboolean( L, 1);
  } else if(data->type == cJSON_False) {
    lua_pushboolean( L, 0);
  } else if(data->type == cJSON_Number) {
    lua_pushnumber( L, data->valuedouble);
  } else if(data->type == cJSON_Number) {
    lua_pushstring( L, data->valuestring);
  } else {
    mjson_data *md = (mjson_data *)lua_newuserdata( L, sizeof( mjson_data ) );
    md->data = data;
    luaL_getmetatable(L, "mjson");
    lua_setmetatable(L, -2);
  }
  return 1;
}


static int mjson_obj_index (lua_State *L) {
  mjson_data *md = (mjson_data *)luaL_checkudata(L, 1, "mjson");
  if( lua_type( L, 2 ) == LUA_TNUMBER ) {
    int k = luaL_checkinteger( L, 2 );
    cJSON *item = cJSON_GetArrayItem(md->data, k);
    return mjson_value(L, item);
  } else if( lua_type( L, 2 ) == LUA_TSTRING ) {
    const char *k = luaL_checkstring( L, 2 );
    cJSON *item = cJSON_GetObjectItem(md->data, k);
    return mjson_value(L, item);
  }
  return luaL_error( L, "wrong arg range" );
}

static int mjson_obj_gc (lua_State *L) {
  mjson_data *md = (mjson_data *)luaL_checkudata(L, 1, "mjson");
  cJSON_Delete(md->data);
  return 0;
}

static int mjson_obj_newindex (lua_State *L) {
  mjson_data *md = (mjson_data *)luaL_checkudata(L, 1, "mjson");
  cJSON *item = NULL;
  int itemType = lua_type( L, 3 );
  if( itemType == LUA_TNIL ) {
    item = cJSON_CreateNull();
  } else if( lua_type( L, 3 ) == LUA_TBOOLEAN ) {
    item = cJSON_CreateBool(lua_toboolean( L, 3 ));
  } else if( lua_type( L, 3 ) == LUA_TNUMBER ) {
    item = cJSON_CreateNumber(luaL_checknumber( L, 3 ));
  } else if( lua_type( L, 3 ) == LUA_TSTRING ) {
    item = cJSON_CreateString(luaL_checkstring( L, 3 ));
  } else if( lua_type( L, 3 ) == LUA_TUSERDATA ) {
    mjson_data *sub = (mjson_data *)luaL_checkudata(L, 3, "mjson");
    item = sub->data;
  } else {
    return luaL_error( L, "wrong arg range" );
  }
  if( lua_type( L, 2 ) == LUA_TNUMBER ) {
    int k = luaL_checkinteger( L, 2 );
    cJSON_ReplaceItemInArray(md->data, k, item);
  } else if( lua_type( L, 2 ) == LUA_TSTRING ) {
    const char *k = luaL_checkstring( L, 2 );
    cJSON_AddItemToObject(md->data, k, item);
  }
  return 0;
}

// Lua: decode( str )
static int mjson_decode( lua_State *L )
{
  luaL_argcheck(L, lua_gettop(L) == 1, 1, "expected 1 argument");

  size_t json_len;
  const char *json = luaL_checklstring(L, 1, &json_len);

  cJSON *data = cJSON_Parse(json);
  
  return mjson_value(L, data);
}

// Lua: encode( mjson )
static int mjson_encode( lua_State *L )
{
  luaL_argcheck(L, lua_gettop(L) == 1, 1, "expected 1 argument");

  mjson_data *md = (mjson_data *)luaL_checkudata(L, 1, "mjson");
  char *json = cJSON_PrintUnformatted(md->data);
  lua_pushstring( L, json);
  free(json);
  return 1;
}

// Lua: new_array( )
static int mjson_new_array( lua_State *L )
{
  luaL_argcheck(L, lua_gettop(L) == 0, 1, "expected 0 argument");

  mjson_data *md = (mjson_data *)lua_newuserdata( L, sizeof( mjson_data ) );
  md->data = cJSON_CreateArray();
  luaL_getmetatable(L, "mjson");
  lua_setmetatable(L, -2);

  return 1;
}

// Lua: new_object( )
static int mjson_new_object( lua_State *L )
{
  luaL_argcheck(L, lua_gettop(L) == 0, 1, "expected 0 argument");

  mjson_data *md = (mjson_data *)lua_newuserdata( L, sizeof( mjson_data ) );
  md->data = cJSON_CreateObject();
  luaL_getmetatable(L, "mjson");
  lua_setmetatable(L, -2);

  return 1;
}

// Module function map
static const LUA_REG_TYPE mjson_map[] =
{
  { LSTRKEY( "new_array" ),   LFUNCVAL( mjson_new_array ) },
  { LSTRKEY( "new_object" ),   LFUNCVAL( mjson_new_object ) },
  { LSTRKEY( "decode" ),       LFUNCVAL( mjson_decode ) },
  { LSTRKEY( "encode" ),       LFUNCVAL( mjson_encode ) },
  { LNILKEY, LNILVAL }
};

static const LUA_REG_TYPE mjson_obj_map[] =
{
  { LSTRKEY( "__index" ),   LFUNCVAL( mjson_obj_index )},
  { LSTRKEY( "__newindex" ),LFUNCVAL( mjson_obj_newindex )},
  { LSTRKEY( "__gc" ),      LFUNCVAL( mjson_obj_gc )},
  { LNILKEY, LNILVAL }
};

int luaopen_mjson( lua_State *L ) {
  luaL_rometatable( L, "mjson",  (void *)mjson_obj_map );
  return 0;
}

NODEMCU_MODULE(MJSON, "mjson", mjson_map, luaopen_mjson);
