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
#include "tileset.h"
#include "map.h"
#include "game.h"
#include "ini.h"
#include "resources.h"
#include "utils.h"
#include "particles.h"
#include "log.h"

#define MAX_TIMEOUTS 20

/* Don't tamper with this variable from your Lua scripts. */
#define STATE_DATA_VAR	"___state_data"

struct _update_function {
	int ref;
	struct _update_function *next;
};

struct lustate_data {
	struct bitmap *bmp;
	struct map *map;	

	struct _timeout_element {
		int fun;
		int time;
		Uint32 start;
	} timeout[MAX_TIMEOUTS];
	int n_timeout;
	
	struct _update_function *update_fcn;
	
	int change_state;
	char *next_state;
};

/* LUA FUNCTIONS *********************************************************************************/

static struct lustate_data *get_state_data(lua_State *L) {
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

static int l_log(lua_State *L) {
	const char * s = lua_tolstring(L, 1, NULL);
	if(s) {
		sublog("Lua", "%s", s);
	}
	return 0;
}

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

static void process_timeouts(lua_State *L) {
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

/* ***********************************************************************************************/

static int l_onUpdate(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);

	if(lua_gettop(L) == 1 && lua_isfunction(L, -1)) {
		struct _update_function *fn;
				
		fn = malloc(sizeof *fn);
		if(!fn)
			luaL_error(L, "Out of memory");
		
		/* And create a reference to it in the special LUA_REGISTRYINDEX */
		fn->ref = luaL_ref(L, LUA_REGISTRYINDEX);		
		rlog("Registering onUpdate() callback %d", fn->ref);
		
		fn->next = sd->update_fcn;
		sd->update_fcn = fn;
		
		lua_pushinteger(L, fn->ref);
		
	} else {
		luaL_error(L, "onUpdate() requires a function as a parameter");
	}
	
	return 1;
}

static int l_createParticle(lua_State *L) {
	float x = luaL_checknumber(L, -6);
	float y = luaL_checknumber(L, -5);
	float dx = luaL_checknumber(L, -4);
	float dy = luaL_checknumber(L, -3);
	int life = luaL_checkinteger(L, -2); 
	int color = bm_color_atoi(luaL_checkstring(L, -1));	
	add_particle(x, y, dx, dy, life, color);
	return 0;
}

static int l_changeState(lua_State *L) {
	const char *next_state = luaL_checkstring(L, -1);
	struct lustate_data *sd = get_state_data(L);
	
	sd->next_state = strdup(next_state);
	sd->change_state = 1;
	
	return 0;
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
	struct lustate_data *sd;
	cell_obj **o;

	o = lua_newuserdata(L, sizeof *o);	
	luaL_setmetatable(L, "CellObj");
	*o = NULL;
	
	lua_getglobal(L, STATE_DATA_VAR);	
	if(!lua_islightuserdata(L, -1)) {
		/* Don't ever do anything to ___the_map in your Lua scripts */
		luaL_error(L, "Variable %s got tampered with.", STATE_DATA_VAR);
	}
	sd = lua_touserdata(L, -1);
	lua_pop(L, 1);
	
	if(!sd->map) {
		/* This Lua state does not have a map, so return an empty list */
		return 1;		
	}
	
	selector = luaL_checkstring(L,1);
	if(selector[0] == '.') {
		sel_fun = class_selector;
	} else {		
		sel_fun = id_selector;
	}
	
	/* Is there a non-O(n) way to find cells matching a selector? 
	 * I ought to put the ids in a hashtable at least
	 */
	for(i = 0; i < sd->map->nr * sd->map->nc; i++) {
		struct map_cell *c = &sd->map->cells[i];
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
	struct lustate_data *sd = get_state_data(L);
	cell_obj **os = luaL_checkudata(L,1, "CellObj");
	cell_obj *o;
	int l = luaL_checkint(L,2);
	int si = luaL_checkint(L,3);
	int ti = luaL_checkint(L,4);
	
	if(l < 0 || l > 2) {
		lua_pushfstring(L, "Invalid level passed to CellObj.set()");
		lua_error(L);
	}
	
	assert(sd->map); /* cant create a CellObj if this is false. */
	if(si < 0 || si >= ts_get_num(&sd->map->tiles)) {
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

/* Graphics object G *****************************************************************************/

static int gr_setcolor(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	const char *c = luaL_checkstring(L,1);
	bm_set_color_s(sd->bmp, c);
	return 0;
}

static int gr_putpixel(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x = luaL_checkint(L,1);
	int y = luaL_checkint(L,2);
	bm_putpixel(sd->bmp, x, y);
	return 0;
}

static int gr_line(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	bm_line(sd->bmp, x0, y0, x1, y1);
	return 0;
}

static int gr_rect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	bm_rect(sd->bmp, x0, y0, x1, y1);
	return 0;
}

static int gr_fillrect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	bm_fillrect(sd->bmp, x0, y0, x1, y1);
	return 0;
}

static int gr_circle(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x = luaL_checkint(L,1);
	int y = luaL_checkint(L,2);
	int r = luaL_checkint(L,3);
	bm_circle(sd->bmp, x, y, r);
	return 0;
}

static int gr_fillcircle(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x = luaL_checkint(L,1);
	int y = luaL_checkint(L,2);
	int r = luaL_checkint(L,3);
	bm_fillcircle(sd->bmp, x, y, r);
	return 0;
}

static int gr_ellipse(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	bm_ellipse(sd->bmp, x0, y0, x1, y1);
	return 0;
}

static int gr_roundrect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	int r = luaL_checkint(L,5);
	bm_roundrect(sd->bmp, x0, y0, x1, y1, r);
	return 0;
}

static int gr_fillroundrect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	int r = luaL_checkint(L,5);
	bm_fillroundrect(sd->bmp, x0, y0, x1, y1, r);
	return 0;
}

