#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#ifdef WIN32
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "bmp.h"
#include "states.h"
#include "musl.h"
#include "game.h"
#include "ini.h"
#include "resources.h"
#include "utils.h"
#include "particles.h"
#include "log.h"

/* Structures ********************************************************************************/

struct mu_data {
	struct musl *mu;
	const char *file;
	char *script;
	struct bitmap *bmp;
	struct game_state *state;
};
	
/* Musl Functions ****************************************************************************/

/*@ CLS([color]) 
 *# Clears the screen.
 *# If the color
 */
static struct mu_par mus_cls(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct mu_data *md = mu_get_data(m);
	struct bitmap *bmp = md->bmp;

	if(argc > 0) {
		bm_set_color_s(bmp, mu_par_str(m, 0, argc, argv));
	} else {
		bm_set_color_s(bmp, md->state->style.bg);
	}
	
	bm_clear(bmp);

	return rv;
}

/*@ SHOW() 
 *# Displays the current screen.
 */
static struct mu_par mus_show(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	advanceFrame();
	if(quit) mu_halt(m);
	
	mu_set_int(m, "mouse_x", mouse_x);
	mu_set_int(m, "mouse_y", mouse_y);
	
	return rv;
}

/*@ COLOR(["#rrggbb"]) 
 *# If no color is specified, the "foreground" parameter in the game config is used.
 */
static struct mu_par mus_color(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct mu_data *md = mu_get_data(m);
	struct bitmap *bmp = md->bmp;

	if(argc > 0) {
		const char *text = mu_par_str(m, 0, argc, argv);	
		bm_set_color_s(bmp, text);
	} else {
		bm_set_color_s(bmp, md->state->style.fg);
	}
	
	return rv;
}

/*@ FONT("font") 
 *# Sets the font used for drawing.
 */
static struct mu_par mus_font(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv;
	struct mu_data *md = mu_get_data(m);
	struct bitmap *bmp = md->bmp;
	enum bm_fonts font;
	
	if(argc > 0) {
		const char *name = mu_par_str(m, 0, argc, argv);
		font = bm_font_index(name);
	} else {
		font = md->state->style.font;
	}
	bm_std_font(bmp, font);
	
	rv.type = mu_str;
	rv.v.s = strdup(bm_font_name(font));

	return rv;
}

/*@ GOTOXY(x, y) 
 *# Moves the cursor used for {{PRINT()}} to position X,Y
 *# on the screen.
 */
static struct mu_par mus_gotoxy(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	
	mu_set_int(m, "_px", mu_par_num(m, 0, argc, argv));
	mu_set_int(m, "_py", mu_par_num(m, 1, argc, argv));

	return rv;
}

/*@ PRINT(str...) 
 *# Prints strings at the position specified through GOTOXY(),
 *# then moves the cursor to the next line.
 */
static struct mu_par mus_print(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	int x, y, i, h = 8;
	
	x = mu_get_int(m, "_px");
	y = mu_get_int(m, "_py");
	
	for(i = 0; i < argc; i++) {
		const char *text = mu_par_str(m, i, argc, argv);
		int hh = bm_text_height(bmp, text);
		bm_puts(bmp, x, y, text);
		x += bm_text_width(bmp, text);
		if(hh > h)
			h = hh;
	}	
	mu_set_int(m, "_py", y + h + 1);
	
	return rv;
}

/*@ CENTERTEXT(text)
 *# Draws text centered on the screen.
 */
static struct mu_par mus_centertext(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	const char *text = mu_par_str(m, 0, argc, argv);
	int x, y;
	
	x = (bmp->w - bm_text_width(bmp, text))>>1;
	y = (bmp->h - bm_text_height(bmp, text))>>1;
	
	bm_puts(bmp, x, y, text);
	
	return rv;
}

