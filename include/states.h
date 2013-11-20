
struct game_state {
	
	const char *name;
	
	void *data;
	
	int (*init)(struct game_state *s);
	int (*update)(struct game_state *s, struct bitmap *bmp);
	int (*deinit)(struct game_state *s);
	
	/* Styles for GUI elements when drawing the state */
	struct {
		const char *fg;
		const char *bg;
		
		int margin;
		int padding;
		
		int btn_padding;
		int btn_border_radius;
		
		int border;
		int border_radius;
		const char *border_color;
		
		enum bm_fonts font;
		
		struct bitmap *bmp;
		const char *image_align;
		const char *image_trans;
		int image_margin;
		
	} style;
};

/* There could be an array somewhere to push and pop states. */
struct game_state *current_state();

int change_state(struct game_state *next);

int push_state(struct game_state *next);
int pop_state(struct game_state *next);

int set_state(const char *name);

void states_initialize();

struct game_state *get_lua_state(const char *name); /* luastate.c */
struct game_state *get_mus_state(const char *name); /* mustate.c */

