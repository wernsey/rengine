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

/* keys_down[] and keys_up[] tracks whether a key was 
    pressed or released within the last frame.
    They're useful for "debouncing" the keyboard for
    menus and so on (suppose you have to press Esc to get to
    a menu and Esc again to exit the menu, then you don't want
    the menu to flicker because the user held the key for more
    than one frame).
*/
extern char keys_down[];
extern char keys_up[];

void reset_keys();
int kb_hit();

extern unsigned int frame_counter;
void advanceFrame();

