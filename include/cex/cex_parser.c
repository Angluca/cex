#include "cex_parser.h"

// NOTE: lx$ are the temporary macro (will be #undef at the end of this file)
#define lx$next(lx)                                                                                \
    ({                                                                                             \
        if (*lx->cur == '\n') {                                                                    \
            lx->line++;                                                                            \
        }                                                                                          \
        *(lx->cur++);                                                                              \
    })
#define lx$rewind(lx)                                                                              \
    ({                                                                                             \
        if (lx->cur > lx->content) {                                                               \
            lx->cur--;                                                                             \
            if (*lx->cur == '\n') {                                                                \
                lx->line++;                                                                        \
            }                                                                                      \
        }                                                                                          \
    })
#define lx$peek(lx) (lx->cur < lx->content_end) ? *lx->cur : '\0'
#define lx$skip_space(lx, c)                                                                       \
    while (c && isspace((c))) {                                                                    \
        lx$next(lx);                                                                               \
        (c) = *lx->cur;                                                                            \
    }

static CexLexer_c
CexLexer_create(char* content, u32 content_len, bool fold_scopes)
{
    uassert(content != NULL);
    if (content_len == 0) {
        content_len = strlen(content);
    }
    CexLexer_c lx = { .content = content,
                      .content_end = content + content_len,
                      .cur = content,
                      .fold_scopes = fold_scopes };
    return lx;
}

static cex_token_s
CexLexer_scan_ident(CexLexer_c* lx)
{
    cex_token_s t = { .type = CexTkn__ident, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        if (!(isalpha(c) || c == '_' || c == '$')) {
            lx$rewind(lx);
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
        switch (c) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '/':
            case '*':
            case '+':
            case '-':
                lx$rewind(lx);
                return t;
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
                    lx$rewind(lx);
                    return t;
                }
                break;
            case '*':
                if (t.type == CexTkn__comment_multi) {
                    if (lx->cur[0] == '/') {
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
    cex_token_s t = { .type = CexTkn__preproc, .value = { .buf = lx->cur, .len = 0 } };

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
CexLexer_scan_scope(CexLexer_c* lx)
{
    cex_token_s t = { .type = CexTkn__unk, .value = { .buf = lx->cur, .len = 0 } };

    if (!lx->fold_scopes) {
        char c = lx$peek(lx);
        switch (c) {
            case '(':
                t.type = CexTkn__lparen;
                break;
            case '[':
                t.type = CexTkn__lbracket;
                break;
            case '{':
                t.type = CexTkn__lbrace;
                break;
            default:
                unreachable();
        }
        t.value.len = 1;
        lx$next(lx);
        return t;
    } else {
        char scope_stack[128] = { 0 };
        u32 scope_depth = 0;

#define scope$push(c) /* temp macro! */                                                            \
    if (++scope_depth < sizeof(scope_stack)) {                                                     \
        scope_stack[scope_depth - 1] = c;                                                          \
    }
#define scope$pop_if(c) /* temp macro! */                                                          \
    if (scope_depth > 0 && scope_depth <= sizeof(scope_stack) &&                                   \
        scope_stack[scope_depth - 1] == c) {                                                       \
        scope_stack[--scope_depth] = '\0';                                                         \
    }

        char c = lx$peek(lx);
        switch (c) {
            case '(':
                t.type = CexTkn__paren_block;
                break;
            case '[':
                t.type = CexTkn__bracket_block;
                break;
            case '{':
                t.type = CexTkn__brace_block;
                break;
            default:
                unreachable();
        }

        while ((c = lx$peek(lx))) {
            switch (c) {
                case '{':
                    scope$push(c);
                    break;
                case '[':
                    scope$push(c);
                    break;
                case '(':
                    scope$push(c);
                    break;
                case '}':
                    scope$pop_if('{');
                    break;
                case ']':
                    scope$pop_if('[');
                    break;
                case ')':
                    scope$pop_if('(');
                    break;
                case '"': 
                case '\'': {
                    var s = CexLexer_scan_string(lx);
                    t.value.len += s.value.len + 2;
                    continue;
                }
                case '/': {
                    if (lx->cur[1] == '/' || lx->cur[1] == '*') {
                        var s = CexLexer_scan_comment(lx);
                        t.value.len += s.value.len;
                        continue;
                    }
                    break;
                }
                case '#': {
                    char* ppstart = lx->cur;
                    var s = CexLexer_scan_preproc(lx);
                    if (s.value.buf){
                        t.value.len += s.value.len + (s.value.buf-ppstart) + 1;
                    }
                    continue;
                }
            }
            t.value.len++;
            lx$next(lx);

            if (scope_depth == 0) {
                return t;
            }
        }

#undef scope$push
#undef scope$pop_if

        if (scope_depth != 0) {
            t.type = CexTkn__error;
            t.value.buf = NULL;
            t.value.len = 0;
        }
        return t;
    }
}

static cex_token_s
CexLexer_next_token(CexLexer_c* lx)
{

#define tok$new(tok_type)                                                                          \
    ({                                                                                             \
        lx$next(lx);                                                                               \
        (cex_token_s){ .type = tok_type, .value = { .buf = lx->cur - 1, .len = 1 } };              \
    })

    char c;
    while ((c = lx$peek(lx))) {
        lx$skip_space(lx, c);
        if (!c) {
            break;
        }

        if (isalpha(c) || c == '_') {
            return CexLexer_scan_ident(lx);
        }
        if (isdigit(c)) {
            return CexLexer_scan_number(lx);
        }

        switch (c) {
            case '\'':
            case '"':
                return CexLexer_scan_string(lx);
            case '/':
                if (lx->cur[1] == '/' || lx->cur[1] == '*') {
                    return CexLexer_scan_comment(lx);
                } else {
                    break;
                }
                break;
            case '*':
                return tok$new(CexTkn__star);
            case '.':
                return tok$new(CexTkn__dot);
            case ',':
                return tok$new(CexTkn__comma);
            case ';':
                return tok$new(CexTkn__eos);
            case ':':
                return tok$new(CexTkn__colon);
            case '?':
                return tok$new(CexTkn__question);
            case '{':
            case '(':
            case '[':
                return CexLexer_scan_scope(lx);
            case '}':
                return tok$new(CexTkn__rbrace);
            case ')':
                return tok$new(CexTkn__rparen);
            case ']':
                return tok$new(CexTkn__rbracket);
            case '#':
                return CexLexer_scan_preproc(lx);
            default:
                break;
        }

        return tok$new(CexTkn__unk);
    }

#undef tok$new
    return (cex_token_s){ 0 }; // EOF
}

#undef lx$next
#undef lx$peek
#undef lx$skip_space
