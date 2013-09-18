#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "lexer.h"

#define MAX_BUFFER	512

struct lexer {
	const char *in;
	int sym;
	int lineno;
	const char *opers;
	const struct lx_keywords *keywds;
	char text[MAX_BUFFER];
};

struct lexer *lx_create(const char *in, const char *opers, const struct lx_keywords *keywds) {
	struct lexer *lx = malloc(sizeof *lx);
	if(!lx) return NULL;
	lx->in = in;
	lx->sym = LX_ERROR;
	lx->lineno = 1;
	lx->opers = opers?opers:"";
	lx->keywds = keywds;
	
	lx_getsym(lx); /* Load the first symbol. */
	
	return lx;
}

void lx_free(struct lexer *lx) {
	free(lx);
}

int lx_accept(struct lexer *lx, int sym) {
	if(lx->sym == sym) {
		lx_getsym(lx);
		return 1;
	}
	return 0;
}

int lx_expect(struct lexer *lx, int sym) {
	if(lx_accept(lx, sym))
		return 1;
	snprintf(lx->text, MAX_BUFFER, "unexpected symbol");
	return 0;
}

int lx_sym(struct lexer *lx) {
	return lx->sym;
}

const char *lx_text(struct lexer *lx) {
	return lx->text;
}

int lx_lineno(struct lexer *lx) {
	return lx->lineno;
}

int lx_getsym(struct lexer *lx) {
	
start:
	if(lx->in[0] == '\0') 
		return (lx->sym = LX_END);
	
	while(isspace(lx->in[0])) {
		if(lx->in[0] == '\n')
			lx->lineno++;
		lx->in++;
	}
	
	if(lx->in[0] == '#') {
		while(lx->in[0] != '\n' && lx->in[0] != '\0')
			lx->in++;
		goto start;
	}
	
	if(isalpha(lx->in[0]) || lx->in[0] == '_') {
		int i = 0;
		lx->sym = LX_IDENT;
		while(isalnum(lx->in[0]) || lx->in[0] == '_') {
			lx->text[i++] = *lx->in++;
			if(i == MAX_BUFFER - 1) {
				snprintf(lx->text, MAX_BUFFER, "identifier too long");				
				return (lx->sym = LX_ERROR);
			}
		}
		lx->text[i] = '\0';
		
		if(lx->keywds) {
			int i = 0;
			while(lx->keywds[i].word != NULL) {
				if(!strcmp(lx->text, lx->keywds[i].word)) {
					lx->sym = lx->keywds[i].value;
					break;
				}
				i++;
			}
		}
		
	} else if(isdigit(lx->in[0])) {
		/* Note that we don't handle negative numbers here.
			you have to do that at a higher level.
		*/
		int i = 0;
		lx->sym = LX_NUMBER;
		while(isdigit(lx->in[0])) {
			lx->text[i++] = *lx->in++;
			if(i == MAX_BUFFER - 1) {
				snprintf(lx->text, MAX_BUFFER, "number too long");				
				return (lx->sym = LX_ERROR);
			}
		}
		if(lx->in[0] == '.') {
			lx->text[i++] = *lx->in++;
			if(i == MAX_BUFFER - 1) {
				snprintf(lx->text, MAX_BUFFER, "number too long");				
				return (lx->sym = LX_ERROR);
			}
			while(isdigit(lx->in[0])) {
				lx->text[i++] = *lx->in++;
				if(i == MAX_BUFFER - 1) {
					snprintf(lx->text, MAX_BUFFER, "number too long");				
					return (lx->sym = LX_ERROR);
				}
			}
		}
		if(tolower(lx->in[0]) == 'e') {
			lx->text[i++] = *lx->in++;
			if(i == MAX_BUFFER - 1) {
				snprintf(lx->text, MAX_BUFFER, "number too long");				
				return (lx->sym = LX_ERROR);
			}
			if(strchr("+-", lx->in[0])) {
				lx->text[i++] = *lx->in++;
				if(i == MAX_BUFFER - 1) {
					snprintf(lx->text, MAX_BUFFER, "number too long");				
					return (lx->sym = LX_ERROR);
				}
			}
			while(isdigit(lx->in[0])) {
				lx->text[i++] = *lx->in++;
				if(i == MAX_BUFFER - 1) {
					snprintf(lx->text, MAX_BUFFER, "number too long");				
					return (lx->sym = LX_ERROR);
				}
			}
		}		
		lx->text[i] = '\0';
	} else if(lx->in[0] == '"') {
		int i = 0;
		lx->in++;
		while(lx->in[0] != '"') {
			if(i == MAX_BUFFER - 1) {
				snprintf(lx->text, MAX_BUFFER, "string literal too long");				
				return (lx->sym = LX_ERROR);
			}
			switch(lx->in[0]) {
				case '\0' : {
						snprintf(lx->text, MAX_BUFFER, "unterminated string literal");				
						return (lx->sym = LX_ERROR);
					} 
				case '\\' : {
						lx->in++;
						switch(lx->in[0]) {
							case '\0' : {
									snprintf(lx->text, MAX_BUFFER, "unterminated string literal");				
									return (lx->sym = LX_ERROR);
								}
							case '/' : lx->text[i++] = '/'; break;
							case '\\' : lx->text[i++] = '\\'; break;
							case 'b' : lx->text[i++] = '\b'; break;
							case 'f' : lx->text[i++] = '\f'; break;
							case 'n' : lx->text[i++] = '\n'; break;
							case 'r' : lx->text[i++] = '\r'; break;
							case 't' : lx->text[i++] = '\t'; break;
							case '"' : lx->text[i++] = '"'; break;
							default: {
									snprintf(lx->text, MAX_BUFFER, "bad escape sequence \\%c", lx->in[0]);				
									return (lx->sym = LX_ERROR);
								}
						}						
					} break;
				default : {					
						lx->text[i++] = lx->in[0];
						if(lx->in[0] == '\n') {
							lx->lineno++;
						}
					} break;
			} 
			lx->in++;
		}
		lx->text[i] = '\0';
		lx->sym = LX_STRING;
		lx->in++;
	} else if(strchr(lx->opers, lx->in[0])) {
		return (lx->sym = *lx->in++);
	} else {
		snprintf(lx->text, MAX_BUFFER, "Unexpected token '%c'", lx->in[0]);
		return (lx->sym = LX_ERROR);
	}
	
	return lx->sym;
}

