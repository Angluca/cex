#include "App.h"
#include <cex/argparse/argparse.h>
#include "lib/mylib.h"
#include <cex/cex.h>

Exception
App_create(App_c* app, i32 argc, char* argv[], const Allocator_i* allocator)
{
    memset(app, 0, sizeof(*app));

    uassert(allocator != NULL && "expected allocator");

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt_group("Basic options"),
        argparse$opt_bool('f', "flag", &app->is_flag, .help = "some bool flag"),
        argparse$opt_i64('\0', "num", &app->num_arg, .help = "numerical arg (default: 234)"),
        argparse$opt_str('n', "name", &app->name_arg, .help = "name string arg"),
    };

    const char* usage = "[-f/--flag] [--num=<234>] [-n/--name=<name_str>] [[--] arg1 arg2 argN]";

    argparse_c args = { .options = options,
                        .options_len = arr$len(options),
                        .usage = usage,
                        .description = "App description here",
                        .epilog = "\n Footer desc" };

    if (argparse.parse(&args, argc, argv) != EOK || argparse.argc(&args) == 0) {
        argparse.usage(&args);
        return Error.argsparse;
    }

    app->args = argparse.argv(&args);
    app->args_count = argparse.argc(&args);
    app->allocator = allocator;

    if (app->num_arg == 0) {
        // NOTE: missing arguments, are set to 0 by default
        // we can use this property to set our defaults as well
        app->num_arg = 234;
    }

    return Error.ok;
}


Exception
App_main(App_c* app, const Allocator_i* allocator)
{
    (void)allocator;

    log$debug("Hello, CEX!");
    log$info("Flag Arg: %d", app->is_flag);
    log$info("Num Arg: %ld", app->num_arg);
    log$info("Str Arg: %s", app->name_arg);

    log$info("You passed %d args", app->args_count);

    // NOTE: something weird with mylib.mul() results
    // try to run tests `cex test run all` and see if it's correct
    log$info("Num Arg * 3 = %u", mylib.mul(app->num_arg, 3));

    for$array(it, app->args, app->args_count) {
        log$debug("Arg #%ld: %s", it.idx, *it.val);
    }

    return EOK;
}

void
App_destroy(App_c* app, const Allocator_i* allocator)
{
    (void)app;
    (void)allocator;
    log$debug("App is shutting down\n");
}
