/*
Game database functions.
The Game database is actually an INI file (technically a
in-memory representation of one) that stores various aspects
that you may want to track during the game.

It has two parts: The global part that stores information that is
accessible from any state in the game (for example the player's name
and amount of gold), and the local part that stores information about
a specific state (for example whether the red door on Level 3 of the
dungeon is unlocked is information that is only relevant in the
"Level 3" state.
*/
int gdb_new();

void gdb_close();

int gdb_save(const char *filename);

int gdb_load(const char *filename);

void gdb_put(const char *key, const char *value);

const char *gdb_get(const char *key);

/* This one returns NULL if the key was not found: */
const char *gdb_get_null(const char *key);

int gdb_has(const char *key);

void gdb_local_put(const char *key, const char *value);

const char *gdb_local_get(const char *key);

/* This one returns NULL if the key was not found: */
const char *gdb_local_get_null(const char *key);

int gdb_local_has(const char *key);
