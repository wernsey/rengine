#include <stdio.h>
#include <string.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

#include "bmp.h"
#include "states.h"

static lua_State *L = NULL;

static int ls_init(struct game_state *s) {
	L = luaL_newstate();
	luaL_openlibs(L);
	return 1;
}

static int ls_update(struct game_state *s, struct bitmap *bmp) {
	if(!L) 
		return 0;
	return 1;
}

static int ls_deinit(struct game_state *s) {
	lua_close(L);
	L = NULL;
	return 1;
}

struct game_state ls_state = {
	NULL,
	ls_init,
	ls_update,
	ls_deinit
};