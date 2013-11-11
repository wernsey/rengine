extern int quit;

extern int fps;

extern struct ini_file *game_ini;

extern int mouse_x, mouse_y;
extern int mouse_btns, mouse_clck;

extern char keys[];

void reset_keys();
int kb_hit();

void advanceFrame();

