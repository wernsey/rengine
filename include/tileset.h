#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

#define TS_FLAG_BARRIER	1

#define TS_CLASS_MAXLEN	20

struct tile_meta {
	int ti;
	char *clas;
	int flags;
};

struct tileset {
	struct bitmap *bm;	
	
	char *name;
	
	int nmeta;
	struct tile_meta *meta;
};

struct tile_collection {
	int tw, th;
	int border;
	
	struct tileset **tilesets;
	int ntilesets;
};

void ts_init(struct tile_collection *tc, int tw, int th, int border);

int ts_add(struct tile_collection *tc, const char *filename);

void ts_deinit(struct tile_collection *tc);

struct tileset *ts_get(struct tile_collection *tc, int i);

int ts_get_num(struct tile_collection *tc);

struct tileset *ts_find(struct tile_collection *tc, const char *name);

int ts_index_of(struct tile_collection *tc, const char *name);

struct tile_meta *ts_has_meta_ti(struct tile_collection *tc, struct tileset *t, int ti);

struct tile_meta *ts_has_meta(struct tile_collection *tc, struct tileset *t, int row, int col);

struct tile_meta *ts_get_meta(struct tile_collection *tc, struct tileset *t, int row, int col);

int ts_save_all(struct tile_collection *tc, const char *filename);

int ts_write_all(struct tile_collection *tc, FILE *file);

int ts_load_all(struct tile_collection *tc, const char *filename);

struct json; /* see json.h */
int ts_read_all(struct tile_collection *tc, struct json *j);

int ts_valid_class(const char *clas);
 
#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
