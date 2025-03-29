#pragma once
#include "argparse.h"

static const char*
argparse__prefix_skip(const char* str, const char* prefix)
{
    usize len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static Exception
argparse__error(argparse_c* self, const argparse_opt_s* opt, const char* reason, bool is_long)
{
    (void)self;
    if (is_long) {
        fprintf(stdout, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        fprintf(stdout, "error: option `-%c` %s\n", opt->short_name, reason);
    }

    return Error.argument;
}

static void
argparse_usage(argparse_c* self)
{
    uassert(self->_ctx.argv != NULL && "usage before parse!");

    fprintf(stdout, "Usage:\n");
    if (self->usage) {

        for$iter(str_s, it, str.slice.iter_split(str.sstr(self->usage), "\n", &it.iterator))
        {
            if (it.val->len == 0) {
                break;
            }

            char* fn = strrchr(self->program_name, '/');
            if (fn != NULL) {
                fprintf(stdout, "%s ", fn + 1);
            } else {
                fprintf(stdout, "%s ", self->program_name);
            }

            if (fwrite(it.val->buf, sizeof(char), it.val->len, stdout)) {
                ;
            }

            fputc('\n', stdout);
        }
    } else {
        fprintf(stdout, "%s [options] [--] [arg1 argN]\n", self->program_name);
    }

    // print description
    if (self->description) {
        fprintf(stdout, "%s\n", self->description);
    }

    fputc('\n', stdout);


    // figure out best width
    usize usage_opts_width = 0;
    usize len = 0;
    for$eachp(opt, self->options, self->options_len){
        len = 0;
        if (opt->short_name) {
            len += 2;
        }
        if (opt->short_name && opt->long_name) {
            len += 2; // separator ", "
        }
        if (opt->long_name) {
            len += strlen(opt->long_name) + 2;
        }
        switch (opt->type) {
            case CexArgParseType__boolean:
                break;
            case CexArgParseType__string:
                break;
            case CexArgParseType__i8:
            case CexArgParseType__u8:
            case CexArgParseType__i16:
            case CexArgParseType__u16:
            case CexArgParseType__i32:
            case CexArgParseType__u32:
            case CexArgParseType__i64:
            case CexArgParseType__u64:
            case CexArgParseType__f32:
            case CexArgParseType__f64:
                len += strlen("=<xxx>");
                break;

            default: 
                break;
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) {
            usage_opts_width = len;
        }
    }
    usage_opts_width += 4; // 4 spaces prefix

    for$eachp(opt, self->options, self->options_len){
        usize pos = 0;
        usize pad = 0;
        if (opt->type == CexArgParseType__group) {
            fputc('\n', stdout);
            fprintf(stdout, "%s", opt->help);
            fputc('\n', stdout);
            continue;
        }
        pos = fprintf(stdout, "    ");
        if (opt->short_name) {
            pos += fprintf(stdout, "-%c", opt->short_name);
        }
        if (opt->long_name && opt->short_name) {
            pos += fprintf(stdout, ", ");
        }
        if (opt->long_name) {
            pos += fprintf(stdout, "--%s", opt->long_name);
        }

        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            fputc('\n', stdout);
            pad = usage_opts_width;
        }
        fprintf(stdout, "%*s%s\n", (int)pad + 2, "", opt->help);
    }

    // print epilog
    if (self->epilog) {
        fprintf(stdout, "%s\n", self->epilog);
    }
}
static Exception
argparse__getvalue(argparse_c* self, argparse_opt_s* opt, bool is_long)
{
    if (!opt->value) {
        goto skipped;
    }

    switch (opt->type) {
        case CexArgParseType__boolean:
            *(bool*)opt->value = !(*(bool*)opt->value);
            opt->is_present = true;
            break;
        case CexArgParseType__string:
            if (self->_ctx.optvalue) {
                *(const char**)opt->value = self->_ctx.optvalue;
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                self->_ctx.cpidx++;
                *(const char**)opt->value = *++self->_ctx.argv;
            } else {
                return argparse__error(self, opt, "requires a value", is_long);
            }
            opt->is_present = true;
            break;
        case CexArgParseType__i8:
        case CexArgParseType__u8:
        case CexArgParseType__i16:
        case CexArgParseType__u16:
        case CexArgParseType__i32:
        case CexArgParseType__u32:
        case CexArgParseType__i64:
        case CexArgParseType__u64:
        case CexArgParseType__f32:
        case CexArgParseType__f64:
            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return argparse__error(self, opt, "requires a value", is_long);
                }
                uassert(opt->convert != NULL);
                e$except_silent(err, opt->convert(self->_ctx.optvalue, opt->value))
                {
                    return argparse__error(self, opt, "argument parsing error", is_long);
                }
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                self->_ctx.cpidx++;
                self->_ctx.argv++;
                e$except_silent(err, opt->convert(*self->_ctx.argv, opt->value))
                {
                    return argparse__error(self, opt, "argument parsing error", is_long);
                }
            } else {
                return argparse__error(self, opt, "requires a value", is_long);
            }
            if (opt->type == CexArgParseType__f32) {
                f32 res = *(f32*)opt->value;
                if (isnanf(res) || res == INFINITY || res == -INFINITY) {
                    return argparse__error(
                        self,
                        opt,
                        "argument parsing error (float out of range)",
                        is_long
                    );
                }
            } else if (opt->type == CexArgParseType__f64) {
                f64 res = *(f64*)opt->value;
                if (isnanf(res) || res == INFINITY || res == -INFINITY) {
                    return argparse__error(
                        self,
                        opt,
                        "argument parsing error (float out of range)",
                        is_long
                    );
                }
            }
            opt->is_present = true;
            break;
        default:
            uassert(false && "unhandled");
            return Error.runtime;
    }

