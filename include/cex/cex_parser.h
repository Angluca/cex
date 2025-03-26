#include "all.h"

typedef enum CexTkn_e {
    CexTkn__eof = 0,
    CexTkn__ident,
    CexTkn__number,
    CexTkn__unk,
} CexTkn_e;

typedef struct cex_token_s {
    CexTkn_e type;
    str_s value;
} cex_token_s;

typedef struct CexLexer_c {
    char* content;
    char* cur;
    u32 content_len;
    u32 line;
} CexLexer_c;
