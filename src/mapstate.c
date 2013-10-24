#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#ifdef WIN32
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "bmp.h"
#include "states.h"
#include "map.h"
#include "game.h"
#include "ini.h"
#include "resources.h"
#include "tileset.h"
#include "utils.h"

#define MAX_TIMEOUTS 20

#define MAP_VARIABLE		"___the_map"
#define TIMEOUT_VARIABLE	"___timeouts"

struct timeout {
	struct _timeout_element {
		int fun;
		int time;
		Uint32 start;
	} to[MAX_TIMEOUTS];
	int top;
};

/* LUA FUNCTIONS *********************************************************************************/

static int l_log(lua_State *L) {
	const char * s = lua_tolstring(L, 1, NULL);
	if(s) {
		fprintf(log_file, "lua: %s\n", s);
		fflush(log_file);
	}
	return 0;
}

static int l_set_timeout(lua_State *L) {
	struct timeout *timeouts;
	
	lua_getglobal(L, TIMEOUT_VARIABLE);
	if(!lua_islightuserdata(L, -1)) {
		luaL_error(L, "Variable %s got tampered with.", TIMEOUT_VARIABLE);
		return 0; /* satisfy the compiler */
	} else {
		timeouts = lua_touserdata(L, -1);
	}
	lua_pop(L, 1);

	/* This link was useful: 
	http://stackoverflow.com/questions/2688040/how-to-callback-a-lua-function-from-a-c-function 
	*/
	if(lua_gettop(L) == 2 && lua_isfunction(L, -2) && lua_isnumber(L, -1)) {
		
		if(timeouts->top == MAX_TIMEOUTS) {
			luaL_error(L, "Maximum number of timeouts [%d] reached", MAX_TIMEOUTS);
		}
		
		/* Push the callback function on to the top of the stack */
		lua_pushvalue(L, -2);
		
		/* And create a reference to it in the special LUA_REGISTRYINDEX */
		timeouts->to[timeouts->top].fun = luaL_ref(L, LUA_REGISTRYINDEX);
		
		timeouts->to[timeouts->top].time = luaL_checkinteger(L, 2);		
		timeouts->to[timeouts->top].start = SDL_GetTicks();
		
		timeouts->top++;
		
	} else {
		luaL_error(L, "setTimeout() requires a function and a time as parameters");
	}
	
	return 0;
}

static void process_timeouts(lua_State *L) {
	int i = 0;	
	struct timeout *timeouts;
	
	lua_getglobal(L, TIMEOUT_VARIABLE);
	if(!lua_islightuserdata(L, -1)) {
		luaL_error(L, "Variable %s got tampered with.", TIMEOUT_VARIABLE);
		return;
	} else {
		timeouts = lua_touserdata(L, -1);
	}
	lua_pop(L, 1);
	
	while(i < timeouts->top) {
		Uint32 elapsed = SDL_GetTicks() - timeouts->to[i].start;
		if(elapsed > timeouts->to[i].time) {			
			/* Retrieve the callback */
			lua_rawgeti(L, LUA_REGISTRYINDEX, timeouts->to[i].fun);
			
			/* Call it */
			if(lua_pcall(L, 0, 0, 0)) {
				fprintf(log_file, "error: Unable to execute setTimeout() callback\n");
				fprintf(log_file, "lua: %s\n", lua_tostring(L, -1));				
			}
			/* Release the reference so that it can be collected */
			luaL_unref(L, LUA_REGISTRYINDEX, timeouts->to[i].fun);
			
			/* Now delete this timeout by replacing it with the last one */
			timeouts->to[i] = timeouts->to[--timeouts->top];
		} else {
			i++;
		}
	}
	fflush(log_file);
}

/* The C(ell) object *****************************************************************************/

typedef struct _cell_obj {
	struct map_cell *cell;
	struct _cell_obj *next;
} cell_obj;

static int class_selector(struct map_cell *c, const char *data) {
	if(!c->clas) return 0;
	return !my_stricmp(c->clas, data + 1);
}

static int id_selector(struct map_cell *c, const char *data) {
	if(!c->id) return 0;
	return !my_stricmp(c->id, data);
}

