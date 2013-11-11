#include <stdio.h>
#include <stdlib.h>

#include "pak.h"
#include "bmp.h"
#include "ini.h"
#include "game.h"
#include "utils.h"
#include "hash.h"
#include "log.h"

static struct pak_file *game_pak = NULL;

static const char *pak_file_name = "";

struct hash_tbl *bmp_cache;

void re_initialize() {
	rlog("Initializing resources.");
	bmp_cache = ht_create(128);
}

void bmp_cache_cleanup(const char *key, void *vb) {
	struct bitmap *bmp = (struct bitmap *)vb;
	rlog("Freeing '%s'", key);
	bm_free(bmp);
}

void re_clean_up() {
	rlog("Cleaning up resources");
	ht_free(bmp_cache, bmp_cache_cleanup);
}

int rs_read_pak(const char *filename) {
	game_pak = pak_open(filename);
	if(game_pak) {
		pak_file_name = filename;
		return 1;
	} else {
		return 0;
	}
}

struct ini_file *re_get_ini(const char *filename) {
	int err, line;
	struct ini_file *ini;
	if(game_pak) {
		char *text = pak_get_text(game_pak, filename);
		if(!text) {
			rerror("Unable to find %s in %s", filename, pak_file_name);
			return NULL;
		}
		ini = ini_parse(text, &err, &line);
		if(!ini) {
			rerror("Unable to parse %s:%d %s", filename, line, ini_errstr(err));
		}
		free(text);
	} else {
		ini = ini_read(filename, &err, &line);
		if(!ini) {
			rerror("Unable to read %s:%d %s", filename, line, ini_errstr(err));
		}
	}
	return ini;
}

struct bitmap *re_get_bmp(const char *filename) {
	struct bitmap *bmp;
	
	bmp = ht_find(bmp_cache, filename);
	if(bmp) {
		return bmp;
	}
	
	if(game_pak) {
		FILE *f = pak_get_file(game_pak, filename);
		if(!f) {
			rerror("Unable to locate %s in %s", filename, pak_file_name);
			return NULL;
		}		
		bmp = bm_load_fp(f);
		if(!bmp) {
			rerror("Unable to load bitmap '%s' from %s", filename, pak_file_name);
		}
	} else {
		bmp = bm_load(filename);
		if(!bmp) {
			rerror("Unable to load bitmap '%s'", filename);
		}
	}
	rlog("Cached bitmap '%s'", filename);
	ht_insert(bmp_cache, filename, bmp);
	
	return bmp;
}

char *re_get_script(const char *filename) {
	char *txt;
	if(game_pak) {		
		txt = pak_get_text(game_pak, filename);
		if(!txt) {
			rerror("Unable to load script '%s' from %s", filename, pak_file_name);
		}
	} else {
		txt = my_readfile(filename);
		if(!txt) {
			rerror("Couldn't load script '%s'", filename);
			return 0;
		}
	}
	return txt;
}

