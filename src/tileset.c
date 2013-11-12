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

#define TILE_FILE_VERSION 1.1f

static struct tileset *ts_make(const char *filename);

static void ts_free(struct tileset *t);

void ts_init(struct tile_collection *tc, int tw, int th, int border) {
	tc->tilesets = NULL;
	tc->ntilesets = 0;
	tc->tw = tw;
	tc->th = th;
	tc->border = border;
};

void ts_deinit(struct tile_collection *tc) {
	int i;
	if(!tc->tilesets) 
		return;
	
	for(i = 0; i < tc->ntilesets; i++) {
		ts_free(tc->tilesets[i]);
	}
	free(tc->tilesets);
	tc->tilesets = NULL;
	tc->ntilesets = 0;
}

int ts_add(struct tile_collection *tc, const char *filename) {
	tc->tilesets = realloc(tc->tilesets, (tc->ntilesets + 1) * sizeof *tc->tilesets);
	if(!tc->tilesets) {
		tc->ntilesets = 0;
		return -1;
	}
	tc->tilesets[tc->ntilesets] = ts_make(filename);
	if(!tc->tilesets[tc->ntilesets]) {
		return -1;
	}
	return tc->ntilesets++;
}

struct tileset *ts_get(struct tile_collection *tc, int i) {
	if(i >= tc->ntilesets || i < 0) 
		return NULL;
	return tc->tilesets[i];
}

int ts_get_num(struct tile_collection *tc) {
	return tc->ntilesets;
}

struct tileset *ts_find(struct tile_collection *tc, const char *name) {
	int i;
	if(!name) return NULL;
	for(i = 0; i < tc->ntilesets; i++) {
		if(!strcmp(tc->tilesets[i]->name, name))
			return tc->tilesets[i];
	}
	return NULL;
}

int ts_index_of(struct tile_collection *tc, const char *name) {
	int i;
	if(!name) return -1;
	for(i = 0; i < tc->ntilesets; i++) {
		if(!strcmp(tc->tilesets[i]->name, name))
			return i;
	}
	return -1;
}

static struct tileset *ts_make(const char *filename) {
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


struct tile_meta *ts_has_meta_ti(struct tile_collection *tc, struct tileset *t, int ti) {
	int i;
	if(!t) {
		return NULL;
	} else {
		for(i = 0; i < t->nmeta; i++) {
			struct tile_meta *m = &t->meta[i];
			if(m->ti == ti) {
				return m;
			}
		}	
	}
	return NULL;
}

struct tile_meta *ts_has_meta(struct tile_collection *tc, struct tileset *t, int row, int col) {
	int tr, n;
	if(!t)
		return NULL;
		
	tr = t->bm->w / tc->tw;
	n = row * tr + col;
	
	return ts_has_meta_ti(tc, t, n);
}

struct tile_meta *ts_get_meta(struct tile_collection *tc, struct tileset *t, int row, int col) {
	if(!t) {
		return NULL;
	} else {
		struct tile_meta *m = ts_has_meta(tc, t, row, col);
		if(m) return m;
		
		/* not found - resize the list */
		t->meta = realloc(t->meta, (++t->nmeta) * sizeof *t->meta);
		if(!t->meta) {
			t->nmeta = 0;
			return NULL;
		} else {
			int tr = t->bm->w / tc->tw;
			int n = row * tr + col;
			
			m = &t->meta[t->nmeta - 1];
			
			m->ti = n;
			
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

int ts_save_all(struct tile_collection *tc, const char *filename) {
	FILE *f = fopen(filename, "w");
	int r = ts_write_all(tc, f);
	fclose(f);
	return r;
}

int ts_write_all(struct tile_collection *tc, FILE *f) {
	int i, j;
	
	char buffer[128];
	
	fprintf(f, "{\n");	
	fprintf(f, "\"type\" : \"TILESET\",\n");
	fprintf(f, "\"version\" : %.2f,\n", TILE_FILE_VERSION);
	fprintf(f, "\"count\" : %d,\n", tc->ntilesets);
	fprintf(f, "\"tw\" : %d,\n  \"th\" : %d, \n", tc->tw, tc->th);
	fprintf(f, "\"border\" : %d,\n\n", tc->border);
	fprintf(f, "\"tilesets\": [\n");
	for(i = 0; i < tc->ntilesets; i++) {
		struct tileset *t = tc->tilesets[i];
		fprintf(f, "  {\n");		
		fprintf(f, "  \"name\" : \"%s\",\n", json_escape(t->name, buffer, sizeof buffer));
		fprintf(f, "  \"nmeta\" : %d,\n", t->nmeta);
		fprintf(f, "  \"meta\" : [\n");
		for(j = 0; j < t->nmeta; j++) {
			struct tile_meta *m = &t->meta[j];
			fprintf(f, "    {");
			fprintf(f, "\"ti\":%d, ", m->ti);
			fprintf(f, "\"class\":\"%s\", ", json_escape(m->clas, buffer, sizeof buffer));
			fprintf(f, "\"flags\":%d", m->flags);
			fprintf(f, "}%c\n", (j < t->nmeta - 1) ? ',' : ' ');
		}
		fprintf(f, "  ]\n");
		fprintf(f, "  }%c\n", (i < tc->ntilesets - 1) ? ',' : ' ');		
	}
	fprintf(f, "]\n");
	fprintf(f, "}\n");
	
	return 1;
}

/* Depending on what you want to do, you may
 *	want to call ts_free_all() first.
 */
int ts_load_all(struct tile_collection *tc, const char *filename) {
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
	
	r = ts_read_all(tc, j);
	free(text);
	json_free(j);
	
	return r;
}

int ts_read_all(struct tile_collection *tc, struct json *j) {
	double version;
	
	struct json *a, *e;
	
	if(!json_get_string(j, "type") || strcmp(json_get_string(j, "type"), "TILESET")) {
		return 0;
	}
	
	version = json_get_number(j, "version");
	if(version < 1.1) {
		return 0;
	}
	
	tc->tw = json_get_number(j, "tw");
	tc->th = json_get_number(j, "th");
	tc->border = json_get_number(j, "border");
	
	a = json_get_array(j, "tilesets");
	if(!a) {
		return 0;
	}
	
	e = a->value;
	while(e) {
		int nmeta, y;
		const char *name;
		struct tileset *t;
		struct json *aa, *ee;
		
		name = json_get_string(e, "name");
		nmeta = json_get_number(e, "nmeta");
		(void)nmeta;
				
		y = ts_add(tc, name);
		if(y < 0) {
			return 0;
		}
		t = ts_get(tc, y);
		
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
				
				m->ti = json_get_number(ee, "ti");
				m->flags = json_get_number(ee, "flags");
				m->clas = strdup(clas?clas:"");
				
				ee = ee->next;
			}
		}
		
		e = e->next;
	}
	
	return 1;
}