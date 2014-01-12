/*
The Game Database:
A database of key-value pairs that is globally avaialable throughout the game engine.
It should become part of the savegame system, but it also serves as a way to
exchange data between game states.
It is meant to store things like high scores, how many gold coins the player
has, whether he's completed a sub-quest and so on.
*/
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "ini.h"
#include "log.h"
#include "gamedb.h"

static struct ini_file *gamedb = NULL;

int gdb_new() {
	ini_free(gamedb);
	gamedb = ini_read(NULL, NULL, NULL);
	return gamedb != NULL;
}

void gdb_close() {
	/* Call this only at the end of the program */
	ini_free(gamedb);
	gamedb = NULL;
}

int gdb_save(const char *filename) {
	assert(gamedb);
	int result = ini_write(gamedb, filename);
	if(result != 1) {
		rerror("Unable to save Game database: %s", ini_errstr(result));
		return 0;
	}
	return 1;
}

int gdb_load(const char *filename) {
	int err, line;
	ini_free(gamedb);
	gamedb = ini_read(filename, &err, &line);
	if(!gamedb) {
		rerror("Unable to load Game database: line %d: %s", line, ini_errstr(err));
		return 0;
	}
	return 1;
}

void gdb_put(const char *key, const char *value) {
	if(!ini_put(gamedb, "data", key, value)) {
		rerror("Unable to insert '%s' = '%s' into game database", key, value);
	}
}


const char *gdb_get(const char *key) {
	return ini_get(gamedb, "data", key, "");
}

int gdb_has(const char *key) {
	return ini_get(gamedb, "data", key, NULL) != NULL;
}

