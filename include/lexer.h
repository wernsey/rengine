/* This is a generic lexical analyser intended for 
 * recursive descent parsers as described on the Wikipedia.
 * See http://en.wikipedia.org/wiki/Recursive_descent_parser 
 */

struct lexer;

#define LX_ERROR   -1
#define LX_END      0
#define LX_IDENT    1
#define LX_NUMBER   2
#define LX_STRING   3

struct lx_keywords {
	const char *word;
	int value;
};

struct lexer *lx_create(const char *in, const char *opers, const struct lx_keywords *keywds);

void lx_free(struct lexer *lx);

int lx_accept(struct lexer *lx, int sym);

int lx_expect(struct lexer *lx, int sym);

int lx_sym(struct lexer *lx);

const char *lx_text(struct lexer *lx);

int lx_lineno(struct lexer *lx);

int lx_getsym(struct lexer *lx);
