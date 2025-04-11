#pragma once
#include "all.h"

#define CexTknList  \
    X(eof) \
    X(unk) \
    X(error) \
    X(ident) \
    X(number) \
    X(string) \
    X(char) \
    X(comment_single) \
    X(comment_multi) \
    X(preproc) \
    X(lparen) \
    X(rparen) \
    X(lbrace) \
    X(rbrace) \
    X(lbracket) \
    X(rbracket) \
    X(bracket_block) \
    X(brace_block) \
    X(paren_block) \
    X(star) \
    X(dot) \
    X(comma) \
    X(eq) \
    X(colon) \
    X(question) \
    X(eos) \
    X(typedef) \
    X(func_decl) \
    X(func_def) \
    X(macro_const) \
    X(macro_func) \
    X(var_decl) \
    X(var_def) \
    X(cex_module_struct) \
    X(cex_module_decl) \
    X(cex_module_def) \
    X(global_misc) \
    X(count) \

typedef enum CexTkn_e {
    #define X(name) CexTkn__##name,
    CexTknList
    #undef X
} CexTkn_e;

static const char* CexTkn_str[] = {
    #define X(name) cex$stringize(name),
    CexTknList
    #undef X
};

typedef struct cex_token_s {
    CexTkn_e type;
    str_s value;
} cex_token_s;

typedef struct CexParser_c {
    char* content;
    char* cur;
    char* content_end;
    u32 line;
    bool fold_scopes;
} CexParser_c;

CexParser_c
CexParser_create(char* content, u32 content_len, bool fold_scopes);

cex_token_s
CexParser_next_token(CexParser_c* lx);