static int new_cell_obj(lua_State *L) {
	const char *selector;
	int i;
	int (*sel_fun)(struct map_cell *c, const char *data);
	struct map *map;
	cell_obj **o;

	o = lua_newuserdata(L, sizeof *o);	
	luaL_setmetatable(L, "CellObj");
	
	lua_getglobal(L, MAP_VARIABLE);	
	if(!lua_islightuserdata(L, -1)) {
		/* Don't ever do anything to ___the_map in your Lua scripts */
		luaL_error(L, "Variable %s got tampered with.", MAP_VARIABLE);
	}
	map = lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	*o = NULL;	
	selector = luaL_checkstring(L,1);
	if(selector[0] == '.') {
		sel_fun = class_selector;
	} else {		
		sel_fun = id_selector;
	}
	
	/* Is there a non-O(n) way to find cells matching a selector? 
	 * I ought to put the ids in a hashtable at least
	 */
	for(i = 0; i < map->nr * map->nc; i++) {
		struct map_cell *c = &map->cells[i];
		if(sel_fun(c, selector)) {
			cell_obj *co = malloc(sizeof *co);
			co->cell = c;
			co->next = *o;				
			*o = co;
			if(sel_fun == id_selector)
				break;
		}
	}
	
	return 1;
}

static int gc_cell_obj(lua_State *L) {
	cell_obj **o = luaL_checkudata(L,1, "CellObj");
	while(*o) {
		cell_obj *t = *o;
		*o = (*o)->next;		
		free(t);
	}
	return 0;
}

static int cell_tostring(lua_State *L) {
	
	cell_obj **os = luaL_checkudata(L,1, "CellObj");
	cell_obj *o = *os;
	int count = 0;
	while(o) {
		count++;
		o=o->next;
	}
	lua_pushfstring(L, "CellObj[%d]", count);
	return 1;
}

static int cell_set(lua_State *L) {
	
	cell_obj **os = luaL_checkudata(L,1, "CellObj");
	cell_obj *o;
	int l = luaL_checkint(L,2);
	int si = luaL_checkint(L,3);
	int ti = luaL_checkint(L,4);
	
	if(l < 0 || l > 2) {
		lua_pushfstring(L, "Invalid level passed to CellObj.set()");
		lua_error(L);
	}
	if(si < 0 || si >= ts_get_num()) {
		lua_pushfstring(L, "Invalid si passed to CellObj.set()");
		lua_error(L);
	}
	/* FIXME: error checking on ti? */
	
	o = *os;
	while(o) {
		struct map_cell *c = o->cell;
		/* TODO: Maybe you ought to store
		this change in some sort of list
		so that the savedgames can handle 
		changes to the map like this. */
		c->tiles[l].ti = ti;
		c->tiles[l].si = si;
		o = o->next;
	}
	
	/* Push the CellObj back onto the stack so that other methods can be called on it */
	lua_pushvalue(L, -4);
	
	return 1;
}

/*
	I may have been doing this wrong. See http://lua-users.org/wiki/UserDataWithPointerExample
*/
static void cell_obj_meta(lua_State *L) {
	/* Create the metatable for MyObj */
	luaL_newmetatable(L, "CellObj");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index"); /* CellObj.__index = CellObj */
	
	/* FIXME: Add other methods. */
	lua_pushcfunction(L, cell_set);
	lua_setfield(L, -2, "set");
	
	lua_pushcfunction(L, cell_tostring);
	lua_setfield(L, -2, "__tostring");	
	lua_pushcfunction(L, gc_cell_obj);
	lua_setfield(L, -2, "__gc");	
	
	/* The global method C() */
	lua_pushcfunction(L, new_cell_obj);
	lua_setglobal(L, "C");
}

/* STATE FUNCTIONS *******************************************************************************/

