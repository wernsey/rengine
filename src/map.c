#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <unistd.h>

#include "tileset.h"
#include "bmp.h"
#include "map.h"
#include "json.h"
#include "utils.h"
#include "log.h"
#include "paths.h"

#define MAP_FILE_VERSION 1.2

struct map *map_create(int nr, int nc, int tw, int th, int nl) {
	int i;
	struct map *m = malloc(sizeof *m);	
	if(!m) 
		return NULL;
	m->nr = nr;
	m->nc = nc;
	m->nl = nl;
	m->dirty = 0;
	
	ts_init(&m->tiles, tw, th);
	
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

void map_get(struct map *m, int layer, int x, int y, int *tsi, int *ti) {
	struct map_cell *c;
	struct map_tile *tile;
	
	assert(tsi);
	assert(ti);
	
	if(layer >= m->nl || x < 0 || x >= m->nc || y < 0 || y >= m->nr) 
		return;
	
	assert(y * m->nc + x < m->nr * m->nc);
	
	c = &m->cells[y * m->nc + x];
	tile = &c->tiles[layer];
	
	*tsi = tile->si;
	*ti = tile->ti;
}

void map_render(struct map *m, struct bitmap *bmp, int layer, int scroll_x, int scroll_y) {
	
	struct tileset *ts = NULL;
	int tsi = -1, nht = 0;
	
	int i, j;
	int x, y;
	
	if(layer >= m->nl)
		return;

	y = -scroll_y;	
	for(j = 0; j < m->nr; j++) {
		x = -scroll_x;
		for(i = 0; i < m->nc; i++) {
			struct map_cell *cl = &m->cells[j * m->nc + i];
			struct map_tile *tile = &cl->tiles[layer];
			if(tile->ti >= 0) {
				int r, c;
				if(tsi != tile->si) {
					ts = ts_get(&m->tiles, tile->si);
					assert(ts);
					tsi = tile->si;
					nht = ts->bm->w / m->tiles.tw;
				}
				assert(ts != NULL);
				assert(nht > 0);
				
				r = tile->ti / nht;
				c = tile->ti % nht;
				
				bm_maskedblit(bmp, x, y, ts->bm, c * (m->tiles.tw + ts->border), r * (m->tiles.th + ts->border), m->tiles.tw, m->tiles.th);
			}
			x += m->tiles.tw;
		}
		y += m->tiles.th;
	}
}

struct map_cell *map_get_cell(struct map *m, int x, int y) {
	return &m->cells[y * m->nc + x];
}

void map_free(struct map *m) {
	int i;
	if(!m) return;
	for(i = 0; i < m->nr * m->nc; i++) {
		free(m->cells[i].tiles);
		free(m->cells[i].id);
		free(m->cells[i].clas);
	}
	ts_deinit(&m->tiles);
	free(m->cells);
	free(m);
}

static char *get_relpath(const char *mapfile, char *rel, size_t rellen) {
	char *from;
	
	if(strrchr(mapfile, '/')) {
		from = strdup(mapfile);
		strrchr(from, '/')[0] = '\0';
	} else {
		from = strdup("");
	}
	
	char * rv = relpath(from, "", rel, rellen);
	
	free(from);
	return rv;
}

int map_save(struct map *m, const char *filename) {
	int i, j;
	char buffer[128];

	/*
	rwd is the relative path to the working directory (i.e the relative path 
	to where game.ini can be found. Thus, if you're saving the map as
	mygame/maps/level1.map and game.ini is in mygame then rwd should be ../
	*/
	char rwd[128];
	
	FILE *f = fopen(filename, "w");
	if(!f) {
		rerror("Unable to open %s for writing map file.", filename);
		return 0;
	}
	rlog("Saving map file %s", filename);
	
	fprintf(f, "{\n");
	fprintf(f, "\"type\" : \"2D_TILE_MAP\",\n");
	fprintf(f, "\"version\" : %.2f,\n", MAP_FILE_VERSION);
	
	get_relpath(filename, rwd, sizeof rwd);
	fprintf(f, "\"rel-work-directory\":\"%s\",\n", json_escape(rwd, buffer, sizeof buffer));
	
	fprintf(f, "\"rows\" : %d,\n\"columns\" : %d,\n", m->nr, m->nc);
	fprintf(f, "\"num_layers\" : %d,\n", m->nl);
	fprintf(f, "\"cells\" : [\n");
	for(i = 0; i < m->nr * m->nc; i++) {
		struct map_cell *c = &m->cells[i];
		fprintf(f, "  {\"tiles\": [");
		for(j = 0; j < m->nl; j++) {
			struct map_tile *t = &c->tiles[j];
			fprintf(f, "{\"si\":%d, \"ti\":%d}", t->si, t->ti);
			if(j < m->nl - 1) fputc(',', f);
		}
		fprintf(f, "]");
		if(c->flags)
			fprintf(f, ", \"flags\": %d", c->flags);
		if(c->clas)
				fprintf(f, ", \"class\":\"%s\"", json_escape(c->clas, buffer, sizeof buffer));
		if(c->id)
			fprintf(f, ", \"id\":\"%s\"", json_escape(c->id, buffer, sizeof buffer));
		fprintf(f, "}%c\n", (i < m->nr * m->nc - 1) ? ',' : ' ');	
	}
	fprintf(f, "],\n");
	
	fprintf(f, "\"tilesets\" : ");
	ts_write_all(&m->tiles, f);
	
	fprintf(f, "}\n");
	fclose(f);
	return 1;
}
	
struct map *map_load(const char *filename, int cd) {	
	struct map *m;
	char *text = my_readfile (filename);	
	if(!text) {
		rerror("Unable to read map file %s", filename);
		return NULL;
	}
	rlog("Parsing map file %s", filename);
	m = map_parse(text, cd);	
	free(text);
	return m;
}

struct map *map_parse(const char *text, int cd) {
	double version;
	struct map *m = NULL;
	int nr, nc, tw, th, nl;
	struct json *j, *a, *e;
	int p,q;
	const char *s;
	
