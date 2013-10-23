
struct game_state {
	
	const char *name;
	
	void *data;
	
	int (*init)(struct game_state *s);
	int (*update)(struct game_state *s, struct bitmap *bmp);
	int (*deinit)(struct game_state *s);
};

/* TODO: There could be an array somewhere to push and pop states. */
extern struct game_state *current_state;

int change_state(struct game_state *next);

int set_state(const char *name);

struct game_state *get_map_state(const char *name); /* mapstate.c */
struct game_state *get_mus_state(const char *name); /* mustate.c */
