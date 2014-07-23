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
		int x = luaL_checkint(L,2);
		int y = luaL_checkint(L,3);
		if(x < 0) x = 0;
		if(x >= (*bp)->w) x = (*bp)->w - 1;
		if(y < 0) y = 0;
		if(y >= (*bp)->h) y = (*bp)->h - 1;		
		bm_picker(*bp, x, y);
	}
	bm_get_color(*bp, &r, &g, &b);
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

void register_map_functions(lua_State *L);

/*1 G
 *# {{G}} is the Graphics object that allows you to draw primitives on the screen. \n
 *# 
 *# These fields are also available:
 *{
 ** {{G.FPS}} - The configured frames per second of the game (see [[game.ini]]).
 ** {{G.SCREEN_WIDTH}} - The configured width of the screen.
 ** {{G.SCREEN_HEIGHT}} - The configured height of the screen.
 ** {{G.frameCounter}} - A counter that gets incremented on every frame. It may be useful for certain kinds of animations.
 *}
 */

/*@ G.setColor("color"), G.setColor(R, G, B), G.setColor()
 *# Sets the [[color|Colors]] used to draw the graphics primitives\n
 *# {{G.setColor("color")}} sets the color to the specified string value.\n
 *# {{G.setColor(R, G, B)}} sets the color to the specified R, G, B values.
 *# R,G,B values must be in the range [0..255].\n
 *# {{G.setColor()}} sets the color to the foreground specified in the styles.
 */
static int gr_setcolor(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);	
	if(lua_gettop(L) == 3) {
		int R = luaL_checkinteger(L,1);
		int G = luaL_checkinteger(L,2);
		int B = luaL_checkinteger(L,3);		
		bm_set_color(sd->bmp, R, G, B);
	} else if(lua_gettop(L) == 1) {
		const char *c = luaL_checkstring(L,1);
		bm_set_color_s(sd->bmp, c);
	} else if(lua_gettop(L) == 0) {
		bm_set_color_s(sd->bmp, get_style(sd->state, "foreground"));
	} else
		luaL_error(L, "Invalid parameters to G.setColor()");	
	return 0;
}

/*@ R,G,B = G.getColor()
 *# Gets the [[color|Colors]] used to draw the graphics primitives.\n
 */
static int gr_getcolor(lua_State *L) {
	int r,g,b;
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	bm_get_color(sd->bmp, &r, &g, &b);
	lua_pushinteger(L, r);
	lua_pushinteger(L, g);
	lua_pushinteger(L, b);
	return 3;
}

/*@ G.clip(x0,y0, x1,y1)
 *# Sets the clipping rectangle when drawing primitives.
 */
