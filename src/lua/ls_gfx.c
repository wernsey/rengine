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

#include "bmp.h"
#include "game.h"
#include "luastate.h"
#include "states.h"

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
		bm_set_color_rgb(sd->bmp, R, G, B);
	} else if(lua_gettop(L) == 1) {
		const char *c = luaL_checkstring(L,1);
		bm_set_color_s(sd->bmp, c);
	} else if(lua_gettop(L) == 0) {
		bm_set_color_s(sd->bmp, get_style(sd->state, "foreground", "white"));
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
	bm_get_color_rgb(sd->bmp, &r, &g, &b);
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
	int x = luaL_checkinteger(L,1);
	int y = luaL_checkinteger(L,2);
	bm_putpixel(sd->bmp, x, y);
	return 0;
}

/*@ G.line(x0, y0, x1, y1)
 *# Draws a line from {{x0,y0}} to {{x1,y1}}
 */
static int gr_line(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkinteger(L,1);
	int y0 = luaL_checkinteger(L,2);
	int x1 = luaL_checkinteger(L,3);
	int y1 = luaL_checkinteger(L,4);
	bm_line(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ G.rect(x0, y0, x1, y1)
 *# Draws a rectangle from {{x0,y0}} to {{x1,y1}}
 */
static int gr_rect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkinteger(L,1);
	int y0 = luaL_checkinteger(L,2);
	int x1 = luaL_checkinteger(L,3);
	int y1 = luaL_checkinteger(L,4);
	bm_rect(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ G.fillRect(x0, y0, x1, y1)
 *# Draws a filled rectangle from {{x0,y0}} to {{x1,y1}}
 */
static int gr_fillrect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkinteger(L,1);
	int y0 = luaL_checkinteger(L,2);
	int x1 = luaL_checkinteger(L,3);
	int y1 = luaL_checkinteger(L,4);
	bm_fillrect(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ G.dithRect(x0, y0, x1, y1)
 *# Draws a dithered rectangle from {{x0,y0}} to {{x1,y1}}
 */
static int gr_dithrect(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkinteger(L,1);
	int y0 = luaL_checkinteger(L,2);
	int x1 = luaL_checkinteger(L,3);
	int y1 = luaL_checkinteger(L,4);
	bm_dithrect(sd->bmp, x0, y0, x1, y1);
	return 0;
}

/*@ G.circle(x, y, r)
 *# Draws a circle centered at {{x,y}} with radius {{r}}
 */
static int gr_circle(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x = luaL_checkinteger(L,1);
	int y = luaL_checkinteger(L,2);
	int r = luaL_checkinteger(L,3);
	bm_circle(sd->bmp, x, y, r);
	return 0;
}

/*@ G.fillCircle(x, y, r)
 *# Draws a filled circle centered at {{x,y}} with radius {{r}}
 */
static int gr_fillcircle(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x = luaL_checkinteger(L,1);
	int y = luaL_checkinteger(L,2);
	int r = luaL_checkinteger(L,3);
	bm_fillcircle(sd->bmp, x, y, r);
	return 0;
}

/*@ G.ellipse(x0, y0, x1, y1)
 *# Draws an ellipse from {{x0,y0}} to {{x1,y1}}
 */
static int gr_ellipse(lua_State *L) {
	struct lustate_data *sd = get_state_data(L);
	assert(sd->bmp);
	int x0 = luaL_checkinteger(L,1);
	int y0 = luaL_checkinteger(L,2);
	int x1 = luaL_checkinteger(L,3);
	int y1 = luaL_checkinteger(L,4);
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
	int x0 = luaL_checkinteger(L,1);
	int y0 = luaL_checkinteger(L,2);
	int x1 = luaL_checkinteger(L,3);
	int y1 = luaL_checkinteger(L,4);
	int r = luaL_checkinteger(L,5);
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
	int x0 = luaL_checkinteger(L,1);
	int y0 = luaL_checkinteger(L,2);
	int x1 = luaL_checkinteger(L,3);
	int y1 = luaL_checkinteger(L,4);
	int r = luaL_checkinteger(L,5);
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
	int x0 = luaL_checkinteger(L,1);
	int y0 = luaL_checkinteger(L,2);
	int x1 = luaL_checkinteger(L,3);
	int y1 = luaL_checkinteger(L,4);
	int x2 = luaL_checkinteger(L,5);
	int y2 = luaL_checkinteger(L,6);
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
	unsigned int col = bm_lerp(bm_color_atoi(c1), bm_color_atoi(c2), v);
	bm_set_color(sd->bmp, col);
	
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
		font = bm_font_index(get_style(sd->state, "font", "normal"));
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

void register_gfx_functions(lua_State *L) {
    luaL_newlib(L, graphics_funcs);
	SET_TABLE_INT_VAL("FPS", fps);
	SET_TABLE_INT_VAL("SCREEN_WIDTH", virt_width);
	SET_TABLE_INT_VAL("SCREEN_HEIGHT", virt_height);
	SET_TABLE_INT_VAL("frameCounter", frame_counter);
	lua_setglobal(L, "G");
}