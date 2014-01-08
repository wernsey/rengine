#include <stdio.h>

#include "bmp.h"

/* There is also a re_get_bmp(const char *filename)
 * defined in rengine/src/resources.c which uses the
 * resource cache and is able to load bitmaps from PAK
 * files and so on. 
 * The reason for this one's existence is to have
 * a re_get_bmp() function that can be linked with the
 * editor, but that doesn't require a lot of other
 * stuff to be linked in.
 */
struct bitmap *re_get_bmp(const char *filename) {
	struct bitmap *bmp = bm_load(filename);
	if(!bmp) {
		rerror("Unable to load bitmap '%s'", filename);
	}	
	return bmp;
}