static int gr_bezier3(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	int x2 = luaL_checkint(L,5);
	int y2 = luaL_checkint(L,6);
	bm_bezier3(sd->bmp, x0, y0, x1, y1, x2, y2);
	return 0;
}

static int gr_lerp(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	const char *c1 = luaL_checkstring(L,1);
	const char *c2 = luaL_checkstring(L,2);
	lua_Number v = luaL_checknumber(L,3);	
	int col = bm_lerp(bm_color_atoi(c1), bm_color_atoi(c2), v);
	bm_set_color_i(sd->bmp, col);
	
	lua_pushinteger(L, col);
	return 1;
}

static const luaL_Reg graphics_funcs[] = {
  {"setcolor",      gr_setcolor},
  {"pixel",         gr_putpixel},
  {"line",          gr_line},
  {"rect",          gr_rect},
  {"fillrect",      gr_fillrect},
  {"circle",        gr_circle},
  {"fillcircle",    gr_fillcircle},
  {"ellipse",    	gr_ellipse},
  {"roundrect",    	gr_roundrect},
  {"fillroundrect", gr_fillroundrect},
  {"curve",         gr_bezier3},
  {"lerp",          gr_lerp},
  {0, 0}
};

/* STATE FUNCTIONS *******************************************************************************/

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
	luaL_openlibs(L);
	
	/* Create and init the State Data that the interpreter carries with it. */
	sd = malloc(sizeof *sd);
	if(!sd)
		return 0;
	
	sd->update_fcn = NULL;	
	sd->n_timeout = 0;
	sd->map = NULL;
	sd->bmp = NULL;
		
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
		
		sd->map = map_parse(map_text);
		if(!sd->map) {
			rerror("Unable to parse map '%s' (state %s).", map_file, s->name);
			return 0;		
		}
		free(map_text);
	} else {
		rlog("Lua state %s does not specify a map file.", s->name);
	}	
	
	rlog("Running script %s", script_file);
	
	/* Register some Lua variables. */
	lua_pushcfunction(L, l_log);
    lua_setglobal(L, "log");
	
	lua_pushcfunction(L, l_set_timeout);
    lua_setglobal(L, "setTimeout");
	
	lua_pushcfunction(L, l_onUpdate);
    lua_setglobal(L, "onUpdate");
	
	lua_pushcfunction(L, l_createParticle);
    lua_setglobal(L, "createParticle");
	
	lua_pushcfunction(L, l_changeState);
    lua_setglobal(L, "changeState");
	
	lua_pushinteger(L, 0);
    lua_setglobal(L, "BACKGROUND");
	lua_pushinteger(L, 1);
    lua_setglobal(L, "CENTER");
	lua_pushinteger(L, 2);
    lua_setglobal(L, "FOREGROUND");
	
	lua_pushinteger(L, fps);
    lua_setglobal(L, "FPS");
		
	luaL_newlib(L, graphics_funcs);
	lua_setglobal(L, "G");
	
	cell_obj_meta(L);
	
	/* Load the Lua script, and execute it. */
	if(luaL_loadstring(L, script)) {		
		rerror("Unable to load script %s (state %s).", script_file, s->name);
		sublog("lua", "%s", lua_tostring(L, -1));
		free(script);				
		return 0;
	}
	free(script);
	
	if(lua_pcall(L, 0, 0, 0)) {
		rerror("Unable to execute script %s (state %s).", script_file, s->name);
		sublog("lua", "%s", lua_tostring(L, -1));
		return 0;
	}
		
	return 1;
}

static int lus_update(struct game_state *s, struct bitmap *bmp) {
	int i;
	struct lustate_data *sd = NULL;
	lua_State *L = s->data;
	struct _update_function *fn;
	
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
	
	sd->bmp = bmp;
	
	lua_pushinteger(L, bmp->w);
    lua_setglobal(L, "SCREEN_WIDTH");
	lua_pushinteger(L, bmp->h);
    lua_setglobal(L, "SCREEN_HEIGHT");
	
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
			sublog("lua", "%s", lua_tostring(L, -1));
			/* Should we remove it maybe? */
		}
		
		fn = fn->next;
	}
	
	if(sd->map) {
		for(i = 0; i < 3; i++)
			map_render(sd->map, bmp, i, 0, 0);
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
			rerror("Variable %s got tampered with (map_deinit)", STATE_DATA_VAR);
		} else {
			struct _update_function *fn;
			
			sd = lua_touserdata(L, -1);
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