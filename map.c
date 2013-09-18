#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tileset.h"
#include "bmp.h"
#include "map.h"

struct map *map_create(int nr, int nc, int tw, int th, int nl) {
	int i;
	struct map *m = malloc(sizeof *m);	
	if(!m) 
		return NULL;
	m->nr = nr;
	m->nc = nc;
	m->tw = tw;
	m->th = th;
	m->nl = nl;
	m->dirty = 0;
	m->layers = calloc(nl, sizeof *m->layers);
	if(!m->layers) {
		free(m);
		return NULL;
	}
	for(i = 0; i < nl; i++) {
		int j;
		struct map_layer *l = &m->layers[i];
		l->tiles = calloc(nr * nc, sizeof *l->tiles);
		if(!l->tiles) {
			free(m->layers);
			free(m);
			return NULL;
		}
		for(j = 0; j < nr * nc; j++) {
			l->tiles[j].si = 0;
			l->tiles[j].ti = -1;
			l->tiles[j].clas = NULL;
			l->tiles[j].id = NULL;
		}
	}
	return m;
}

void map_set(struct map *m, int layer, int x, int y, int tsi, int ti) {
	struct map_layer *l;
	struct map_tile *tile;
	if(layer >= m->nl || x < 0 || x >= m->nc || y < 0 || y >= m->nr) 
		return;
	
	assert(y * m->nc + x < m->nr * m->nc);
	
	l = &m->layers[layer];
	tile = &l->tiles[y * m->nc + x];
	
	tile->si = tsi;
	tile->ti = ti;
	
	m->dirty = 1;
}

void map_render(struct map *m, struct bitmap *bmp, int layer, int scroll_x, int scroll_y) {
	struct map_layer *l;
	
	struct tileset *ts = NULL;
	int tsi = -1, nht = 0;
	
	int i, j;
	int x, y;
	
	if(layer >= m->nl)
		return;
	
	l = &m->layers[layer];
	
	/* FIXME: The transparent color ought to be part of the 
		tileset.
	*/	
	y = -scroll_y;	
	for(j = 0; j < m->nr; j++) {
		x = -scroll_x;
		for(i = 0; i < m->nc; i++) {
			struct map_tile *tile = &l->tiles[j * m->nc + i];
			if(tile->ti >= 0) {
				int r, c;
				if(tsi != tile->si) {
					ts = ts_get(tile->si);
					assert(ts);
					tsi = tile->si;
					nht = ts->bm->w / m->tw;
				}
				assert(ts != NULL);
				assert(nht > 0);
				
				r = tile->ti / nht;
				c = tile->ti % nht;
				
				bm_maskedblit(bmp, x, y, ts->bm, c * m->tw, r * m->th, m->tw, m->th);
			}
			x += m->tw;
		}
		y += m->th;
	}
}

void map_free(struct map *m) {
	int i;
	for(i = 0; i < m->nl; i++) {
		free(m->layers[i].tiles);
	}
	free(m->layers);
	free(m);
}
