#include <stdio.h>
#include <stdlib.h>

#ifdef WIN32
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "ini.h"
#include "bmp.h"
#include "states.h"
#include "utils.h"
#include "game.h"
#include "particles.h"
#include "resources.h"

/* Globals *******************************************/

struct game_state *current_state;

static struct game_state *get_state(const char *name);

static struct {
	const char *fg;
	const char *bg;
	
	int margin;
	int padding;
	
	int btn_padding;
	int btn_border_radius;
	
	int border;
	int border_radius;
	const char *border_color;
	
	enum bm_fonts font;
	
	struct bitmap *bmp;
	const char *image_align;
	const char *image_trans;
	int image_margin;
	
} style;

static void apply_styles(const char *state) {	
	const char *font, *image;
	
	/* This could use a lot less code. See mustate.c
		On the otherhand, mustate.c does some unnecessary lookups,
		so there's that.
	*/
	style.bg = ini_get(game_ini, state, "background", NULL);	
	if(!style.bg)
		style.bg = ini_get(game_ini, "styles", "background", "black");
	
	style.fg = ini_get(game_ini, state, "foreground", NULL);
	if(!style.fg)
		style.fg = ini_get(game_ini, "styles", "foreground", "white");
	
	style.margin = atoi(ini_get(game_ini, state, "margin", "-1"));
	if(style.margin < 0)
		style.margin = atoi(ini_get(game_ini, "styles", "margin", "1"));
	
	if(style.margin < 0) style.margin = 0;
	
	style.padding = atoi(ini_get(game_ini, state, "padding", "-1"));
	if(style.padding < 0)
		style.padding = atoi(ini_get(game_ini, "styles", "padding", "1"));
	
	if(style.padding < 0) style.padding = 0;
	
	style.border = atoi(ini_get(game_ini, state, "border", "-1"));
	if(style.border < 0)
		style.border = atoi(ini_get(game_ini, "styles", "border", "0"));
	
	if(style.border < 0) style.border = 0;
	
	style.border_radius = atoi(ini_get(game_ini, state, "border-radius", "-1"));
	if(style.border_radius < 0)
		style.border_radius = atoi(ini_get(game_ini, "styles", "border-radius", "0"));
	
	if(style.border_radius < 0) style.border_radius = 0;
	
	style.border_color = ini_get(game_ini, state, "border-color", NULL);
	if(!style.border_color) {
		style.border_color = ini_get(game_ini, "styles", "border-color", style.fg);
	}
		
	style.btn_padding = atoi(ini_get(game_ini, state, "button-padding", "-1"));
	if(style.btn_padding < 0)
		style.btn_padding = atoi(ini_get(game_ini, "styles", "button-padding", "5"));
	
	if(style.btn_padding < 1) style.btn_padding = 1;
	
	style.btn_border_radius = atoi(ini_get(game_ini, state, "button-border-radius", "-1"));
	if(style.btn_border_radius < 0)
		style.btn_border_radius = atoi(ini_get(game_ini, "styles", "button-border-radius", "1"));
	
	if(style.btn_border_radius < 1) style.btn_border_radius = 1;
		
	font = ini_get(game_ini, state, "font", NULL);
	if(!font)
		font = ini_get(game_ini, "styles", "font", "normal");
	
	if(!my_stricmp(font, "bold")) {
		style.font = BM_FONT_BOLD;
	} else if(!my_stricmp(font, "circuit")) {
		style.font = BM_FONT_CIRCUIT;
	} else if(!my_stricmp(font, "hand")) {
		style.font = BM_FONT_HAND;
	} else if(!my_stricmp(font, "small")) {
		style.font = BM_FONT_SMALL;
	} else if(!my_stricmp(font, "smallinv")) {
		style.font = BM_FONT_SMALL_I;
	} else if(!my_stricmp(font, "thick")) {
		style.font = BM_FONT_THICK;
	} else {
		style.font = BM_FONT_NORMAL;
	}
	
	image = ini_get(game_ini, state, "image", NULL);
	if(image) {
		style.bmp = re_get_bmp(image);
		if(!style.bmp) {
			fprintf(log_file, "error: Unable to load '%s'\n", image);
		} else {		
			style.image_align = ini_get(game_ini, state, "image-align", "top");
			style.image_trans = ini_get(game_ini, state, "image-mask", NULL);
			
			style.image_margin = atoi(ini_get(game_ini, state, "image-margin", "0"));
			if(style.image_margin < 0)
				style.image_margin = 0;
		}
	} else {
		style.bmp = NULL;
	}
}