#ifdef TEST
const char *in = "  matrix foo bar   \n"
					" f_00 _quux\n"
					" # This is a comment\n"
					"vec: [5 15.5 15]\t 123.456\n"
					"\"string\\tin \\\"quotes\\\"\"\n"
					"3 {} 4\n"
					"\"Another\nString\"\n"
					"vertex { 5 6 7.6e+3 12.3e-2 }\n";
					
struct lx_keywords keywds[] = {
	{"vertex", 256},
	{"triangle", 257},
	{"matrix", 258},
	{NULL, 0}};

int main(int argc, char *argv[]) {
	struct lexer *lx = lx_create(in, "{[]}:", keywds);
		
	printf("Instring:\n%s\n-------------\n", in);
	
	while(lx_sym(lx) != LX_END) {
		switch(lx_sym(lx)) {
			case LX_ERROR: fprintf(stderr, "error:%d: %s\n", lx_lineno(lx), lx_text(lx)); goto end;
			case LX_IDENT: printf("%3d: IDENT: %s\n", lx_lineno(lx), lx_text(lx)); break;
			case LX_NUMBER: printf("%3d: NUMBER: %g\n", lx_lineno(lx), atof(lx_text(lx))); break;
			case LX_STRING: printf("%3d: STRING: \"%s\"\n", lx_lineno(lx), lx_text(lx)); break;
			case 256: printf("%3d: vector\n", lx_lineno(lx)); break;
			case 257: printf("%3d: triangle\n", lx_lineno(lx)); break;
			case 258: printf("%3d: matrix\n", lx_lineno(lx)); break;
			default: {
				if(isprint(lx_sym(lx))) {
					printf("%3d: OPERATOR: %c\n", lx_lineno(lx), lx_sym(lx)); break;
				} else {
					fprintf(stderr, "error:%d: Unexpected symbol type %d\n", lx_lineno(lx), lx_sym(lx));
					break;
				}
			}
		}
		lx_getsym(lx);
	}
	
end:	
	lx_free(lx);
	return 0;
}
#endif
