#if __has_include("cex_config.h")
// Custom config file
#    include "cex_config.h"
#else
#    define cexy$build_dir "build/"
#    define cexy$vcpkg_root cexy$build_dir "vcpkg/"

// Overriding config values
#    define cexy$cc_include "-I.", "-I./lib"
#    define cexy$pkgconf_libs "libcurl", "libzip"
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */

#    if _WIN32
// using mingw libs .a
#        define cexy$build_ext_lib_stat ".a"
#        define cexy$vcpkg_triplet "x64-mingw-static"
#elif defined(__APPLE__) || defined(__MACH__)
#        define cexy$vcpkg_triplet "x64-osx"
#    else
#        define cexy$vcpkg_triplet "x64-linux"
#    endif
#endif


#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"

Exception cmd_vcpkg_install(int argc, char** argv, void* user_ctx);

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
            cexy$cmd_test, /* feel free to make your own if needed */
            cexy$cmd_app,  /* feel free to make your own if needed */
            { .name = "vcpkg-install",
              .func = cmd_vcpkg_install,
              .help = "Download and bulld vcpkg libs" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }
    void* my_user_ctx = NULL; // passed as `user_ctx` to command
    if (argparse.run_command(&args, my_user_ctx)) { return 1; }
    return 0;
}

/// Automatically sets vcpkg repo (only for this project) and installs dependencies
Exception
cmd_vcpkg_install(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    log$info("Building vcpkg app\n");

    e$assert(os.cmd.exists("git") && "git not installed?");

    mem$scope(tmem$, _)
    {
        if (!os.path.exists(cexy$vcpkg_root)) {
            e$ret(os.fs.mkpath(cexy$vcpkg_root));
            e$ret(os$cmd(
                "git",
                "clone",
                "--depth=1",
                "https://github.com/microsoft/vcpkg.git",
                cexy$vcpkg_root
            ));
        }
        e$ret(os.fs.chdir(cexy$vcpkg_root));
        if (os.platform.current() == OSPlatform__win) {
            if (!os.path.exists("vcpkg.exe")) {
                e$ret(os$cmd("cmd.exe", "/c", "bootstrap-vcpkg.bat"));
            }
        } else {
            if (!os.path.exists("vcpkg")) { e$ret(os$cmd("bootstrap-vcpkg.sh")); }
        }
        e$ret(os$cmd(
            "./vcpkg",
            "install",
            str.fmt(_, "--triplet=%s", cexy$vcpkg_triplet),
            // NOTE: names of libs for vcpkg and pkgconf_libs can be different
            "curl",
            "libzip"
        ));
    }
    return EOK;
}
