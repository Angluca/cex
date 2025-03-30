#include <cex/all.c>
#include <cex/all.h>

test$case(test_argparse_init_short)
{
    bool force = 0;
    bool test = 0;
    int int_num = 0;
    float flt_num = 0.f;
    const char* path = NULL;

    const char* usage = "basic [options] [[--] args]\n"
                        "basic [options]\n";

    argparse_c argparse = {
        .options =
            (argparse_opt_s[]){
                argparse$opt_help(),
                argparse$opt_group("Basic options"),
                argparse$opt(&force, 'f', "force", "force to do"),
                argparse$opt(&test, 't', "test", "test only"),
                argparse$opt(&path, 'p', "path", "path to read", .required = true),
                argparse$opt(&int_num, 'i', "int", "selected integer"),
                argparse$opt(&flt_num, 's', "float", "selected float"),
                { 0 }, // array opt term
            },
        .usage = usage,
        .description = "\nA brief description of what the program does and how it works.",
        "\nAdditional description of the program after the description of the arguments.",
    };

    char* argv[] = {
        "program_name", "-f", "-t", "-p", "mypath/ok", "-i", "2000", "-s", "20.20",
    };
    int argc = arr$len(argv);

    tassert_eq(EOK, argparse_parse(&argparse, argc, argv));
    tassert_eq(argparse.options_len, 7);
    tassert_eq(force, 1);
    tassert_eq(test, 1);
    tassert_eq(path, "mypath/ok");
    tassert_eq(int_num, 2000);
    tassert_eq((u32)(flt_num * 100), (u32)(20.20 * 100));

    return EOK;
}

test$case(test_argparse_init_long)
{
    bool force = 0;
    bool test = 0;
    int int_num = 0;
    float flt_num = 0.f;
    const char* path = NULL;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
        argparse$opt(&test, 't', "test", "test only", NULL, 0, 0),
        argparse$opt(&path, 'p', "path", "path to read", .callback = NULL, 0),
        argparse$opt(&int_num, 'i', "int", .help = "selected integer", 0),
        argparse$opt(&flt_num, 's', "float", .help = "selected float", 0),
    };

    const char* usage = "basic [options] [[--] args]\n"
                        "basic [options]\n";

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
        .usage = usage,
        .description = "\nA brief description of what the program does and how it works.",
        .epilog = "\nEnd description after the list of the arguments.",
    };

    char* argv[] = {
        "test_program_name", "--force", "--test", "--path=mypath/ok", "--int=2000", "--float=20.20",
    };
    int argc = arr$len(argv);


    tassert_eq(EOK, argparse.parse(&args, argc, argv));
    tassert_eq("test_program_name", args.program_name);
    tassert_eq(force, 1);
    tassert_eq(test, 1);
    tassert_eq(path, "mypath/ok");
    tassert_eq(int_num, 2000);
    tassert_eq((u32)(flt_num * 100), (u32)(20.20 * 100));

    argparse.usage(&args);

    return EOK;
}

test$case(test_argparse_required)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do", .required = true),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);


    tassert_eq(Error.argsparse, argparse.parse(&args, argc, argv));
    tassert_eq(options[2].required, 1);
    tassert_eq(options[2].is_present, 0);
    tassert_eq(force, 100);

    return EOK;
}

test$case(test_argparse_bad_opts_help)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);


    tassert_eq(Error.ok, argparse.parse(&args, argc, argv));

    argparse.usage(&args);

    return EOK;
}

test$case(test_argparse_bad_opts_long)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', NULL),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);

    tassert_eq(Error.ok, argparse.parse(&args, argc, argv));

    argparse.usage(&args);

    return EOK;
}

test$case(test_argparse_bad_opts_short)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, '\0', "foo"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);

    tassert_eq(Error.ok, argparse.parse(&args, argc, argv));

    argparse.usage(&args);

    return EOK;
}

test$case(test_argparse_bad_opts_both_no_long_short)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, '\0', NULL),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name" };

    int argc = arr$len(argv);

    uassert_disable();
    argparse.usage(&args);

    tassert_eq(Error.argument, argparse.parse(&args, argc, argv));

    return EOK;
}

