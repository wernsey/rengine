/*
 * This was just a little test program to test my modifications to
 * bmp.c and ini.c to be able to read BMP and INI files from PAK
 * archives.
 */
#include <stdio.h>
#include <stdlib.h>

#include "pak.h"
#include "bmp.h"
#include "ini.h"

int main(int argc, char *argv[]) {
	struct pak_file *p;
	FILE *f;
	struct bitmap *b;
	
	char *text;
	ini_file * ini; 
	int err, line;

	pak_verbose = 2;
	
	p = pak_open("test.pak");
	if(!p) {
		fprintf(stderr, "error: unable to open test.pak\n");
		return 1;
	}
	
	f = pak_get_file(p, "save.bmp");
	if(!f) {
		fprintf(stderr, "error: unable to locate save.bmp in test.pak\n");
		return 1;
	}
	
	b = bm_load_fp(f);
	
	bm_set_color_s(b, "white");
	bm_line(b, 0, 0, b->w, b->h);
	bm_save(b, "pakbmp.bmp");
	bm_free(b);
	
	text = pak_get_text(p, "game.ini");	
	if(!f) {
		fprintf(stderr, "error: unable to locate game.ini in test.pak\n");
		return 1;
	}
	
	printf("===========\n%s\n===========\n", text);
	
	ini = ini_parse(text, &err, &line);
	if(!ini) {
		fprintf(stderr, "error: ini: %d: %s\n", line, ini_errstr(err));
		return 1;
	}
	
	free(text);
	
	printf("== DUMP: ==\n");
	ini_write(ini, NULL);
	printf("===========\n");
	
	pak_close(p);
	
	return 0;
}
