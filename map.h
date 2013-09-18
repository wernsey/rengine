#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

struct map_tile {
	char si;   /* Index of the tile set */
	short ti; /* Tile index within the set */	
	char flags;
	char *clas, *id;
};

struct map_layer {
	struct map_tile *tiles;
};

struct map {
	int nr, nc;
	int tw, th;
	char dirty;
	int nl;
	struct map_layer *layers;
};

struct map *map_create(int nr, int nc, int tw, int th, int nl);

void map_set(struct map *m, int layer, int x, int y, int tsi, int ti);

void map_render(struct map *m, struct bitmap *bmp, int layer, int scroll_x, int scroll_y);

void map_free(struct map *m);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
