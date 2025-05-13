#if __has_include("cex_config.h")
// Custom config file
#    include "cex_config.h"
#else
// Overriding config values
#    define cexy$cc_include "-I.", "-I./lib"
#    define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#endif

#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"

Exception cmd_build_lib(int argc, char** argv, void* user_ctx);

int
main(int argc, char** argv)
{
    cexy$initialize(); // cex self rebuild and init

    e$except (err, cmd_build_lib(argc, argv, NULL)) { return 1; }
    return 0;
}

/// Custom build command for building static lib
Exception
cmd_build_lib(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;

    mem$scope(tmem$, _)
    {
        if (!os.path.exists(cexy$build_dir "/sqlite3.o") || !os.path.exists("./lib/sqlite3.h")) {
            e$assert(os.cmd.exists("curl") && "curl command is not installed");
            e$assert(os.cmd.exists("unzip") && "unzip command is not installed");
            log$info("Downloading SQLite source\n");
            char* sqlite_url = "https://www.sqlite.org/2025/sqlite-amalgamation-3490200.zip";
            e$ret(os.fs.mkdir(cexy$build_dir));
            e$ret(os$cmd("curl", "--silent", "--output-dir", cexy$build_dir, "-LO", sqlite_url));
            char* archive_path = str.fmt(
                _,
                "%s/%S",
                cexy$build_dir,
                os.path.split(sqlite_url, false)
            );
            e$ret(os$cmd("unzip", "-o", "-d", cexy$build_dir, archive_path));
            e$ret(os.fs.remove(archive_path));

            log$info("Building SQLite source\n");
            char* sqlite_src_dir = str.replace(archive_path, ".zip", "", _);
            e$ret(os.fs.chdir(sqlite_src_dir));

            double t = os.timer();
            os_fs_stat_s stats = os.fs.stat("./sqlite3.c");
            e$ret(os$cmd(cexy$cc, "-c", "-O2", "-g", "sqlite3.c", "-o", "../sqlite3.o"));
            log$info(
                "Compilation time: sqlite3.c file size: %ldKB compiled in %g seconds\n",
                stats.size / 1024,
                os.timer() - t
            );
            arr$(char*) args = arr$new(args, _);
            arr$pushm(args, cexy$cc, "shell.c", "../sqlite3.o", "-o", "../sqlite3");
            if (os.platform.current() == OSPlatform__win) {
                arr$pushm(args, "-lpthread", "-lm");
            } else {

                arr$pushm(args, "-lpthread", "-ldl", "-lm");
            }
            arr$push(args, NULL);
            e$ret(os$cmda(args));

            for$each (hdr, os.fs.find("*.h", false, _)) {
                char* dst = os$path_join(_, "..", "..", "lib", hdr);
                if (os.path.exists(dst)) { e$ret(os.fs.remove(dst)); }
                e$ret(os.fs.mkpath(dst));
                e$ret(os.fs.copy(hdr, dst));
            }
            // Cleanup
            e$ret(os.fs.chdir("../.."));
            e$ret(os.fs.remove_tree(sqlite_src_dir));
            log$info("SQLite build completed\n");
        }
    }
    e$assert(os.path.exists(cexy$build_dir "/sqlite3.o"));
    e$assert(os.path.exists("./lib/sqlite3.h"));

    // NOTE: only build when main.c is changed or hello_sqlite not exists
    if (cexy.src_include_changed(cexy$build_dir "/hello_sqlite", "src/main.c", NULL)) {
        log$info("Launching build\n");
        e$ret(os$cmd(
            cexy$cc,
            "src/main.c",
            cexy$build_dir "/sqlite3.o",
            cexy$cc_include,
            "-lpthread",
            "-lm",
            "-o",
            cexy$build_dir "/hello_sqlite"
        ));
    }

    log$info("Build complete launch: %s/%s hello.db\n", cexy$build_dir, "hello_sqlite");

    return EOK;
}
