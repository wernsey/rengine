#include <stdio.h>
#include <stdlib.h>

#include "bmp.h"
#include "states.h"
#include "game.h"
#include "resources.h"
#include "log.h"

#include "rengine.xbm"

static struct bitmap *dummy;

static int dem_init(struct game_state *s) {
	dummy = bm_fromXbm(rengine_width, rengine_height, rengine_bits);
	if(!dummy) {
		rerror("unable to load rengine.xbm");
		return 0;
	}
	bm_set_color_s(dummy, "#000000");
	return 1;
}

static int dem_update(struct game_state *s, struct bitmap *bmp) {
	
	bm_set_color_s(bmp, "black");
	bm_clear(bmp);
	
	bm_set_color_s(bmp, "red");
	bm_line(bmp, 0, 10, 10, 0);
	bm_fill(bmp, 5, 0);
	
	bm_set_color_s(bmp, "orange");
	bm_line(bmp, 0, 20, 20, 0);
	bm_fill(bmp, 15, 0);
	
	bm_set_color_s(bmp, "green");
	bm_line(bmp, 0, 30, 30, 0);
	bm_fill(bmp, 25, 0);
	
	bm_set_color_s(bmp, "blue");
	bm_line(bmp, 0, 40, 40, 0);
	bm_fill(bmp, 35, 0);
	
	bm_set_color_s(bmp, "grey");
	bm_printfs(bmp, 72, 37, 2, "Rengine");
	bm_set_color_s(bmp, "white");
	bm_printfs(bmp, 70, 35, 2, "Rengine");
	
	bm_maskedblit(bmp, (bmp->w - dummy->w)>>1, (bmp->h - dummy->h)>>1, dummy, 0, 0, dummy->w, dummy->h);
	return 1;
}

static int dem_deinit(struct game_state *s) {
	return 1;
}

struct game_state *get_demo_state(const char *name) {
	struct game_state *state = malloc(sizeof *state);
	if(!state)
		return NULL;
	
	state->init = dem_init;
	state->update = dem_update;
	state->deinit = dem_deinit;
	
	return state;
}