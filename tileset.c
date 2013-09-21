#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#include "bmp.h"
#include "tileset.h"
#include "lexer.h"
#include "json.h"
#include "utils.h"

#define TILE_FILE_VERSION 1.0f

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
	
	char buffer[128];
	
	fprintf(f, "{\n");	
	fprintf(f, "\"type\" : \"TILESET\",\n");
	fprintf(f, "\"version\" : %.2f,\n", TILE_FILE_VERSION);
	fprintf(f, "\"count\" : %d,\n", ntilesets);
	fprintf(f, "\"tilesets\": [\n");
	for(i = 0; i < ntilesets; i++) {
		struct tileset *t = tilesets[i];
		fprintf(f, "  {\n");
		
		fprintf(f, "  \"name\" : \"%s\",\n", json_escape(t->name, buffer, sizeof buffer));
		fprintf(f, "  \"tw\" : %d,\n  \"th\" : %d, \n", t->tw, t->th);
		fprintf(f, "  \"border\" : %d,\n  \"nmeta\" : %d, \n", t->border, t->nmeta);
		fprintf(f, "  \"meta\" : [\n");
		for(j = 0; j < t->nmeta; j++) {
			struct tile_meta *m = &t->meta[j];
			fprintf(f, "    {");
			fprintf(f, "\"num\":%d, ", m->num);
			fprintf(f, "\"class\":\"%s\", ", json_escape(m->clas, buffer, sizeof buffer));
			fprintf(f, "\"flags\":%d", m->flags);
			fprintf(f, "}%c\n", (j < t->nmeta - 1) ? ',' : ' ');
		}
		fprintf(f, "  ]\n");
		fprintf(f, "  }%c\n", (i < ntilesets - 1) ? ',' : ' ');		
	}
	fprintf(f, "]\n");
	fprintf(f, "}\n");
	
	return 1;
}

/* Depending on what you want to do, you may
 *	want to call ts_free_all() first.
 */
int ts_load_all(const char *filename) {
	int r;
	struct json *j;
	char *text = my_readfile (filename);	
	if(!text)
		return 0;
	
	j = json_parse(text);
	if(!j) {
		/* TODO: Error handling can be better. */
		return 0;
	}
	
	r = ts_read_all(j);
	free(text);
	json_free(j);
	
	return r;
}

int ts_read_all(struct json *j) {
	double version;
	
	struct json *a, *e;
	
	if(!json_get_string(j, "type") || strcmp(json_get_string(j, "type"), "TILESET")) {
		return 0;
	}
	
	version = json_get_number(j, "version");
	if(version < 1.0) {
		return 0;
	}
	
	a = json_get_array(j, "tilesets");
	if(!a) {
		return 0;
	}
	
	e = a->value;
	while(e) {
		int tw, th, border, nmeta, y;
		const char *name;
		struct tileset *t;
		struct json *aa, *ee;
		
		name = json_get_string(e, "name");
		tw = json_get_number(e, "tw");
		th = json_get_number(e, "th");
		border = json_get_number(e, "border");
		nmeta = json_get_number(e, "nmeta");
		(void)nmeta;
				
		y = ts_add(name, tw, th, border);
		if(y < 0) {
			return 0;
		}
		t = ts_get(y);
		
		t->nmeta = 0;
		t->meta = NULL;
		
		aa = json_get_array(e, "meta");
		assert(json_array_len(aa) == nmeta);
		if(aa) {
			t->nmeta = json_array_len(aa);
			t->meta = malloc(t->nmeta * sizeof *t->meta);
			
			assert(t->nmeta == nmeta);
			
			ee = aa->value;
			y = 0;
			while(ee) {			
				struct tile_meta *m = &t->meta[y++];
				const char *clas = json_get_string(ee, "class");
				
				m->num = json_get_number(ee, "num");
				m->flags = json_get_number(ee, "flags");
				m->clas = strdup(clas?clas:"");
				
				ee = ee->next;
			}
		}
		
		e = e->next;
	}
	
	return 1;
}