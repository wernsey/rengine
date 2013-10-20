
void re_initialize();
void re_clean_up();

int rs_read_pak(const char *filename);

struct ini_file *re_get_ini(const char *filename);

/* FIXME: 
	Loaded bitmaps should be cached.
	There should also be a function to clear the cache.
*/
struct bitmap *re_get_bmp(const char *filename);

char *re_get_script(const char *filename);