test$case(test_argparse_help_print)
{
    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-h" };

    int argc = arr$len(argv);

    tassert_eq(Error.argsparse, argparse.parse(&args, argc, argv));

    return EOK;
}

test$case(test_argparse_help_print_long)
{
    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "--help" };

    int argc = arr$len(argv);

    tassert_eq(Error.argsparse, argparse.parse(&args, argc, argv));

    return EOK;
}


test$case(test_argparse_arguments)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_er(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eq(force, 100);
    tassert_eq(argc, 3);                 // unchanged
    tassert_eq(argv[0], "program_name"); // unchanged
    tassert_eq(argv[1], "arg1");         // unchanged
    tassert_eq(argv[2], "arg2");         // unchanged
    //
    tassert_eq(argparse.argc(&args), 2);
    tassert_eq(argparse.argv(&args)[0], "arg1");
    tassert_eq(argparse.argv(&args)[1], "arg2");

    return EOK;
}

test$case(test_argparse_arguments_after_options)
{
    bool force = 0;
    int int_num = 2000;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
        argparse$opt(&int_num, 'i', "int", "selected integer"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-f", "--int=100", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_er(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eq(force, 1);
    tassert_eq(int_num, 100);
    tassert_eq(argc, 5);                 // unchanged
    tassert_eq(argv[0], "program_name"); // unchanged
    tassert_eq(argv[1], "-f");           // unchanged
    tassert_eq(argv[2], "--int=100");    // unchanged
    tassert_eq(argv[3], "arg1");         // unchanged
    tassert_eq(argv[4], "arg2");         // unchanged
    //
    // tassert_eq(argparse.argc(&args), 2);
    tassert_eq(argparse.argv(&args)[0], "arg1");
    tassert_eq(argparse.argv(&args)[1], "arg2");

    return EOK;
}

test$case(test_argparse_arguments_stacked_short_opt)
{
    bool force = 0;
    bool int_num = 1;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
        argparse$opt(&int_num, 'i', "int_flag", "selected integer"),
        argparse$opt(&int_num2, 'z', "int", "selected integer"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-fi", "--int=100", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_er(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eq(force, 1);
    tassert_eq(int_num, 0);
    tassert_eq(int_num2, 100);
    tassert_eq(argc, 5);                 // unchanged
    tassert_eq(argv[0], "program_name"); // unchanged
    tassert_eq(argv[1], "-fi");          // unchanged
    tassert_eq(argv[2], "--int=100");    // unchanged
    tassert_eq(argv[3], "arg1");         // unchanged
    tassert_eq(argv[4], "arg2");         // unchanged
    //
    tassert_eq(argparse.argc(&args), 2);
    tassert_eq(argparse.argv(&args)[0], "arg1");
    tassert_eq(argparse.argv(&args)[1], "arg2");

    return EOK;
}

test$case(test_argparse_arguments_double_dash)
{
    bool force = 1;
    bool int_num = 0;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
        argparse$opt(&int_num, 'i', "int_flag", "selected integer"),
        argparse$opt(&int_num2, 'z', "int", "selected integer"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-fi", "--", "--int=100", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_er(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eq(force, 0);
    tassert_eq(int_num, 1);
    // NOTE: this must be untouched because we have -- prior
    tassert_eq(int_num2, -100);
    tassert_eq(argc, 6);                 // unchanged
    tassert_eq(argv[0], "program_name"); // unchanged
    tassert_eq(argv[1], "-fi");          // unchanged
    tassert_eq(argv[2], "--");           // unchanged
    tassert_eq(argv[3], "--int=100");    // unchanged
    tassert_eq(argv[4], "arg1");         // unchanged
    tassert_eq(argv[5], "arg2");         // unchanged
    //
    tassert_eq(argparse.argc(&args), 3);
    tassert_eq(argparse.argv(&args)[0], "--int=100");
    tassert_eq(argparse.argv(&args)[1], "arg1");
    tassert_eq(argparse.argv(&args)[2], "arg2");

    return EOK;
}

test$case(test_argparse_arguments__option_follows_argument_not_allowed)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
        argparse$opt(&int_num, 'i', "int_flag", "selected integer"),
        argparse$opt(&int_num2, 'z', "int", "selected integer"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "arg1", "-fi", "arg2", "--int=100" };
    int argc = arr$len(argv);


    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv));

    // All untouched
    tassert_eq(force, 100);
    tassert_eq(int_num, -100);
    tassert_eq(int_num2, -100);
    tassert_eq(argc, 5);                 // unchanged
    tassert_eq(argv[0], "program_name"); // unchanged
    tassert_eq(argv[1], "arg1");         // unchanged
    tassert_eq(argv[2], "-fi");          // unchanged
    tassert_eq(argv[3], "arg2");         // unchanged
    tassert_eq(argv[4], "--int=100");    // unchanged
    //
    tassert_eq(argparse.argc(&args), 0);

    return EOK;
}