static void draw_border(struct bitmap *bmp) {
	bm_set_color_s(bmp, style.border_color);
	if(style.border > 0) {
		if(style.border > 1) {
			if(style.border_radius > 0) {
				bm_fillroundrect(bmp, style.margin, style.margin, bmp->w - 1 - style.margin, bmp->h - 1 - style.margin, style.border_radius);
			} else {
				bm_fillrect(bmp, style.margin, style.margin, bmp->w - 1 - style.margin, bmp->h - 1 - style.margin);
			}			
			bm_set_color_s(bmp, style.bg);
			if(style.border_radius > 0) {
				bm_fillroundrect(bmp, style.margin + style.border, style.margin + style.border, 
					bmp->w - 1 - (style.margin + style.border), bmp->h - 1 - (style.margin + style.border), 
					style.border_radius - (style.border >> 1));
			} else {
				bm_fillrect(bmp, style.margin + style.border, style.margin + style.border, 
					bmp->w - 1 - (style.margin + style.border), bmp->h - 1 - (style.margin + style.border));
			}
		} else {
			if(style.border_radius > 0) {
				bm_roundrect(bmp, style.margin, style.margin, bmp->w - 1 - style.margin, bmp->h - 1 - style.margin, style.border_radius);
			} else {
				bm_rect(bmp, style.margin, style.margin, bmp->w - 1 - style.margin, bmp->h - 1 - style.margin);
			}
		}
	}
}

/* State Functions ***********************************/

static void basic_state(struct game_state *s, struct bitmap *bmp) {
	int tw, th;
	int w, h;
	int tx, ty;
	int ix = 0, iy = 0;
	
	const char *text = ini_get(game_ini, s->name, "text", "?");
	const char *halign = ini_get(game_ini, s->name, "halign", "center");
	const char *valign = ini_get(game_ini, s->name, "valign", "center");
	
	bm_set_color_s(bmp, style.bg);
	bm_clear(bmp);
	
	draw_border(bmp);
	
	bm_std_font(bmp, style.font);
	
	w = tw = bm_text_width(bmp, text);
	h = th = bm_text_height(bmp, text);
	
	if(style.bmp) { 
		if(!my_stricmp(style.image_align, "left") || !my_stricmp(style.image_align, "right")) {
			w += style.bmp->w + (style.image_margin << 1);
			h = MY_MAX(style.bmp->h + style.image_margin, th);
		} else {
			h += style.bmp->h + (style.image_margin << 1);
			w = MY_MAX(style.bmp->w + style.image_margin, tw);
		}
	}
	
	if(!my_stricmp(halign, "left")) {
		tx = (style.margin + style.border + style.padding);
	} else if(!my_stricmp(halign, "right")) {
		tx = bmp->w - 1 - w - (style.margin + style.border + style.padding);
	} else {		
		tx = (bmp->w - w) >> 1;
	}
	
	if(!my_stricmp(valign, "top")) {
		ty = (style.margin + style.border + style.padding);
	} else if(!my_stricmp(valign, "bottom")) {
		ty = bmp->h - 1 - h - (style.margin + style.border + style.padding);
	} else { 
		ty = (bmp->h - h) >> 1;
	}
	
	if(style.bmp) { 
		if(!my_stricmp(style.image_align, "left")) {
			ix = tx;
			tx += style.bmp->w + style.image_margin;
			iy = ty;
		} else if(!my_stricmp(style.image_align, "right")) {
			ix = tx + tw + style.image_margin;
			iy = ty;
		} else if(!my_stricmp(style.image_align, "bottom")) {
			iy = ty + th + style.image_margin;
			ix = tx;
		} else {
			/* "top" */
			iy = ty;
			ty += style.bmp->h + style.image_margin;
			ix = tx;
		}
		
		if(!my_stricmp(style.image_align, "left") || !my_stricmp(style.image_align, "right")) {
			if(th > style.bmp->h) {
				iy += (th - style.bmp->h) >> 1;
			} else {
				ty += (style.bmp->h - th) >> 1;
			}
		} else {
			if(tw > style.bmp->w) {
				ix += (tw - style.bmp->w) >> 1;
			} else {
				tx += (style.bmp->w - tw) >> 1;
			}
		}
	
		if(style.image_trans) {
			bm_set_color_s(style.bmp, style.image_trans);
			bm_maskedblit(bmp, ix, iy, style.bmp, 0, 0, style.bmp->w, style.bmp->h);
		} else {
			bm_blit(bmp, ix, iy, style.bmp, 0, 0, style.bmp->w, style.bmp->h);
		}
	}

	bm_set_color_s(bmp, style.fg);	
	bm_puts(bmp, tx, ty, text);	
}

