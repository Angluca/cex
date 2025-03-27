#include "all.h"

typedef enum CexTkn_e {
    CexTkn__eof = 0,
    CexTkn__ident,
    CexTkn__number,
    CexTkn__string,
    CexTkn__comment_single,
    CexTkn__comment_multi,
    CexTkn__preproc,
    CexTkn__lparen,
    CexTkn__rparen,
    CexTkn__lbrace,
    CexTkn__rbrace,
    CexTkn__lbracket,
    CexTkn__rbracket,
    CexTkn__unk,
} CexTkn_e;

typedef struct cex_token_s {
    CexTkn_e type;
    str_s value;
} cex_token_s;

typedef struct CexLexer_c {
    char* content;
    char* cur;
    char* content_end;
    u32 line;
    bool fold_scopes;
} CexLexer_c;
