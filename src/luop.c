/*
Me learning how to use the Lua API to
expose C structures as objects in Lua scripts.

My references:
http://rubenlaguna.com/wp/2012/12/09/accessing-cpp-objects-from-lua/


gcc -I /usr/local/include luop.c -L/usr/local/lib -llua
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <lua.h>
#include <lauxlib.h>
#include <lualib.h>

/* I'm gonna have an object with
	methods obj:setA(a), obj:setB(b)
	obj:getA() and obj:getB()
	and a constructor Obj.new()
*/
typedef struct {
	int a, b;	
	char *data;
} my_obj;

static int set_a(lua_State *L) {
	my_obj *o = luaL_checkudata(L,1, "MyObj");
	o->a = luaL_checkint(L,2);
	return 0;
}

static int set_b(lua_State *L) {
	my_obj *o = luaL_checkudata(L,1, "MyObj");
	o->b = luaL_checkint(L,2);
	return 0;
}

static int get_a(lua_State *L) {
	my_obj *o = luaL_checkudata(L,1, "MyObj");
	lua_pushnumber(L, o->a);
	return 1;
}

static int get_b(lua_State *L) {
	my_obj *o = luaL_checkudata(L,1, "MyObj");
	lua_pushnumber(L, o->b);
	return 1;
}

static int obj_to_string(lua_State *L) {
	char buffer[128];
	my_obj *o = luaL_checkudata(L,1, "MyObj");
	lua_pushfstring(L, "obj(%d,%d)", o->a, o->b);
	return 1;
}


static int new_obj(lua_State *L) {
	my_obj *o = lua_newuserdata(L, sizeof *o);
	luaL_setmetatable(L, "MyObj");
	o->a = 10;
	o->b = 20;
	o->data = malloc(100);
	return 1;
}

static int gc_obj(lua_State *L) {
	my_obj *o = luaL_checkudata(L,1, "MyObj");
	printf("gc'ing object (%d,%d)\n", o->a, o->b);
	free(o->data);
	return 0;
}

int main(int argc, char *argv[]) {
	
	lua_State *L = luaL_newstate();
	luaL_openlibs(L);
	
	/* Create the metatable for MyObj */
	luaL_newmetatable(L, "MyObj");
	lua_pushvalue(L, -1);
	lua_setfield(L, -2, "__index"); // MyObj.__index = MyObj
	lua_pushcfunction(L, set_a);
	lua_setfield(L, -2, "setA");
	lua_pushcfunction(L, set_b);
	lua_setfield(L, -2, "setB");
	lua_pushcfunction(L, get_a);
	lua_setfield(L, -2, "getA");
	lua_pushcfunction(L, get_b);
	lua_setfield(L, -2, "getB");
	lua_pushcfunction(L, obj_to_string);
	lua_setfield(L, -2, "__tostring");
	
	lua_pushcfunction(L, gc_obj);
	lua_setfield(L, -2, "__gc");
	
	
	
	lua_pushcfunction(L, new_obj);
	lua_setglobal(L, "newObj");
		
	/* Run the script */
	lua_settop(L,0);
	if(luaL_dofile(L, "./samplescript.lua")) {
		fprintf(stderr, "error: %s\n", lua_tostring(L,-1));
		lua_pop(L, 1);
		exit(EXIT_FAILURE);
	}
	
	lua_close(L);
	
	return 0;
}