	j = json_parse(text);
	if(!j) {
		rerror("Unable to parse map file JSON");
		return 0;
	}
	
	if(!json_get_string(j, "type") || strcmp(json_get_string(j, "type"), "2D_TILE_MAP")) {
		rerror("JSON object is not of type 2D_TILE_MAP");
		json_free(j);
		return 0;
	}
		
	version = json_get_number(j, "version");
	if(version < 1.2) {
		rerror("Map version (%f) is too old", version);
		json_free(j);
		return NULL;
	}
	
	if(cd) {
		const char *rwd = json_get_string(j, "rel-work-directory");
		if(chdir(rwd)) {
			rerror("chdir(%s): %s", rwd, strerror(errno));
		}
	}
	
	nr = json_get_number(j, "rows");
	nc = json_get_number(j, "columns");
	tw = json_get_number(j, "tile_width");
	th = json_get_number(j, "tile_height");
	nl = json_get_number(j, "num_layers");
	
	m = map_create(nr, nc, tw, th, nl);
	if(!m) {
		rerror("Unable to create map.");
		return NULL;
	}
		
	a = json_get_object(j, "tilesets");
	if(!ts_read_all(&m->tiles, a)) {
		rerror("Not loading map because tilesets couldn't be loaded.");
		return NULL;
	}
	
	a = json_get_array(j, "cells");
	e = a->value;
	p = 0;
	while(e) {		
		struct map_cell *c;	
		struct json *aa, *ee;
		
		assert(p < nr * nc);
		c = &m->cells[p++];	
		
		ee = json_get_member(e, "flags");
		if(ee)
			c->flags = json_as_number(ee);
		else
			c->flags = 0;
		
		s = json_get_string(e, "id");
		if(s)
			c->id = strdup(s);
		s = json_get_string(e, "class");
		if(s)
			c->clas = strdup(s);
		
		aa = json_get_array(e, "tiles");
		ee = aa->value;
		q = 0;
		while(ee) {
			struct map_tile *mt;
			assert(q < m->nl);
			mt = &c->tiles[q++];
			
			mt->ti = json_get_number(ee, "ti");
			mt->si = json_get_number(ee, "si");
			
			ee = ee->next;
		}
		
		e = e->next;
	}
	
	json_free(j);
	
	return m;
}