static int basic_init(struct game_state *s) {	
	apply_styles(s->name);
	reset_keys();
	clear_particles();
	return 1;
}

static int basic_deinit(struct game_state *s) {
	return 1;
}

/* Static State **************************************/

static int static_update(struct game_state *s, struct bitmap *bmp) {
	
	basic_state(s, bmp);
	
	if(kb_hit() || mouse_clck & SDL_BUTTON(1)) {
		const char *nextstate = ini_get(game_ini, s->name, "nextstate", NULL);
		struct game_state *next;
		
		if(!nextstate)
			change_state(NULL);
		
		next = get_state(nextstate);
		if(!next) {
			fprintf(log_file, "error: State '%s' does not exist\n", nextstate);
		}
		
		change_state(next);
	}		
	
	return 1;
}

static int static_init(struct game_state *s) {	
	return basic_init(s);
}

static int static_deinit(struct game_state *s) {
	return basic_deinit(s);
}

static struct game_state *get_static_state(const char *name) {
	struct game_state *state = malloc(sizeof *state);
	if(!state)
		return NULL;
	state->name = name;
	
	state->init = static_init;
	state->update = static_update;
	state->deinit = static_deinit;
	
	return state;
}

/* Left-Right State **********************************/

struct left_right {
	int dir; /* left=0; right=1 */
};

static void draw_button(struct bitmap *bmp, int x, int y, int w, int h, int pad_left, int pad_top, const char *label, int highlight) {
	bm_set_color_s(bmp, style.fg);
	if(highlight) {
		if(style.btn_border_radius > 1) 
			bm_fillroundrect(bmp, x, y, x + w, y + h, style.btn_border_radius);
		else
			bm_fillrect(bmp, x, y, x + w, y + h);
		bm_set_color_s(bmp, style.bg);
	} else {
		if(style.btn_border_radius > 1) 
			bm_roundrect(bmp, x, y, x + w, y + h, style.btn_border_radius);
		else
			bm_rect(bmp, x, y, x + w, y + h);
	}
	bm_puts(bmp, x + pad_left, y + pad_top, label);
}

