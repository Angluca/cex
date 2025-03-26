#include "cex_parser.h"

// NOTE: lx$ are the temporary macro (will be #undef at the end of this file)
#define lx$next(lx)                                                                                \
    ({                                                                                             \
        if (*lx->cur == '\n') {                                                                    \
            lx->line++;                                                                            \
        }                                                                                          \
        *(lx->cur++);                                                                              \
    })
#define lx$peek(lx) (lx->cur < lx->content_end ) ? *lx->cur : '\0'
#define lx$skip_space(lx, c) while(c && isspace((c))){ lx$next(lx); (c) = *lx->cur; }

static CexLexer_c
CexLexer_create(char* content, u32 content_len)
{
    uassert(content != NULL);
    if (content_len == 0) {
        content_len = strlen(content);
    }
    CexLexer_c lx = { .content = content, .content_end = content + content_len, .cur = content };
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
        if (isspace(c)) {
            break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexLexer_scan_string(CexLexer_c* lx)
{
    cex_token_s t = { .type = CexTkn__string, .value = { .buf = lx->cur + 1, .len = 0 } };
    lx$next(lx);
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case '\\': // escape char, unconditionally skip next
                lx$next(lx);
                t.value.len++;
                break;
            case '"':
                return t;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexLexer_scan_comment(CexLexer_c* lx)
{
    cex_token_s t = { .type = lx->cur[1] == '/' ? CexTkn__comment_single : CexTkn__comment_multi,
                      .value = { .buf = lx->cur, .len = 2 } };
    lx$next(lx);
    lx$next(lx);
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case '\n':
                if (t.type == CexTkn__comment_single) {
                    return t;
                }
                break;
            case '*':
                if (t.type == CexTkn__comment_multi) {
                    if (lx->cur[0] == '/'){
                        lx$next(lx);
                        t.value.len += 2;
                        return t;
                    }
                }
                break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexLexer_scan_preproc(CexLexer_c* lx)
{
    lx$next(lx);
    char c = *lx->cur;
    lx$skip_space(lx, c);
    cex_token_s t = { .type = CexTkn__preproc,
                      .value = { .buf = lx->cur, .len = 0 } };

    while ((c = lx$next(lx))) {
        switch (c) {
            case '\n':
                return t;
            case '\\':
                // next line concat for #define
                lx$next(lx);
                t.value.len++;
                break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexLexer_next_token(CexLexer_c* lx)
{
    char c;
    while ((c = lx$peek(lx))) {
        lx$skip_space(lx, c);

        if (isalpha(c) || c == '_') {
            return CexLexer_scan_ident(lx);
        }
        if (isdigit(c)) {
            return CexLexer_scan_number(lx);
        }

        switch (c) {
            case '"':
                return CexLexer_scan_string(lx);
            case '/':
                if (lx->cur[1] == '/' || lx->cur[1] == '*') {
                    return CexLexer_scan_comment(lx);
                } else {
                    uassert(false && "TODO: division");
                }
                break;
            case '#':
                return CexLexer_scan_preproc(lx);
            default:
                break;
        }

        cex_token_s t = { .type = CexTkn__unk, .value = { .buf = lx->cur, .len = 0 } };
        lx$next(lx);
        return t;
    }
    return (cex_token_s){ 0 }; // EOF
}

#undef lx$next
#undef lx$peek
#undef lx$skip_space