test$case(test_argparse_arguments__parsing_error)
{
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt(&int_num2, 'z', "int", "selected integer"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "--int=foo" };
    int argc = arr$len(argv);

    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv));
    tassert_eq(int_num2, 0);

    return EOK;
}

test$case(test_argparse_arguments__option_follows_argument__allowed)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
        argparse$opt(&int_num, 'i', "int_flag", "selected integer"),
        argparse$opt(&int_num2, 'z', "int", "selected integer"),
    };

    argparse_c
        args = { .options = options,
                 .options_len = arr$len(options),
                 .flags = {
                     .stop_at_non_option = true, // NOTE: this allows options after args (commands!)
                 } };

    char* argv[] = { "program_name", "arg1", "-fi", "arg2", "--int=100" };
    int argc = arr$len(argv);

    tassert_er(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eq(argparse.argc(&args), 4);
    tassert_eq(argparse.argv(&args)[0], "arg1");
    tassert_eq(argparse.argv(&args)[1], "-fi");
    tassert_eq(argparse.argv(&args)[2], "arg2");
    tassert_eq(argparse.argv(&args)[3], "--int=100");

    return EOK;
}

test$case(test_argparse_arguments__command_mode)
{
    bool force = 0;
    bool int_num = 1;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
        argparse$opt(&int_num, 'i', "int_flag", "selected integer"),
        argparse$opt(&int_num2, 'z', "int", "selected integer"),
    };

    argparse_c
        args = { .options = options,
                 .options_len = arr$len(options),
                 .flags = {
                     .stop_at_non_option = true, // NOTE: this allows options after args (commands!)
                 } };

    char* argv[] = { "program_name", "cmd_name", "-fi", "--int=100", "cmd_arg_something" };
    int argc = arr$len(argv);

    tassert_eq(args.program_name, NULL);
    tassert_er(Error.ok, argparse.parse(&args, argc, argv));
    // params are untouched
    tassert_eq(force, 0);
    tassert_eq(int_num, 1);
    tassert_eq(int_num2, -100);
    tassert_eq(args.program_name, "program_name");
    tassert_eq(argparse.argc(&args), 4);
    tassert_eq(argparse.argv(&args)[0], "cmd_name");
    tassert_eq(argparse.argv(&args)[1], "-fi");
    tassert_eq(argparse.argv(&args)[2], "--int=100");
    tassert_eq(argparse.argv(&args)[3], "cmd_arg_something");

    argparse_c cmd_args = {
        .options = options,
        .options_len = arr$len(options),
    };
    tassert_er(Error.ok, argparse.parse(&cmd_args, argparse.argc(&args), argparse.argv(&args)));
    tassert_eq(args.program_name, "program_name");
    tassert_eq(force, 1);
    tassert_eq(int_num, 0);
    tassert_eq(int_num2, 100);
    tassert_eq(cmd_args.program_name, "cmd_name");

    tassert_eq(argparse.argc(&cmd_args), 1);
    tassert_eq(argparse.argv(&cmd_args)[0], "cmd_arg_something");

    return EOK;
}

