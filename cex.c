#define cexy$cc_args "-Wall", "-Wextra"
#define BUILD_DIR "build/"

#define CEX_IMPLEMENTATION
#define CEXBUILD
#include "cex.h"

#if defined __has_include
#  if __has_include(<nonexistent_header.h>)
#    include <nonexistent_header.h>
#  else
#    warning "Header nonexistent_header.h not found, skipping..."
#  endif
#else
#  warning "Compiler does not support __has_include, cannot check header"
#endif

Exception cmd_check(int argc, const char** argv, void* user_ctx);
Exception cmd_build(int argc, const char** argv, void* user_ctx);

int
main(int argc, char** argv)
{
    // clang-format off
    argparse_c args = {
        .commands =
            (argparse_cmd_s[]){
                { .name = "check", .func = cmd_check, .help = "Validates build environment" },
                { .name = "build", .func = cmd_build, .help = "Builds project", .is_default = 1 },
                { 0 }, // null term
            },
    };
    // clang-format on

    if (argparse.parse(&args, argc, argv)) {
        return 1;
    }
    e$except(err, argparse.run_command(&args, NULL))
    {
        return 1;
    }
    return 0;
}

Exception
cmd_check(int argc, const char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    io.printf("CEX health report\n");
    io.printf("cexy$cc: %s\n", cexy$cc);
    io.printf("cexy$cc_args: %s\n", cex$stringize(cexy$cc_args));
    return EOK;
}

Exception
cmd_build(int argc, const char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    log$debug("Start building something\n");
    e$ret(os.fs.mkdir(BUILD_DIR));
    e$assert(false && "foo");

    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        // arr$pushm(args, cexy$cc, cexy$cc_args, "", "-o", target, str.fmt(_, "%s.c", target));
        // arr$pushm(args, "boo", "bar");
        // os$cmda(args);
        os$cmd(cexy$cc, "-Wall", "-Wextra", "-o", "cex", "cex.c");
        const char* arg[] = { "ls", NULL };
        os$cmda(arg);
    }
    return EOK;
}
