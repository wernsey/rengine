#include <stdio.h>
#include <stdlib.h>

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
#include "mappings.h"
#include "particles.h"

/* Globals ***********************************************************************************/

static struct musl *mu = NULL;

static const char *mu_script_file = "";
static char *mu_script = NULL;

static int mu_x = 0, mu_y = 0;

/* Musl Functions ****************************************************************************/

/*@ CLS([color]) 
 *# Clears the screen.
 *# If the color
 */
static struct mu_par mus_cls(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = mu_get_data(mu);

	if(argc > 0) {
		bm_set_color_s(bmp, mu_par_str(m, 0, argc, argv));
	} else {
		bm_set_color_s(bmp, mu_get_str(mu, "background"));
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
	
	mu_set_int(mu, "mouse_x", mouse_x);
	mu_set_int(mu, "mouse_y", mouse_y);
	
	return rv;
}

/*@ COLOR(["#rrggbb"]) 
 *# If no color is specified, the "foreground" parameter in the game config is used.
 */
static struct mu_par mus_color(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = mu_get_data(mu);

	if(argc > 0) {
		const char *text = mu_par_str(m, 0, argc, argv);	
		bm_set_color_s(bmp, text);
	} else {
		bm_set_color_s(bmp, mu_get_str(mu, "foreground"));
	}
	
	return rv;
}

/*@ FONT("font") 
 *# Sets the font used for drawing.
 */
static struct mu_par mus_font(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = mu_get_data(mu);
	const char *name;

	if(argc > 0) {
		name = mu_par_str(m, 0, argc, argv);
	} else {
		name = mu_get_str(mu, "font");
	}
	
	enum bm_fonts font = font_index(name);

	return rv;
}

/*@ GOTOXY(x, y) 
 *# Moves the cursor used for {{PRINT()}} to position X,Y
 *# on the screen.
 */
static struct mu_par mus_gotoxy(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	
	mu_x = mu_par_num(m, 0, argc, argv);
	mu_y = mu_par_num(m, 1, argc, argv);

	return rv;
}

/*@ PRINT(str...) 
 *# Prints strings at the position specified through GOTOXY(),
 *# then moves the cursor to the next line.
 */
static struct mu_par mus_print(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = mu_get_data(mu);
	int x = mu_x, i, h = 8;
	
	for(i = 0; i < argc; i++) {
		const char *text = mu_par_str(m, i, argc, argv);
		int hh = bm_text_height(bmp, text);
		bm_puts(bmp, x, mu_y, text);
		x += bm_text_width(bmp, text);
		if(hh > h)
			h = hh;
	}
	
	mu_y += h + 1;
	
	return rv;
}

/*@ KBHIT([key]) 
 *# Checks whether keys have been pressed.
 *# If {{key}} is supplied, that specific key is checked,
 *# otherwise all keys are checked.
 *# It is advisable to call {{KBCLR()}} after a successful
 *# {{KBHIT()}}, otherwise {{KBHIT()}} will keep on firing.
 */
static struct mu_par mus_kbhit(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	if(argc > 0) {
		const char *name = mu_par_str(m, 0, argc, argv);
		rv.v.i = keys[key_index(name)];
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
		keys[key_index(name)] = 0;
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
	fputs("Musl: ", log_file);	
	for(i = 0; i < argc; i++) {
		const char *text = mu_par_str(m, i, argc, argv);
		fputs(text, log_file);
	}	
	fputs("\n", log_file);
	fflush(log_file);
	
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
	struct bitmap *bmp = mu_get_data(mu);
	bm_putpixel(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv));
	return rv;
}

/*@ LINE(x1,y1,x2,y2) 
 *# Draws a line from x1,y1 to x2,y2
 */
static struct mu_par mus_line(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = mu_get_data(mu);
	bm_line(bmp, mu_par_num(m, 0, argc, argv), mu_par_num(m, 1, argc, argv),
		mu_par_num(m, 2, argc, argv),mu_par_num(m, 3, argc, argv));
	return rv;
}

/* State Functions ***************************************************************************/

static int mus_init(struct game_state *s) {
	
	int tmp;
	
	fprintf(log_file, "info: Initializing Musl state '%s'\n", s->data);

	reset_keys();
	
	mu_script_file = ini_get(game_ini, s->data, "script", NULL);	
	if(!mu_script_file) {
		fprintf(log_file, "error: No script specified in Musl state '%s'\n", s->data);
		fflush(log_file);
		return 0;
	}
	
	if(!(mu = mu_create())) {
		fprintf(log_file, "error: Couldn't create Musl interpreter\n");
		fflush(log_file);
		return 0;
	}
	
	fprintf(log_file, "info: Loading Musl script '%s'\n", mu_script_file);
	
	mu_script = re_get_script(mu_script_file);
	if(!mu_script) {
		fprintf(log_file, "error: Couldn't load Musl script '%s'\n", mu_script_file);
		fflush(log_file);
		return 0;
	}
	
	mu_add_func(mu, "gotoxy", mus_gotoxy);
	mu_add_func(mu, "cls", mus_cls);
	mu_add_func(mu, "color", mus_color);
	mu_add_func(mu, "font", mus_font);
	mu_add_func(mu, "print", mus_print);
	mu_add_func(mu, "kbhit", mus_kbhit);
	mu_add_func(mu, "kbclr", mus_kbclr);
	mu_add_func(mu, "show", mus_show);
	mu_add_func(mu, "log", mus_log);
	mu_add_func(mu, "delay", mus_delay);
	mu_add_func(mu, "clear_particles", mus_clr_par);
	
	mu_add_func(mu, "pixel", mus_putpixel);
	mu_add_func(mu, "line", mus_line);	
	
	mu_set_int(mu, "mouse_x", mouse_x);
	mu_set_int(mu, "mouse_y", mouse_y);
	
	/* Some variables from the ini file */
	
	mu_set_str(mu, "background", ini_get(game_ini, s->data, "background", ini_get(game_ini, "styles", "background", "black")));
	mu_set_str(mu, "foreground", ini_get(game_ini, s->data, "foreground", ini_get(game_ini, "styles", "foreground", "white")));
	
	tmp = atoi(ini_get(game_ini, s->data, "margin", ini_get(game_ini, "styles", "margin", "1")));	
	if(tmp < 0) 
		tmp = 0;
	mu_set_int(mu, "margin", tmp);
	
	tmp = atoi(ini_get(game_ini, s->data, "padding", ini_get(game_ini, "styles", "padding", "1")));	
	if(tmp < 0) 
		tmp = 0;
	mu_set_int(mu, "padding", tmp);
	
	tmp = atoi(ini_get(game_ini, s->data, "border", ini_get(game_ini, "styles", "border", "1")));	
	if(tmp < 0) 
		tmp = 0;
	mu_set_int(mu, "border", tmp);
	
	tmp = atoi(ini_get(game_ini, s->data, "border-radius", ini_get(game_ini, "styles", "border-radius", "0")));	
	if(tmp < 0) 
		tmp = 0;
	mu_set_int(mu, "border_radius", tmp);
	
	mu_set_str(mu, "border_color", ini_get(game_ini, s->data, "border-color", ini_get(game_ini, "styles", "border-color", "white")));
	
	tmp = atoi(ini_get(game_ini, s->data, "button-padding", ini_get(game_ini, "styles", "button-padding", "5")));	
	if(tmp < 0) 
		tmp = 0;
	mu_set_int(mu, "button_padding", tmp);
	
	tmp = atoi(ini_get(game_ini, s->data, "button-border-radius", ini_get(game_ini, "styles", "button-border-radius", "1")));	
	if(tmp < 0) 
		tmp = 0;
	mu_set_int(mu, "button_border_radius", tmp);
	
	mu_set_str(mu, "font", ini_get(game_ini, s->data, "font", ini_get(game_ini, "styles", "font", "normal")));
	
	return 1;
}

static int mus_update(struct game_state *s, struct bitmap *bmp) {
	const char *next_state;
	
	if(!mu || !mu_script) {
		fprintf(log_file, "error: Unable to run Musl script because of earlier problems\n");
		fflush(log_file);
		change_state(NULL);
		return 0;
	}
	
	mu_set_data(mu, bmp);
	
	mu_set_int(mu, "width", bm_width(bmp));
	mu_set_int(mu, "height", bm_height(bmp));

	if(!mu_run(mu, mu_script)) {
		fprintf(log_file, "error: %s:Line %d: %s:\n>> %s\n", mu_script_file, mu_cur_line(mu), mu_error_msg(mu),
				mu_error_text(mu));
		fflush(log_file);
		change_state(NULL);
		return 1;
	}
	
	next_state = mu_get_str(mu, "nextstate");
	if(!next_state) {
		fprintf(log_file, "error: Musl script didn't specify a nextstate\n");
		fflush(log_file);
		change_state(NULL);
	} else {	
		set_state(next_state);
	}

	return 1;
}

static int mus_deinit(struct game_state *s) {
	if(mu)
		mu_cleanup(mu);
	if(mu_script)
		free(mu_script);
	mu_script = NULL;
	return 1;
}

struct game_state mus_state = {
	NULL,
	mus_init,
	mus_update,
	mus_deinit
};