test$case(test_argparse_arguments__command__help)
{
    int force = 100;
    int int_num = -100;
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
        argparse$opt(&int_num, 'i', "int_flag", "selected integer"),
        argparse$opt(&int_num2, 'z', "int", "selected integer"),
    };

    argparse_opt_s cmd_options[] = {
        argparse$opt_help(),
        argparse$opt_group("Command  options"),
        argparse$opt(&int_num2, 'z', "int", "some cmd int"),
    };

    argparse_c
        args = { .options = options,
                 .options_len = arr$len(options),
                 .flags = {
                     .stop_at_non_option = true, // NOTE: this allows options after args (commands!)
                 } };

    char* argv[] = { "program_name", "cmd_name", "-h" };
    int argc = arr$len(argv);

    tassert_er(Error.ok, argparse.parse(&args, argc, argv));

    argparse_c cmd_args = {
        .options = cmd_options,
        .options_len = arr$len(cmd_options),
        .description = "this is a command description",
        .usage = "some command usage",
        .epilog = "command epilog",
    };
    tassert_er(
        Error.argsparse,
        argparse.parse(&cmd_args, argparse.argc(&args), argparse.argv(&args))
    );

    return EOK;
}

test$case(test_argparse_arguments__int_parsing)
{
    int int_num2 = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt(&int_num2, 'i', "int", "selected integer"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv0[] = { "program_name", "--int=99" };
    int argc = arr$len(argv0);

    tassert_er(Error.ok, argparse.parse(&args, argc, argv0));

    char* argv[] = { "program_name", "--int=foo1" };

    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv));
    tassert_eq(int_num2, 0);

    int_num2 = -100;
    char* argv2[] = { "program_name", "--int=1foo" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv2));
    tassert_eq(int_num2, 0); // still set, but its strtol() issue

    int_num2 = -100;
    char* argv3[] = { "program_name",
                      "--int=9999999999999999999999999999999999999999999999999999" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv3));

    int_num2 = -100;
    char* argv4[] = { "program_name", "-i" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv4));
    tassert_eq(int_num2, -100);

    int_num2 = -100;
    char* argv5[] = { "program_name", "--int=" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv5));
    tassert_eq(int_num2, -100);

    int_num2 = -100;
    char* argv6[] = { "program_name", "--int", "-6" };
    tassert_er(Error.ok, argparse.parse(&args, 3, argv6));
    tassert_eq(int_num2, -6);

    int_num2 = -100;
    char* argv7[] = { "program_name", "--int=9.8" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv7));
    tassert_eq(int_num2, 0);

    return EOK;
}

test$case(test_argparse_arguments__float_parsing)
{
    f32 fnum = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt(&fnum, 'f', "flt", "selected float"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv0[] = { "program_name", "--flt=99" };
    int argc = arr$len(argv0);
    tassert_er(Error.ok, argparse.parse(&args, argc, argv0));

    char* argv[] = { "program_name", "--flt=foo1" };

    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv));
    tassert_eq(fnum, 0);

    fnum = -100;
    char* argv2[] = { "program_name", "--flt=1foo" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv2));
    tassert_eq(fnum, 0);

    fnum = -100;
    char* argv3[] = { "program_name",
                      "--flt=9999999999999999999999999999999999999999999999999999" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv3));


    fnum = -100;
    char* argv4[] = { "program_name", "-f" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv4));
    tassert_eq(fnum, -100);

    fnum = -100;
    char* argv5[] = { "program_name", "--flt=" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv5));
    tassert_eq(fnum, -100);

    fnum = -100;
    char* argv6[] = { "program_name", "--flt", "6" };
    tassert_er(Error.ok, argparse.parse(&args, 3, argv6));
    tassert_eq(fnum, 6);

    fnum = -100;
    char* argv7[] = { "program_name", "--flt=-9.8" };
    tassert_er(Error.ok, argparse.parse(&args, argc, argv7));
    tassert_eq(fnum * 100, -9.8 * 100);

    fnum = -100;
    char* argv8[] = { "program_name",
                      "--flt=-9999999999999999999999999999999999999999999999999999" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv8));

    fnum = -100;
    char* argv9[] = { "program_name", "--flt=NaN" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv9));

    fnum = -100;
    char* argv10[] = { "program_name", "--flt=inf" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv10));

    fnum = -100;
    char* argv11[] = { "program_name", "--flt=-inf" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv11));
    return EOK;
}

