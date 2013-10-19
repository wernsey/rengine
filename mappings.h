typedef struct {
	const char *s;
	int i;
} string_int_mapping;

extern string_int_mapping font_names[];

int font_index(const char *name);

const char *font_name(int index);
