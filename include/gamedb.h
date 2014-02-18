
int gdb_new();

void gdb_close();

int gdb_save(const char *filename);

int gdb_load(const char *filename);

void gdb_put(const char *key, const char *value);

const char *gdb_get(const char *key);

int gdb_has(const char *key);

void gdb_local_put(const char *key, const char *value);

const char *gdb_local_get(const char *key);

int gdb_local_has(const char *key);
