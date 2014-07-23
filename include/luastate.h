
#define MAX_TIMEOUTS 20

/* Don't tamper with this variable from your Lua scripts. */
#define STATE_DATA_VAR	"___state_data"

#define GLOBAL_FUNCTION(name, fun)	lua_pushcfunction(L, fun); lua_setglobal(L, name);
#define SET_TABLE_INT_VAL(k, v)     lua_pushstring(L, k); lua_pushinteger(L, v); lua_rawset(L, -3);

struct callback_function {
	int ref;
	struct callback_function *next;
};

struct lustate_data {
	
	struct game_state *state;
	
	struct bitmap *bmp;
	struct map *map;	

	struct _timeout_element {
		int fun;
		int time;
		Uint32 start;
	} timeout[MAX_TIMEOUTS];
	int n_timeout;
	
	struct callback_function *update_fcn, *last_fcn;	
	struct callback_function *atexit_fcn;
	
	int change_state;
	char *next_state;
};

struct lustate_data *get_state_data(lua_State *L);
void process_timeouts(lua_State *L);
