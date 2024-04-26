#include "lexer.h"
#include "dynarr.h"
#include "separators.h"

#include <string.h>

#define SIZEADD_TOKEN 16
#define SIZEADD_LEXSTATE 16

#define QUOTE_CHAR '"'
#define ESCAPING_CHAR '\\'

struct token {
    enum toktype type;
    char *lexeme;
    int len, size;
};

struct lex_state {
    int flags;
    struct token cur;
    struct token *tokens;
    int len, size;
};

static void init_token(struct token *tok)
{
    INIT_DYNARR(tok->lexeme, tok->len, tok->size);
    tok->type = string;
}

static void add_char_token(struct token *tok, char c)
{
    (tok->len)++;
    SIZEMOD_DYNARR(tok->lexeme, char, 
        tok->len, tok->size, SIZEADD_TOKEN);
    (tok->lexeme)[tok->len-1] = c;
    (tok->lexeme)[tok->len] = 0;
}

static void add_cur_lexstate(struct lex_state *state)
{
    SIZEMOD_DYNARR(state->tokens, struct token, 
        state->len, state->size, SIZEADD_LEXSTATE);
    *(state->tokens + state->len) = state->cur;
    (state->len)++;
}

static int is_in_separators(const char *str, int len)
{
    const char *const *tmp = separators;
    while(*tmp) {
        if(strncmp(str, *tmp, len) == 0)
            return 1;
        tmp++;
    } 
    return 0;
}

#define BUFSIZE_SEPADD 10

static int check_sep_add(const struct token *tok, char c)
{
    static char buf[BUFSIZE_SEPADD];
    strncpy(buf, tok->lexeme, tok->len);
    buf[tok->len] = c;
    return is_in_separators(buf, tok->len+1);
}

#define IN_QUOTES 1
#define IN_ESCAPING 2
#define LINE_FINISHED 4

#define ADD_IN_QUOTES(CUR, STATE, C) \
    if(C == QUOTE_CHAR) \
        (STATE)->flags &= ~IN_QUOTES; \
    else \
        add_char_token(CUR, C)

#define ADD_QUOTE(CUR, STATE) \
    add_char_token(CUR, 0); \
    ((CUR)->len)--; \
    (STATE)->flags |= IN_QUOTES

#define ADD_WHITESPACE(CUR, STATE) \
    if((CUR)->lexeme) { \
        add_cur_lexstate(STATE); \
        init_token(CUR); \
    }

#define ADD_SEPARATOR(CUR, STATE, C) \
    ADD_WHITESPACE(CUR, STATE); \
    add_char_token(CUR, C); \
    (CUR)->type = separator


static void add_in_string(struct lex_state *state, char c)
{
    struct token *cur = &state->cur;
    if(c == '\r')
        ;
    else if(state->flags & IN_ESCAPING) {
        add_char_token(cur, c);
        state->flags &= ~IN_ESCAPING;
    } else if(c == ESCAPING_CHAR) {
        state->flags |= IN_ESCAPING;
    } else if(state->flags & IN_QUOTES) {
        ADD_IN_QUOTES(cur, state, c);
    } else if(c == QUOTE_CHAR) {
        ADD_QUOTE(cur, state);
    } else if(c == '\n') {
        ADD_WHITESPACE(cur, state);
        state->flags |= LINE_FINISHED;
    } else if(c == ' ' || c == '\t') {
        ADD_WHITESPACE(cur, state);
    } else if(is_in_separators(&c, 1)) {
        ADD_SEPARATOR(cur, state, c);
    } else 
        add_char_token(cur, c);
}

struct lex_state *init_lex_state()
{
    struct lex_state *res = malloc(sizeof(*res));
    res->flags = 0;
    init_token(&res->cur);
    INIT_DYNARR(res->tokens, res->len, res->size);
    return res;
}

int lexer(struct lex_state *state, const char *add)
{
    const char *start = add;
    while(*add) {
        struct token *cur = &state->cur;
        switch(cur->type) {
        case string:
            add_in_string(state, *add);
            add++;
            break;
        case separator:
            if(check_sep_add(cur, *add)) {
                add_char_token(cur, *add);
                add++;
            } else {
                add_cur_lexstate(state);
                init_token(cur);
            }
        }
        if(state->flags & LINE_FINISHED)
            return add - start;
    }
    return 0;
}

void free_lex_state(struct lex_state *state)
{
    int i;
    for(i = 0; i < state->len; i++) {
        struct token *tok = state->tokens + i;
        free(tok->lexeme);
    }
    free(state->cur.lexeme);
    free(state->tokens);
    free(state);
}

int get_tokens_len(const struct lex_state *state)
{
    return state->len;
}

const char *get_lexeme(const struct lex_state *state, int index)
{
    return (state->tokens + index)->lexeme;
}

enum toktype get_toktype(const struct lex_state *state, int index)
{
    return (state->tokens + index)->type;
}
