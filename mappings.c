#ifdef WIN32
#include <SDL.h>
#else
#include <SDL/SDL.h>
#endif

#include "mappings.h"
#include "utils.h"
#include "bmp.h"

/* FIXME: Unless I can think of anything else that needs to be mapped,
	everything in here can be moved to bmp.c
	(there were functions to map keyboard codes to strings and back in here,
	but that's just because SDL 1.2 didn't have those.)
*/

static int find_mapping(string_int_mapping map[], const char *name, int notfound) {
	int i = 0;
	while(map[i].s) {
		i++;
		if(!my_stricmp(map[i].s, name)) {
			return map[i].i;
		}
	}
	return notfound;
}

static const char *find_mapping_rev(string_int_mapping map[], int value) {
	int i = 0;
	while(map[i].s) {
		i++;
		if(map[i].i == value) {
			return map[i].s;
		}
	}
	return NULL;
}

string_int_mapping font_names[] = {
	{"NORMAL", BM_FONT_NORMAL},
	{"BOLD", BM_FONT_BOLD},
	{"CIRCUIT", BM_FONT_CIRCUIT},
	{"HAND", BM_FONT_HAND},
	{"SMALL", BM_FONT_SMALL},
	{"SMALL_I", BM_FONT_SMALL_I},
	{"THICK", BM_FONT_THICK},
	{NULL, 0}
};

int font_index(const char *name) {
	return find_mapping(font_names, name, BM_FONT_NORMAL);
}

const char *font_name(int index) {
	const char *n = find_mapping_rev(font_names, index);
	if(!n)
		return "NORMAL";
	return n;
}
