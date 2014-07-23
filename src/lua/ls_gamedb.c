#ifdef WIN32
#include <SDL.h>
#include <SDL_mixer.h>
#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>
#else
#include <SDL2/SDL.h>
#include <SDL2/SDL_mixer.h>
#include <lua5.2/lua.h>
#include <lua5.2/lauxlib.h>
#include <lua5.2/lualib.h>
#endif
#include "gamedb.h"

/*1 GameDB
 *# The {/Game Database/}. 
 *# These functions are used to store key-value pairs in a
 *# globally available memory area. The purpose is twofold:
 *{
 ** To share data between different game states.
 ** To save game-specific variables in the "Save Game"/"Load Game" functionality.
 *}
 */

/*@ GameDB.set(key, value)
 *# Saves a key-value pair in the game database.\n
 *# {{key}} and {{value}} are converted to strings internally.
 */
static int gamedb_set(lua_State *L) {	
	const char *key = luaL_checkstring(L, 1);
	const char *val = luaL_checkstring(L, 2);
	gdb_put(key, val);
	return 0;
}

/*@ GameDB.get(key, [default])
 *# Retrieves the value associated with the {{key}} from the game database (as a string).\n
 *# If the key does not exist in the database it will either return {{default}} 
 *# if it is provided, otherwise {{nil}}.
 */
static int gamedb_get(lua_State *L) {	
	const char *key = luaL_checkstring(L, 1);
	const char *val = gdb_get_null(key);
	if(val)
		lua_pushstring(L, val);
	else {
		if(lua_gettop(L) > 1) 
			lua_pushvalue(L, 2);
		else
			lua_pushnil(L);
	}
	return 1;
}

/*@ GameDB.has(key)
 *# Returns {{true}} if the {{key}} is stored in the Game database.
 */
static int gamedb_has(lua_State *L) {	
	const char *key = luaL_checkstring(L, 1);
	lua_pushboolean(L, gdb_has(key));
	return 1;
}

/*@ LocalDB.set(key, value)
 *# Saves a key-value pair in the game database.\n
 *# {{key}} and {{value}} are converted to strings internally.
 */
static int gamedb_local_set(lua_State *L) {	
	const char *key = luaL_checkstring(L, 1);
	const char *val = luaL_checkstring(L, 2);
	gdb_local_put(key, val);
	return 0;
}

/*@ LocalDB.get(key, [default])
 *# Retrieves the value associated with the {{key}} from the game database (as a string).\n
 *# If the key does not exist in the database it will either return {{default}} 
 *# if it is provided, otherwise {{nil}}.
 */
static int gamedb_local_get(lua_State *L) {	
	const char *key = luaL_checkstring(L, 1);
	const char *val = gdb_local_get_null(key);
	if(val)
		lua_pushstring(L, val);
	else {
		if(lua_gettop(L) > 1) 
			lua_pushvalue(L, 2);
		else
			lua_pushnil(L);
	}
	return 1;
}

/*@ LocalDB.has(key)
 *# Returns {{true}} if the {{key}} is stored in the Game database.
 */
static int gamedb_local_has(lua_State *L) {	
	const char *key = luaL_checkstring(L, 1);
	lua_pushboolean(L, gdb_local_has(key));
	return 1;
}

static const luaL_Reg gdb_funcs[] = {
  {"set",  gamedb_set},
  {"get",  gamedb_get},
  {"has",  gamedb_has},
  {0, 0}
};

static const luaL_Reg gdb_local_funcs[] = {
  {"set",  gamedb_local_set},
  {"get",  gamedb_local_get},
  {"has",  gamedb_local_has},
  {0, 0}
};

void register_gamedb_functions(lua_State *L) {
	
	luaL_newlib(L, gdb_funcs);
	lua_setglobal(L, "GameDB");	
	luaL_newlib(L, gdb_local_funcs);
	lua_setglobal(L, "LocalDB");

}