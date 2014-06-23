
void re_initialize();
void re_clean_up();

#if 0
/* As of now, consider these functions deprecated */
void re_push();
void re_pop();
#endif

int rs_read_pak(const char *filename);

struct ini_file *re_get_ini(const char *filename);

struct bitmap *re_get_bmp(const char *filename);

struct bitmap *re_clone_bmp(struct bitmap *b, const char *newname);

#ifdef _SDL_MIXER_H
Mix_Chunk *re_get_wav(const char *filename);
Mix_Music *re_get_mus(const char *filename);
#endif

char *re_get_script(const char *filename);
