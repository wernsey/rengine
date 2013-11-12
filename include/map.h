#ifndef MAP_H
#define MAP_H
#if defined(__cplusplus) || defined(c_plusplus)
extern "C" {
#endif

struct map_tile {
	short si; /* Index of the tile set */
	short ti; /* Tile index within the set */
};

struct map_cell {
	struct map_tile *tiles;
	char *id;
	char *clas;
	int flags; 
};

struct map {
	int nr, nc;
	char dirty;
	
	int nl;
	struct map_cell *cells;
	
	struct tile_collection tiles;
};

struct map *map_create(int nr, int nc, int tw, int th, int nl);

void map_set(struct map *m, int layer, int x, int y, int tsi, int ti);

void map_get(struct map *m, int layer, int x, int y, int *tsi, int *ti);

void map_render(struct map *m, struct bitmap *bmp, int layer, int scroll_x, int scroll_y);

void map_free(struct map *m);

int map_save(struct map *m, const char *filename);

struct map *map_load(const char *filename);

struct map *map_parse(const char *text);

struct map_cell *map_get_cell(struct map *m, int x, int y);

#if defined(__cplusplus) || defined(c_plusplus)
} /* extern "C" */
#endif
#endif /* MAP_H */