static int map_init(struct game_state *s) {
	
	const char *map_file, *script_file;
	char *map_text, *script;
	struct map *map;
	lua_State *L = NULL;
	struct timeout *timeouts;
	
	fprintf(log_file, "info: Initializing Map state '%s'\n", s->name);
	
	map_file = ini_get(game_ini, s->name, "map", NULL);
	if(!map_file) {
		fprintf(log_file, "error: Map state '%s' doesn't specify a map file.\n", s->name);
		return 0;
	}
	
	script_file = ini_get(game_ini, s->name, "script", NULL);	
	if(!script_file) {
		fprintf(log_file, "error: Map state '%s' doesn't specify a script file.\n", s->name);
		return 0;
	}
	
	map_text = re_get_script(map_file);
	
	map = map_parse(map_text);
	if(!map) {
		fprintf(log_file, "error: Unable to parse map %s (state %s).\n", map_file, s->name);
		return 0;		
	}
	free(map_text);
	
	script = re_get_script(script_file);
	if(!script) {
		fprintf(log_file, "error: Script %s was not found (state %s).\n", script_file, s->name);
		return 0;
	}
	L = luaL_newstate();
	luaL_openlibs(L);
	
	lua_pushlightuserdata(L, map);
	if(!L) 
		return 0;
	lua_setglobal(L, MAP_VARIABLE);
	
	timeouts = malloc(sizeof *timeouts);
	if(!timeouts)
		return 0;
	lua_pushlightuserdata(L, timeouts);	
	lua_setglobal(L, TIMEOUT_VARIABLE);
	timeouts->top = 0;
	
	lua_pushcfunction(L, l_log);
    lua_setglobal(L, "log");
	
	lua_pushcfunction(L, l_set_timeout);
    lua_setglobal(L, "setTimeout");
	
	lua_pushinteger(L, 0);
    lua_setglobal(L, "BACKGROUND");
	lua_pushinteger(L, 1);
    lua_setglobal(L, "CENTER");
	lua_pushinteger(L, 2);
    lua_setglobal(L, "FOREGROUND");
	
	cell_obj_meta(L);
	
	if(luaL_loadstring(L, script)) {		
		fprintf(log_file, "error: Unable to load script %s (state %s).\n", script_file, s->name);
		fprintf(log_file, "lua: %s\n", lua_tostring(L, -1));
		free(script);
				
		return 0;
	}
	free(script);
	
	if(lua_pcall(L, 0, 0, 0)) {
		fprintf(log_file, "error: Unable to execute script %s (state %s).\n", script_file, s->name);
		fprintf(log_file, "lua: %s\n", lua_tostring(L, -1));
		return 0;
	}
	
	s->data = L;
		
	return 1;
}

static int map_update(struct game_state *s, struct bitmap *bmp) {
	int i;
	struct map *map;
	lua_State *L = s->data;
	
	assert(L);
	
	lua_getglobal(L, MAP_VARIABLE);
	if(!lua_islightuserdata(L, -1)) {
		/* Don't ever do anything to ___the_map in your Lua scripts */
		fprintf(log_file, "error: Variable %s got tampered with (map_update)\n", MAP_VARIABLE);
		return 0;
	}
	map = lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	/* TODO: Maybe background colour metadata in the map file? */
	bm_set_color_s(bmp, "black");
	bm_clear(bmp);
	
	process_timeouts(L);
	
	for(i = 0; i < 3; i++)
		map_render(map, bmp, i, 0, 0);
	
	if(kb_hit()) { /* FIXME */
		change_state(NULL);
		return 0;
	}
	
	return 1;
}

static int map_deinit(struct game_state *s) {
	struct map *map = NULL;	
	lua_State *L = s->data;
	struct timeout *timeouts;
	
	lua_getglobal(L, MAP_VARIABLE);
	if(!lua_islightuserdata(L, -1)) {
		/* Don't ever do anything to ___the_map in your Lua scripts */
		fprintf(log_file, "error: Variable %s got tampered with (map_deinit)\n", MAP_VARIABLE);
	} else {
		map = lua_touserdata(L, -1);
	}
	lua_pop(L, 1);
	
	lua_getglobal(L, TIMEOUT_VARIABLE);
	if(!lua_islightuserdata(L, -1)) {
		fprintf(log_file, "error: Variable %s got tampered with (map_deinit)\n", TIMEOUT_VARIABLE);
	} else {
		timeouts = lua_touserdata(L, -1);
		free(timeouts);
	}
	lua_pop(L, 1);
	
	lua_close(L);
	L = NULL;
	
	map_free(map);
	ts_free_all();
	
	return 1;
}

struct game_state *get_map_state(const char *name) {
	struct game_state *state = malloc(sizeof *state);
	if(!state)
		return NULL;
	state->name = name;
	
	state->init = map_init;
	state->update = map_update;
	state->deinit = map_deinit;
	
	return state;
}