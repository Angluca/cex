/*
 *  Freestanding implementation for linux platform (no libc, memory allocators, file system, etc)
 */

// Disables all CEX modules except base types, each module have to be re-enabled
#define cex$enable_minimal
#define cex$enable_mem
#define cex$enable_ds
#define cex$enable_str
#define cex$enable_io
#define cex$enable_os

#define CEX_IMPLEMENTATION
#include "cex.h"

#include "platform.c"
#include "platform_io.c"
#include "platform_mem.c"
#include "platform_str.c"
#include "platform_os.c"

Exception cmd_build(int argc, char** argv, void* user_ctx);

int
main(int argc, char** argv)
{
    (void)argc;
    (void)argv;

    log$info("This is Freestanding with os\n");

    argparse_c args = {
        .description = "Freestanding arg parse description",
        // BUG: getting segfault in gcc!
        argparse$cmd_list(
            { .name = "build", .func = cmd_build, .help = "Build standalone CEX program" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) {};

    // BUG: getting segfault when passing f64 in log$info(), probably compiler issue for freestanding
    //    because failing at arguments passing for va_arg, at first cexsp__fprintf() asm instruction
    u64 timer = os.timer();
    log$info("os.timer = %lu\n", timer);

    log$info("Done\n");

    return 0;
}

Exception
cmd_build(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    return EOK;
}
