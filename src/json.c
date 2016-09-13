/*
 * Disclaimer: I'm not really making an effort at this point in time
 * to be fully compliant with the JSON specs.
 * For now I just need my own tools to work.
 * 
 * TODO: Run through Valgrind.
 *
 * FIXME: The error handling is not that great.
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <stdarg.h>
#include <assert.h>

#include "json.h"
#include "lexer.h"
#include "hash.h"
#include "utils.h"

#define J_TRUE  256
#define J_FALSE 257
#define J_NULL  258

static void _json_error(const char *fmt, ...) {
	va_list arg;	
	va_start(arg, fmt);
	fputs("error:", stderr);
	vfprintf(stderr, fmt, arg);
	fputc('\n', stderr);
	va_end(arg);
}

void (*json_error)(const char *fmt, ...) = _json_error;

static JSON *json_parse_value(struct lexer *lx);
static JSON *json_parse_array(struct lexer *lx);
static JSON *json_parse_object(struct lexer *lx);

char *json_escape(const char *in, char *out, size_t len) {
	size_t i = 0;
	assert(len > 0);
	while(in[0]) {
		if(i == len - 1) {
			out[i - 1] = 0;
			return &out[i - 1];
		} else {
			
			switch(in[0]) {
				case '\"':
				case '\\':
				case '/':
				case '\b':
				case '\f':
				case '\n':
				case '\r':
				case '\t': {					
					out[i++] = '\\';
					if(i == len - 1) {
						out[i - 1] = 0;
						return &out[i - 1];
					}
					
					switch(in[0]) {
						case '\"': out[i++] = '"'; break;
						case '\\': out[i++] = '\\'; break;
						case '/': out[i++] = '/'; break;
						case '\b': out[i++] = 'b'; break;
						case '\f': out[i++] = 'f'; break;
						case '\n': out[i++] = 'n'; break;
						case '\r': out[i++] = 'r'; break;
						case '\t': out[i++] = 't'; break;
					}
					
				} break;
				/* FIXME: \uXXXX cases for unicode characters */
				default: {
					out[i++] = in[0];
				} break;
			}
			
		}
		in++;
	}
	out[i] = '\0';
	return out;
}

JSON *json_read(const char *filename) {
	JSON *v = NULL;
	char *text = my_readfile(filename);	
	if(!text)
		return NULL;
	v = json_parse(text);
	free(text);
	return v;
}

static void json_free_member(const char *key, void *val) {
	json_free(val);
}

void json_free(JSON *v) {
	switch(v->type) {
		case j_string:
		case j_number: free(v->value); break;
		case j_object: {
			Hash_Tbl *h = v->value;
			 ht_free(h, json_free_member);
		} break;
		case j_array: {
			while(v->value) {
				JSON *t = v->value;
				v->value = t->next;
				json_free(t);
			} 
			
		}
		default: break;
	}
	free(v);
}

static struct lx_keywords json_keywds[] = {
	{"true", J_TRUE},
	{"false", J_FALSE},
	{"null", J_NULL},
	{NULL, 0}};

JSON *json_parse(const char *text) {
	struct lexer * lx = lx_create(text, "{}[]-:,", json_keywds);
	JSON *v = json_parse_object(lx);
	lx_free(lx);
	return v;
}

static JSON *json_parse_object(struct lexer *lx) {
	
	Hash_Tbl *h = ht_create (16);
	JSON *v = malloc(sizeof *v);
	v->type = j_object;
	v->value = h;
	v->next = NULL;
	
	if(!lx_expect(lx, '{')) {
		json_error("line %d: %s", lx_lineno(lx), lx_text(lx));
		free(v);
		return NULL;
	}
	if(lx_sym(lx) != '}') {
		do {
			char *key;
			JSON *value;
			lx_accept(lx, LX_STRING);
			key = strdup(lx_text(lx));
			lx_accept(lx, ':');		
			value = json_parse_value(lx);
			if(!value) {
				free(key);
				free(v);
				return NULL;
			}
			
			ht_put (h, key, value);
			
			free(key);	
			
		} while(lx_accept(lx, ','));
	}
	if(!lx_expect(lx, '}')) {
		json_error("line %d: %s", lx_lineno(lx), lx_text(lx));
		free(v);
		return NULL;
	}
		
	return v;
}

static JSON *json_parse_array(struct lexer *lx) {
	
	JSON *v = malloc(sizeof *v);
	v->type = j_array;
	v->value = NULL;
	v->next = NULL;
	
	JSON *tail = NULL;
	
	if(!lx_expect(lx, '[')) {
		json_error("line %d: %s", lx_lineno(lx), lx_text(lx));
		return NULL;
	}
	if(lx_sym(lx) != ']') {
		do {		
			JSON *value = json_parse_value(lx);
			if(!value) 
				return NULL;
			if(v->value) {
				assert(tail);
				tail->next = value;			
			} else {
				v->value = value;
			}	
			tail = value;
			
		} while(lx_accept(lx, ','));
	}
	if(!lx_expect(lx, ']')) {
		json_error("line %d: %s", lx_lineno(lx), lx_text(lx));
		return NULL;
	}
	
	return v;
}

