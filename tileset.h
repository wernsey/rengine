#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define TS_FLAG_BARRIER	1

#define TS_CLASS_MAXLEN	20

struct tile_meta {
	int num;
	char *clas;
	int flags;
};

struct tileset {
	struct bitmap *bm;	
	
	char *name;
	
	int tw, th;
	int border;
	
	int nmeta;
	struct tile_meta *meta;
};

int ts_add(const char *filename, int tw, int th, int border);

void ts_free_all();

struct tileset *ts_get(int i);

int ts_get_num();

struct tileset *ts_find(const char *name);

int ts_index_of(const char *name);

struct tile_meta *ts_has_meta(struct tileset *t, int row, int col);

struct tile_meta *ts_get_meta(struct tileset *t, int row, int col);

int ts_save_all(const char *filename);

int ts_write_all(FILE *file);

int ts_load_all(const char *filename);

int ts_read_all(const char *text);

int ts_valid_class(const char *clas);
 
#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
