#if __has_include("cex_config.h")
// Custom config file
#    include "cex_config.h"
#else
// Overriding config values
#    define cexy$cc_include "-I.", "-I./lib"
#    define cexy$pkgconf_libs "libcurl", "libzip"
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */

#    if _WIN32
// using mingw libs .a
#        define cexy$build_ext_lib_stat ".a"
// NOTE: windows is a special case, the best way to manage dependencies to have vcpkg
//       you have to manually install vcpkg and configure paths. Currently it uses static
//       environment and mingw because it was tested under MSYS2
//
//  Also install the following in `classic` mode:
//  > vcpkg install --triplet=x64-mingw-static curl
//  > vcpkg install --triplet=x64-mingw-static libzip

#        define cexy$vcpkg_triplet "x64-mingw-static"
#        define cexy$vcpkg_root "c:/vcpkg/"
#    else
// NOTE: linux / macos will use system wide libs
//       make sure you installed libcurl-dev libzip-dev via package manager
//       names of packages will depend on linux distro and macos home brew.
#    endif
#endif


#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"

Exception cmd_build_unzip(int argc, char** argv, void* user_ctx);

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
            { .name = "build-unzip", .func = cmd_build_unzip, .help = "Build cex_unzip app" },
        ),
    };
    if (argparse.parse(&args, argc, argv)) { return 1; }
    void* my_user_ctx = NULL; // passed as `user_ctx` to command
    if (argparse.run_command(&args, my_user_ctx)) { return 1; }
    return 0;
}

// NOTE:there is also cex_downloader app, it will be built using built-in cexy app engine
//      the only dependency needded for `cex_downloder` is `libcurl`, the next define string
//      must be included before #include "cex.h":
//      #define cexy$pkgconf_libs "libcurl"
// cmd_build_downloader(int argc, char** argv, void* user_ctx)
// {
//    // No need for extra build
//    // run with: ./cex app run cex_downloader
// }

/// Custom build command for building cex_unzip app (does low level building itself without cexy)
Exception
cmd_build_unzip(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    log$info("Building cex_unzip app\n");
    mem$scope(tmem$, _)
    {
        // This scope shows minimal steps for building programs in CEX 
        // 1. Finding main.c from name: cex_unzip -> `./src/cex_unzip/main.c`
        // 2. Checking if main.c needs rebuild (also checks all #include of main.c for changes)
        // 3. Building dynamic `cc` arguments + including dependency args
        char* app_name = "cex_unzip";
        char* app_src;
        e$ret(cexy.app.find_app_target_src(_, app_name, &app_src));
        char* app_exe = cexy.target_make(app_src, cexy$build_dir, app_name, _);
        if (cexy.src_include_changed(app_exe, app_src, NULL)) {
            log$info("Src: %s Tgt: %s\n", app_src, app_exe);

            arr$(char*) args = arr$new(args, _);
            arr$pushm(
                args,
                cexy$cc,
                "-Wall",
                "-Wextra",
                "-Werror",
                cexy$cc_include // -I / search path
            );

            // cexy$pkgconf: commands provides compiler flags for system dependencies,
            //   it calls `pkg-config` on the system, but can work with vcpkg environment too
            e$ret(cexy$pkgconf(_, &args, "--cflags", "libzip"));
            arr$pushm(args, (char*)app_src, "-o", app_exe);
            e$ret(cexy$pkgconf(_, &args, "--libs", "libzip"));
            arr$pushm(args, NULL);
            e$ret(os$cmda(args));

            log$info("Done, run with: %s\n", app_exe);
        }
    }
    return EOK;
}
