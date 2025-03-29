/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#pragma once
#include "all.h"

struct argparse_c;
struct argparse_opt_s;

typedef Exception (*argparse_callback_f)(
    struct argparse_c* self,
    const struct argparse_opt_s* option,
    void* ctx
);
typedef Exception (*argparse_convert_f)(const char* s, void* out_val);


enum argparse_option_flags
{
    ARGPARSE_OPT_NONEG = 1, /* disable negation */
};
/**
 *  argparse option
 *
 *  `type`:
 *    holds the type of the option
 *
 *  `short_name`:
 *    the character to use as a short option name, '\0' if none.
 *
 *  `long_name`:
 *    the long option name, without the leading dash, NULL if none.
 *
 *  `value`:
 *    stores pointer to the value to be filled.
 *
 *  `help`:
 *    the short help message associated to what the option does.
 *    Must never be NULL (except for ARGPARSE_OPT_END).
 *
 *  `required`:
 *    if 'true' option presence is mandatory (default: false)
 *
 *
 *  `callback`:
 *    function is called when corresponding argument is parsed.
 *
 *  `data`:
 *    associated data. Callbacks can use it like they want.
 *
 *  `flags`:
 *    option flags.
 *
 *  `is_present`:
 *    true if option present in args
 */
typedef struct argparse_opt_s
{
    u8 type;
    void* value;
    const char short_name;
    const char* long_name;
    const char* help;
    bool required;
    argparse_callback_f callback;
    void* callback_data;
    argparse_convert_f convert;
    bool is_present; // also setting in in argparse$opt* macro, allows optional parameter sugar
} argparse_opt_s;

enum CexArgParseType_e
{
    CexArgParseType__na,
    CexArgParseType__group,
    CexArgParseType__boolean,
    CexArgParseType__string,
    CexArgParseType__i8,
    CexArgParseType__u8,
    CexArgParseType__i16,
    CexArgParseType__u16,
    CexArgParseType__i32,
    CexArgParseType__u32,
    CexArgParseType__i64,
    CexArgParseType__u64,
    CexArgParseType__f32,
    CexArgParseType__f64,
    CexArgParseType__generic,
};

/**
 * argpparse
 */
typedef struct argparse_c
{
    // user supplied options
    argparse_opt_s* options;
    u32 options_len;

    const char* usage;        // usage text (can be multiline), each line prepended by program_name
    const char* description;  // a description after usage
    const char* epilog;       // a description at the end
    const char* program_name; // program name in usage (by default: argv[0])

    struct
    {
        u32 stop_at_non_option : 1;
        u32 ignore_unknown_args : 1;
    } flags;
    //
    //
    // internal context
    struct
    {
        int argc;
        char** argv;
        char** out;
        int cpidx;
        const char* optvalue; // current option value
        bool has_argument;
    } _ctx;
} argparse_c;


// built-in option macros
#define argparse$opt(value, ...)                                                                   \
    ({                                                                                             \
        u32 val_type = _Generic(                                                                   \
            (value),                                                                               \
            bool*: CexArgParseType__boolean,                                                       \
            i8*: CexArgParseType__i8,                                                              \
            u8*: CexArgParseType__u8,                                                              \
            i16*: CexArgParseType__i16,                                                            \
            u16*: CexArgParseType__u16,                                                            \
            i32*: CexArgParseType__i32,                                                            \
            u32*: CexArgParseType__u32,                                                            \
            i64*: CexArgParseType__i64,                                                            \
            u64*: CexArgParseType__u64,                                                            \
            f32*: CexArgParseType__f32,                                                            \
            f64*: CexArgParseType__f64,                                                            \
            const char**: CexArgParseType__string,                                                 \
            char**: CexArgParseType__string,                                                       \
            default: CexArgParseType__generic                                                      \
        );                                                                                         \
        argparse_convert_f conv_f = _Generic(                                                      \
            (value),                                                                               \
            bool*: NULL,                                                                           \
            const char**: NULL,                                                                    \
            char**: NULL,                                                                          \
            i8*: (argparse_convert_f)str.convert.to_i8,                                            \
            u8*: (argparse_convert_f)str.convert.to_u8,                                            \
            i16*: (argparse_convert_f)str.convert.to_i16,                                          \
            u16*: (argparse_convert_f)str.convert.to_u16,                                          \
            i32*: (argparse_convert_f)str.convert.to_i32,                                          \
            u32*: (argparse_convert_f)str.convert.to_u32,                                          \
            i64*: (argparse_convert_f)str.convert.to_i64,                                          \
            u64*: (argparse_convert_f)str.convert.to_u64,                                          \
            f32*: (argparse_convert_f)str.convert.to_f32,                                          \
            f64*: (argparse_convert_f)str.convert.to_f64,                                          \
            default: NULL                                                                          \
        );                                                                                         \
        (argparse_opt_s){ val_type,                                                                \
                          (value),                                                                 \
                          __VA_ARGS__,                                                             \
                          .convert = (argparse_convert_f)conv_f,                                   \
                          .is_present = 0 };                                                       \
    })
// clang-format off
#define argparse$opt_group(h)     { CexArgParseType__group, NULL, '\0', NULL, h, false, NULL, 0, 0, .is_present=0 }
#define argparse$opt_help()       {CexArgParseType__boolean, NULL, 'h', "help",                           \
                                        "show this help message and exit", false,    \
                                        NULL, 0, .is_present = 0}
// clang-format on

__attribute__((visibility("hidden"))
) extern const struct __module__argparse argparse; // CEX Autogen
struct __module__argparse
{
    // Autogenerated by CEX
    // clang-format off

u32             (*argc)(argparse_c* self);
char**          (*argv)(argparse_c* self);
Exception       (*parse)(argparse_c* self, int argc, char** argv);
void            (*usage)(argparse_c* self);
    // clang-format on
};
__attribute__((visibility("hidden"))
) extern const struct __module__argparse argparse; // CEX Autogen
