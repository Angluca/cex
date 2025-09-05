#pragma once
#include "all.h"
#if defined(CEX_BUILD) || defined(CEX_NEW)

typedef struct _cex__codegen_s
{
    sbuf_c* buf;
    u32 indent;
    Exc error;
} _cex__codegen_s;

/*
 *                  CODE GEN MACROS
 */
#    define cg$var __cex_code_gen
#    define cg$init(out_sbuf)                                                                      \
        _cex__codegen_s cex$tmpname(code_gen) = { .buf = (out_sbuf) };                             \
        _cex__codegen_s* cg$var = &cex$tmpname(code_gen)
#    define cg$is_valid() (cg$var != NULL && cg$var->buf != NULL && cg$var->error == EOK)
#    define cg$indent() ({ cg$var->indent += 4; })
#    define cg$dedent()                                                                            \
        ({                                                                                         \
            if (cg$var->indent >= 4) cg$var->indent -= 4;                                          \
        })

/*
 *                  CODE MACROS
 */
#    define cg$pn(text)                                                                            \
        ((text && text[0] == '\0') ? _cex__codegen_print_line(cg$var, "\n")                        \
                                   : _cex__codegen_print_line(cg$var, "%s\n", text))
#    define cg$pf(format, ...) _cex__codegen_print_line(cg$var, format "\n", __VA_ARGS__)
#    define cg$pa(format, ...) _cex__codegen_print(cg$var, true, format, __VA_ARGS__)

// clang-format off
#define cg$scope(format, ...) \
    for (_cex__codegen_s* cex$tmpname(codegen_scope)  \
                __attribute__ ((__cleanup__(_cex__codegen_print_scope_exit))) =     \
                _cex__codegen_print_scope_enter(cg$var, format, __VA_ARGS__),       \
        *cex$tmpname(codegen_sentinel) = cg$var;                                    \
        cex$tmpname(codegen_sentinel) && cex$tmpname(codegen_scope) != NULL;        \
        cex$tmpname(codegen_sentinel) = NULL)
// clang-format on


#    define cg$func(format, ...) cg$scope(format, __VA_ARGS__)

#    define cg$if(format, ...) cg$scope("if (" format ") ", __VA_ARGS__)
#    define cg$elseif(format, ...)                                                                   \
        cg$pa(" else ", "");                                                                       \
        cg$if(format, __VA_ARGS__)
#    define cg$else()                                                                                \
        cg$pa(" else", "");                                                                        \
        cg$scope(" ", "")


#    define cg$while(format, ...) cg$scope("while (" format ") ", __VA_ARGS__)
#    define cg$for(format, ...) cg$scope("for (" format ") ", __VA_ARGS__)
#    define cg$foreach(format, ...) cg$scope("for$each (" format ") ", __VA_ARGS__)


#    define cg$switch(format, ...) cg$scope("switch (" format ") ", __VA_ARGS__)
#    define cg$case(format, ...)                                                                     \
        for (_cex__codegen_s * cex$tmpname(codegen_scope)                                          \
                                   __attribute__((__cleanup__(_cex__codegen_print_case_exit))) =   \
                 _cex__codegen_print_case_enter(cg$var, "case " format, __VA_ARGS__),              \
                                   *cex$tmpname(codegen_sentinel) = cg$var;                        \
             cex$tmpname(codegen_sentinel) && cex$tmpname(codegen_scope) != NULL;                  \
             cex$tmpname(codegen_sentinel) = NULL)
#    define cg$default()                                                                             \
        for (_cex__codegen_s * cex$tmpname(codegen_scope)                                          \
                                   __attribute__((__cleanup__(_cex__codegen_print_case_exit))) =   \
                 _cex__codegen_print_case_enter(cg$var, "default", NULL),                          \
                                   *cex$tmpname(codegen_sentinel) = cg$var;                        \
             cex$tmpname(codegen_sentinel) && cex$tmpname(codegen_scope) != NULL;                  \
             cex$tmpname(codegen_sentinel) = NULL)


void _cex__codegen_print_line(_cex__codegen_s* cg, char* format, ...);
void _cex__codegen_print(_cex__codegen_s* cg, bool rep_new_line, char* format, ...);
_cex__codegen_s* _cex__codegen_print_scope_enter(_cex__codegen_s* cg, char* format, ...);
void _cex__codegen_print_scope_exit(_cex__codegen_s** cgptr);
_cex__codegen_s* _cex__codegen_print_case_enter(_cex__codegen_s* cg, char* format, ...);
void _cex__codegen_print_case_exit(_cex__codegen_s** cgptr);
void _cex__codegen_indent(_cex__codegen_s* cg);

#endif // ifdef CEX_BUILD
