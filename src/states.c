#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

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
#include "log.h"

/* Globals *******************************************/

#define MAX_NESTED_STATES 20

static struct game_state *game_states[MAX_NESTED_STATES];
static int state_top = 0;

/*****************************************************/

static struct game_state *get_state(const char *name);

/* Styles ********************************************/

static void apply_styles(struct game_state *s) {	
	const char *font, *image;
	
	/* This could use a lot less code. See mustate.c
		On the otherhand, mustate.c does some unnecessary lookups,
		so there's that.
	*/
	s->style.bg = ini_get(game_ini, s->name, "background", NULL);	
	if(!s->style.bg)
		s->style.bg = ini_get(game_ini, "styles", "background", "black");
	
	s->style.fg = ini_get(game_ini, s->name, "foreground", NULL);
	if(!s->style.fg)
		s->style.fg = ini_get(game_ini, "styles", "foreground", "white");
	
	s->style.margin = atoi(ini_get(game_ini, s->name, "margin", "-1"));
	if(s->style.margin < 0)
		s->style.margin = atoi(ini_get(game_ini, "styles", "margin", "1"));
	
	if(s->style.margin < 0) s->style.margin = 0;
	
	s->style.padding = atoi(ini_get(game_ini, s->name, "padding", "-1"));
	if(s->style.padding < 0)
		s->style.padding = atoi(ini_get(game_ini, "styles", "padding", "1"));
	
	if(s->style.padding < 0) s->style.padding = 0;
	
	s->style.border = atoi(ini_get(game_ini, s->name, "border", "-1"));
	if(s->style.border < 0)
		s->style.border = atoi(ini_get(game_ini, "styles", "border", "0"));
	
	if(s->style.border < 0) s->style.border = 0;
	
	s->style.border_radius = atoi(ini_get(game_ini, s->name, "border-radius", "-1"));
	if(s->style.border_radius < 0)
		s->style.border_radius = atoi(ini_get(game_ini, "styles", "border-radius", "0"));
	
	if(s->style.border_radius < 0) s->style.border_radius = 0;
	
	s->style.border_color = ini_get(game_ini, s->name, "border-color", NULL);
	if(!s->style.border_color) {
		s->style.border_color = ini_get(game_ini, "styles", "border-color", s->style.fg);
	}
	
	s->style.btn_padding = atoi(ini_get(game_ini, s->name, "button-padding", "-1"));
	if(s->style.btn_padding < 0)
		s->style.btn_padding = atoi(ini_get(game_ini, "styles", "button-padding", "5"));
	
	if(s->style.btn_padding < 1) s->style.btn_padding = 1;
	
	s->style.btn_border_radius = atoi(ini_get(game_ini, s->name, "button-border-radius", "-1"));
	if(s->style.btn_border_radius < 0)
		s->style.btn_border_radius = atoi(ini_get(game_ini, "styles", "button-border-radius", "1"));
	
	if(s->style.btn_border_radius < 1) s->style.btn_border_radius = 1;
		
	font = ini_get(game_ini, s->name, "font", NULL);
	if(!font)
		font = ini_get(game_ini, "styles", "font", "normal");
	
	if(!my_stricmp(font, "bold")) {
		s->style.font = BM_FONT_BOLD;
	} else if(!my_stricmp(font, "circuit")) {
		s->style.font = BM_FONT_CIRCUIT;
	} else if(!my_stricmp(font, "hand")) {
		s->style.font = BM_FONT_HAND;
	} else if(!my_stricmp(font, "small")) {
		s->style.font = BM_FONT_SMALL;
	} else if(!my_stricmp(font, "smallinv")) {
		s->style.font = BM_FONT_SMALL_I;
	} else if(!my_stricmp(font, "thick")) {
		s->style.font = BM_FONT_THICK;
	} else {
		s->style.font = BM_FONT_NORMAL;
	}
	
	image = ini_get(game_ini, s->name, "image", NULL);
	if(image) {
		s->style.bmp = re_get_bmp(image);
		if(!s->style.bmp) {
			rerror("Unable to load '%s'", image);
		} else {		
			s->style.image_align = ini_get(game_ini, s->name, "image-align", "top");
			s->style.image_trans = ini_get(game_ini, s->name, "image-mask", NULL);
			
			s->style.image_margin = atoi(ini_get(game_ini, s->name, "image-margin", "0"));
			if(s->style.image_margin < 0)
				s->style.image_margin = 0;
		}
	} else {
		s->style.bmp = NULL;
	}
}

