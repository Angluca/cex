#pragma once
#include "all.h"
#ifdef CEX_BUILD

typedef struct cex_codegen_s
{
    sbuf_c* buf;
    u32 indent;
    Exc error;
} cex_codegen_s;

/*
 *                  CODE GEN MACROS
 */
#define cg$var __cex_code_gen
#define cg$init(out_sbuf)                                                                          \
    cex_codegen_s cex$tmpname(code_gen) = { .buf = (out_sbuf) };                                   \
    cex_codegen_s* cg$var = &cex$tmpname(code_gen)
#define cg$is_valid() (cg$var != NULL && cg$var->buf != NULL && cg$var->error == EOK)
#define cg$indent() ({ cg$var->indent += 4; })
#define cg$dedent()                                                                                \
    ({                                                                                             \
        if (cg$var->indent >= 4)                                                                   \
            cg$var->indent -= 4;                                                                   \
    })

/*
 *                  CODE MACROS
 */
#define $pn(text) cex_codegen_print_line(cg$var, "%s\n", text)
#define $pf(format, ...) cex_codegen_print_line(cg$var, format "\n", __VA_ARGS__)
#define $pa(format, ...) cex_codegen_print(cg$var, true, format, __VA_ARGS__)

// clang-format off
#define $scope(format, ...) \
    for (cex_codegen_s* cex$tmpname(codegen_scope)  \
                __attribute__ ((__cleanup__(cex_codegen_print_scope_exit))) =  \
                cex_codegen_print_scope_enter(cg$var, format, __VA_ARGS__),\
        *cex$tmpname(codegen_sentinel) = cg$var;\
        cex$tmpname(codegen_sentinel); \
        cex$tmpname(codegen_sentinel) = NULL)
#define $func(format, ...) $scope(format, __VA_ARGS__)
// clang-format on


#define $if(format, ...) $scope("if (" format ") ", __VA_ARGS__)
#define $elseif(format, ...)                                                                       \
    $pa(" else ", "");                                                                             \
    $if(format, __VA_ARGS__)
#define $else()                                                                                    \
    $pa(" else", "");                                                                              \
    $scope(" ", "")


#define $while(format, ...) $scope("while (" format ") ", __VA_ARGS__)
#define $for(format, ...) $scope("for (" format ") ", __VA_ARGS__)
#define $foreach(format, ...) $scope("for$each (" format ") ", __VA_ARGS__)


#define $switch(format, ...) $scope("switch (" format ") ", __VA_ARGS__)
#define $case(format, ...)                                                                         \
    for (cex_codegen_s * cex$tmpname(codegen_scope)                                                \
                             __attribute__((__cleanup__(cex_codegen_print_case_exit))) =           \
             cex_codegen_print_case_enter(cg$var, "case " format, __VA_ARGS__),                    \
                             *cex$tmpname(codegen_sentinel) = cg$var;                              \
         cex$tmpname(codegen_sentinel);                                                            \
         cex$tmpname(codegen_sentinel) = NULL)
#define $default()                                                                                 \
    for (cex_codegen_s * cex$tmpname(codegen_scope)                                                \
                             __attribute__((__cleanup__(cex_codegen_print_case_exit))) =           \
             cex_codegen_print_case_enter(cg$var, "default", NULL),                                \
                             *cex$tmpname(codegen_sentinel) = cg$var;                              \
         cex$tmpname(codegen_sentinel);                                                            \
         cex$tmpname(codegen_sentinel) = NULL)


static void cex_codegen_print(cex_codegen_s* cg, bool rep_new_line, const char* format, ...);
static cex_codegen_s* cex_codegen_print_scope_enter(cex_codegen_s* cg, const char* format, ...);
static void cex_codegen_print_scope_exit(cex_codegen_s** cgptr);
static cex_codegen_s* cex_codegen_print_case_enter(cex_codegen_s* cg, const char* format, ...);
static void cex_codegen_print_case_exit(cex_codegen_s** cgptr);

#endif // ifdef CEX_BUILD
