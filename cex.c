#define cex$cc_args "-Wall", "-Wextra"
#define BUILD_DIR "build_cex/"

#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"

Exception cmd_build(void);
Exception cmd_check_health(void);

int
main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    io.printf("hello\n");
    e$except(err, cmd_check_health())
    {
        return 1;
    }
    e$except(err, cmd_build())
    {
        return 1;
    }
    return 0;
}
Exception
cmd_check_health(void)
{
    io.printf("CEX health report\n");
    io.printf("cex$cc: %s\n", cex$cc);
    io.printf("cex$cc_args: %s\n", cex$stringize(cex$cc_args));
    return EOK;
}

Exception
cmd_build(void)
{
    log$debug("Start building something\n");
    e$ret(os.fs.mkdir(BUILD_DIR));

// os$cmd(cex$cc, cex$cc_args, "-o", "cex2", "cex.c");
// os$cmd(cex$cc, cex$cc_args, "-o", NULL, "cex.c");
    mem$scope(tmem$, _)
    {
        char* target = "cex"; 
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, 
                  cex$cc,
                  cex$cc_args,
                  "",
                  "-o", target, 
                  str.fmt(_, "%s.c", target));
        arr$pushm(args, "boo", "bar");
        os$cmda(args);
        // os$cmd(cex$cc, "-Wall", "-Wextra", "-o", "cex2", "cex .c");
        // os$cmd(cex$cc, "-Wall", "-Wextra", "-o", "cex2", "cex .c");
        // const char* arg[] = { "ls", NULL };
        // os$cmda(arg);
    }
#define DEPS "bar"

    return EOK;
}
