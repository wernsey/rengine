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
#include <assert.h>

#include "tileset.h"
#include "map.h"
#include "luastate.h"

/*1 Map
 *# The Map object provides access to the Map through
 *# a variety of functions.
 *#
 *# These fields are also available:
 *{
 ** {{Map.BACKGROUND}} - Constant for the Map's background layer. See {{Map.render()}}
 ** {{Map.CENTER}} - Constant for the Map's center layer. See {{Map.render()}}
 ** {{Map.FOREGROUND}} - Constant for the Map's foreground layer. See {{Map.render()}}
 ** {{Map.ROWS}} - The number of rows in the map.
 ** {{Map.COLS}} - The number of columns in the map.
 ** {{Map.TILE_WIDTH}} - The width (in pixels) of the cells on the map.
 ** {{Map.TILE_HEIGHT}} - The height (in pixels) of the cells on the map.
 *}
 *# The {{Map}} object is only available if the {{map}}
 *# parameter has been set in the state's configuration
 *# in the {{game.ini}} file.
 */

/*@ Map.render(layer, [scroll_x, scroll_y])
 *# Renders the specified layer of the map (background, center or foreground).
 *# The center layer is where all the action in the game occurs and sprites and so on moves around.
 *# The background is drawn behind the center layer for objects in the background. It needs to be drawn first,
 *# so that the center and foreground layers are drawn over it.
 *# The foreground layer is drawn last and contains objects in the foreground. 
 */
static int render_map(lua_State *L) {	
	int layer = luaL_checknumber(L,1) - 1;
	
	int sx = 0, sy = 0;
	struct lustate_data *sd = get_state_data(L);
	
	if(!sd->map) {
		luaL_error(L, "Attempt to render non-existent Map");
	} 
	if(layer < 0 || layer >= 3) {
		luaL_error(L, "Attempt to render non-existent Map layer %d", layer);
	}
	if(!sd->bmp) {
		luaL_error(L, "Attempt to render Map outside of a screen update");	
	}
	
	if(lua_gettop(L) > 2) {
		sx = luaL_checknumber(L,2);
		sy = luaL_checknumber(L,3);
	}
	
	map_render(sd->map, sd->bmp, layer, sx, sy);
	return 0;
}

/*@ Map.cell(r,c)
 *# Returns a cell on the map at row {{r}}, column {{c}} as 
 *# a {{CellObj}} instance.\n
 *# {{r}} must be between {{1}} and {{Map.ROWS}} inclusive.
 *# {{c}} must be between {{1}} and {{Map.COLS}} inclusive.
 */
static int get_cell_obj(lua_State *L) {
	int r = luaL_checknumber(L,1) - 1;
	int c = luaL_checknumber(L,2) - 1;	
	struct lustate_data *sd = get_state_data(L);	
	struct map_cell **o;
	
	assert(sd->map);
	
	if(c < 0 || c >= sd->map->nc) 
		luaL_error(L, "Invalid r value in Map.cell()");
	if(r < 0 || r >= sd->map->nr) 
		luaL_error(L, "Invalid c value in Map.cell()");
	
	o = lua_newuserdata(L, sizeof *o);	
	luaL_setmetatable(L, "CellObj");
		
	*o = map_get_cell(sd->map, c, r);
	
	return 1;
}

static const luaL_Reg map_funcs[] = {
  {"render",      	render_map},
  {"cell",      	get_cell_obj},
  {0, 0}
};

/*1 CellObj
 *# The CellObj encapsulates an individual cell on the 
 *# map. You then use the methods of the CellObj to manipulate
 *# the cell.
 *#
 *# Cell Objects are obtained through the {{Map.cell(r,c)}} function.
 */

/*@ CellObj:__tostring()
 *# Returns a string representation of the CellObj
 */
static int cell_tostring(lua_State *L) {	
	luaL_checkudata(L,1, "CellObj");
	lua_pushstring(L, "CellObj");
	return 1;
}

/*@ CellObj:__gc()
 *# Frees the CellObj when it is garbage collected.
 */
static int gc_cell_obj(lua_State *L) {
	/* Nothing to do, since we don't actually own the pointer to the map_cell */
	return 0;
}

/*@ CellObj:set(layer, si, ti)
 *# Sets the specific layer of the cells encapsulated in the CellObj to 
 *# the {{si,ti}} value, where
 *{
 ** {{si}} is the Set Index
 ** {{ti}} is the Tile Index
 *}
 */
