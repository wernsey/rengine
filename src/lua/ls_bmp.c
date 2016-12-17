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
#include "resources.h"
#include "luastate.h"

/*1 BmpObj
 *# The Bitmap object encapsulates a bitmap in the engine.
 *# Instances of BmpObj are drawn to the screen with the [[G.blit()|Lua-state#gblitbmp-dx-dy-sx-sy-w-h]] function
 */

/*@ Bmp(filename)
 *# Loads the bitmap file specified by {{filename}} from the
 *# [[Resources|Resource Management]] and returns it
 *# encapsulated within a `BmpObj` instance.
 */
static int new_bmp_obj(lua_State *L) {
	const char *filename = luaL_checkstring(L,1);
	
	struct bitmap **bp = lua_newuserdata(L, sizeof *bp);	
	luaL_setmetatable(L, "BmpObj");
	
	*bp = re_get_bmp(filename);
	if(!*bp) {
		luaL_error(L, "Unable to load bitmap '%s'", filename);
	}
	return 1;
}

/*@ BmpObj:__tostring()
 *# Returns a string representation of the `BmpObj` instance.
 */
static int bmp_tostring(lua_State *L) {	
	struct bitmap **bp = luaL_checkudata(L,1, "BmpObj");
	struct bitmap *b = *bp;
	lua_pushfstring(L, "BmpObj[%dx%d]", b->w, b->h);
	return 1;
}

/*@ BmpObj:__gc()
 *# Garbage collects the `BmpObj` instance.
 */
static int gc_bmp_obj(lua_State *L) {
	/* No need to free the bitmap: It's in the resource cache. */
	return 0;
}

/*@ BmpObj:clone()
 *# Returns a copy of the `BmpObj` instance.
 */
static int bmp_clone(lua_State *L) {	
	struct bitmap **bp = luaL_checkudata(L,1, "BmpObj");
	struct bitmap *b = *bp;
	char buffer[32];
	static int nextnum = 1;
	snprintf(buffer, sizeof buffer, "clone%d", nextnum++);
	struct bitmap *clone = re_clone_bmp(b, buffer);
	if(!clone)
		luaL_error(L, "Unable to clone bitmap");
	bp = lua_newuserdata(L, sizeof *bp);	
	luaL_setmetatable(L, "BmpObj");
	*bp = clone;
	return 1;
}

/*@ BmpObj:setMask(color)
 *# Sets the color used as a mask when the bitmap is drawn to the screen.
 */
static int bmp_set_mask(lua_State *L) {	
	struct bitmap **bp = luaL_checkudata(L,1, "BmpObj");
	const char *mask = luaL_checkstring(L, 2);
	bm_set_color_s(*bp, mask);
	return 0;
}

/*@ BmpObj:width()
 *# Returns the width of the bitmap
 */
static int bmp_width(lua_State *L) {	
	struct bitmap **bp = luaL_checkudata(L,1, "BmpObj");
	lua_pushinteger(L, (*bp)->w);
	return 1;
}

/*@ BmpObj:height()
 *# Returns the height of the bitmap
 */
static int bmp_height(lua_State *L) {	
	struct bitmap **bp = luaL_checkudata(L,1, "BmpObj");
	lua_pushinteger(L, (*bp)->h);
	return 1;
}

/*@ BmpObj:adjust(R,G,B, [A]), BmpObj:adjust(v)  
 *# Adjusts the color of the bitmap by multiplying the RGB value of each pixel 
 *# in the bitmap with the factors R, G, B and A.\n
 *# If A is not specified, it is taken as 1.0 (leaving the alpha channel unchanged).\n
 *# If only one parameter {{v}} is specified, {{v}} is used for the R, G and B values.
 *# In this case, the alpha channel is also treated as 1.0.
 */
static int bmp_adjust(lua_State *L) {	
	struct bitmap **bp = luaL_checkudata(L,1, "BmpObj");
	if(lua_gettop(L) == 2) {
		float v = luaL_checknumber(L,2);
		bm_adjust_rgba(*bp, v, v, v, 1.0f);
	} else if(lua_gettop(L) >= 4) {
		float R, G, B, A = 1.0;
		R = luaL_checknumber(L,2);
		G = luaL_checknumber(L,3);
		B = luaL_checknumber(L,4);
		if(lua_gettop(L) > 4)
			A = luaL_checknumber(L,5);
		bm_adjust_rgba(*bp, R, G, B, A);
	}
	return 0;
}

/*@ R,G,B = BmpObj:getColor([x,y])
 *# Gets the [[color|Colors]] used as mask by the bitmap.\n
 *# If the {{x,y}} parameters are supplied, the color of the
 *# pixel at {{x,y}} is returned.
 */
static int bmp_getcolor(lua_State *L) {
	int r,g,b;
	struct bitmap **bp = luaL_checkudata(L,1, "BmpObj");
	if(lua_gettop(L) == 3) {
		int x = luaL_checknumber(L,2);
		int y = luaL_checknumber(L,3);
		if(x < 0) x = 0;
		if(x >= (*bp)->w) x = (*bp)->w - 1;
		if(y < 0) y = 0;
		if(y >= (*bp)->h) y = (*bp)->h - 1;		
		bm_picker(*bp, x, y);
	}
	bm_get_color_rgb(*bp, &r, &g, &b);
	lua_pushinteger(L, r);
	lua_pushinteger(L, g);
	lua_pushinteger(L, b);
	return 3;
}

static void bmp_obj_meta(lua_State *L) {
	/* Create the metatable for MyObj */
	luaL_newmetatable(L, "BmpObj");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index"); /* BmpObj.__index = BmpObj */
	
	/* Add methods */
	lua_pushcfunction(L, bmp_clone);
	lua_setfield(L, -2, "clone");
	lua_pushcfunction(L, bmp_set_mask);
	lua_setfield(L, -2, "setMask");
	lua_pushcfunction(L, bmp_width);
	lua_setfield(L, -2, "width");
	lua_pushcfunction(L, bmp_height);
	lua_setfield(L, -2, "height");
	lua_pushcfunction(L, bmp_getcolor);
	lua_setfield(L, -2, "getColor");
	lua_pushcfunction(L, bmp_adjust);
	lua_setfield(L, -2, "adjust");
	
	lua_pushcfunction(L, bmp_tostring);
	lua_setfield(L, -2, "__tostring");	
	lua_pushcfunction(L, gc_bmp_obj);
	lua_setfield(L, -2, "__gc");	
	
	/* The global method Bmp() */
	lua_pushcfunction(L, new_bmp_obj);
	lua_setglobal(L, "Bmp");
}

void register_bmp_functions(lua_State *L) {
    bmp_obj_meta(L);
}
