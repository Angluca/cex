#include "cex_code_gen.h"
#include "all.h"
#if defined(CEX_BUILD) || defined(CEX_NEW)

void
_cex__codegen_indent(_cex__codegen_s* cg)
{
    if (unlikely(cg->error != EOK)) { return; }
    for (u32 i = 0; i < cg->indent; i++) {
        Exc err = sbuf.append(cg->buf, " ");
        if (unlikely(err != EOK && cg->error != EOK)) { cg->error = err; }
    }
}

#    define cg$printva(cg) /* temp macro! */                                                       \
        do {                                                                                       \
            va_list va;                                                                            \
            va_start(va, format);                                                                  \
            Exc err = sbuf.appendfva(cg->buf, format, va);                                         \
            if (unlikely(err != EOK && cg->error != EOK)) { cg->error = err; }                     \
            va_end(va);                                                                            \
        } while (0)

void
_cex__codegen_print(_cex__codegen_s* cg, bool rep_new_line, char* format, ...)
{
    if (unlikely(cg->error != EOK)) { return; }
    if (rep_new_line) {
        usize slen = sbuf.len(cg->buf);
        if (slen && cg->buf[0][slen - 1] == '\n') { sbuf.shrink(cg->buf, slen - 1); }
    }
    cg$printva(cg);
}

void
_cex__codegen_print_line(_cex__codegen_s* cg, char* format, ...)
{
    if (unlikely(cg->error != EOK)) { return; }
    if (format[0] != '\n') { _cex__codegen_indent(cg); }
    cg$printva(cg);
}

_cex__codegen_s*
_cex__codegen_print_scope_enter(_cex__codegen_s* cg, char* format, ...)
{
    usize slen = sbuf.len(cg->buf);
    if (slen && cg->buf[0][slen - 1] == '\n') { _cex__codegen_indent(cg); }
    cg$printva(cg);
    _cex__codegen_print(cg, false, "%c\n", '{');
    cg->indent += 4;
    return cg;
}

void
_cex__codegen_print_scope_exit(_cex__codegen_s** cgptr)
{
    uassert(*cgptr != NULL);
    _cex__codegen_s* cg = *cgptr;

    if (cg->indent >= 4) { cg->indent -= 4; }
    _cex__codegen_indent(cg);
    _cex__codegen_print(cg, false, "%c\n", '}');
}


_cex__codegen_s*
_cex__codegen_print_case_enter(_cex__codegen_s* cg, char* format, ...)
{
    _cex__codegen_indent(cg);
    cg$printva(cg);
    _cex__codegen_print(cg, false, ": %c\n", '{');
    cg->indent += 4;
    return cg;
}

void
_cex__codegen_print_case_exit(_cex__codegen_s** cgptr)
{
    uassert(*cgptr != NULL);
    _cex__codegen_s* cg = *cgptr;

    if (cg->indent >= 4) { cg->indent -= 4; }
    _cex__codegen_indent(cg);
    _cex__codegen_print_line(cg, "break;\n", '}');
    _cex__codegen_indent(cg);
    _cex__codegen_print(cg, false, "%c\n", '}');
}

#    undef cg$printva
#endif // #ifdef CEX_BUILD