static void draw_border(struct game_state *s, struct bitmap *bmp) {
	if(s->style.border > 0) {
		bm_set_color_s(bmp, s->style.border_color);
		if(s->style.border > 1) {
			if(s->style.border_radius > 0) {
				bm_fillroundrect(bmp, s->style.margin, s->style.margin, bmp->w - 1 - s->style.margin, bmp->h - 1 - s->style.margin, s->style.border_radius);
			} else {
				bm_fillrect(bmp, s->style.margin, s->style.margin, bmp->w - 1 - s->style.margin, bmp->h - 1 - s->style.margin);
			}			
			bm_set_color_s(bmp, s->style.bg);
			if(s->style.border_radius > 0) {
				bm_fillroundrect(bmp, s->style.margin + s->style.border, s->style.margin + s->style.border, 
					bmp->w - 1 - (s->style.margin + s->style.border), bmp->h - 1 - (s->style.margin + s->style.border), 
					s->style.border_radius - (s->style.border >> 1));
			} else {
				bm_fillrect(bmp, s->style.margin + s->style.border, s->style.margin + s->style.border, 
					bmp->w - 1 - (s->style.margin + s->style.border), bmp->h - 1 - (s->style.margin + s->style.border));
			}
		} else {
			if(s->style.border_radius > 0) {
				bm_roundrect(bmp, s->style.margin, s->style.margin, bmp->w - 1 - s->style.margin, bmp->h - 1 - s->style.margin, s->style.border_radius);
			} else {
				bm_rect(bmp, s->style.margin, s->style.margin, bmp->w - 1 - s->style.margin, bmp->h - 1 - s->style.margin);
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
	
	bm_set_color_s(bmp, s->style.bg);
	bm_clear(bmp);
	
	draw_border(s, bmp);
	
	bm_std_font(bmp, s->style.font);
	
	w = tw = bm_text_width(bmp, text);
	h = th = bm_text_height(bmp, text);
	
	if(s->style.bmp) { 
		if(!my_stricmp(s->style.image_align, "left") || !my_stricmp(s->style.image_align, "right")) {
			w += s->style.bmp->w + (s->style.image_margin << 1);
			h = MY_MAX(s->style.bmp->h + s->style.image_margin, th);
		} else {
			h += s->style.bmp->h + (s->style.image_margin << 1);
			w = MY_MAX(s->style.bmp->w + s->style.image_margin, tw);
		}
	}
	
	if(!my_stricmp(halign, "left")) {
		tx = (s->style.margin + s->style.border + s->style.padding);
	} else if(!my_stricmp(halign, "right")) {
		tx = bmp->w - 1 - w - (s->style.margin + s->style.border + s->style.padding);
	} else {		
		tx = (bmp->w - w) >> 1;
	}
	
	if(!my_stricmp(valign, "top")) {
		ty = (s->style.margin + s->style.border + s->style.padding);
	} else if(!my_stricmp(valign, "bottom")) {
		ty = bmp->h - 1 - h - (s->style.margin + s->style.border + s->style.padding);
	} else { 
		ty = (bmp->h - h) >> 1;
	}
	
	if(s->style.bmp) { 
		if(!my_stricmp(s->style.image_align, "left")) {
			ix = tx;
			tx += s->style.bmp->w + s->style.image_margin;
			iy = ty;
		} else if(!my_stricmp(s->style.image_align, "right")) {
			ix = tx + tw + s->style.image_margin;
			iy = ty;
		} else if(!my_stricmp(s->style.image_align, "bottom")) {
			iy = ty + th + s->style.image_margin;
			ix = tx;
		} else {
			/* "top" */
			iy = ty;
			ty += s->style.bmp->h + s->style.image_margin;
			ix = tx;
		}
		
		if(!my_stricmp(s->style.image_align, "left") || !my_stricmp(s->style.image_align, "right")) {
			if(th > s->style.bmp->h) {
				iy += (th - s->style.bmp->h) >> 1;
			} else {
				ty += (s->style.bmp->h - th) >> 1;
			}
		} else {
			if(tw > s->style.bmp->w) {
				ix += (tw - s->style.bmp->w) >> 1;
			} else {
				tx += (s->style.bmp->w - tw) >> 1;
			}
		}
	
		if(s->style.image_trans) {
			bm_set_color_s(s->style.bmp, s->style.image_trans);
			bm_maskedblit(bmp, ix, iy, s->style.bmp, 0, 0, s->style.bmp->w, s->style.bmp->h);
		} else {
			bm_blit(bmp, ix, iy, s->style.bmp, 0, 0, s->style.bmp->w, s->style.bmp->h);
		}
	}

	bm_set_color_s(bmp, s->style.fg);	
	bm_puts(bmp, tx, ty, text);	
}

static int basic_init(struct game_state *s) {	
	
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
			rerror("State '%s' does not exist.", nextstate);
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

static void draw_button(struct game_state *s, struct bitmap *bmp, int x, int y, int w, int h, int pad_left, int pad_top, const char *label, int highlight) {
	bm_set_color_s(bmp, s->style.fg);
	if(highlight) {
		if(s->style.btn_border_radius > 1) 
			bm_fillroundrect(bmp, x, y, x + w, y + h, s->style.btn_border_radius);
		else
			bm_fillrect(bmp, x, y, x + w, y + h);
		bm_set_color_s(bmp, s->style.bg);
	} else {
		if(s->style.btn_border_radius > 1) 
			bm_roundrect(bmp, x, y, x + w, y + h, s->style.btn_border_radius);
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

	bw = MY_MAX(bw1, bw2) + (s->style.btn_padding << 1);
	bh = MY_MAX(bh1, bh2) + (s->style.btn_padding << 1);

	/* Left button */
	bx1 = (s->style.margin + s->style.border + s->style.padding);
	by1 = bmp->h - 1 - bh - (s->style.margin + s->style.border + s->style.padding);	

	/* Right button */
	bx2 = bmp->w - 1 - bw - (s->style.margin + s->style.border + s->style.padding);
	by2 = bmp->h - 1 - bh - (s->style.margin + s->style.border + s->style.padding);
	
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
					rerror("State '%s' does not exist", nextstate);
				}
				
				change_state(next);
			} else {
				const char *nextstate = ini_get(game_ini, s->name, "right-state", NULL);
				struct game_state *next;
				
				if(!nextstate)
					change_state(NULL);
				
				next = get_state(nextstate);
				if(!next) {
					rerror("State '%s' does not exist", nextstate);
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
	draw_button(s, bmp, bx1, by1, bw, bh, ((bw-bw1)>>1), ((bh-bh1)>>1), left_label, !lr->dir);

	draw_button(s, bmp, bx2, by2, bw, bh, ((bw-bw2)>>1), ((bh-bh2)>>1), right_label, lr->dir);
	
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

/*
#define MAX_NESTED_STATES 20

static struct game_state *game_states[MAX_NESTED_STATES];
static int state_top = 0;
*/

struct game_state *current_state() {
	assert(state_top >= 0 && state_top < MAX_NESTED_STATES);
	return game_states[state_top];
}

void states_initialize() {
	int i;
	for(i = 0; i < MAX_NESTED_STATES; i++) {
		game_states[i] = NULL;
	}
}

int change_state(struct game_state *next) {
	
	if(game_states[state_top] && game_states[state_top]->deinit) {
		if(!game_states[state_top]->deinit(game_states[state_top])) {
			rerror("Deinitialising old state");
			return 0;
		}
		free(game_states[state_top]);
	}	
	game_states[state_top] = next;	
	if(game_states[state_top]){
		if(game_states[state_top]->init && !game_states[state_top]->init(game_states[state_top])) {
			rerror("Initialising new state");
			return 0;
		}
		apply_styles(game_states[state_top]);
	}
	return 1;
}

int push_state(struct game_state *next) {
	if(state_top > MAX_NESTED_STATES - 1) {
		rerror("Too many nested states");
		return 0;
	}
	state_top++;
	return change_state(next);
}

int pop_state(struct game_state *next) {
	int r;
	if(state_top <= 0) {
		rerror("State stack underflow.");
		return 0;
	}
	r = change_state(NULL);
	state_top--;
	return r;
}

static struct game_state *get_state(const char *name) {
	const char *type;
	
	struct game_state *next = NULL;
	
	if(!name) return NULL;
	
	if(!ini_has_section(game_ini, name)) {
		rerror("No state %s in ini file", name);
		quit = 1;
		return NULL;
	}
	
	type = ini_get(game_ini, name, "type", NULL);
	if(!type) {
		rerror("No state type for state '%s' in ini file", name);
		quit = 1;
		return NULL;
	}
	
	if(!my_stricmp(type, "static")) {
		next = get_static_state(name);
	} else if(!my_stricmp(type, "lua")) {
		next = get_lua_state(name);
	} else if(!my_stricmp(type, "leftright")) {
		next = get_leftright_state(name);
	} else if(!my_stricmp(type, "musl")) {
		next = get_mus_state(name);
	} else {
		rerror("Invalid type '%s' for state '%s' in ini file", type, name);
		quit = 1;
		return NULL;
	}
	if(!next) {
		rerror("Memory allocation error obtaining state %s", name);
		quit = 1;
	}
	return next;
}

int set_state(const char *name) {
	struct game_state *next;
	rlog("Changing to state '%s'", name);
	next = get_state(name);	
	return change_state(next);
}

