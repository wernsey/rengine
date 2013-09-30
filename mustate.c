#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"
#include "states.h"
#include "musl.h"
#include "game.h"
#include "ini.h"
#include "resources.h"
#include "utils.h"

/* Globals ***********************************************************************************/

static struct musl *mu = NULL;

static const char *mu_script_file = "";
static char *mu_script = NULL;

static int mu_x = 0, mu_y = 0;

/* Musl Functions ****************************************************************************/

/*@ GOTOXY(x, y) 
 *# 
 */
static struct mu_par mus_gotoxy(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	
	mu_x = mu_par_num(m, 0);
	mu_y = mu_par_num(m, 1);

	return rv;
}

/*@ CLS([color]) 
 *# 
 */
static struct mu_par mus_cls(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = mu_get_data(mu);

	if(argc > 0) {
		const char *text = mu_par_str(m, 0);
		bm_set_color_s(bmp, text);
	}
	
	bm_clear(bmp);

	return rv;
}

/*@ COLOR("#rrggbb") 
 *# 
 */
static struct mu_par mus_color(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = mu_get_data(mu);
	const char *text = mu_par_str(m, 0);
	
	bm_set_color_s(bmp, text);

	return rv;
}

/*@ FONT("font") 
 *# 
 */
static struct mu_par mus_font(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	struct bitmap *bmp = mu_get_data(mu);
	const char *name = mu_par_str(m, 0);
	
	enum bm_fonts font;
	
	if(!my_stricmp(name, "bold")) {
		font = BM_FONT_BOLD;
	} else if(!my_stricmp(name, "circuit")) {
		font = BM_FONT_CIRCUIT;
	} else if(!my_stricmp(name, "hand")) {
		font = BM_FONT_HAND;
	} else if(!my_stricmp(name, "small")) {
		font = BM_FONT_SMALL;
	} else if(!my_stricmp(name, "smallinv")) {
		font = BM_FONT_SMALL_I;
	} else if(!my_stricmp(name, "thick")) {
		font = BM_FONT_THICK;
	} else {
		font = BM_FONT_NORMAL;
	}	
	bm_std_font(bmp, font);

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
		const char *text = mu_par_str(m, i);
		int hh = bm_text_height(bmp, text);
		bm_puts(bmp, x, mu_y, text);
		x += bm_text_width(bmp, text);
		if(hh > h)
			h = hh;
	}
	
	mu_y += h + 1;
	
	return rv;
}

/*@ KBHIT() 
 *# 
 */
static struct mu_par mus_kbhit(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	rv.v.i = kb_hit();
	return rv;
}

/*@ KBCLEAR() 
 *# 
 */
static struct mu_par mus_kbclear(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	reset_keys();
	return rv;
}

/*@ SHOW() 
 *# 
 */
static struct mu_par mus_show(struct musl *m, int argc, struct mu_par argv[]) {
	struct mu_par rv = {mu_int, {0}};
	advanceFrame();
	if(quit) mu_halt(m);
	
	mu_set_num(mu, "mouse_x", mouse_x);
	mu_set_num(mu, "mouse_y", mouse_y);
	
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
		const char *text = mu_par_str(m, i);
		fputs(text, log_file);
	}	
	fputs("\n", log_file);
	fflush(log_file);
	
	return rv;
}

/* State Functions ***************************************************************************/

static int mus_init(struct game_state *s) {
	
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
	mu_add_func(mu, "kbclear", mus_kbclear);
	mu_add_func(mu, "show", mus_show);
	mu_add_func(mu, "log", mus_log);
	
	mu_set_num(mu, "mouse_x", mouse_x);
	mu_set_num(mu, "mouse_y", mouse_y);
	
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
	
	mu_set_num(mu, "width", bm_width(bmp));
	mu_set_num(mu, "height", bm_height(bmp));

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