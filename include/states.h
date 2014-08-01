
struct game_state {
	
	char *name;
	
	void *data;
	
	int (*init)(struct game_state *s);
	int (*update)(struct game_state *s, struct bitmap *bmp);
	int (*deinit)(struct game_state *s);
	
	struct hash_tbl *styles;
};

const char *get_style(struct game_state *s, const char *name, const char *def);

/* There could be an array somewhere to push and pop states. */
struct game_state *current_state();

int change_state(struct game_state *next);

#if 0
/* As of now, consider these functions deprecated */
int push_state(struct game_state *next);
int pop_state(struct game_state *next);
#endif

struct game_state *get_state(const char *name);
int set_state(const char *name);

void states_initialize();

struct game_state *get_lua_state(const char *name); /* luastate.c */
struct game_state *get_mus_state(const char *name); /* mustate.c */

