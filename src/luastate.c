/*
*# [Lua](http://www.lua.org/home.html) is a popular scripting language for games. 
*# Through Rengine's Lua State you get access to a Lua interpreter, with most of 
*# Rengine's functionality exposed.
*#
*# Links:
*{
** The [Lua Homepage](http://www.lua.org/home.html)
** A great introduction to Lua is the book [Programming In Lua](http://www.lua.org/pil/contents.html)
*}
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>


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

#include "bmp.h"
#include "states.h"
#include "tileset.h"
#include "map.h"
#include "game.h"
#include "ini.h"
#include "utils.h"
#include "log.h"
#include "gamedb.h"
#include "resources.h"
#include "luastate.h"

/*
These are the Lua scripts in the ../scripts/ directory.
The script files are converted to C code strings using the Bace utility.
These strings are then executed through the Lua interpreter to provide
a variety of built in Lua code.
*/
extern const char base_lua[];
extern size_t base_lua_len;

/* LUA FUNCTIONS */

struct lustate_data *get_state_data(lua_State *L) {
	struct lustate_data *sd;
	lua_getglobal(L, STATE_DATA_VAR);
	if(!lua_islightuserdata(L, -1)) {
		luaL_error(L, "Variable %s got tampered with.", STATE_DATA_VAR);
		return 0; /* satisfy the compiler */
	} else {
		sd = lua_touserdata(L, -1);
	}
	lua_pop(L, 1);
	return sd;
}

/*1 Global functions 
 *# These functions have global scope in the Lua script.
 */

/*@ log(message)
 *# Writes a message to Rengine's log file.
 */
static int l_log(lua_State *L) {
	const char * s = lua_tolstring(L, 1, NULL);
	if(s) {
		sublog("Lua", "%s", s);
	}
	return 0;
}

/*@ import(path)
 *# Loads a Lua script from the resource on the specified path.
 *# This function is needed because the standard Lua functions like
 *# require() and dofile() are disabled in the Rengine sandbox.
 */
static int l_import(lua_State *L) {	
	const char *path = lua_tolstring(L, 1, NULL);
	char * script = re_get_script(path);
	if(!script)
		luaL_error(L, "Could not import %s", path);
	rlog("Imported %s", path);
	if(luaL_dostring(L, script)) {
		sublog("lua", "%s: %s", path, lua_tostring(L, -1));
	}
	free(script);
	return 0;
}

/*@ setTimeout(func, millis)
 *# Waits for {{millis}} milliseconds, then calls {{func}}
 */
static int l_set_timeout(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);

	/* This link was useful: 
	http://stackoverflow.com/questions/2688040/how-to-callback-a-lua-function-from-a-c-function 
	*/
	if(lua_gettop(L) == 2 && lua_isfunction(L, -2) && lua_isnumber(L, -1)) {
		
		if(sd->n_timeout == MAX_TIMEOUTS) {
			luaL_error(L, "Maximum number of timeouts [%d] reached", MAX_TIMEOUTS);
		}
		
		/* Push the callback function on to the top of the stack */
		lua_pushvalue(L, -2);
		
		/* And create a reference to it in the special LUA_REGISTRYINDEX */
		sd->timeout[sd->n_timeout].fun = luaL_ref(L, LUA_REGISTRYINDEX);
		
		sd->timeout[sd->n_timeout].time = luaL_checkinteger(L, 2);		
		sd->timeout[sd->n_timeout].start = SDL_GetTicks();
		
		sd->n_timeout++;
		
	} else {
		luaL_error(L, "setTimeout() requires a function and a time as parameters");
	}
	
	return 0;
}

void process_timeouts(lua_State *L) {
	struct lustate_data *sd;
	int i = 0;
	
	lua_getglobal(L, STATE_DATA_VAR);
	if(!lua_islightuserdata(L, -1)) {
		/* Can't call LuaL_error here. */
		rerror("Variable %s got tampered with.", STATE_DATA_VAR);
		return;
	} else {
		sd = lua_touserdata(L, -1);
	}
	lua_pop(L, 1);
	
	while(i < sd->n_timeout) {
		Uint32 elapsed = SDL_GetTicks() - sd->timeout[i].start;
		if(elapsed > sd->timeout[i].time) {			
			/* Retrieve the callback */
			lua_rawgeti(L, LUA_REGISTRYINDEX, sd->timeout[i].fun);
			
			/* Call it */
			if(lua_pcall(L, 0, 0, 0)) {
				rerror("Unable to execute setTimeout() callback: %s", lua_tostring(L, -1));				
			}
			/* Release the reference so that it can be collected */
			luaL_unref(L, LUA_REGISTRYINDEX, sd->timeout[i].fun);
			
			/* Now delete this timeout by replacing it with the last one */
			sd->timeout[i] = sd->timeout[--sd->n_timeout];
		} else {
			i++;
		}
	}
}

