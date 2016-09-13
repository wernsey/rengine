enum json_type {
	j_string,
	j_number,
	j_object,
	j_array,
	j_true,
	j_false,
	j_null
};

typedef struct json {
	enum json_type type;
	void *value;
	
	/* For now, I store arrays in a linked list. Fix later.
	In fairness, my stuff just iterates through the array,
	so the linked list provides some convenience.
	*/
	struct json *next; 
} JSON;

extern void (*json_error)(const char *fmt, ...);

char *json_escape(const char *in, char *out, size_t len);

JSON *json_read(const char *filename);

JSON *json_parse(const char *text);

void json_free(JSON *v);

void json_dump(JSON *v);

/* TODO: Missing boolean, null types */

double json_as_number(JSON *j);

const char *json_as_string(JSON *j);

JSON *json_as_object(JSON *j);

JSON *json_as_array(JSON *j);

int json_is_number(JSON *j);

int json_is_string(JSON *j);

int json_is_object(JSON *j);

int json_is_array(JSON *j);

JSON *json_get_member(JSON *j, const char *name);

double json_get_number(JSON *j, const char *name);

const char *json_get_string(JSON *j, const char *name);

JSON *json_get_object(JSON *j, const char *name);

JSON *json_get_array(JSON *j, const char *name);

int json_array_len(JSON *j);

JSON *json_array_nth(JSON *j, int n);
