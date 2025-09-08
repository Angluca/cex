#if __has_include("cex_config.h")
// Custom config file
#    include "cex_config.h"
#else
// Overriding config values
#    define cexy$cc_include "-I.", "-I./lib", cexy$build_dir"/cex.obj"
// #    define cexy$cc_args_sanitizer "-fstack-protector-strong"
// #    define cexy$cc_args "-I.", "-I./lib"
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#endif

#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"

Exception cmd_build_lib(int argc, char** argv, void* user_ctx);

Exception
prebuild_cex(void)
{
    e$ret(os.fs.mkdir(cexy$build_dir));

    char* src[] = { "./cex.h" };
    if (cexy.src_changed(cexy$build_dir "/cex.obj", src, arr$len(src))) {
        log$info("Pre-Building cex.h -> cex.obj\n"); 
        e$ret(os$cmd(
            cexy$cc,
            "-DCEX_IMPLEMENTATION",
            "-x",
            "c",
            "-c",
            "./cex.h",
            cexy$cc_args_sanitizer,
            "-o",
            cexy$build_dir "/cex.obj"
        ));
    }

    return EOK;
}

int
main(int argc, char** argv)
{

    cexy$initialize(); // cex self rebuild and init


    argparse_c args = {
        .description = cexy$description,
        .epilog = cexy$epilog,
        .usage = cexy$usage,
        argparse$cmd_list(
            cexy$cmd_all,
            cexy$cmd_fuzz, /* feel free to make your own if needed */
            cexy$cmd_test, /* feel free to make your own if needed */
            cexy$cmd_app,  /* feel free to make your own if needed */
            { .name = "build-lib", .func = cmd_build_lib, .help = "Custom build command" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }

    e$except (err, prebuild_cex()) { return 1; }

    void* my_user_ctx = NULL; // passed as `user_ctx` to command
    if (argparse.run_command(&args, my_user_ctx)) { return 1; }
    return 0;
}

/// Custom build command for building static lib
Exception
cmd_build_lib(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    log$info("Launching custom command\n");
    return EOK;
}
