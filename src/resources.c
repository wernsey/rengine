#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include <SDL.h>
#include <SDL_mixer.h>

#include "pak.h"
#include "bmp.h"
#include "ini.h"
#include "utils.h"
#include "hash.h"
#include "log.h"

static struct pak_file *game_pak = NULL;

static const char *pak_file_name = "";

/* The cache forms a stack, so that it can be pushed 
	and popped as the game states are pushed and popped. */
struct resource_cache {
	
	Hash_Tbl *bmp_cache;
	Hash_Tbl *wav_cache;
	Hash_Tbl *mus_cache;
	
	/* I expect as development continues, other 
	things will be cached as well */
	
	struct resource_cache *parent;
} *re_cache = NULL;

static struct resource_cache *re_cache_create() {
	struct resource_cache *rc = malloc(sizeof *rc);
	rc->bmp_cache = ht_create(128);
	rc->wav_cache = ht_create(128);
	rc->mus_cache = ht_create(128);
	rc->parent = NULL;
	return rc;
}

static void bmp_cache_cleanup(const char *key, void *vb) {
	struct bitmap *bmp = vb;
	rlog("Freeing '%s'", key);
	bm_free(bmp);
}

static void wav_cache_cleanup(const char *key, void *vb) {
	Mix_Chunk *sound = vb;
	rlog("Freeing '%s'", key);
	Mix_FreeChunk(sound);
}

static void mus_cache_cleanup(const char *key, void *vb) {
	Mix_Music *music = vb;
	rlog("Freeing '%s'", key);
	Mix_FreeMusic(music);
}

static void re_cache_destroy(struct resource_cache *rc) {
	ht_free(rc->bmp_cache, bmp_cache_cleanup);
	ht_free(rc->wav_cache, wav_cache_cleanup);
	ht_free(rc->mus_cache, mus_cache_cleanup);
	free(rc);
}

void re_initialize() {
	rlog("Initializing resources.");	
	re_cache = re_cache_create();
}

void re_clean_up() {
	rlog("Cleaning up resources");
	while(re_cache) {
		struct resource_cache *t = re_cache;
		re_cache = re_cache->parent;
		re_cache_destroy(t);
	}
}


#if 0
/* As of now, consider these functions deprecated */
void re_push() {
	struct resource_cache *rc = re_cache_create();
	rc->parent = re_cache;
	re_cache = rc;
}

void re_pop() {
	struct resource_cache *rc = re_cache;
	
	/* You're not supposed to call re_push() and re_pop() 
	outside of the calls to push_state() and pop_state() */
	assert(re_cache); 
	assert(re_cache->parent);
	
	re_cache = re_cache->parent;
	re_cache_destroy(rc);	
}
#endif

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

static SDL_RWops *re_get_RWops(const char *filename) {
    if(game_pak) {
		return pak_get_rwops(game_pak, filename);
	} 
	return SDL_RWFromFile(filename, "rb");
}

/* There is also a re_get_bmp(const char *filename)
 * defined in rengine/editor/resources.c which doesn't
 * use the resource cache.
 */
struct bitmap *re_get_bmp(const char *filename) {
	struct bitmap *bmp;
	
	/* Search through the current resource cache and
		all its parents for the desired node. */
	struct resource_cache *rc = re_cache;	
	while(rc) {
		bmp = ht_get(rc->bmp_cache, filename);
		if(bmp) {
			return bmp;
		}
		rc = rc->parent;
	}
	
	/* Not cached. Load it. */
	if(game_pak) {
		SDL_RWops *rw = re_get_RWops(filename);
		if(!rw) {
			rerror("Unable to locate %s in %s", filename, pak_file_name);
			return NULL;
		}
        bmp = bm_load_rw(rw);
        if(!bmp) {
            rerror("Unable to load bitmap '%s' from %s", filename, pak_file_name);
        }		
    } else {
        SDL_RWops *rw = SDL_RWFromFile(filename, "rb");
		if(!rw) {
			rerror("Unable to open %s", filename);
			return NULL;
		}        
        bmp = bm_load_rw(rw);
        if(!bmp) {
            rerror("Unable to load bitmap '%s'", filename);
        }
        SDL_RWclose(rw);
    }
    if(bmp) {	
        /* Insert it to the cache on the top of the stack. */
        ht_put(re_cache->bmp_cache, filename, bmp);
        rlog("Cached bitmap '%s'", filename);
	}
    
	return bmp;
}

struct bitmap *re_clone_bmp(struct bitmap *b, const char *newname) {
	struct resource_cache *rc = re_cache;	
	struct bitmap *clone = ht_get(rc->bmp_cache, newname);
	if(clone) {
		rerror("Attempt to clone bitmap with an existing name %s", newname);
		return NULL;
	}
	clone = bm_copy(b);
	ht_put(re_cache->bmp_cache, newname, clone);
	rlog("Cached cloned bitmap as '%s'", newname);
	return clone;
}

Mix_Chunk *re_get_wav(const char *filename) {
	Mix_Chunk *chunk = NULL;
	
	struct resource_cache *rc = re_cache;	
	while(rc) {
		chunk = ht_get(rc->wav_cache, filename);
		if(chunk) {
			return chunk;
		}
		rc = rc->parent;
	}
	
	SDL_RWops * ops = re_get_RWops(filename);
	if(!ops) {
		return NULL;
	}	
	chunk = Mix_LoadWAV_RW(ops, 1);
	if(!chunk) {
		return NULL;
	}
	
	ht_put(re_cache->wav_cache, filename, chunk);
	rlog("Cached WAV '%s'", filename);
	
	return chunk;
}

Mix_Music *re_get_mus(const char *filename) {	
	Mix_Music *music = NULL;
	
	struct resource_cache *rc = re_cache;	
	while(rc) {
		music = ht_get(rc->mus_cache, filename);
		if(music) {
			return music;
		}
		rc = rc->parent;
	}
	
	SDL_RWops * ops = re_get_RWops(filename);
	if(!ops) {
		return NULL;
	}	
	music = Mix_LoadMUS_RW(ops, 1);
	if(!music) {
		return NULL;
	}
	
	ht_put(re_cache->mus_cache, filename, music);
	rlog("Cached Music '%s'", filename);
	
	return music;
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
	/* Scripts are not cached due to the nature they are used. */
	return txt;
}

