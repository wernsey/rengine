extern int quit;

extern int virt_width, virt_height;
extern int fps;

struct bitmap *get_screen();

extern struct ini_file *game_ini;

extern int show_cursor;
extern int mouse_x, mouse_y;
extern int mouse_btns, mouse_clck;

/* keys[] tracks the state of the entire keyboard */
extern char keys[];

void reset_keys();

/* Returns true if a key was pressed during the last frame */
int kb_hit();

/* Returns the specific SDL_Scancode of the key that was pressed during the
last frame (if kb_hit() returned true), SDL_SCANCODE_UNKNOWN otherwise */
int readkey();

/* Stores the directory from which the application was run
	(for storing saved games and screenshots in the correct directory) */
extern char initial_dir[];

extern unsigned int frame_counter;
void advanceFrame();

/* Converts a keyboard scancode to an ASCII cheracter. */
int scancode_to_ascii(int code, int shift, int caps);

const char *about_text;