static int leftright_update(struct game_state *s, struct bitmap *bmp) {	
	int bw1, bh1, bw2, bh2, bw, bh;
	int bx1, by1, bx2, by2;
	int k;

	int clicked = 0;
	
	const char *left_label = ini_get(game_ini, s->name, "left-label", "Left");
	const char *right_label = ini_get(game_ini, s->name, "right-label", "Right");
	
	struct left_right *lr = s->data;
	
	basic_state(s, bmp);
	
	/* FIXME: styles to position the buttons better */
	bw1 = bm_text_width(bmp, left_label);
	bh1 = bm_text_height(bmp, left_label);

	bw2 = bm_text_width(bmp, right_label);
	bh2 = bm_text_height(bmp, right_label);

	bw = MY_MAX(bw1, bw2) + (style.btn_padding << 1);
	bh = MY_MAX(bh1, bh2) + (style.btn_padding << 1);

	/* Left button */
	bx1 = (style.margin + style.border + style.padding);
	by1 = bmp->h - 1 - bh - (style.margin + style.border + style.padding);	

	/* Right button */
	bx2 = bmp->w - 1 - bw - (style.margin + style.border + style.padding);
	by2 = bmp->h - 1 - bh - (style.margin + style.border + style.padding);
	
	/* Cursor over  */
	if(mouse_x >= bx1 && mouse_x <= bx1 + bw && mouse_y >= by1 && mouse_y <= by1 + bh) {
		clicked = mouse_clck & SDL_BUTTON(1);
		lr->dir = 0;		
	} else if(mouse_x >= bx2 && mouse_x <= bx2 + bw && mouse_y >= by2 && mouse_y <= by2 + bh) {
		clicked = mouse_clck & SDL_BUTTON(1);
		lr->dir = 1;
	} 
	
	if((k = kb_hit()) || clicked) {
		if(k == SDL_SCANCODE_SPACE || k == SDL_SCANCODE_RETURN || clicked) {				
			if(!lr->dir) {
				const char *nextstate = ini_get(game_ini, s->name, "left-state", NULL);
				struct game_state *next;
				
				if(!nextstate)
					change_state(NULL);
				
				next = get_state(nextstate);
				if(!next) {
					fprintf(log_file, "error: State '%s' does not exist\n", nextstate);
				}
				
				change_state(next);
			} else {
				const char *nextstate = ini_get(game_ini, s->name, "right-state", NULL);
				struct game_state *next;
				
				if(!nextstate)
					change_state(NULL);
				
				next = get_state(nextstate);
				if(!next) {
					fprintf(log_file, "error: State '%s' does not exist\n", nextstate);
				}
				
				change_state(next);				
			}
			return 1;
		}
		
		if(k == SDL_SCANCODE_LEFT || k == SDL_SCANCODE_UP) {
			lr->dir = 0;
		} else if(k == SDL_SCANCODE_RIGHT || k == SDL_SCANCODE_DOWN) {
			lr->dir = 1;
		}
	} 
		
	/* Draw the buttons */	
	draw_button(bmp, bx1, by1, bw, bh, ((bw-bw1)>>1), ((bh-bh1)>>1), left_label, !lr->dir);

	draw_button(bmp, bx2, by2, bw, bh, ((bw-bw2)>>1), ((bh-bh2)>>1), right_label, lr->dir);
	
	return 1;
}

static int leftright_init(struct game_state *s) {
	struct left_right *lr = malloc(sizeof *lr);
	if(!lr)
		return 0;
	lr->dir = 0;
	s->data = lr;
	return basic_init(s);
}

static int leftright_deinit(struct game_state *s) {
	free(s->data);
	return basic_deinit(s);
}

static struct game_state *get_leftright_state(const char *name) {
	struct game_state *state = malloc(sizeof *state);
	if(!state)
		return NULL;
	state->name = name;
	
	state->init = leftright_init;
	state->update = leftright_update;
	state->deinit = leftright_deinit;
	
	return state;
}

/* Functions *****************************************/

int change_state(struct game_state *next) {
	if(current_state && current_state->deinit) {
		if(!current_state->deinit(current_state)) {
			fprintf(log_file, "error: Deinitialising new state\n");
			return 0;
		}
		free(current_state);
	}	
	current_state = next;	
	if(current_state && current_state->init) {
		if(!current_state->init(current_state)) {
			fprintf(log_file, "error: Initialising new state\n");
			return 0;
		}
	}
	fflush(log_file);
	return 1;
}

static struct game_state *get_state(const char *name) {
	const char *type;
	
	struct game_state *next = NULL;
	
	if(!name) return NULL;
	
	if(!ini_has_section(game_ini, name)) {
		fprintf(log_file, "error: No state %s in ini file\n", name);
		quit = 1;
		return NULL;
	}
	
	type = ini_get(game_ini, name, "type", NULL);
	if(!type) {
		fprintf(log_file, "error: No state type for state '%s' in ini file\n", name);
		quit = 1;
		return NULL;
	}
	
	if(!my_stricmp(type, "static")) {
		next = get_static_state(name);
	} else if(!my_stricmp(type, "map")) {
		next = get_map_state(name);
	} else if(!my_stricmp(type, "leftright")) {
		next = get_leftright_state(name);
	} else if(!my_stricmp(type, "musl")) {
		next = get_mus_state(name);
	} else {
		fprintf(log_file, "error: Invalid type '%s' for state '%s' in ini file\n", type, name);
		quit = 1;
		return NULL;
	}
	if(!next) {
		fprintf(log_file, "error: Memory allocation error obtaining state %s\n", name);
		quit = 1;
	}
	return next;
}

int set_state(const char *name) {
	struct game_state *next;
	fprintf(log_file, "info: Changing to state '%s'\n", name);
	next = get_state(name);	
	return change_state(next);
}