static int gr_clip(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);	
	int x0 = luaL_checkinteger(L,1);
	int y0 = luaL_checkinteger(L,2);
	int x1 = luaL_checkinteger(L,3);
	int y1 = luaL_checkinteger(L,4);
	bm_clip(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ x0,y0, x1,y1 = G.getClip()
 *# Gets the current clipping rectangle for drawing primitives.
 */
static int gr_getclip(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
    lua_pushinteger(L, sd->bmp->clip.x0);
    lua_pushinteger(L, sd->bmp->clip.y0);
    lua_pushinteger(L, sd->bmp->clip.x1);
    lua_pushinteger(L, sd->bmp->clip.y1);
	return 4;
}

/*@ G.unclip()
 *# Resets the clipping rectangle when drawing primitives.
 */
static int gr_unclip(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	bm_unclip(sd->bmp);
	return 0;
}
/*@ G.pixel(x,y)
 *# Plots a pixel at {{x,y}} on the screen.
 */
static int gr_putpixel(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x = luaL_checkint(L,1);
	int y = luaL_checkint(L,2);
	bm_putpixel(sd->bmp, x, y);
	return 0;
}

/*@ G.line(x0, y0, x1, y1)
 *# Draws a line from {{x0,y0}} to {{x1,y1}}
 */
static int gr_line(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	bm_line(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ G.rect(x0, y0, x1, y1)
 *# Draws a rectangle from {{x0,y0}} to {{x1,y1}}
 */
static int gr_rect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	bm_rect(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ G.fillRect(x0, y0, x1, y1)
 *# Draws a filled rectangle from {{x0,y0}} to {{x1,y1}}
 */
static int gr_fillrect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	bm_fillrect(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ G.dithRect(x0, y0, x1, y1)
 *# Draws a dithered rectangle from {{x0,y0}} to {{x1,y1}}
 */
static int gr_dithrect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	bm_dithrect(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ G.circle(x, y, r)
 *# Draws a circle centered at {{x,y}} with radius {{r}}
 */
static int gr_circle(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x = luaL_checkint(L,1);
	int y = luaL_checkint(L,2);
	int r = luaL_checkint(L,3);
	bm_circle(sd->bmp, x, y, r);
	return 0;
}

/*@ G.fillCircle(x, y, r)
 *# Draws a filled circle centered at {{x,y}} with radius {{r}}
 */
static int gr_fillcircle(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x = luaL_checkint(L,1);
	int y = luaL_checkint(L,2);
	int r = luaL_checkint(L,3);
	bm_fillcircle(sd->bmp, x, y, r);
	return 0;
}

/*@ G.ellipse(x0, y0, x1, y1)
 *# Draws an ellipse from {{x0,y0}} to {{x1,y1}}
 */
static int gr_ellipse(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	bm_ellipse(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ G.roundRect(x0, y0, x1, y1, r)
 *# Draws a rectangle from {{x0,y0}} to {{x1,y1}}
 *# with rounded corners of radius {{r}}
 */
static int gr_roundrect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	int r = luaL_checkint(L,5);
	bm_roundrect(sd->bmp, x0, y0, x1, y1, r);
	return 0;
}

/*@ G.fillRoundRect(x0, y0, x1, y1, r)
 *# Draws a rectangle from {{x0,y0}} to {{x1,y1}}
 *# with rounded corners of radius {{r}}
 */
static int gr_fillroundrect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	int r = luaL_checkint(L,5);
	bm_fillroundrect(sd->bmp, x0, y0, x1, y1, r);
	return 0;
}

/*@ G.curve(x0, y0, x1, y1, x2, y2)
 *# Draws a Bezier curve from {{x0,y0}} to {{x2,y2}}
 *# with {{x1,y1}} as the control point.
 *# Note that it doesn't pass through {{x1,y1}}
 */
static int gr_bezier3(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkint(L,1);
	int y0 = luaL_checkint(L,2);
	int x1 = luaL_checkint(L,3);
	int y1 = luaL_checkint(L,4);
	int x2 = luaL_checkint(L,5);
	int y2 = luaL_checkint(L,6);
	bm_bezier3(sd->bmp, x0, y0, x1, y1, x2, y2);
	return 0;
}

/*@ G.lerp(col1, col2, frac)
 *# Returns a [[color|Colors]] that is some fraction {{frac}} along 
 *# the line from {{col1}} to {{col2}}.
 *# For example, `G.lerp("red", "blue", 0.33)` will return a color
 *# that is 1/3rd of the way from red to blue.
 */
static int gr_lerp(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	const char *c1 = luaL_checkstring(L,1);
	const char *c2 = luaL_checkstring(L,2);
	lua_Number v = luaL_checknumber(L,3);	
	int col = bm_lerp(bm_color_atoi(c1), bm_color_atoi(c2), v);
	bm_set_color_i(sd->bmp, col);
	
	lua_pushinteger(L, col);
	return 1;
}

/*@ G.setFont(font)
 *# Sets the [[font|Fonts]] used for the {{G.print()}} function. 
 */
static int gr_setfont(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	enum bm_fonts font;	
	if(lua_gettop(L) > 0) {
		const char *name = luaL_checkstring(L,1);
		font = bm_font_index(name);
	} else {
		font = bm_font_index(get_style(sd->state, "font"));
	}
	bm_std_font(sd->bmp, font);	
	return 0;
}

/*@ G.print(x,y,text)
 *# Prints the {{text}} to the screen, with its top left position at {{x,y}}.
 */
static int gr_print(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x = luaL_checkinteger(L, 1);
	int y = luaL_checkinteger(L, 2);
	const char *s = luaL_checkstring(L, 3);
	
	bm_puts(sd->bmp, x, y, s);
	
	return 0;
}

/*@ G.textDims(text)
 *# Returns the width,height in pixels that the {{text}}
 *# will occupy on the screen.
 *X local w,h = G.textDims(message);
 */
static int gr_textdims(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	const char *s = luaL_checkstring(L, 1);
	
	lua_pushinteger(L, bm_text_width(sd->bmp, s));
	lua_pushinteger(L, bm_text_height(sd->bmp, s));
	
	return 2;
}

/*@ G.blit(bmp, dx, dy, [sx, sy, [dw, dh, [sw, sh]]])
 *# Draws an instance {{bmp}} of {{BmpObj}} to the screen at {{dx, dy}}.\n
 *# {{sx,sy}} specify the source x,y position and {{dw,dh}} specifies the
 *# width and height of the destination area to draw.\n
 *# {{sx,sy}} defaults to {{0,0}} and {{dw,dh}} defaults to the entire 
 *# source bitmap.\n
 *# If {{sw,sh}} is specified, the bitmap is scaled so that the area on the 
 *# source bitmap from {{sx,sy}} with dimensions {{sw,sh}} is drawn onto the
 *# screen at {{dx,dy}} with dimensions {{dw, dh}}.
 */
static int gr_blit(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	struct bitmap **bp = luaL_checkudata(L, 1, "BmpObj");
	
	int dx = luaL_checkinteger(L, 2);
	int dy = luaL_checkinteger(L, 3);
	
	int sx = 0, sy = 0, w = (*bp)->w, h = (*bp)->h;
	
	if(lua_gettop(L) > 4) {
		sx = luaL_checkinteger(L, 4);
		sy = luaL_checkinteger(L, 5);
	}
	if(lua_gettop(L) > 6) {
		w = luaL_checkinteger(L, 6);
		h = luaL_checkinteger(L, 7);
	}
	if(lua_gettop(L) > 8) {
		int sw = luaL_checkinteger(L, 8);
		int sh = luaL_checkinteger(L, 9);
		bm_blit_ex(sd->bmp, dx, dy, w, h, *bp, sx, sy, sw, sh, 1);
	} else {
		bm_maskedblit(sd->bmp, dx, dy, *bp, sx, sy, w, h);
	}
	
	return 0;
}

static const luaL_Reg graphics_funcs[] = {
  {"setColor",      gr_setcolor},
  {"getColor",      gr_getcolor},
  {"clip", 			gr_clip},
  {"getClip", 		gr_getclip},
  {"unclip", 		gr_unclip},
  {"pixel",         gr_putpixel},
  {"line",          gr_line},
  {"rect",          gr_rect},
  {"fillRect",      gr_fillrect},
  {"dithRect",      gr_dithrect},
  {"circle",        gr_circle},
  {"fillCircle",    gr_fillcircle},
  {"ellipse",    	gr_ellipse},
  {"roundRect",    	gr_roundrect},
  {"fillRoundRect", gr_fillroundrect},
  {"curve",         gr_bezier3},
  {"lerp",          gr_lerp},
  {"print",         gr_print},
  {"setFont",       gr_setfont},
  {"textDims",      gr_textdims},
  {"blit",          gr_blit},
  {0, 0}
};

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
		string[0] = scancode_to_ascii(scancode, shift, caps);
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

/* Declared in src/lua/ls_audio.c */
void register_sound_functions(lua_State *L);

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

/* STATE FUNCTIONS */

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
	luaL_newlib(L, graphics_funcs);
	SET_TABLE_INT_VAL("FPS", fps);
	SET_TABLE_INT_VAL("SCREEN_WIDTH", virt_width);
	SET_TABLE_INT_VAL("SCREEN_HEIGHT", virt_height);
	SET_TABLE_INT_VAL("frameCounter", frame_counter);
	lua_setglobal(L, "G");
    
	/* The Bitmap object is constructed through the Bmp() function that loads 
		a bitmap through the resources module that can be drawn with G.blit() */
	bmp_obj_meta(L);
	
	/* The input object Input gives you access to the keyboard and mouse. */
	luaL_newlib(L, keyboard_funcs);
	lua_setglobal(L, "Keyboard");
	
	luaL_newlib(L, mouse_funcs);
	SET_TABLE_INT_VAL("LEFT", 1);
	SET_TABLE_INT_VAL("MIDDLE", 2);
	SET_TABLE_INT_VAL("RIGHT", 3);
	lua_setglobal(L, "Mouse");
	
	luaL_newlib(L, gdb_funcs);
	lua_setglobal(L, "GameDB");	
	luaL_newlib(L, gdb_local_funcs);
	lua_setglobal(L, "LocalDB");
	
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