skipped:
    if (opt->callback) {
        opt->is_present = true;
        return opt->callback(self, opt, opt->callback_data);
    } else {
        if (opt->short_name == 'h') {
            argparse_usage(self);
            return Error.argsparse;
        }
    }

    return Error.ok;
}

static Exception
argparse__options_check(argparse_c* self, bool reset)
{
    for$eachp(opt, self->options, self->options_len)
    {
        if (opt->type != CexArgParseType__group) {
            if (reset) {
                opt->is_present = 0;
                if (!(opt->short_name || opt->long_name)) {
                    uassert(
                        (opt->short_name || opt->long_name) && "options both long/short_name NULL"
                    );
                    return Error.argument;
                }
                if (opt->value == NULL && opt->short_name != 'h') {
                    uassertf(
                        opt->value != NULL,
                        "option value [%c/%s] is null\n",
                        opt->short_name,
                        opt->long_name
                    );
                    return Error.argument;
                }
            } else {
                if (opt->required && !opt->is_present) {
                    fprintf(
                        stdout,
                        "Error: missing required option: -%c/--%s\n",
                        opt->short_name,
                        opt->long_name
                    );
                    return Error.argsparse;
                }
            }
        }

        switch (opt->type) {
            case CexArgParseType__group:
                continue;
            case CexArgParseType__boolean:
            case CexArgParseType__string: {
                uassert(opt->convert == NULL && "unexpected to be set for strings/bools");
                uassert(opt->callback == NULL && "unexpected to be set for strings/bools");
                break;
            }
            case CexArgParseType__i8:
            case CexArgParseType__u8:
            case CexArgParseType__i16:
            case CexArgParseType__u16:
            case CexArgParseType__i32:
            case CexArgParseType__u32:
            case CexArgParseType__i64:
            case CexArgParseType__u64:
            case CexArgParseType__f32:
            case CexArgParseType__f64:
                uassert(opt->convert != NULL && "expected to be set for standard args");
                uassert(opt->callback == NULL && "unexpected to be set for standard args");
                continue;
            case CexArgParseType__generic:
                uassert(opt->convert == NULL && "unexpected to be set for generic args");
                uassert(opt->callback != NULL && "expected to be set for generic args");
                continue;
            default:
                uassertf(false, "wrong option type: %d", opt->type);
        }
    }

    return Error.ok;
}

static Exception
argparse__short_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        if (options->short_name == *self->_ctx.optvalue) {
            self->_ctx.optvalue = self->_ctx.optvalue[1] ? self->_ctx.optvalue + 1 : NULL;
            return argparse__getvalue(self, options, false);
        }
    }
    return Error.not_found;
}

