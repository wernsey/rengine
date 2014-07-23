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

#include "game.h"
#include "states.h"
#include "luastate.h"

/*1 Game object
 *# Functions in the {{Game}} scope
 */

/*@ Game.changeState(newstate)
 *# Changes the game's [[state|State Machine]] to the state identified by {{newstate}}
 */
static int l_changeState(lua_State *L) {
	const char *next_state = luaL_checkstring(L, -1);
	struct lustate_data *sd = get_state_data(L);
	
	sd->next_state = strdup(next_state);
	sd->change_state = 1;
	
	return 0;
}

/*@ Game.getStyle(style, [default])
 *# Retrieves a specific [[Style]] from the [[game.ini]] file.
 */
static int l_getstyle(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	const char * s = luaL_checkstring(L,1);
    const char * v = get_style(sd->state, s);
    if(v) {
        lua_pushstring(L, v);
    } else {
        if(lua_gettop(L) > 1) {
            lua_pushstring(L, luaL_checkstring(L,2));
        } else {
            lua_pushstring(L, "");
        }
    }
	
	return 1;
}

/*@ Game.advanceFrame()
 *# Low-level function to advance the animation frame on the screen.\n
 *# It flips the back buffer, retrieves user inputs and processes system 
 *# events.\n
 *# In general you should not call this function directly, since Rengine
 *# does the above automatically. This function is provided for the special
 *# case where you need a Lua script to run in a loop, and during each 
 *# iteration update the screen and get new user input.\n
 *# An example of such a case is drwaing a GUI in a Lua script
 *# and handling the event loop of the GUI in Lua itself.\n
 *# It does not clear the screen - you'll need to do that yourself.\n
 *# Callbacks registered through {{onUpdate()}} will {/not/} be processed.
 *# Timeouts set through {{setTimeout()}} will be processed, however.\n
 *# It returns false if the user wants to quit. 
 *# If you use such an event loop, you should break when the function returns 
 *# `false`, otherwise your application won't quit when the user closes the window.\n
 */
static int l_advanceFrame(lua_State *L) {	
    static int times = 0;
	process_timeouts(L);
	advanceFrame();
	lua_pushboolean(L, !quit);
    if(quit && ++times > 10) {
        luaL_error(L, "Application ignored repeat requests to terminate");
    }	
    lua_getglobal (L, "G");
	lua_pushstring(L, "frameCounter"); 
	lua_pushinteger(L, frame_counter); 
	lua_rawset(L, -3);
	lua_pop(L, 1);
	return 1;
}

static const luaL_Reg game_funcs[] = {
  {"changeState",     l_changeState},
  {"getStyle",        l_getstyle},
  {"advanceFrame",    l_advanceFrame},
  {0, 0}
};

void register_game_functions(lua_State *L) {
	luaL_newlib(L, game_funcs);
	lua_setglobal(L, "Game");    
}