#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "tileset.h"
#include "bmp.h"
#include "map.h"
#include "json.h"

#define MAP_FILE_VERSION 1.0

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
	
	m->cells = calloc(nr * nc, sizeof *m->cells);
	
	for(i = 0; i < nr * nc; i++) {
		int j;
		struct map_cell *c = &m->cells[i];
		c->tiles = calloc(nl, sizeof *c->tiles);
		for(j = 0; j < nl; j++) {
			c->tiles[j].ti = -1;
		}
	}
	
	return m;
}

void map_set(struct map *m, int layer, int x, int y, int tsi, int ti) {
	struct map_cell *c;
	struct map_tile *tile;
	if(layer >= m->nl || x < 0 || x >= m->nc || y < 0 || y >= m->nr) 
		return;
	
	assert(y * m->nc + x < m->nr * m->nc);
	
	c = &m->cells[y * m->nc + x];
	tile = &c->tiles[layer];
	
	tile->si = tsi;
	tile->ti = ti;
	
	m->dirty = 1;
}

void map_render(struct map *m, struct bitmap *bmp, int layer, int scroll_x, int scroll_y) {
	
	struct tileset *ts = NULL;
	int tsi = -1, nht = 0;
	
	int i, j;
	int x, y;
	
	if(layer >= m->nl)
		return;
		
	/* FIXME: The transparent color ought to be part of the tileset. */	
	y = -scroll_y;	
	for(j = 0; j < m->nr; j++) {
		x = -scroll_x;
		for(i = 0; i < m->nc; i++) {
			struct map_cell *cl = &m->cells[j * m->nc + i];
			struct map_tile *tile = &cl->tiles[layer];
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
	for(i = 0; i < m->nr * m->nc; i++) {
		free(m->cells[i].tiles);
	}
	free(m->cells);
	free(m);
}

int map_save(struct map *m, const char *filename) {
	int i, j;
	char buffer[128];
	
	FILE *f = fopen(filename, "w");
	fprintf(f, "{\n");
	fprintf(f, "\"type\" : \"2D_TILE_MAP\",\n");
	fprintf(f, "\"version\" : %.2f,\n", MAP_FILE_VERSION);
	
	fprintf(f, "\"rows\" : %d,\n\"columns\" : %d,\n", m->nr, m->nc);
	fprintf(f, "\"tile_width\" : %d,\n\"tile_height\" : %d,\n", m->tw, m->th);
	
	fprintf(f, "\"num_layers\" : %d,\n", m->nl);
	fprintf(f, "\"cells\" : [\n");
	for(i = 0; i < m->nr * m->nc; i++) {
		struct map_cell *c = &m->cells[i];
		fprintf(f, "  { \"tiles\": [");
		for(j = 0; j < m->nl; j++) {
			struct map_tile *t = &c->tiles[j];
			fprintf(f, "{\"si\":%d, \"ti\":%d}", t->si, t->ti);
			if(j < m->nl - 1) fputc(',', f);
		}
		fprintf(f, "], \"flags\": %d", c->flags);
		if(c->clas)
				fprintf(f, ", \"class\":\"%s\"", json_escape(c->clas, buffer, sizeof buffer));
		if(c->id)
			fprintf(f, ", \"id\":\"%s\"", json_escape(c->id, buffer, sizeof buffer));
		fprintf(f, "}%c\n", (i < m->nr * m->nc - 1) ? ',' : ' ');	
	}
	fprintf(f, "],\n");
	
	fprintf(f, "\"tilesets\" : ");
	ts_write_all(f);
	
	fprintf(f, "}\n");
	fclose(f);
	return 1;
}

