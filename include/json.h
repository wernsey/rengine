enum json_type {
	j_string,
	j_number,
	j_object,
	j_array,
	j_true,
	j_false,
	j_null
};

struct json {
	enum json_type type;
	void *value;
	
	/* For now, I store arrays in a linked list. Fix later.
	In fairness, my stuff just iterates through the array,
	so the linked list provides some convenience.
	*/
	struct json *next; 
};

char *json_escape(const char *in, char *out, size_t len);

struct json *json_read(const char *filename);

struct json *json_parse(const char *text);

void json_free(struct json *v);

void json_dump(struct json *v);

double json_as_number(struct json *j);

const char *json_as_string(struct json *j);

struct json *json_as_object(struct json *j);

struct json *json_as_array(struct json *j);

struct json *json_get_member(struct json *j, const char *name);

double json_get_number(struct json *j, const char *name);

const char *json_get_string(struct json *j, const char *name);

struct json *json_get_object(struct json *j, const char *name);

struct json *json_get_array(struct json *j, const char *name);

int json_array_len(struct json *j);

struct json *json_array_nth(struct json *j, int n);
