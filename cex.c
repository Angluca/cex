#if __has_include("cex_config.h")
    #include "cex_config.h"
#else
    #define cexy$cc_include "-I.", "-I./lib/"
    #define CEX_LOG_LVL 4 /* 0 (mute all) - 5 (log$trace) */
#endif

#define CEX_IMPLEMENTATION
#define CEX_BUILD
#include "cex.h"


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
        .usage = cexy$usage,
        .epilog = cexy$epilog,
        argparse$cmd_list(
            cexy$cmd_all,
            { .name = "test", .func = cexy.cmd.simple_test, .help = "Test running" },
        ),
    };
    // clang-format on

    if (argparse.parse(&args, argc, argv)) {
        return 1;
    }
    if (argparse.run_command(&args, NULL)) {
        return 1;
    }
    return 0;
}

static void
embed_code(sbuf_c* buf, char* code_path)
{
    uassert(buf != NULL);
    uassertf(os.path.exists(code_path), "not exists: %s", code_path);

    FILE* fh;
    e$except(err, io.fopen(&fh, code_path, "r"))
    {
        exit(1);
    }
    e$goto(sbuf.append(buf, "\""), fail);

    char c;
    while ((c = fgetc(fh))) {
        if (feof(fh)) {
            break;
        }
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
        if (!cexy.src_changed("cex.h", src)) {
            return;
        }
        const char* bundle[] = {
            "src/cex_base.h", "src/mem.h",          "src/AllocatorHeap.h", "src/AllocatorArena.h",
            "src/ds.h",       "src/_sprintf.h",     "src/str.h",           "src/sbuf.h",
            "src/io.h",       "src/argparse.h",     "src/_subprocess.h",   "src/os.h",
            "src/test.h",     "src/cex_code_gen.h", "src/cexy.h",          "src/CexParser.h",
            "src/cex_maker.h"

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
            for$iter(str_s, it, str.slice.iter_split(content, "\n", &it.iterator))
            {
                if (str.slice.match(it.val, "#pragma once*")) {
                    continue;
                }
                if (str.slice.match(it.val, "#include \"*\"")) {
                    continue;
                }
                $pf("%S", it.val);
            }
        }


        $pn("\n");
        $pn("/*");
        $pn("*                   CEX IMPLEMENTATION ");
        $pn("*/");
        $pn("\n\n");
        $pn("#if defined(CEX_IMPLEMENTATION) || defined(CEX_NEW)\n");

        e$except_null(cex_header = io.file.load("src/cex_header.c", _))
        {
            exit(1);
        }
        $pn(cex_header);

        $pa("\n\n#define _cex_main_boilerplate %s\n", "\\");
        embed_code(&hbuf, "src/cex_boilerplate.c");

        for$each(hdr, bundle)
        {
            char* cfile = str.replace(hdr, ".h", ".c", _);
            $pn("\n");
            $pn("/*");
            $pf("*                          %s", cfile);
            $pn("*/");
            FILE* fh;
            e$except(err, io.fopen(&fh, cfile, "r"))
            {
                exit(1);
            }
            str_s content;
            e$except(err, io.fread_all(fh, &content, _))
            {
                exit(1);
            }
            for$iter(str_s, it, str.slice.iter_split(content, "\n", &it.iterator))
            {
                if (str.slice.match(it.val, "#pragma once*")) {
                    continue;
                }
                if (str.slice.match(it.val, "#include \"*\"")) {
                    continue;
                }
                $pf("%S", it.val);
            }
        }


        $pn("\n\n#endif // ifndef CEX_IMPLEMENTATION");
        $pn("\n\n#endif // ifndef CEX_HEADER_H");

        u32 cex_lines = 0;
        (void)cex_lines;
        for$each(c, hbuf, sbuf.len(&hbuf))
        {
            if (c == '\n') {
                cex_lines++;
            }
        }
        log$debug("Saving cex.h: new size: %dKB lines: %d\n", sbuf.len(&hbuf) / 1024, cex_lines);
        e$except(err, io.file.save("cex.h", hbuf))
        {
            exit(1);
        }
    }
}