test$case(test_argparse_arguments__double_parsing)
{
    f64 fnum = -100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt(&fnum, 'f', "flt", "selected float"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv0[] = { "program_name", "--flt=99" };
    int argc = arr$len(argv0);
    tassert_er(Error.ok, argparse.parse(&args, argc, argv0));

    char* argv[] = { "program_name", "--flt=foo1" };

    fnum = 0;
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv));
    tassert_eq(fnum, 0);

    fnum = -100;
    char* argv2[] = { "program_name", "--flt=1foo" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv2));

    fnum = -100;
    char* argv3[] = { "program_name",
                      "--flt=99999999999999999999999999999999999999999999999999999999999" };
    tassert_er(Error.ok, argparse.parse(&args, argc, argv3));


    fnum = -100;
    char* argv4[] = { "program_name", "-f" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv4));
    tassert_eq(fnum, -100);

    fnum = -100;
    char* argv5[] = { "program_name", "--flt=" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv5));
    tassert_eq(fnum, -100);

    fnum = -100;
    char* argv6[] = { "program_name", "--flt", "6" };
    tassert_er(Error.ok, argparse.parse(&args, 3, argv6));
    tassert_eq(fnum, 6);

    fnum = -100;
    char* argv7[] = { "program_name", "--flt=-9.8" };
    tassert_er(Error.ok, argparse.parse(&args, argc, argv7));
    tassert_eq(fnum * 100, -9.8 * 100);

    fnum = -100;
    char* argv8[] = { "program_name",
                      "--flt=-9999999999999999999999999999999999999999999999999999" };
    tassert_er(Error.ok, argparse.parse(&args, argc, argv8));

    fnum = -100;
    char* argv9[] = { "program_name", "--flt=NaN" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv9));

    fnum = -100;
    char* argv10[] = { "program_name", "--flt=inf" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv10));

    fnum = -100;
    char* argv11[] = { "program_name", "--flt=-inf" };
    tassert_er(Error.argsparse, argparse.parse(&args, argc, argv11));

    return EOK;
}

test$case(test_argparse_int_short_arg__argc_remainder)
{
    int force = 100;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-f", "10", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_er(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eq(force, 10);
    tassert_eq(argc, 5); // unchanged

    tassert_eq(argparse.argc(&args), 2);
    tassert_eq(argparse.argv(&args)[0], "arg1");
    tassert_eq(argparse.argv(&args)[1], "arg2");

    return EOK;
}

test$case(test_argparse_str_short_arg__argc_remainder)
{
    const char* force = NULL;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-f", "10", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_er(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eq(force, "10");
    tassert_eq(argc, 5); // unchanged

    tassert_eq(argparse.argc(&args), 2);
    tassert_eq(argparse.argv(&args)[0], "arg1");
    tassert_eq(argparse.argv(&args)[1], "arg2");

    return EOK;
}

test$case(test_argparse_float_short_arg__argc_remainder)
{
    f32 force = 0.0;

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt(&force, 'f', "force", "force to do"),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
    };

    char* argv[] = { "program_name", "-f", "10", "arg1", "arg2" };
    int argc = arr$len(argv);


    tassert_er(Error.ok, argparse.parse(&args, argc, argv));
    tassert_eq(force, 10.0f);
    tassert_eq(argc, 5); // unchanged

    tassert_eq(argparse.argc(&args), 2);
    tassert_eq(argparse.argv(&args)[0], "arg1");
    tassert_eq(argparse.argv(&args)[1], "arg2");

    return EOK;
}

test$main();