/*@ KBHIT([key]) 
 *# Checks whether keys have been pressed.
 *# If {{key}} is supplied, that specific key is checked,
 *# otherwise all keys are checked.
 *# It is advisable to call {{KBCLR()}} after a successful
 *# {{KBHIT()}}, otherwise {{KBHIT()}} will keep on firing.
 *# A list of the keycodes are here: http://wiki.libsdl.org/SDL_Keycode
 */
static struct mu_par mus_kbhit(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	if(argc > 0) {
		const char *name = mu_par_str(m, 0, argc, argv);
		rv.v.i = keys[SDL_GetScancodeFromName(name)];
	} else {
		rv.v.i = kb_hit();
	}
	return rv;
}

/*@ KBCLR([key]) 
 *# Clears the keyboard.
 *# If {{key}} is supplied that specific key is cleared,
 *# otherwise all keys are cleared.
 */
static struct mu_par mus_kbclr(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	if(argc > 0) {
		const char *name = mu_par_str(m, 0, argc, argv);
		keys[SDL_GetScancodeFromName(name)] = 0;
	} else {
		reset_keys();
	}
	return rv;
}

/*@ LOG(str...) 
 *# Prints strings to the logfile.
 */
static struct mu_par mus_log(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	int i;
	for(i = 0; i < argc; i++) {
		const char *text = mu_par_str(m, i, argc, argv);
		sublog("Musl", "%s", text);
	}	
	return rv;
}


/*@ CLEAR_PARTICLES() 
 *# Removes al particles
 */
static struct mu_par mus_clr_par(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	clear_particles();
	return rv;
}


/*@ DELAY([millis]) 
 *# Waits for a couple of milliseconds
 */
static struct mu_par mus_delay(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	Uint32 start = SDL_GetTicks();
	rv.v.i = mu_par_num(m, 0, argc, argv);
	do {
		advanceFrame();
		if(quit) {
			mu_halt(m);
			break;
		}
		SDL_Delay(1000/fps);
	} while(SDL_GetTicks() - start < rv.v.i);
	return rv;
}

/*@ PIXEL(x,y) 
 *# Draws a single pixel
 */
static struct mu_par mus_putpixel(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_putpixel(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv));
	return rv;
}

/*@ LINE(x1,y1,x2,y2) 
 *# Draws a line from x1,y1 to x2,y2
 */
static struct mu_par mus_line(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_line(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv),mu_par_num(m, 3, argc, argv));
	return rv;
}

/*@ RECT(x1,y1,x2,y2) 
 *# Draws a rectangle from x1,y1 to x2,y2
 */
static struct mu_par mus_rect(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_rect(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv),mu_par_num(m, 3, argc, argv));
	return rv;
}

/*@ FILLRECT(x1,y1,x2,y2) 
 *# Draws a filled rectangle from x1,y1 to x2,y2
 */
static struct mu_par mus_fillrect(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_fillrect(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv),mu_par_num(m, 3, argc, argv));
	return rv;
}

/*@ CIRCLE(x,y,r) 
 *# Draws a circle centered at x,y of radius r
 */
static struct mu_par mus_circle(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_circle(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv));
	return rv;
}

/*@ FILLCIRCLE(x,y,r) 
 *# Draws a filled circle centered at x,y of radius r
 */
static struct mu_par mus_fillcircle(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_fillcircle(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv));
	return rv;
}

/*@ ELLIPSE(x1,y1,x2,y2) 
 *# Draws a ellipse from x1,y1 to x2,y2
 */
static struct mu_par mus_ellipse(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_ellipse(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv),mu_par_num(m, 3, argc, argv));
	return rv;
}

/*@ ROUNDRECT(x1,y1,x2,y2,r) 
 *# Draws a rectangle from x1,y1 to x2,y2 with rounded corners of radius r
 */
static struct mu_par mus_roundrect(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_roundrect(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv),mu_par_num(m, 3, argc, argv),mu_par_num(m, 4, argc, argv));
	return rv;
}

/*@ FILLROUNDRECT(x1,y1,x2,y2,r) 
 *# Draws a filled rectangle from x1,y1 to x2,y2 with rounded corners of radius r
 */
