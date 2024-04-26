#ifndef LEXER_H_SENTRY
#define LEXER_H_SENTRY

/* This lexical analyzer saves tokens in 
 * struct lex_state every time you call
 * lexer with new string. You can add some
 * strings in separators.h, lexer will generate
 * for them separate tokens. 
 * It provides escaping symbol '\' symbol and
 * strings in quotes '"' */

enum toktype {
    string,
    separator
};

struct lex_state *init_lex_state();

/* This function generates tokens to lex_state 
 * from the string add.
 * It will end when lex_state has the correct full line, 
 * than it returns length of the read part of the add parameter.
 * It can read the whole string and doesn't get the correct line,
 * than it returns 0 */
int lexer(struct lex_state *state, const char *add);

void free_lex_state(struct lex_state *state);

int get_tokens_len(const struct lex_state *state);

const char *get_lexeme(const struct lex_state *state, int index);

enum toktype get_toktype(const struct lex_state *state, int index);

#endif
