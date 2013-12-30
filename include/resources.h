
void re_initialize();
void re_clean_up();

void re_push();

void re_pop();

int rs_read_pak(const char *filename);

struct ini_file *re_get_ini(const char *filename);

struct bitmap *re_get_bmp(const char *filename);

Mix_Chunk *re_get_wav(const char *filename);

char *re_get_script(const char *filename);
