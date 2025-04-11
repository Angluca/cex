#include "CexParser.h"

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

CexParser_c
CexParser_create(char* content, u32 content_len, bool fold_scopes)
{
    uassert(content != NULL);
    if (content_len == 0) {
        content_len = strlen(content);
    }
    CexParser_c lx = { .content = content,
                       .content_end = content + content_len,
                       .cur = content,
                       .fold_scopes = fold_scopes };
    return lx;
}

static cex_token_s
CexParser__scan_ident(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__ident, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        if (!(isalnum(c) || c == '_' || c == '$')) {
            lx$rewind(lx);
            break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexParser__scan_number(CexParser_c* lx)
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
CexParser__scan_string(CexParser_c* lx)
{
    cex_token_s t = { .type = (*lx->cur == '"' ? CexTkn__string : CexTkn__char),
                      .value = { .buf = lx->cur + 1, .len = 0 } };
    lx$next(lx);
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case '\\': // escape char, unconditionally skip next
                lx$next(lx);
                t.value.len++;
                break;
            case '"':
            case '\'':
                return t;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexParser__scan_comment(CexParser_c* lx)
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
CexParser__scan_preproc(CexParser_c* lx)
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
CexParser__scan_scope(CexParser_c* lx)
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
                    var s = CexParser__scan_string(lx);
                    t.value.len += s.value.len + 2;
                    continue;
                }
                case '/': {
                    if (lx->cur[1] == '/' || lx->cur[1] == '*') {
                        var s = CexParser__scan_comment(lx);
                        t.value.len += s.value.len;
                        continue;
                    }
                    break;
                }
                case '#': {
                    char* ppstart = lx->cur;
                    var s = CexParser__scan_preproc(lx);
                    if (s.value.buf) {
                        t.value.len += s.value.len + (s.value.buf - ppstart) + 1;
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

cex_token_s
CexParser_next_token(CexParser_c* lx)
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

        if (isalpha(c) || c == '_' || c == '$') {
            return CexParser__scan_ident(lx);
        }
        if (isdigit(c)) {
            return CexParser__scan_number(lx);
        }

        switch (c) {
            case '\'':
            case '"':
                return CexParser__scan_string(lx);
            case '/':
                if (lx->cur[1] == '/' || lx->cur[1] == '*') {
                    return CexParser__scan_comment(lx);
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
            case '=': {
                if (lx->cur > lx->content) {
                    switch (lx->cur[-1]) {
                        case '=':
                        case '*':
                        case '/':
                        case '~':
                        case '!':
                        case '+':
                        case '-':
                        case '>':
                        case '<':
                            goto unkn;
                        default:
                            break;
                    }
                }
                switch (lx->cur[1]) {
                    case '=':
                        goto unkn;
                    default:
                        break;
                }

                return tok$new(CexTkn__eq);
            }
            case '{':
            case '(':
            case '[':
                return CexParser__scan_scope(lx);
            case '}':
                return tok$new(CexTkn__rbrace);
            case ')':
                return tok$new(CexTkn__rparen);
            case ']':
                return tok$new(CexTkn__rbracket);
            case '#':
                return CexParser__scan_preproc(lx);
            default:
                break;
        }


    unkn:
        return tok$new(CexTkn__unk);
    }

#undef tok$new
    return (cex_token_s){ 0 }; // EOF
}

cex_token_s
CexParser_next_entity(CexParser_c* lx, arr$(cex_token_s) * children)
{
    uassert(lx->fold_scopes && "CexParser must be with fold_scopes=true");
    uassert(children != NULL && "non initialized arr$");
    uassert(*children != NULL && "non initialized arr$");
    cex_token_s result = { 0 };

    arr$clear(*children);
    cex_token_s t;
    cex_token_s* prev_t = NULL;
    bool has_cex_namespace = false;
    u32 i = 0;
    (void)i;
    while ((t = CexParser_next_token(lx)).type) {
#ifdef CEXTEST
        log$debug("%02d: %-15s %S\n", i, CexTkn_str[t.type], t.value);
#endif
        if (unlikely(!result.type)) {
            result.type = CexTkn__global_misc;
            result.value = t.value;
            if (t.type == CexTkn__preproc) {
                result.value.buf--;
                result.value.len++;
            }
        } else {
            // Extend token text
            result.value.len = t.value.buf - result.value.buf + t.value.len;
            uassert(result.value.len < 1024 * 1024 && "token diff it too high, bad pointers?");
        }
        arr$push(*children, t);
        switch (t.type) {
            case CexTkn__preproc: {
                if (str.slice.match(t.value, "define *\\(*\\)*")) {
                    result.type = CexTkn__macro_func;
                } else if (str.slice.match(t.value, "define *")) {
                    result.type = CexTkn__macro_const;
                } else {
                    result.type = CexTkn__preproc;
                }
                goto end;
            }
            case CexTkn__paren_block: {
                if (result.type == CexTkn__var_decl) {
                    if (!str.slice.match(t.value, "\\(\\(*\\)\\)")) {
                        result.type = CexTkn__func_decl; // Check if not __attribute__(())
                    }
                } else {
                    if (prev_t && prev_t->type == CexTkn__ident) {
                        if (!str.slice.match(prev_t->value, "__attribute__")) {
                            result.type = CexTkn__func_decl;
                        }
                    }
                }
                break;
            }
            case CexTkn__brace_block: {
                if (result.type == CexTkn__func_decl) {
                    result.type = CexTkn__func_def;
                    goto end;
                }
                break;
            }
            case CexTkn__eq: {
                if (has_cex_namespace) {
                    result.type = CexTkn__cex_module_def;
                } else {
                    result.type = CexTkn__var_def;
                }
                break;
            }

            case CexTkn__ident: {
                if (str.slice.match(t.value, "(typedef|struct|enum|union)")) {
                    if (result.type != CexTkn__var_decl) {
                        result.type = CexTkn__typedef;
                    }
                } else if (str.slice.match(t.value, "extern")) {
                    result.type = CexTkn__var_decl;
                } else if (str.slice.match(t.value, "__cex_namespace__*")) {
                    has_cex_namespace = true;
                    if (result.type == CexTkn__var_decl) {
                        result.type = CexTkn__cex_module_decl;
                    } else if (result.type == CexTkn__typedef) {
                        result.type = CexTkn__cex_module_struct;
                    }
                }
                break;
            }
            case CexTkn__eos: {
                goto end;
            }
            default: {
            }
        }
        i++;
        prev_t = &(*children)[arr$len(*children) - 1];
    }
end:
    return result;
}

void
CexParser_decl_free(cex_decl_s* decl, IAllocator alloc)
{
    (void)alloc; // maybe used in the future
    if (decl) {
        sbuf.destroy(&decl->args);
        sbuf.destroy(&decl->ret_type);
    }
}

cex_decl_s*
CexParser_decl_parse(cex_token_s decl_token, arr$(cex_token_s) children, IAllocator alloc)
{
    (void)children;
    switch (decl_token.type) {
        case CexTkn__func_decl:
        case CexTkn__func_def:
        case CexTkn__macro_func:
        case CexTkn__macro_const:
            break;
        default:
            return NULL;
    }
    cex_decl_s* result = mem$new(alloc, cex_decl_s);
    if (decl_token.type == CexTkn__func_decl || decl_token.type == CexTkn__func_def) {
        result->args = sbuf.create(128, alloc);
        result->ret_type = sbuf.create(128, alloc);
    }
    result->type = decl_token.type;
    const char* ignore_pattern =
        "(__attribute__|static|inline|__asm__|extern|volatile|restrict|register|__declspec)";

    cex_token_s prev_t = { 0 };
    bool prev_skipped = false;
    for$each(it, children)
    {
        if (it.type == CexTkn__ident) {
            if (str.slice.eq(it.value, str$s("static"))) {
                result->is_static = true;
            }
            if (str.slice.eq(it.value, str$s("inline"))) {
                result->is_inline = true;
            }
            if (str.slice.match(it.value, ignore_pattern)) {
                prev_skipped = true;
                // GCC attribute
                continue;
            }
        } else if (it.type == CexTkn__bracket_block) {
            if (str.slice.starts_with(it.value, str$s("[["))) {
                // Clang/C23 attribute
                continue;
            }
        } else if (it.type == CexTkn__comment_multi || it.type == CexTkn__comment_single) {
            if (result->name.buf == NULL && sbuf.len(&result->ret_type) == 0 &&
                sbuf.len(&result->args) == 0) {
                result->docs = it.value;
            }
        } else if (it.type == CexTkn__brace_block) {
            result->body = it.value;
        } else if (it.type == CexTkn__paren_block) {
            if (prev_skipped) {
                prev_skipped = false;
                continue;
            }

            if (prev_t.type == CexTkn__ident) {
                result->name = prev_t.value;
            }

            if (it.value.len > 2) {
                // strip initial ()
                CexParser_c lx = CexParser_create(it.value.buf + 1, it.value.len - 2, true);
                cex_token_s t = { 0 };
                bool skip_next = false;
                while ((t = CexParser_next_token(&lx)).type) {
#ifdef CEXTEST
                    log$debug("arg token: type: %s '%S'\n", CexTkn_str[t.type], t.value);
#endif
                    if (t.type == CexTkn__ident && str.slice.match(t.value, ignore_pattern)) {
                        skip_next = true;
                        continue;
                    }
                    if (t.type == CexTkn__paren_block && skip_next) {
                        skip_next = false;
                        continue;
                    }
                    if (str.slice.starts_with(t.value, str$s("[["))) {
                        continue;
                    }

                    switch (t.type) {
                        case CexTkn__eof:
                        case CexTkn__comma:
                        case CexTkn__star:
                        case CexTkn__rbracket:
                        case CexTkn__lbracket:
                            break;
                        default:
                            if (sbuf.len(&result->args) > 0) {
                                e$goto(sbuf.append(&result->args, " "), fail);
                            }
                    }

                    e$goto(sbuf.appendf(&result->args, "%S", t.value), fail);

                    skip_next = false;
                }
            }
        }
        prev_skipped = false;
        prev_t = it;
    }

    return result;

fail:
    CexParser_decl_free(result, alloc);
    return NULL;
}


#undef lx$next
#undef lx$peek
#undef lx$skip_space