static int cell_set(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	struct map_cell **cp = luaL_checkudata(L,1, "CellObj");
	struct map_cell *c = *cp;
	
	int l = luaL_checknumber(L,2) - 1;
	int si = luaL_checknumber(L,3);
	int ti = luaL_checknumber(L,4);
	
	if(l < 0 || l > 2) {
		luaL_error(L, "Invalid level passed to CellObj:set()");
	}
	
	assert(sd->map); /* cant create a CellObj if this is false. */
	if(si < 0 || si >= ts_get_num(&sd->map->tiles)) {
		luaL_error(L, "Invalid si passed to CellObj:set()");
	}
	/* FIXME: error checking on ti? */
	
	/* TODO: Maybe you ought to store this change in some sort of list
	   so that the savedgames can handle changes to the map like this. */
	c->tiles[l].ti = ti;
	c->tiles[l].si = si;
	
	/* Push the CellObj back onto the stack so that other methods can be called on it */
	lua_pushvalue(L, -4);
	
	return 1;
}

/*@ CellObj:getId()
 *# Returns the {/id/} of a cell as defined in the Rengine Editor
 */
static int cell_get_id(lua_State *L) {
	struct map_cell **cp = luaL_checkudata(L,1, "CellObj");
	struct map_cell *c = *cp;	
	lua_pushstring(L, c->id ? c->id : "");
	return 1;
}

/*@ CellObj:getClass()
 *# Returns the {/class/} of a cell as defined in the Rengine Editor
 */
static int cell_get_class(lua_State *L) {
	struct map_cell **cp = luaL_checkudata(L,1, "CellObj");
	struct map_cell *c = *cp;	
	lua_pushstring(L, c->clas ? c->clas : "");
	return 1;
}

/*@ CellObj:isBarrier()
 *# Returns whether the cell is a barrier
 */
static int cell_is_barrier(lua_State *L) {
	struct map_cell **cp = luaL_checkudata(L,1, "CellObj");
	struct map_cell *c = *cp;	
	lua_pushboolean(L, c->flags & TS_FLAG_BARRIER);
	return 1;
}

/*@ CellObj:setBarrier(b)
 *# Sets whether the cell is a barrier
 */
static int cell_set_barrier(lua_State *L) {
	struct map_cell **cp = luaL_checkudata(L,1, "CellObj");
	struct map_cell *c = *cp;	
	luaL_checktype(L, 2, LUA_TBOOLEAN);
	int b = lua_toboolean(L, 2);
	if(b)
		c->flags |= TS_FLAG_BARRIER;
	else
		c->flags &= ~TS_FLAG_BARRIER;
	return 0;
}

/*
	I may have been doing this wrong. See http://lua-users.org/wiki/UserDataWithPointerExample
*/
static void cell_obj_meta(lua_State *L) {
	/* Create the metatable for MyObj */
	luaL_newmetatable(L, "CellObj");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index"); /* CellObj.__index = CellObj */
	
	/* Add methods here */
	lua_pushcfunction(L, cell_set);
	lua_setfield(L, -2, "set");
	lua_pushcfunction(L, cell_get_id);
	lua_setfield(L, -2, "getId");
	lua_pushcfunction(L, cell_get_class);
	lua_setfield(L, -2, "getClass");
	lua_pushcfunction(L, cell_is_barrier);
	lua_setfield(L, -2, "isBarrier");
	lua_pushcfunction(L, cell_set_barrier);
	lua_setfield(L, -2, "setBarrier");
	
	lua_pushcfunction(L, cell_tostring);
	lua_setfield(L, -2, "__tostring");	
	lua_pushcfunction(L, gc_cell_obj);
	lua_setfield(L, -2, "__gc");	
	
	/* The global method C() */
	lua_pushcfunction(L, get_cell_obj);
	lua_setglobal(L, "Cell");
}

void register_map_functions(lua_State *L) {
    
    struct lustate_data * sd = get_state_data(L);
    
    luaL_newlib(L, map_funcs);
    SET_TABLE_INT_VAL("BACKGROUND", 1);
    SET_TABLE_INT_VAL("CENTER", 2);
    SET_TABLE_INT_VAL("FOREGROUND", 3);
    SET_TABLE_INT_VAL("ROWS", sd->map->nr);
    SET_TABLE_INT_VAL("COLS", sd->map->nc);
    SET_TABLE_INT_VAL("TILE_WIDTH", sd->map->tiles.tw);
    SET_TABLE_INT_VAL("TILE_HEIGHT", sd->map->tiles.th);     
    
	/* There is a function C('selector') that returns a CellObj object that
		gives you access to the cells on the map. */
	cell_obj_meta(L);
}