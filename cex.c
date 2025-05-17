#if __has_include("cex_config.h")
// These settings can be set via `./cex -D CEX_WINE config` command
#    include "cex_config.h"
#endif

#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"


Exception cmd_custom_test(int argc, char** argv, void* user_ctx);
Exception cmd_fuzz_test(int argc, char** argv, void* user_ctx);
void cex_bundle(void);

int
main(int argc, char** argv)
{
    cex_bundle();
    cexy$initialize();

#ifdef CEX_VALGRIND
    e$except (err, os.env.set("CEX_VALGRIND", "1")) {}
#endif


    // clang-format off
    argparse_c args = {
        .description = cexy$description,
        .usage = cexy$usage,
        .epilog = cexy$epilog,
        argparse$cmd_list(
            cexy$cmd_all,
            { .name = "test", .func = cmd_custom_test, .help = "Test running" },
            { .name = "fuzz", .func = cmd_fuzz_test, .help = "libFuzzer tester" },
            cexy$cmd_app,  /* feel free to make your own if needed */
        ),
    };
    // clang-format on

    if (argparse.parse(&args, argc, argv)) { return 1; }
    if (argparse.run_command(&args, NULL)) { return 1; }
    return 0;
}

Exception
cmd_fuzz_test(int argc, char** argv, void* user_ctx)
{
    // Extended test runner
    mem$scope(tmem$, _)
    {
        argparse_c cmd_args = {
            .program_name = "./cex",
            .usage = "fuzz fuzz/some/fuzz_file.c [-fuz-options]",
            .description = "Compiles and runs fuzz test on target",
            .epilog = _cexy$cmd_test_epilog,
            argparse$opt_list(argparse$opt_help(), ),
        };

        e$ret(argparse.parse(&cmd_args, argc, argv));
        char* src = argparse.next(&cmd_args);
        if (!os.path.exists(src)) { return e$raise(Error.not_found, "target not found: %s", src); }
        e$assert(os.cmd.exists("clang") && "only clang is supported");
        char* dir = os.path.dirname(src, _);
        char* file = os.path.basename(src, _);
        e$assert(str.ends_with(file, ".c"));
        str_s prefix = str.sub(file, 0, -2);
        char* inc_proj = str.fmt(_, "-I%s", os.path.abs(".", _));

        e$ret(os.fs.chdir(dir));

        char* target_exe = str.fmt(_, "./%S.fuzz", prefix);
        e$ret(os$cmd(
            "clang",
            inc_proj,
            "-g",
            "-fsanitize=address,fuzzer,undefined",
            "-fsanitize-undefined-trap-on-error",
            "-o",
            target_exe,
            file
        ));
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, target_exe, "-timeout=10", str.fmt(_, "-artifact_prefix=%S.", prefix));

        char* dict_file = str.fmt(_, "%S.dict", prefix);
        if (os.path.exists(dict_file)) { arr$push(args, str.fmt(_, "-dict=%s", dict_file)); }

        char* corpus_dir = str.fmt(_, "%S_corpus", prefix);
        if (os.path.exists(corpus_dir)) { arr$push(args, corpus_dir); }

        arr$push(args, NULL);
        e$ret(os$cmda(args));
    }
    return EOK;
}

Exception
cmd_custom_test(int argc, char** argv, void* user_ctx)
{
    // Extended test runner
    mem$scope(tmem$, _)
    {
        e$ret(os.fs.mkpath("tests/build/"));
        e$assert(os.path.exists("tests/build/"));
        log$trace("Finding/building simple os apps in tests/os_test/*.c\n");
        arr$(char*) test_app_src = os.fs.find("tests/os_test/*.c", false, _);

        for$each (src, test_app_src) {
            char* tgt_ext = NULL;
            char* test_launcher[] = { cexy$debug_cmd };
            if (arr$len(test_launcher) > 0 && str.eq(test_launcher[0], "wine")) {
                tgt_ext = str.fmt(_, ".%s", "win");
            } else {
                tgt_ext = str.fmt(_, ".%s", os.platform.to_str(os.platform.current()));
            }
            // target app is: build/tests/os_test/<app_name>.[platform][.exe]
            char* target = cexy.target_make(src, cexy$build_dir, tgt_ext, _);

            if (!cexy.src_include_changed(target, src, NULL)) { continue; }

            e$ret(os$cmd(cexy$cc, "-g", "-Wall", "-Wextra", "-o", target, src));
        }
    }

    return cexy.cmd.simple_test(argc, argv, user_ctx);
}