static JSON *json_parse_value(struct lexer *lx) {
	if(lx_sym(lx) == '{')
		return json_parse_object(lx);	
	else if(lx_sym(lx) == '[')
		return json_parse_array(lx);
	else {
		JSON *v = malloc(sizeof *v);
		v->type = j_null;
		v->value = NULL;
		v->next = NULL;
		if(lx_sym(lx) == LX_NUMBER || lx_sym(lx) == '-') {
			v->type = j_number;
			if(lx_sym(lx) == '-') {
				lx_getsym(lx);
				char *val = malloc(strlen(lx_text(lx)) + 2);
				sprintf(val, "-%s", lx_text(lx));
				v->value = val;
			} else
				v->value = strdup(lx_text(lx));
		} else if(lx_sym(lx) == LX_STRING) {
			v->type = j_string;
			v->value = strdup(lx_text(lx));
		} else if(lx_sym(lx) == J_TRUE) {
			v->type = j_true;
		} else if(lx_sym(lx) == J_FALSE) {
			v->type = j_false;
		}else if(lx_sym(lx) == J_NULL) {
			v->type = j_null;
		} else {
			if(isprint(lx_sym(lx))) {
				json_error("line %d: Unexpected operator: %c", lx_lineno(lx), lx_sym(lx));
			} else {
				json_error("line %d: Unexpected symbol type %d", lx_lineno(lx), lx_sym(lx));
			}
			return NULL;
		}
		lx_getsym(lx);	
		
		return v;
	}
}

static void json_dump_L(JSON *v, int L) {
	char buffer[256];
	switch(v->type) {
		case j_string: {
			printf("\"%s\"", json_escape(v->value, buffer, sizeof buffer));
		} break;
		case j_number: printf("%s", (const char *)v->value); break;
		case j_object: {
			Hash_Tbl *h = v->value;
			const char *key = ht_next (h, NULL);			
			puts("{");
			while(key) {
				printf("%*c\"%s\" : ", L*2 + 1, ' ', json_escape(key, buffer, sizeof buffer));
				json_dump_L(ht_get(h, key), L + 1);
				key = ht_next(h, key);
				if(key) 
					puts(",");
			}
			printf("}");
		} break;
		case j_array: {
			JSON *m = v->value;
			puts("[");
			while(m) {
				printf("%*c", L*2, ' ');
				json_dump_L(m, L + 1);
				m = m->next;
				if(m)
					puts(",");
			}
			printf("]");
		} break;
		case j_true: {
			printf("true");
		} break;
		case j_false: {
			printf("false");
		} break;
		case j_null: {
			printf("null");
		} break;
		default: break;
	}
}

void json_dump(JSON *v) {
	json_dump_L(v, 0);
}

double json_as_number(JSON *j) {
	if(j->type == j_number)
		return atof(j->value);
	return 0.0;
}

const char *json_as_string(JSON *j) {
	if(j->type == j_string)
		return j->value;
	return NULL;
}

JSON *json_as_object(JSON *j) {
	if(j->type == j_object)
		return j;
	return NULL;	
}

JSON *json_as_array(JSON *j) {
	if(j->type == j_array)
		return j;
	return NULL;
}

int json_is_number(JSON *j) {
	return j->type == j_number;
}

int json_is_string(JSON *j) {
	return j->type == j_string;
}

int json_is_object(JSON *j) {
	return j->type == j_object;
}

int json_is_array(JSON *j) {
	return j->type == j_array;
}

JSON *json_get_member(JSON *j, const char *name) {
	Hash_Tbl *h = j->value;
	return ht_get(h, name);
}

double json_get_number(JSON *j, const char *name) {
	JSON *v = json_get_member(j, name);
	if(v) 
		return json_as_number(v);
	return 0.0;
}

const char *json_get_string(JSON *j, const char *name) {
	JSON *v = json_get_member(j, name);
	if(v) 
		return json_as_string(v);
	return NULL;
}

JSON *json_get_object(JSON *j, const char *name) {
	JSON *v = json_get_member(j, name);
	if(v) 
		return json_as_object(v);
	return NULL;	
}

JSON *json_get_array(JSON *j, const char *name) {
	JSON *v = json_get_member(j, name);
	if(v) 
		return json_as_array(v);
	return NULL;
}

int json_array_len(JSON *j) {
	int c = 0;
	if(j->type != j_array)
		return 0;
	j = j->value;
	while(j) {
		c++;
		j = j->next;
	}
	return c;
}

JSON *json_array_nth(JSON *j, int n) {
	int c = 0;
	if(j->type != j_array)
		return 0;
	j = j->value;
	while(j) {
		if(c++ == n)
			return j;
		j = j->next;
	}
	return NULL;
}

/*
gcc -o json -DJTEST json.c lexer.c hash.c utils.c
*/
#ifdef JTEST
int main(int argc, char *argv[]) {
	JSON *j;
	if(argc < 2) 
		return EXIT_FAILURE;
	j = json_read(argv[1]);
	if(!j) {
		json_error("Unable to parse %s", argv[1]);
		return EXIT_FAILURE;
	} else {
		json_dump(j);
		json_free(j);
		return EXIT_SUCCESS;
	}
}
#endif
