#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "bmp.h"
#include "tileset.h"

#define TILE_FILE_VERSION	1.0f

static struct tileset **tilesets = NULL;
static int ntilesets = 0;

static struct tileset *ts_make(const char *filename, int tw, int th, int border);

static void ts_free(struct tileset *t);

int ts_add(const char *filename, int tw, int th, int border) {
	tilesets = realloc(tilesets, (ntilesets + 1) * sizeof *tilesets);
	if(!tilesets) {
		ntilesets = 0;
		return -1;
	}
	tilesets[ntilesets] = ts_make(filename, tw, th, border);
	if(!tilesets[ntilesets]) {
		return -1;
	}
	return ntilesets++;
}

void ts_free_all() {
	int i;
	if(!tilesets) 
		return;
	
	for(i = 0; i < ntilesets; i++) {
		ts_free(tilesets[i]);
	}
	free(tilesets);
	tilesets = NULL;
	ntilesets = 0;
}

struct tileset *ts_get(int i) {
	if(i >= ntilesets || i < 0) 
		return NULL;
	return tilesets[i];
}

int ts_get_num() {
	return ntilesets;
}

struct tileset *ts_find(const char *name) {
	int i;
	if(!name) return NULL;
	for(i = 0; i < ntilesets; i++) {
		if(!strcmp(tilesets[i]->name, name))
			return tilesets[i];
	}
	return NULL;
}

int ts_index_of(const char *name) {
	int i;
	if(!name) return -1;
	for(i = 0; i < ntilesets; i++) {
		if(!strcmp(tilesets[i]->name, name))
			return i;
	}
	return -1;
}

static struct tileset *ts_make(const char *filename, int tw, int th, int border) {
	struct bitmap *bm = bm_load(filename);
	if(bm) {
		struct tileset *t = malloc(sizeof *t);
		if(!t) { 
			bm_free(bm);
			return NULL;
		}
		
		t->name = strdup(filename);
		
		t->bm = bm;
		
		/* FIXME: The transparent color of the 
			tileset should be configurable and 
			saved/loaded from the timeset file
		*/
		bm_set_color_s(t->bm, "#FF00FF");
		
		t->tw = tw;
		t->th = th;
		t->border = border;
		
		t->nmeta = 0;
		t->meta = NULL;
		
		return t;
	}
	return NULL;
}

static void ts_free(struct tileset *t) {
	if(!t) return;
	free(t->name);
	bm_free(t->bm);
	free(t);
}

struct tile_meta *ts_has_meta(struct tileset *t, int row, int col) {
	int i;
	if(!t) {
		return NULL;
	} else {
		int tr = t->bm->w / t->tw;
		int n = row * tr + col;
		
		for(i = 0; i < t->nmeta; i++) {
			struct tile_meta *m = &t->meta[i];
			if(m->num == n) {
				return m;
			}
		}	
	}
	return NULL;
}

struct tile_meta *ts_get_meta(struct tileset *t, int row, int col) {
	if(!t) {
		return NULL;
	} else {
		struct tile_meta *m = ts_has_meta(t, row, col);
		if(m) return m;
		
		/* not found - resize the list */
		t->meta = realloc(t->meta, (++t->nmeta) * sizeof *t->meta);
		if(!t->meta) {
			t->nmeta = 0;
			return NULL;
		} else {
			int tr = t->bm->w / t->tw;
			int n = row * tr + col;
			
			m = &t->meta[t->nmeta - 1];
			
			m->num = n;
			
			m->clas = strdup("");
			if(!m->clas) {
				t->nmeta--;
				return NULL;
			}
			
			m->flags = 0;
			
			return m;
		}
	}
}

int ts_valid_class(const char *clas) {
	if(strlen(clas) > TS_CLASS_MAXLEN) 
		return 0;
	while(*clas) {
		if(!isalnum(*clas) && *clas != '_')
			return 0;
		clas++;
	}
	return 1;
}

int ts_save_all(const char *filename) {
	FILE *f = fopen(filename, "w");
	int r = ts_write_all(f);
	fclose(f);
	return r;
}

int ts_write_all(FILE *f) {
	int i, j;
	fprintf(f, "TILESET\n%5.2f %d\n", TILE_FILE_VERSION, ntilesets);
	for(i = 0; i < ntilesets; i++) {
		struct tileset *t = tilesets[i];
		fprintf(f, "%s\n%d %d %d %d\n", t->name, t->tw, t->th, t->border, t->nmeta);
		for(j = 0; j < t->nmeta; j++) {
			struct tile_meta *m = &t->meta[j];
			if(m->clas[0] == '\0')
				fprintf(f, "%d %s %04X\n", m->num, "#null#", m->flags);
			else
				fprintf(f, "%d %s %04X\n", m->num, m->clas, m->flags);
		}
	}
	return 1;
}

int ts_load_all(const char *filename) {
	FILE *f = fopen(filename, "r");
	int r = ts_read_all(f);
	fclose(f);
	return r;
}

int ts_read_all(FILE *f) {
	char buffer[128];
	int i, n;
	float version;
	
	/* Depending on what you want to do, you may
		want to call ts_free_all() first */
	fscanf(f,"%s", buffer);
	if(strcmp(buffer, "TILESET")) {
		return 0;
	}
		
	fscanf(f, "%f %d", &version, &n);
	for(i = 0; i < n; i++) {
		int tw, th, border, nmeta, j;
		struct tileset *t;
		fgets(buffer, sizeof buffer, f);
		fgets(buffer, sizeof buffer, f);
		for(j = 0; buffer[j]; j++) {
			if(buffer[j] == '\n')
				buffer[j] = '\0';
		}
		fscanf(f, "%d %d %d %d", &tw, &th, &border, &nmeta);
		j = ts_add(buffer, tw, th, border);
		if(j < 0) {
			return 0;
		}
		t = ts_get(j);
		
		t->nmeta = nmeta;
		t->meta = malloc(nmeta * sizeof *t->meta);
		if(!t->meta) {
			t->nmeta = 0;
			return 0;
		}
		for(j = 0; j < nmeta; j++) {
			struct tile_meta *m = &t->meta[j];
			fscanf(f, "%d %s %x", &m->num, buffer, &m->flags);
			if(!strcmp(buffer, "#null#"))
				buffer[0] = '\0';				
			m->clas = strdup(buffer);
		}
	}
	
	return 1;
}