static Exception
argparse__long_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        const char* rest;
        if (!options->long_name) {
            continue;
        }
        rest = argparse__prefix_skip(self->_ctx.argv[0] + 2, options->long_name);
        if (!rest) {
            if (options->type != CexArgParseType__boolean) {
                continue;
            }
            rest = argparse__prefix_skip(self->_ctx.argv[0] + 2 + 3, options->long_name);
            if (!rest) {
                continue;
            }
        }
        if (*rest) {
            if (*rest != '=') {
                continue;
            }
            self->_ctx.optvalue = rest + 1;
        }
        return argparse__getvalue(self, options, true);
    }
    return Error.not_found;
}


static Exception
argparse__report_error(argparse_c* self, Exc err)
{
    // invalidate argc
    self->_ctx.argc = 0;

    if (err == Error.not_found) {
        fprintf(stdout, "error: unknown option `%s`\n", self->_ctx.argv[0]);
    } else if (err == Error.integrity) {
        fprintf(stdout, "error: option `%s` follows argument\n", self->_ctx.argv[0]);
    }
    return Error.argsparse;
}

static Exception
argparse_parse(argparse_c* self, int argc, char** argv)
{
    if (self->options_len == 0) {
        argparse_opt_s* _opt = self->options;
        while(_opt != NULL) {
            if (_opt->type == CexArgParseType__na) {
                break;
            }
            self->options_len++;
            _opt++;
        }
    }
    uassert(argc > 0);
    uassert(argv != NULL);
    uassert(argv[0] != NULL);

    if (self->program_name == NULL) {
        self->program_name = argv[0];
    }

    // reset if we have several runs
    memset(&self->_ctx, 0, sizeof(self->_ctx));

    self->_ctx.argc = argc - 1;
    self->_ctx.argv = argv + 1;
    self->_ctx.out = argv;

    e$except_silent(err, argparse__options_check(self, true))
    {
        return err;
    }

    for (; self->_ctx.argc; self->_ctx.argc--, self->_ctx.argv++) {
        const char* arg = self->_ctx.argv[0];
        if (arg[0] != '-' || !arg[1]) {
            self->_ctx.has_argument = true;

            if (self->flags.stop_at_non_option) {
                self->_ctx.argc--;
                self->_ctx.argv++;
                break;
            }
            continue;
        }
        // short option
        if (arg[1] != '-') {
            if (self->_ctx.has_argument) {
                // options are not allowed after arguments
                return argparse__report_error(self, Error.integrity);
            }

            self->_ctx.optvalue = arg + 1;
            self->_ctx.cpidx++;
            e$except_silent(err, argparse__short_opt(self, self->options))
            {
                return argparse__report_error(self, err);
            }
            while (self->_ctx.optvalue) {
                e$except_silent(err, argparse__short_opt(self, self->options))
                {
                    return argparse__report_error(self, err);
                }
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            self->_ctx.argc--;
            self->_ctx.argv++;
            self->_ctx.cpidx++;
            break;
        }
        // long option
        if (self->_ctx.has_argument) {
            // options are not allowed after arguments
            return argparse__report_error(self, Error.integrity);
        }
        e$except_silent(err, argparse__long_opt(self, self->options))
        {
            return argparse__report_error(self, err);
        }
        self->_ctx.cpidx++;
        continue;
    }

    e$except_silent(err, argparse__options_check(self, false))
    {
        return err;
    }

    self->_ctx.out += self->_ctx.cpidx + 1; // excludes 1st argv[0], program_name
    self->_ctx.argc = argc - self->_ctx.cpidx - 1;

    return Error.ok;
}

static u32
argparse_argc(argparse_c* self)
{
    uassert(self->_ctx.out != NULL && "argparse.parse() not called?");
    return self->_ctx.argc;
}

static char**
argparse_argv(argparse_c* self)
{
    uassert(self->_ctx.out != NULL && "argparse.parse() not called?");
    return self->_ctx.out;
}


const struct __module__argparse argparse = {
    // Autogenerated by CEX
    // clang-format off
    .usage = argparse_usage,
    .parse = argparse_parse,
    .argc = argparse_argc,
    .argv = argparse_argv,
    // clang-format on
};