/*@ onUpdate(func)
 *# Registers the function {{func}} to be called every frame
 *# when Rengine draws the screen.
 */
static int l_onUpdate(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);

	if(lua_gettop(L) == 1 && lua_isfunction(L, -1)) {
		struct callback_function *fn;
				
		fn = malloc(sizeof *fn);
		if(!fn)
			luaL_error(L, "Out of memory");
		
		/* And create a reference to it in the special LUA_REGISTRYINDEX */
		fn->ref = luaL_ref(L, LUA_REGISTRYINDEX);		
		rlog("Registering onUpdate() callback %d", fn->ref);
		
		fn->next = NULL;
		if(!sd->last_fcn) {
			assert(sd->update_fcn == NULL);
			sd->last_fcn = fn;
			sd->update_fcn = fn;
		} else {
			sd->last_fcn->next = fn;
			sd->last_fcn = fn;
		}
		
		lua_pushinteger(L, fn->ref);
		
	} else {
		luaL_error(L, "onUpdate() requires a function as a parameter");
	}
	
	return 1;
}

/*@ atExit(func)
 *# Registers the function {{func}} to be called when the
 *# current state is exited.\n
 *# These functions should not do any drawing.
 */
static int l_atExit(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);

	if(lua_gettop(L) == 1 && lua_isfunction(L, -1)) {
		struct callback_function *fn;
				
		fn = malloc(sizeof *fn);
		if(!fn)
			luaL_error(L, "Out of memory");
		
		/* And create a reference to it in the special LUA_REGISTRYINDEX */
		fn->ref = luaL_ref(L, LUA_REGISTRYINDEX);		
		rlog("Registering atExit() callback %d", fn->ref);
		
		fn->next = sd->atexit_fcn;
		sd->atexit_fcn = fn;
		
		lua_pushinteger(L, fn->ref);		
	} else {
		luaL_error(L, "atExit() requires a function as a parameter");
	}
	
	return 1;
}

struct callback_function *atexit_fcn, *last_atexit_fcn;

/* Declared in src/lua/ls_game.c */
void register_game_functions(lua_State *L);

/* Declared in src/lua/ls_bmp.c */
void register_bmp_functions(lua_State *L);

/* Declared in src/lua/ls_gfx.c */
void register_gfx_functions(lua_State *L);

/* Declared in src/lua/ls_map.c */
void register_map_functions(lua_State *L);

/* Declared in src/lua/ls_input.c */
void register_input_functions(lua_State *L);

/* Declared in src/lua/ls_audio.c */
void register_sound_functions(lua_State *L);

/* Declared in src/lua/ls_gamedb.c */
void register_gamedb_functions(lua_State *L);

/*
Based on the discussion at http://stackoverflow.com/a/966778/115589
I've copied the code from Lua's linit.c and removed the functions
deemed unsafe.
*/
#define ALLOW_UNSAFE 0
static const luaL_Reg sandbox_libs[] = {
  {"_G", luaopen_base},
  {LUA_COLIBNAME, luaopen_coroutine},
  {LUA_TABLIBNAME, luaopen_table},
  {LUA_STRLIBNAME, luaopen_string},
  {LUA_BITLIBNAME, luaopen_bit32},
  {LUA_MATHLIBNAME, luaopen_math},
#if ALLOW_UNSAFE
  {LUA_LOADLIBNAME, luaopen_package},
  {LUA_IOLIBNAME, luaopen_io},
  {LUA_OSLIBNAME, luaopen_os},
  {LUA_DBLIBNAME, luaopen_debug},
#endif
  {NULL, NULL}
};

