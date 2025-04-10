#if __has_include("cex_config.h")
#include "cex_config.h"
#else
#define cexy$cc_include "-I.", "-I./lib/"
#endif

#define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#define CEX_IMPLEMENTATION
#define CEXBUILD
#include "cex.h"


Exception cmd_check(int argc, char** argv, void* user_ctx);
Exception cmd_build(int argc, char** argv, void* user_ctx);
Exception cmd_test(int argc, char** argv, void* user_ctx);
void cex_bundle(void);

int
main(int argc, char** argv)
{
    cex_bundle();
    cexy$initialize();

    // clang-format off
    argparse_c args = {
        .description = cexy$description,
        .epilog = cexy$epilog,
        .commands = (argparse_cmd_s[]) {
                { .name = "check", .func = cmd_check, .help = "Validates build environment" },
                { .name = "build", .func = cmd_build, .help = "Builds project", .is_default = 1 },
                { .name = "test", .func = cmd_test, .help = "Test running" },
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

void
cex_bundle(void)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) src = os.fs.find("src/*.[hc]", false, _);
        if (!cexy.src_changed("cex.h", src)) {
            return;
        }
        const char* bundle[] = {
            "src/cex_base.h", "src/mem.h",      "src/AllocatorHeap.h", "src/AllocatorArena.h",
            "src/ds.h",       "src/_sprintf.h", "src/str.h",           "src/sbuf.h",
            "src/io.h",       "src/argparse.h", "src/_subprocess.h",   "src/os.h",
            "src/test.h",     "src/cexy.h",     "src/CexLexer.h",
        };
        log$debug("Bundling cex.h: [%s]\n", str.join(bundle, arr$len(bundle), ", ", _));

        // Using CEX code generation engine for bundling
        sbuf_c hbuf = sbuf.create(1024 * 1024, _);
        cg$init(&hbuf);
        $pn("#pragma once");
        $pn("#ifndef CEX_HEADER_H");
        $pn("#define CEX_HEADER_H");

        char* cex_header;
        e$except_null(cex_header = io.file.load("src/cex_header.h", _))
        {
            exit(1);
        }
        $pn(cex_header);

        for$each(hdr, bundle)
        {
            $pn("\n");
            $pn("/*");
            $pf("*                          %s", hdr);
            $pn("*/");
            FILE* fh;
            e$except(err, io.fopen(&fh, hdr, "r"))
            {
                exit(1);
            }
            str_s content;
            e$except(err, io.fread_all(fh, &content, _))
            {
                exit(1);
            }
            printf("File %s, size: %ld\n", hdr, content.len);
            for$iter(str_s, it, str.slice.iter_split(content, "\n", &it.iterator))
            {
                io.printf("line: %04ld: %S\n", it.idx.i, it.val);

                if (str.slice.match(it.val, "#pragma once*")) {
                    continue;
                }
                if (str.slice.match(it.val, "#include \"*\"")) {
                    continue;
                }
                if (str.slice.match(it.val, "#include <*>")) {
                    continue;
                }
                // $pf("%S", *it.val);
            }
            fflush(stdout);
            break;
        }

        u32 cex_lines = 0;
        for$each(c, hbuf, sbuf.len(&hbuf)) {
            if (c == '\n') {
                cex_lines++;
            }
        }
        log$debug("Saving cex.h: new size: %dKB lines: %d\n", sbuf.len(&hbuf) / 1024, cex_lines);
        e$except(err, io.file.save("cex2.h", hbuf)){
            exit(1);
        }
    }
}

Exception
cmd_test(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    uassert(argc > 0);
    uassert(argv != NULL);

    argparse_c cmd_args = { 0 };
    e$ret(argparse.parse(&cmd_args, argc, argv));
    const char* cmd = argparse.next(&cmd_args);
    const char* target = argparse.next(&cmd_args);

    if (!str.match(cmd, "(run|build|create|clean|debug)")) {
        return e$raise(
            Error.argsparse,
            "Invalid command: '%s', expected {run, build, create, clean}",
            cmd
        );
    }
    if (target == NULL) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            target
        );
    }
    bool is_all = false;
    if (str.eq(target, "all")) {
        is_all = true;
        target = "tests/test_*.c";
    }

    if (!str.match(target, "test*.c")) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            target
        );
    }

    if (str.eq(cmd, "create")) {
        e$assert(!is_all && "all target is not supported by `create` command");
        log$info("test create (TODO)");
        return EOK;
    } else if (str.eq(cmd, "clean")) {
        if (is_all) {
            log$info("Cleaning all tests\n");
            e$ret(os.fs.remove_tree(cexy$build_dir "/tests/"));
        } else {
            log$info("Cleaning target: %s\n", target);
            e$ret(os.fs.remove(target));
        }
        return EOK;
    }

    // Build stage
    u32 n_tests = 0;
    u32 n_built = 0;
    mem$scope(tmem$, _)
    {
        for$each(test_src, os.fs.find(target, true, _))
        {
            char* test_target = cexy.target_make(test_src, cexy$build_dir, ".test", _);
            log$trace("Test src: %s -> %s\n", test_src, test_target);
            n_tests++;
            if (!cexy.src_include_changed(test_target, test_src, NULL)) {
                continue;
            }
            arr$(char*) args = arr$new(args, _);
            arr$pushm(
                args,
                cexy$cc,
                cexy$cc_args,
                cexy$cc_args_test,
                cexy$cc_include,
                // cexy$ld_args,
                test_src,
                // cexy$ld_libs,
                "-o",
                test_target,
            );
            arr$push(args, NULL);
            e$ret(os$cmda(args));
            n_built++;
        }
    }

    log$info("Tests building: %d tests processed, %d tests built\n", n_tests, n_built);

    Exc result = EOK;
    if (str.match(cmd, "(run|debug)")) {
        mem$scope(tmem$, _)
        {
            for$each(test_src, os.fs.find(target, true, _))
            {
                char* test_target = cexy.target_make(test_src, cexy$build_dir, ".test", _);
                arr$(char*) args = arr$new(args, _);
                if (str.eq(cmd, "debug")) {
                    arr$pushm(args, "gdb", "--args", );
                }
                arr$pushm(args, test_target, );
                if (is_all) {
                    arr$push(args, "--quiet");
                }
                arr$pusha(args, cmd_args.argv, cmd_args.argc);
                arr$push(args, NULL);
                if (os$cmda(args)) {
                    log$error("<<<<<<<<<<<<<<<<<< Test failed: %s\n", test_target);
                    result = Error.runtime;
                }
            }
        }
    }
    return result;
}

Exception
cmd_check(int argc, char** argv, void* user_ctx)
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
cmd_build(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;
    log$debug("Start building something\n");
    e$ret(os.fs.mkdir(cexy$build_dir));
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
