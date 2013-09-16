#include <stdio.h>

#include "bmp.h"
#include "states.h"
#include "game.h"
#include "resources.h"

static int ax, ay, dx, dy;
static struct bitmap *dummy;

static int dem_init(struct game_state *s) {
	dummy = re_get_bmp("alien.bmp");
	if(!dummy) {
		fprintf(log_file, "error: unable to load alien.bmp");
		return 0;
	}
	bm_set_color_s(dummy, "#FF00FF");
	ax = 160; ay = 120;
	dx = 1; dy = 2;
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
	
	bm_set_color_s(bmp, "blue");
	bm_fillroundrect(bmp, 100, 100, 200, 200, 7); 
	
	bm_set_color_s(bmp, "silver");
	bm_roundrect(bmp, 100, 100, 200, 200, 7); 
	bm_roundrect(bmp, 102, 102, 198, 198, 5); 
	
	bm_set_color_s(bmp, "black");
	bm_printf(bmp, 106, 107, "Hello\nWorld!");
	bm_set_color_s(bmp, "white");
	bm_printf(bmp, 107, 107, "Hello\nWorld!");
	
	bm_set_color_s(bmp, "grey");
	bm_printfs(bmp, 42, 37, 2, "Rengine'80");
	bm_set_color_s(bmp, "white");
	bm_printfs(bmp, 40, 35, 2, "Rengine'80");
	
	if(ax + dx < -20 || ax + dx >= bmp->w - 20)
		dx = -dx;
	ax += dx;
	
	if(ay + dy < -20 || ay + dy >= bmp->h - 20)
		dy = -dy;
	ay += dy;
	
	bm_maskedblit(bmp, ax, ay, dummy, 0, 0, 33, 34);
	return 1;
}

static int dem_deinit(struct game_state *s) {
	return 1;
}

struct game_state demo_state = {
	NULL,
	dem_init,
	dem_update,
	dem_deinit
};