void open_sandbox_libs (lua_State *L) {
	const luaL_Reg *lib;
	for (lib = sandbox_libs; lib->func; lib++) {
		luaL_requiref(L, lib->name, lib->func, 1);
		lua_pop(L, 1);  /* remove lib */
	}
}

/*
I've tried these, but they didn't work for me
http://stackoverflow.com/a/12256526/115589
http://zeuxcg.org/2010/11/07/lua-callstack-with-c-debugger/
http://stackoverflow.com/a/18177386/115589
*/
static int lua_stacktrace(lua_State *L) {
    /* rlog("Doing Lua Stack Trace"); */
    return 1;
}

static int lus_init(struct game_state *s) {
	
	const char *map_file, *script_file;
	char *map_text, *script;
	lua_State *L = NULL;
	struct lustate_data *sd;
		
	rlog("Initializing Lua state '%s'", s->name);
	
	/* Load the Lua script */
	script_file = ini_get(game_ini, s->name, "script", NULL);	
	if(!script_file) {
		rerror("Lua state '%s' doesn't specify a script file.", s->name);
		return 0;
	}
	script = re_get_script(script_file);
	if(!script) {
		rerror("Script %s was not found (state %s).", script_file, s->name);
		return 0;
	}
	
	/* Create the Lua interpreter */
	L = luaL_newstate();
	if(!L) { 
		rerror("Couldn't create Lua state.");
		return 0;
	}
	s->data = L;
	
	/* Sandbox Lua instead of calling luaL_openlibs(L); */
	open_sandbox_libs(L);
	
	/* Create and init the State Data that the interpreter carries with it. */
	sd = malloc(sizeof *sd);
	if(!sd)
		return 0;
	
	sd->state = s;	
	sd->update_fcn = NULL;	
	sd->last_fcn = NULL;
	sd->atexit_fcn = NULL;	
	
	sd->n_timeout = 0;
	
    sd->bmp = get_screen();
	
    sd->map = NULL;
		
	sd->change_state = 0;
	sd->next_state = NULL;
	
	/* Store the State Data in the interpreter */
	lua_pushlightuserdata(L, sd);		
	lua_setglobal(L, STATE_DATA_VAR);
	
	/* Load the map, if one is specified. */
	map_file = ini_get(game_ini, s->name, "map", NULL);
	if(map_file) {
		map_text = re_get_script(map_file);
		if(!map_text) {
			rerror("Unable to retrieve map resource '%s' (state %s).", map_file, s->name);
			return 0;		
		}
		
		sd->map = map_parse(map_text, 0);
		if(!sd->map) {
			rerror("Unable to parse map '%s' (state %s).", map_file, s->name);
			return 0;		
		}
		free(map_text);
		
        register_map_functions(L);
        
	} else {
		rlog("Lua state %s does not specify a map file.", s->name);
		lua_pushnil(L);
	}
	lua_setglobal(L, "Map");
	
	GLOBAL_FUNCTION("log", l_log);
	GLOBAL_FUNCTION("setTimeout", l_set_timeout);
	GLOBAL_FUNCTION("onUpdate", l_onUpdate);
	GLOBAL_FUNCTION("atExit", l_atExit);
	GLOBAL_FUNCTION("import", l_import);    
    
	/* Register some Lua variables. */	
	register_game_functions(L);
	
    /* The graphics object G gives you access to all the 2D drawing functions */
	register_gfx_functions(L);
    
	/* The Bitmap object is constructed through the Bmp() function that loads 
		a bitmap through the resources module that can be drawn with G.blit() */
	register_bmp_functions(L);
	
	/* The input objects Keyboard and Mouse gives you access to the 
    keyboard and mouse. Did you expect anything else? */
    register_input_functions(L);
        
    register_gamedb_functions(L);
	
    register_sound_functions(L);
    
	if(luaL_dostring(L, base_lua)) {
		rerror("Unable load base library.");
		sublog("lua", "%s", lua_tostring(L, -1));
		free(script);				
		return 0;
	}
	
	/* Load the Lua script itself, and execute it. */
	if(luaL_loadstring(L, script)) {		
		rerror("Unable to load script %s (state %s).", script_file, s->name);
		sublog("lua", "%s", lua_tostring(L, -1));
		free(script);				
		return 0;
	}
	free(script);
	
	rlog("Running script %s", script_file);	
	if(lua_pcall(L, 0, 0, 0)) {
		rerror("Unable to execute script %s (state %s).", script_file, s->name);
		sublog("Lua", "%s", lua_tostring(L, -1));
		/*
		Wouldn't it be nice to be able to print a stack trace here.
		See http://stackoverflow.com/a/12256526/115589
        http://zeuxcg.org/2010/11/07/lua-callstack-with-c-debugger/
        There is also a nice looking debugger (fully implemented in Lua)
        here: https://github.com/slembcke/debugger.lua/blob/master/debugger.lua
		*/
        lua_stacktrace(L);
		return 0;
	}
		
	return 1;
}

