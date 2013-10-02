#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "bmp.h"
#include "states.h"
#include "map.h"
#include "game.h"
#include "ini.h"
#include "resources.h"
#include "tileset.h"

static lua_State *L = NULL;

static struct map *the_map;

static int map_init(struct game_state *s) {
	
	fprintf(log_file, "info: Initializing Map state '%s'\n", s->data);
	
	const char *map_file, *script_file;
	char *map_text, *script;
	
	map_file = ini_get(game_ini, s->data, "map", NULL);
	if(!map_file) {
		fprintf(log_file, "error: Map state '%s' doesn't specify a map file.\n", s->data);
		return 0;
	}
	
	script_file = ini_get(game_ini, s->data, "script", NULL);	
	if(!script_file) {
		fprintf(log_file, "error: Map state '%s' doesn't specify a script file.\n", s->data);
		return 0;
	}
	
	map_text = re_get_script(map_file);
	script = re_get_script(script_file);
	
	the_map = map_parse(map_text);
	if(!the_map) {
		fprintf(log_file, "error: Unable to parse map %s (state %s).\n", map_file, s->data);
		return 0;		
	}
	
	L = luaL_newstate();
	luaL_openlibs(L);
	
	/* TODO: Compile the Lua script. */
	
	/* TODO: You actually need to execute the script here as well, 
		so that callbacks in the script can be registered
	*/
	
	free(map_text);
	free(script);
	
	return 1;
}

static int map_update(struct game_state *s, struct bitmap *bmp) {
	int i;
	
	assert(L);
	assert(the_map);
	
	/* TODO: Maybe background colour metadata in the map file? */
	bm_set_color_s(bmp, "black");
	bm_clear(bmp);
	
	for(i = 0; i < 3; i++)
		map_render(the_map, bmp, i, 0, 0);
	
	if(kb_hit()) {
		change_state(NULL);
		return 0;
	}
	
	return 1;
}

static int map_deinit(struct game_state *s) {
	lua_close(L);
	L = NULL;
	
	map_free(the_map);
	ts_free_all();
	the_map = NULL;
	
	return 1;
}

struct game_state map_state = {
	NULL,
	map_init,
	map_update,
	map_deinit
};