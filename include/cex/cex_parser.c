#include "cex_parser.h"

// NOTE: lx$ are the temporary macro (will be #undef at the end of this file)
#define lx$next(lx)                                                                                \
    ({                                                                                             \
        if (*lx->cur == '\n') {                                                                    \
            lx->line++;                                                                            \
        }                                                                                          \
        *(lx->cur++);                                                                              \
    })
#define lx$peek(lx) *lx->cur

static CexLexer_c
CexLexer_create(char* content, u32 content_len)
{
    uassert(content != NULL);
    if (content_len == 0) {
        content_len = strlen(content);
    }
    CexLexer_c lx = { .content = content, .content_len = content_len, .cur = content };
    return lx;
}

static cex_token_s
CexLexer_scan_ident(CexLexer_c* lx)
{
    cex_token_s t = { .type = CexTkn__ident, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        if (!(isalpha(c) || c == '_' || c == '$')) {
            break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexLexer_scan_number(CexLexer_c* lx)
{
    cex_token_s t = { .type = CexTkn__number, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        if (c == '0') {
            char nc = lx$peek(lx);
            if (nc == 'X' || nc == 'x' || nc == 'b') {
                t.value.len++;
                lx$next(lx);
                while ((nc = lx$next(lx))) {
                    if (isxdigit(nc)) {
                        t.value.len++;
                    } else {
                        return t;
                    }
                }
                continue;
            }
            t.value.len++;
            break;
        } else if (isdigit(c)) {
            t.value.len++;
        } else {
            return t;
        }
    }
    return t;
}

static cex_token_s
CexLexer_next_token(CexLexer_c* lx)
{
    char c;
    while ((c = lx$peek(lx))) {
        if (isspace(c)) {
            lx$next(lx);
            continue;
        }

        if (isalpha(c) || c == '_') {
            return CexLexer_scan_ident(lx);
        }
        if (isdigit(c)) {
            return CexLexer_scan_number(lx);
        }

        cex_token_s t = { .type = CexTkn__unk, .value = { .buf = lx->cur, .len = 0 } };
        lx$next(lx);
        return t;
    }
    return (cex_token_s){ 0 }; // EOF
}

#undef lx$next
#undef lx$peek
