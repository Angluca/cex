#pragma once
#include "all.h"

struct argparse_c;
struct argparse_opt_s;

typedef Exception (*argparse_callback_f)(
    struct argparse_c* self,
    struct argparse_opt_s* option,
    void* ctx
);
typedef Exception (*argparse_convert_f)(char* s, void* out_val);
typedef Exception (*argparse_command_f)(int argc, char** argv, void* user_ctx);

typedef struct argparse_opt_s
{
    u8 type;
    void* value;
    char short_name;
    char* long_name;
    char* help;
    bool required;
    argparse_callback_f callback;
    void* callback_data;
    argparse_convert_f convert;
    bool is_present; // also setting in in argparse$opt* macro, allows optional parameter sugar
} argparse_opt_s;

typedef struct argparse_cmd_s
{
    char* name;
    argparse_command_f func;
    char* help;
    bool is_default;
} argparse_cmd_s;

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

    argparse_cmd_s* commands;
    u32 commands_len;

    char* usage;        // usage text (can be multiline), each line prepended by program_name
    char* description;  // a description after usage
    char* epilog;       // a description at the end
    char* program_name; // program name in usage (by default: argv[0])

    int argc;    // current argument count (excludes program_name)
    char** argv; // current arguments list

    struct
    {
        // internal context
        char** out;
        int cpidx;
        char* optvalue; // current option value
        bool has_argument;
        argparse_cmd_s* current_command;
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

#define argparse$opt_list(...) .options = (argparse_opt_s[]) {__VA_ARGS__ {0} /* NULL TERM */}
#define argparse$opt_group(h)     { CexArgParseType__group, NULL, '\0', NULL, h, false, NULL, 0, 0, .is_present=0 }
#define argparse$opt_help()       {CexArgParseType__boolean, NULL, 'h', "help",                           \
                                        "show this help message and exit", false,    \
                                        NULL, 0, .is_present = 0}
#define argparse$pop(argc, argv) ((argc > 0) ? (--argc, (*argv)++) : NULL)
#define argparse$cmd_list(...) .commands = (argparse_cmd_s[]) {__VA_ARGS__ {0} /* NULL TERM */}

__attribute__((visibility("hidden"))) extern const struct __cex_namespace__argparse argparse;

/**
* @brief Command line argument parsing module
*/
struct __cex_namespace__argparse {
    // Autogenerated by CEX
    // clang-format off

    char*           (*next)(argparse_c* self);
    Exception       (*parse)(argparse_c* self, int argc, char** argv);
    Exception       (*run_command)(argparse_c* self, void* user_ctx);
    void            (*usage)(argparse_c* self);

    // clang-format on
};
