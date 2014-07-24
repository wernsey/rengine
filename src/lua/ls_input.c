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
#include "luastate.h"
#include "log.h"

/*1 Keyboard
 *# Keyboard Input Functions */

/*@ Keyboard.down([key])
 *# Checks whether a key is down on the keyboard.
 *# The parameter {{key}} can be either a scancode or the name of specific key.
 *# See http://wiki.libsdl.org/SDL_Scancode for the names of all the possible keys.
 *# If {{key}} is omitted, the function returns true if _any_ key is down.
 */
static int kb_keydown(lua_State *L) {	
	if(lua_gettop(L) > 0) {
		if(lua_type(L, 1) == LUA_TSTRING) {
			const char *name = luaL_checkstring(L, 1);
			lua_pushboolean(L, keys[SDL_GetScancodeFromName(name)]);
		} else {
			int code = luaL_checkinteger(L, 1);
			if(code < 0 || code >= SDL_NUM_SCANCODES)
				luaL_error(L, "invalid scancode (%d)", code);
			lua_pushboolean(L, keys[code]);
		}
	} else {
		lua_pushboolean(L, kb_hit());
	}	
	return 1;
}

/*@ Keyboard.pressed()
 *# Checks whether any key has been pressed during the last frame.\n
 *# It returns `nil` if no keys were pressed, otherwise it return the name 
 *# of the key that was pressed.
 *# See http://wiki.libsdl.org/SDL_Scancode for the names of all the possible keys.
 */
static int kb_keypressed(lua_State *L) {
	if(kb_hit()) {
		int scancode = readkey();
		lua_pushstring(L, SDL_GetScancodeName(scancode));
	} else
		lua_pushnil(L);
	return 1;
}

/*@ Keyboard.readAscii()
 *# Reads the ASCII value of the last key that was pressed.\n
 *# It returns `nil` if no key was pressed, and an empty string if
 *# the key's value couldn't be converted to ASCII.
 */
static int kb_readAscii(lua_State *L) {
	if(kb_hit()) {
		int scancode = readkey();
		char string[] = {0,0};
		int shift = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
		int mod = SDL_GetModState();
		int caps = mod & KMOD_CAPS ? 1 : 0;
		int numl = mod & KMOD_NUM ? 1 : 0;
		string[0] = scancode_to_ascii(scancode, shift, caps, numl);
		lua_pushstring(L, string);
	} else
		lua_pushnil(L);
	return 1;
}


/*@ Keyboard.fromName(name)
 *# Returns the scan code of a key name.
 */
static int kb_fromname(lua_State *L) {
	const char *name = luaL_checkstring(L, 1);
	lua_pushinteger(L, SDL_GetScancodeFromName(name));
	return 1;
}

/*@ Keyboard.nameScancode(code)
 *# Returns the name of a particular scancode.
 */
static int kb_fromscancode(lua_State *L) {
	int code = luaL_checkinteger(L, 1);
	lua_pushstring(L, SDL_GetScancodeName(code));
	return 1;
}

/*@ Keyboard.reset()
 *# Resets the keyboard input.
 */
static int kb_reset_keys(lua_State *L) {	
	reset_keys();
	return 0;
}

static const luaL_Reg keyboard_funcs[] = {
  {"down",   kb_keydown},
  {"pressed",  kb_keypressed},
  {"readAscii",  kb_readAscii},
  {"fromName",  kb_fromname},
  {"nameScancode",  kb_fromscancode},
  {"reset",  kb_reset_keys},
  {0, 0}
};

/*1 Mouse
 *# Mouse input functions.
 *#
 *# These constants are used with {{Mouse.down()}} and {{Mouse.click()}}
 *# to identify specific mouse buttons:
 *{
 ** {{Mouse.LEFT}}
 ** {{Mouse.MIDDLE}}
 ** {{Mouse.RIGHT}}
 *}
 */

/*@ x,y = Mouse.position()
 *# Returns the {{x,y}} position of the mouse.
 *X local x,y = Mouse.position();
 */
static int in_mousepos(lua_State *L) {	
	lua_pushinteger(L, mouse_x);
	lua_pushinteger(L, mouse_y);
	return 2;
}

/*@ Mouse.down(btn)
 *# Returns true if the button {{btn}} is down.
 */
static int in_mousedown(lua_State *L) {	
	int btn = luaL_checkinteger(L, 1);
	lua_pushboolean(L, mouse_btns & SDL_BUTTON(btn));
	return 1;
}

/*@ Mouse.click(btn)
 *# Returns true if the button {{btn}} was clicked.
 *# A button is considered clicked if it was down the
 *# previous frame and is not anymore this frame.
 */
static int in_mouseclick(lua_State *L) {	
	int btn = luaL_checkinteger(L, 1);
	lua_pushboolean(L, mouse_clck & SDL_BUTTON(btn));
	return 1;
}

/*@ Mouse.cursor([show])
 *# Gets/Sets the visibility of the mouse cursor.
 *# If {{show}} is specified, the visibility of the cursor will be set.
 *# It returns whether the cursor is visible or not.
 */
static int in_mousecursor(lua_State *L) {
    int show;
    if(lua_gettop(L) > 0) {
        show = lua_toboolean(L, 1);
        if(SDL_ShowCursor(show) < 0) {
            rerror("SDL_ShowCursor: %s", SDL_GetError());
        }
    }
    show = SDL_ShowCursor(-1);
    if(show < 0) {
        rerror("SDL_ShowCursor: %s", SDL_GetError());
    }
    lua_pushboolean(L, show);
	return 1;
}

static const luaL_Reg mouse_funcs[] = {
  {"position",  in_mousepos},
  {"down",  in_mousedown},
  {"click",  in_mouseclick},
  {"cursor",  in_mousecursor},
  {0, 0}
};

void register_input_functions(lua_State *L) {
    luaL_newlib(L, keyboard_funcs);
	lua_setglobal(L, "Keyboard");
	
	luaL_newlib(L, mouse_funcs);
	SET_TABLE_INT_VAL("LEFT", 1);
	SET_TABLE_INT_VAL("MIDDLE", 2);
	SET_TABLE_INT_VAL("RIGHT", 3);
	lua_setglobal(L, "Mouse");
}