static struct mu_par mus_fillroundrect(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_fillroundrect(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv),mu_par_num(m, 3, argc, argv),mu_par_num(m, 4, argc, argv));
	return rv;
}

/*@ CURVE(x0,y0,x1,y1,x2,y2) 
 *# Draws a curve from x0,y0 to x2,y2 with x1,y1 as control point
 */
static struct mu_par mus_curve(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	bm_bezier3(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv),mu_par_num(m, 3, argc, argv),
		mu_par_num(m, 4, argc, argv),mu_par_num(m, 5, argc, argv));
	return rv;
}

/*@ LOADBMP(file)
 *# Loads a bitmap into a cache.
 */
static struct mu_par mus_loadbmp(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	const char *filename = mu_par_str(m, 0, argc, argv);
	struct bitmap *bmp = re_get_bmp(filename);
	if(!bmp) {
		mu_throw(m, "Unable to load bitmap '%s'", filename);
	}
	return rv;
}

/*@ DRAW([file])
 *# Draws a bitmap to the screen.\n
 *# This function centers the bitmap on the screen, and is intended for 
 *# things like backgrounds.
 */
static struct mu_par mus_draw(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	const char *filename = mu_par_str(m, 0, argc, argv);
	int dx,dy;
	struct bitmap *src = re_get_bmp(filename);
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	if(!src) {
		mu_throw(m, "Unable to load bitmap '%s'", filename);
	}
	assert(bmp);	
	
	dx = (bmp->w - src->w) >> 1;
	dy = (bmp->h - src->h) >> 1;
	
	bm_blit(bmp, dx, dy, src, 0, 0, src->w, src->h);
	
	return rv;
}

/*@ SETMASK(file, color)
 *# Sets the mask color of the bitmap for subsequent calls to {{BLIT()}}
 */
static struct mu_par mus_setmask(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	const char *filename = mu_par_str(m, 0, argc, argv);
	const char *mask = mu_par_str(m, 1, argc, argv);
	struct bitmap *bmp = re_get_bmp(filename);
	if(!bmp) {
		mu_throw(m, "Unable to load bitmap '%s'", filename);
	}
	bm_set_color_s(bmp, mask);
		
	return rv;
}

/*@ BLIT([file], dx, dy, sx, sy, w, h)
 *# Blits a bitmap identified by {{file}} to the screen at {{dx,dy}}.\n
 *# The color previously set th
 */
static struct mu_par mus_blit(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	const char *filename = mu_par_str(m, 0, argc, argv);
	struct bitmap *src = re_get_bmp(filename);
	struct bitmap *bmp = ((struct mu_data*)mu_get_data(m))->bmp;
	int dx = mu_par_num(m, 1, argc, argv),
		dy = mu_par_num(m, 2, argc, argv),
		sx, sy, w, h;
		
	if(!src) {
		mu_throw(m, "Unable to load bitmap '%s'", filename);
	}
	assert(bmp);
	
	sx = argc > 3 ? mu_par_num(m, 3, argc, argv) : 0;
	sy = argc > 4 ? mu_par_num(m, 4, argc, argv) : 0;
	w = argc > 5 ? mu_par_num(m, 5, argc, argv) : src->w;
	h = argc > 6 ? mu_par_num(m, 6, argc, argv) : src->h;
	
	bm_maskedblit(bmp, dx, dy, src, sx, sy, w, h);
	
	return rv;
}

/* State Functions ***************************************************************************/

