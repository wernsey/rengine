enum json_type {
	j_string,
	j_number,
	j_object,
	j_array,
	j_true,
	j_false,
	j_null
};

struct json_value {
	enum json_type type;
	void *value;
	
	/* For now, I store arrays in a linked list. Fix later */
	struct json_value *next; 
};

char *json_escape(const char *in, char *out, size_t len);

struct json_value *json_read(const char *filename);

struct json_value *json_parse(const char *text);

void json_free(struct json_value *v);

void json_dump(struct json_value *v);