static void
embed_code(sbuf_c* buf, char* code_path)
{
    uassert(buf != NULL);
    uassertf(os.path.exists(code_path), "not exists: %s", code_path);

    FILE* fh;
    e$except (err, io.fopen(&fh, code_path, "r")) { exit(1); }
    e$goto(sbuf.append(buf, "\""), fail);

    char c;
    while ((c = fgetc(fh))) {
        if (feof(fh)) { break; }
        switch (c) {
            case '\\':
                e$goto(sbuf.append(buf, "\\"), fail);
                break;
            case '"':
                e$goto(sbuf.append(buf, "\\"), fail);
                break;
            case '\n':
                e$goto(sbuf.append(buf, "\\n\"\\\n\""), fail);
                continue;
            default:
                break;
        }

        e$goto(sbuf.appendf(buf, "%c", c), fail);
    }

    e$goto(sbuf.append(buf, "\""), fail);

    io.fclose(&fh);
    return;

fail:
    exit(1);
}

void
cex_bundle(void)
{
    mem$scope(tmem$, _)
    {
        arr$(char*) src = os.fs.find("src/*.[hc]", false, _);
        if (!cexy.src_changed("cex.h", src, arr$len(src))) { return; }
        char* bundle[] = {
            "src/cex_base.h", "src/mem.h",          "src/AllocatorHeap.h", "src/AllocatorArena.h",
            "src/ds.h",       "src/_sprintf.h",     "src/str.h",           "src/sbuf.h",
            "src/io.h",       "src/argparse.h",     "src/_subprocess.h",   "src/os.h",
            "src/test.h",     "src/cex_code_gen.h", "src/cexy.h",          "src/CexParser.h",
            "src/json.h",     "src/cex_maker.h"

        };
        log$debug("Bundling cex.h: [%s]\n", str.join(bundle, arr$len(bundle), ", ", _));

        // Using CEX code generation engine for bundling
        sbuf_c hbuf = sbuf.create(1024 * 1024, _);
        cg$init(&hbuf);
        $pn("#pragma once");
        $pn("#ifndef CEX_HEADER_H");
        $pn("#define CEX_HEADER_H");

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);
        char date[16];
        e$except (
            err,
            str.sprintf(
                date,
                sizeof(date),
                "%04d-%02d-%02d",
                tm.tm_year + 1900,
                tm.tm_mon + 1,
                tm.tm_mday
            )
        ) {
            exit(1);
        }

        char* cex_header;
        e$except_null (cex_header = io.file.load("src/cex_header.h", _)) { exit(1); }
        uassert(str.find(cex_header, "{date}") && "{date} template not found in cex_header.h");
        cex_header = str.replace(cex_header, "{date}", date, _);
        uassert(cex_header != NULL);

        $pn(cex_header);

        for$each (hdr, bundle) {
            $pn("\n");
            $pn("/*");
            $pf("*                          %s", hdr);
            $pn("*/");
            FILE* fh;
            e$except (err, io.fopen(&fh, hdr, "r")) { exit(1); }
            str_s content;
            e$except (err, io.fread_all(fh, &content, _)) { exit(1); }
            for$iter (str_s, it, str.slice.iter_split(content, "\n", &it.iterator)) {
                if (str.slice.match(it.val, "#pragma once*")) { continue; }
                if (str.slice.match(it.val, "#include \"*\"")) { continue; }
                $pf("%S", it.val);
            }
        }


        $pn("\n");
        $pn("/*");
        $pn("*                   CEX IMPLEMENTATION ");
        $pn("*/");
        $pn("\n\n");
        $pn("#if defined(CEX_IMPLEMENTATION) || defined(CEX_NEW)\n");

        e$except_null (cex_header = io.file.load("src/cex_header.c", _)) { exit(1); }
        $pn(cex_header);

        $pa("\n\n#define _cex_main_boilerplate %s\n", "\\");
        embed_code(&hbuf, "src/cex_boilerplate.c");

        for$each (hdr, bundle) {
            char* cfile = str.replace(hdr, ".h", ".c", _);
            $pn("\n");
            $pn("/*");
            $pf("*                          %s", cfile);
            $pn("*/");
            FILE* fh;
            e$except (err, io.fopen(&fh, cfile, "r")) { exit(1); }
            str_s content;
            e$except (err, io.fread_all(fh, &content, _)) { exit(1); }
            for$iter (str_s, it, str.slice.iter_split(content, "\n", &it.iterator)) {
                if (str.slice.match(it.val, "#pragma once*")) { continue; }
                if (str.slice.match(it.val, "#include \"*\"")) { continue; }
                $pf("%S", it.val);
            }
        }


        $pn("\n\n#endif // ifndef CEX_IMPLEMENTATION");
        $pn("\n\n#endif // ifndef CEX_HEADER_H");

        u32 cex_lines = 0;
        (void)cex_lines;
        for$each (c, hbuf, sbuf.len(&hbuf)) {
            if (c == '\n') { cex_lines++; }
        }
        log$debug("Saving cex.h: new size: %dKB lines: %d\n", sbuf.len(&hbuf) / 1024, cex_lines);
        e$except (err, io.file.save("cex.h", hbuf)) { exit(1); }
    }
}