static int mus_init(struct game_state *s) {
	struct mu_data *md = malloc(sizeof *md);
	if(!md) 
		return 0;
	md->state = s;
	md->mu = NULL;
	md->script = NULL;
		
	rlog("Initializing Musl state '%s'", s->name);

	reset_keys();
	
	md->file = ini_get(game_ini, s->name, "script", NULL);	
	if(!md->file) {
		rerror("No script specified in Musl state '%s'", s->name);
		return 0;
	}
	
	if(!(md->mu = mu_create())) {
		rerror("Couldn't create Musl interpreter\n");
		return 0;
	}
	
	rlog("Loading Musl script '%s'", md->file);
	
	md->script = re_get_script(md->file);
	if(!md->script) {
		rerror("Couldn't load Musl script '%s'", md->file);
		return 0;
	}
	
	mu_add_func(md->mu, "gotoxy", mus_gotoxy);
	mu_add_func(md->mu, "cls", mus_cls);
	mu_add_func(md->mu, "color", mus_color);
	mu_add_func(md->mu, "font", mus_font);
	mu_add_func(md->mu, "print", mus_print);
	mu_add_func(md->mu, "centertext", mus_centertext);
	
	mu_add_func(md->mu, "kbhit", mus_kbhit);
	mu_add_func(md->mu, "kbclr", mus_kbclr);
	mu_add_func(md->mu, "show", mus_show);
	mu_add_func(md->mu, "log", mus_log);
	mu_add_func(md->mu, "delay", mus_delay);
	mu_add_func(md->mu, "clear_particles", mus_clr_par);
	
	mu_add_func(md->mu, "pixel", mus_putpixel);
	mu_add_func(md->mu, "line", mus_line);	
	mu_add_func(md->mu, "rect", mus_rect);	
	mu_add_func(md->mu, "fillrect", mus_fillrect);	
	mu_add_func(md->mu, "circle", mus_circle);	
	mu_add_func(md->mu, "fillcircle", mus_fillcircle);
	mu_add_func(md->mu, "ellipse", mus_ellipse);
	mu_add_func(md->mu, "roundrect", mus_roundrect);
	mu_add_func(md->mu, "fillroundrect", mus_fillroundrect);
	mu_add_func(md->mu, "curve", mus_curve);	
	
	mu_add_func(md->mu, "loadbmp", mus_loadbmp);
	mu_add_func(md->mu, "draw", mus_draw);
	mu_add_func(md->mu, "setmask", mus_setmask);
	mu_add_func(md->mu, "blit", mus_blit);		
		
	mu_set_int(md->mu, "mouse_x", mouse_x);
	mu_set_int(md->mu, "mouse_y", mouse_y);
	
	mu_set_int(md->mu, "_px", 0);
	mu_set_int(md->mu, "_py", 0);
	
	s->data = md;
	
	return 1;
}

static int mus_update(struct game_state *s, struct bitmap *bmp) {
	const char *next_state;
	struct mu_data *md = s->data;
	
	if(!md->mu || !md->script) {
		rerror("Unable to run Musl script because of earlier problems\n");
		change_state(NULL);
		return 0;
	}
	
	md->bmp = bmp;
	mu_set_data(md->mu, md);
	
	mu_set_int(md->mu, "width", bm_width(bmp));
	mu_set_int(md->mu, "height", bm_height(bmp));

	if(!mu_run(md->mu, md->script)) {
		rerror("%s:Line %d: %s:\n>> %s\n", md->file, mu_cur_line(md->mu), mu_error_msg(md->mu),
				mu_error_text(md->mu));
		change_state(NULL);
		return 1;
	}
	
	next_state = mu_get_str(md->mu, "nextstate");
	if(!next_state) {
		rwarn("Musl script didn't specify a nextstate; terminating...");
		change_state(NULL);
	} else {	
		set_state(next_state);
	}

	return 1;
}

static int mus_deinit(struct game_state *s) {
	struct mu_data *md = s->data;
	if(md->mu)
		mu_cleanup(md->mu);
	if(md->script)
		free(md->script);
	free(md);
	return 1;
}

struct game_state *get_mus_state(const char *name) {
	struct game_state *state = malloc(sizeof *state);
	if(!state)
		return NULL;
	state->name = name;
	
	state->init = mus_init;
	state->update = mus_update;
	state->deinit = mus_deinit;
	
	return state;
}