static int lus_update(struct game_state *s, struct bitmap *bmp) {
	struct lustate_data *sd = NULL;
	lua_State *L = s->data;
	struct callback_function *fn;
	
	assert(L);
	
	lua_getglobal(L, STATE_DATA_VAR);
	if(!lua_isnil(L,-1)) {
		if(!lua_islightuserdata(L, -1)) {
			rerror("Variable %s got tampered with (lus_update)", STATE_DATA_VAR);
			return 0;
		}
		sd = lua_touserdata(L, -1);
	} else {
		rerror("Variable %s got tampered with (lus_update)", STATE_DATA_VAR);
		return 0;
	}
	lua_pop(L, 1);
    
    lua_getglobal (L, "G");
	SET_TABLE_INT_VAL("frameCounter", frame_counter);
	lua_pop(L, 1);
	
	/* TODO: Maybe background colour metadata in the map file? */
	bm_set_color_s(bmp, "black");
	bm_clear(bmp);
	
	process_timeouts(L);
	
	fn = sd->update_fcn;
	while(fn) {				
		/* Retrieve the callback */
		lua_rawgeti(L, LUA_REGISTRYINDEX, fn->ref);
		
		/* Call it */
		if(lua_pcall(L, 0, 0, 0)) {
			rerror("Unable to execute onUpdate() callback (%d)", fn->ref);
			sublog("Lua", "%s", lua_tostring(L, -1));
            lua_stacktrace(L);
			/* Should we remove it maybe? */
		}
		
		fn = fn->next;
	}
	
	if(sd->change_state) {
		if(!sd->next_state) {
			rwarn("Lua script didn't specify a next state; terminating...");
			change_state(NULL);
		} else {	
			rlog("Lua script changing state to %s", sd->next_state);
			set_state(sd->next_state);
		}
	}
	
	return 1;
}

static int lus_deinit(struct game_state *s) {
	lua_State *L = s->data;
	struct lustate_data *sd;
	
	if(!L)
		return 0;
		
	lua_getglobal(L, STATE_DATA_VAR);
	if(!lua_isnil(L,-1)) {
		if(!lua_islightuserdata(L, -1)) {
			rerror("Variable %s got tampered with (lus_deinit)", STATE_DATA_VAR);
		} else {
			struct callback_function *fn;
			
			sd = lua_touserdata(L, -1);			
			
			/* Execute the atExit() callbacks */
			fn = sd->atexit_fcn;
			while(fn) {				
				struct callback_function *old = fn;
				lua_rawgeti(L, LUA_REGISTRYINDEX, fn->ref);
				if(lua_pcall(L, 0, 0, 0)) {
					rerror("Unable to execute atExit() callback (%d)", fn->ref);
					sublog("Lua", "%s", lua_tostring(L, -1));
				}
				fn = fn->next;
				free(old);
			}
			
			/* Remove the map */
			map_free(sd->map);
			
			while(sd->update_fcn) {
				fn = sd->update_fcn;
				sd->update_fcn = sd->update_fcn->next;
				free(fn);
			}
			
			if(sd->next_state);
				free(sd->next_state);
			
			free(sd);
		}
	}
	lua_pop(L, 1);
	
	/* Stop all sounds */
	Mix_HaltChannel(-1);
	Mix_HaltMusic();
	
	lua_close(L);
	L = NULL;
	
	return 1;
}

struct game_state *get_lua_state(const char *name) {
	struct game_state *state = malloc(sizeof *state);
	if(!state)
		return NULL;
	
	state->data = NULL;
	
	state->init = lus_init;
	state->update = lus_update;
	state->deinit = lus_deinit;
	
	return state;
}
