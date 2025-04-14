#include "all.h"
#if defined(CEXBUILD)

static void
cexy_build_self(int argc, char** argv, const char* cex_source)
{
    mem$scope(tmem$, _)
    {
        uassert(str.ends_with(argv[0], "cex"));
        char* bin_path = argv[0];
#ifdef CEX_SELF_BUILD
        log$trace("Checking self build for executable: %s\n", bin_path);
        if (!cexy.src_include_changed(bin_path, cex_source, NULL)) {
            log$trace("%s unchanged, skipping self build\n", cex_source);
            // cex.c/cex.h are up to date no rebuild needed
            return;
        }
#endif

        log$info("Rebuilding self: %s -> %s\n", cex_source, bin_path);
        char* old_name = str.fmt(_, "%s.old", bin_path);
        if (os.path.exists(bin_path)) {
            if (os.fs.remove(old_name)) {
            }
            e$except(err, os.fs.rename(bin_path, old_name))
            {
                goto err;
            }
        }
        arr$(const char*) args = arr$new(args, _);
        arr$pushm(args, cexy$cc, "-DCEX_SELF_BUILD", "-g", "-o", bin_path, cex_source, NULL);
        _os$args_print("CMD:", args, arr$len(args));
        os_cmd_c _cmd = { 0 };
        e$except(err, os.cmd.run(args, arr$len(args), &_cmd))
        {
            goto fail_recovery;
        }
        e$except(err, os.cmd.join(&_cmd, 0, NULL))
        {
            goto fail_recovery;
        }

        // All good new build successful, remove old binary
        if (os.fs.remove(old_name)) {
        }

        // run rebuilt cex executable again
        arr$clear(args);
        arr$pushm(args, bin_path);
        arr$pusha(args, &argv[1], argc - 1);
        arr$pushm(args, NULL);
        _os$args_print("CMD:", args, arr$len(args));
        e$except(err, os.cmd.run(args, arr$len(args), &_cmd))
        {
            goto err;
        }
        if (os.cmd.join(&_cmd, 0, NULL)) {
            goto err;
        }
        exit(0); // unconditionally exit after build was successful
    fail_recovery:
        if (os.path.exists(old_name)) {
            e$except(err, os.fs.rename(old_name, bin_path))
            {
                goto err;
            }
        }
        goto err;
    err:
        exit(1);
    }
}

static bool
cexy_src_include_changed(const char* target_path, const char* src_path, arr$(char*) alt_include_path)
{
    if (unlikely(target_path == NULL)) {
        log$error("target_path is NULL\n");
        return false;
    }
    if (unlikely(src_path == NULL)) {
        log$error("src_path is NULL\n");
        return false;
    }

    var src_meta = os.fs.stat(src_path);
    if (!src_meta.is_valid) {
        (void)e$raise(src_meta.error, "Error src: %s", src_path);
        return false;
    } else if (!src_meta.is_file || src_meta.is_symlink) {
        (void)e$raise("Bad type", "src is not a file: %s", src_path);
        return false;
    }

    var target_meta = os.fs.stat(target_path);
    if (!target_meta.is_valid) {
        if (target_meta.error == Error.not_found) {
            return true;
        } else {
            (void)e$raise(target_meta.error, "target_path: '%s'", target_path);
            return false;
        }
    } else if (!target_meta.is_file || target_meta.is_symlink) {
        (void)e$raise("Bad type", "target_path is not a file: %s", target_path);
        return false;
    }

    if (src_meta.mtime > target_meta.mtime) {
        // File itself changed
        log$debug("Src changed: %s\n", src_path);
        return true;
    }

    mem$scope(tmem$, _)
    {
        arr$(const char*) incl_path = arr$new(incl_path, _);
        if (arr$len(alt_include_path) > 0) {
            for$each(p, alt_include_path)
            {
                arr$push(incl_path, p);
                if (!os.path.exists(p)) {
                    log$warn("alt_include_path not exists: %s\n", p);
                }
            }
        } else {
            const char* def_incl_path[] = { cexy$cc_include };
            for$each(p, def_incl_path)
            {
                const char* clean_path = p;
                if (str.starts_with(p, "-I")) {
                    clean_path = p + 2;
                } else if (str.starts_with(p, "-iquote=")) {
                    clean_path = p + strlen("-iquote=");
                }
                if (!os.path.exists(clean_path)) {
                    log$warn("cexy$cc_include not exists: %s\n", clean_path);
                }
                arr$push(incl_path, clean_path);
            }
        }

        char* code = io.file.load(src_path, _);
        if (code == NULL) {
            (void)e$raise("IOError", "src is not a file: '%s'", src_path);
            return false;
        }

        (void)CexTkn_str;
        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        while ((t = CexParser.next_token(&lx)).type) {
            if (t.type != CexTkn__preproc) {
                continue;
            }
            if (str.slice.starts_with(t.value, str$s("include"))) {
                str_s incf = str.slice.sub(t.value, strlen("include"), 0);
                incf = str.slice.strip(incf);
                if (incf.len <= 4) { // <.h>
                    log$warn(
                        "Bad include in: %s (item: %S at line: %d)\n",
                        src_path,
                        t.value,
                        lx.line
                    );
                    continue;
                }
                log$trace("Processing include: '%S'\n", incf);
                if (!str.slice.match(incf, "\"*.[hc]\"")) {
                    // system includes skipped
                    log$trace("Skipping include: '%S'\n", incf);
                    continue;
                }
                incf = str.slice.sub(incf, 1, -1);

                mem$scope(tmem$, _)
                {
                    char* inc_fn = str.slice.clone(incf, _);
                    uassert(inc_fn != NULL);
                    for$each(inc_dir, incl_path)
                    {
                        char* try_path = os$path_join(_, inc_dir, inc_fn);
                        uassert(try_path != NULL);
                        var src_meta = os.fs.stat(try_path);
                        log$trace("Probing include: %s\n", try_path);
                        if (src_meta.is_valid && src_meta.mtime > target_meta.mtime) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

static bool
cexy_src_changed(const char* target_path, arr$(char*) src_array)
{
    usize src_array_len = arr$len(src_array);
    if (unlikely(src_array == NULL || src_array_len == 0)) {
        if (src_array == NULL) {
            log$error("src_array is NULL, which may indicate error\n");
        } else {
            log$error("src_array is empty\n");
        }
        return false;
    }
    if (unlikely(target_path == NULL)) {
        log$error("target_path is NULL\n");
        return false;
    }
    var target_ftype = os.fs.stat(target_path);
    if (!target_ftype.is_valid) {
        if (target_ftype.error == Error.not_found) {
            log$trace("Target '%s' not exists, needs build.\n", target_path);
            return true;
        } else {
            (void)e$raise(target_ftype.error, "target_path: '%s'", target_path);
            return false;
        }
    } else if (!target_ftype.is_file || target_ftype.is_symlink) {
        (void)e$raise("Bad type", "target_path is not a file: %s", target_path);
        return false;
    }

    bool has_error = false;
    for$each(p, src_array, src_array_len)
    {
        var ftype = os.fs.stat(p);
        if (!ftype.is_valid) {
            (void)e$raise(ftype.error, "Error src: %s", p);
            has_error = true;
        } else if (!ftype.is_file || ftype.is_symlink) {
            (void)e$raise("Bad type", "src is not a file: %s", p);
            has_error = true;
        } else {
            if (has_error) {
                continue;
            }
            if (ftype.mtime > target_ftype.mtime) {
                log$trace("Source changed '%s'\n", p);
                return true;
            }
        }
    }

    return false;
}

static char*
cexy_target_make(
    const char* src_path,
    const char* build_dir,
    const char* name_or_extension,
    IAllocator allocator
)
{
    uassert(allocator != NULL);

    if (src_path == NULL || src_path[0] == '\0') {
        log$error("src_path is NULL or empty\n");
        return NULL;
    }
    if (build_dir == NULL) {
        log$error("build_dir is NULL\n");
        return NULL;
    }
    if (name_or_extension == NULL || name_or_extension[0] == '\0') {
        log$error("name_or_extension is NULL or empty\n");
        return NULL;
    }
    if (!os.path.exists(src_path)) {
        log$error("src_path does not exist: '%s'\n", src_path);
        return NULL;
    }

    char* result = NULL;
    if (name_or_extension[0] == '.') {
        // starts_with .ext, make full path as following: build_dir/src_path/src_file.ext
        result = str.fmt(allocator, "%s%c%s%s", build_dir, os$PATH_SEP, src_path, name_or_extension);
        uassert(result != NULL && "memory error");
    } else {
        // probably a program name, make full path: build_dir/name_or_extension
        result = str.fmt(allocator, "%s%c%s", build_dir, os$PATH_SEP, name_or_extension);
        uassert(result != NULL && "memory error");
    }
    e$except(err, os.fs.mkpath(result))
    {
        mem$free(allocator, result);
    }

    return result;
}

Exception
cexy__test__create(const char* target)
{
    if (os.path.exists(target)) {
        return e$raise(Error.exists, "Test file already exists: %s", target);
    }
    if (str.eq(target, "all") || str.find(target, "*")) {
        return e$raise(
            Error.argument,
            "You must pass exact file path, not pattern, got: %s",
            target
        );
    }

    mem$scope(tmem$, _)
    {
        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        $pn("#define CEX_IMPLEMENTATION");
        $pn("#include \"cex.h\"");
        $pn("");
        $pn("//test$setup_case() {return EOK;}");
        $pn("//test$teardown_case() {return EOK;}");
        $pn("//test$setup_suite() {return EOK;}");
        $pn("//test$teardown_suite() {return EOK;}");
        $pn("");
        $scope("test$case(%s)", "my_test_case")
        {
            $pn("tassert_eq(1, 0);");
            $pn("return EOK;");
        }
        $pn("");
        $pn("test$main();");

        e$ret(io.file.save(target, buf));
    }
    return EOK;
}

Exception
cexy__test__clean(const char* target)
{
    if (os.path.exists(target)) {
        return e$raise(Error.exists, "Test file already exists: %s", target);
    }

    if (str.eq(target, "all")) {
        log$info("Cleaning all tests\n");
        e$ret(os.fs.remove_tree(cexy$build_dir "/tests/"));
    } else {
        log$info("Cleaning target: %s\n", target);
        e$ret(os.fs.remove(target));
    }
    return EOK;
}

Exception
cexy__test__make_target_pattern(const char** target)
{
    if (target == NULL) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            *target
        );
    }
    if (str.eq(*target, "all")) {
        *target = "tests/test_*.c";
    }

    if (!str.match(*target, "*test*.c")) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            *target
        );
    }
    return EOK;
}

Exception
cexy__test__run(const char* target, bool is_debug, int argc, char** argv)
{
    Exc result = EOK;
    u32 n_tests = 0;
    u32 n_failed = 0;
    mem$scope(tmem$, _)
    {
        for$each(test_src, os.fs.find(target, true, _))
        {
            n_tests++;
            char* test_target = cexy.target_make(test_src, cexy$build_dir, ".test", _);
            arr$(char*) args = arr$new(args, _);
            if (is_debug) {
                if (str.ends_with(target, "test_*.c")) {
                    return e$raise(
                        Error.argument,
                        "Debug expect direct file path, i.e. tests/test_some_file.c, got: %s",
                        target
                    );
                }
                arr$pushm(args, cexy$debug_cmd);
            }
            arr$pushm(args, test_target, );
            if (str.ends_with(target, "test_*.c")) {
                arr$push(args, "--quiet");
            }
            arr$pusha(args, argv, argc);
            arr$push(args, NULL);
            if (os$cmda(args)) {
                log$error("<<<<<<<<<<<<<<<<<< Test failed: %s\n", test_target);
                n_failed++;
                result = Error.runtime;
            }
        }
    }
    if (str.ends_with(target, "test_*.c")) {
        log$info(
            "Test run completed: %d tests, %d passed, %d failed\n",
            n_tests,
            n_tests - n_failed,
            n_failed
        );
    }
    return result;
}

static int
cexy__decl_comparator(const void* a, const void* b)
{
    cex_decl_s** _a = (cex_decl_s**)a;
    cex_decl_s** _b = (cex_decl_s**)b;

    // Makes str__sub__ to be placed after
    isize isub_a = str.slice.index_of(_a[0]->name, str$s("__"));
    isize isub_b = str.slice.index_of(_b[0]->name, str$s("__"));
    if (unlikely(isub_a != isub_b)) {
        if (isub_a != -1) {
            return 1;
        } else {
            return -1;
        }
    }

    return str.slice.cmp(_a[0]->name, _b[0]->name);
}

static Exception
cexy__process_gen_struct(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
{
    cg$init(out_buf);
    str_s subn = { 0 };

    $scope("struct __cex_namespace__%S ", ns_prefix)
    {
        $pn("// Autogenerated by CEX");
        $pn("// clang-format off");
        $pn("");
        for$each(it, decls)
        {
            str_s clean_name = str.slice.sub(it->name, ns_prefix.len + 1, 0);
            if (str.slice.starts_with(clean_name, str$s("_"))) {
                // sub-namespace
                isize ns_end = str.slice.index_of(clean_name, str$s("__"));
                e$assert(ns_end >= 0);
                str_s new_ns = str.slice.sub(clean_name, 1, ns_end);
                if (!str.slice.eq(new_ns, subn)) {
                    if (subn.buf != NULL) {
                        // End previous sub-namespace
                        cg$dedent();
                        $pf("} %S;", subn);
                    }

                    // new subnamespace begin
                    $pn("");
                    $pf("struct {", subn);
                    cg$indent();
                    subn = new_ns;
                }
                clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
            }
            $pf("%-15s (*%S)(%s);", it->ret_type, clean_name, it->args);
        }

        if (subn.buf != NULL) {
            // End previous sub-namespace
            cg$dedent();
            $pf("} %S;", subn);
        }
        $pn("");
        $pn("// clang-format on");
    }
    $pa("%s", ";");

    if (!cg$is_valid()) {
        return e$raise(Error.runtime, "Code generation error occured\n");
    }
    return EOK;
}

static Exception
cexy__process_gen_var_def(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
{
    cg$init(out_buf);
    str_s subn = { 0 };

    $scope("const struct __cex_namespace__%S %S = ", ns_prefix, ns_prefix)
    {
        $pn("// Autogenerated by CEX");
        $pn("// clang-format off");
        $pn("");
        for$each(it, decls)
        {
            str_s clean_name = str.slice.sub(it->name, ns_prefix.len + 1, 0);
            if (str.slice.starts_with(clean_name, str$s("_"))) {
                // sub-namespace
                isize ns_end = str.slice.index_of(clean_name, str$s("__"));
                e$assert(ns_end >= 0);
                str_s new_ns = str.slice.sub(clean_name, 1, ns_end);
                if (!str.slice.eq(new_ns, subn)) {
                    if (subn.buf != NULL) {
                        // End previous sub-namespace
                        cg$dedent();
                        $pn("},");
                    }

                    // new subnamespace begin
                    $pn("");
                    $pf(".%S = {", new_ns);
                    cg$indent();
                    subn = new_ns;
                }
                clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
            }
            $pf(".%S = %S,", clean_name, it->name);
        }

        if (subn.buf != NULL) {
            // End previous sub-namespace
            cg$dedent();
            $pf("},", subn);
        }
        $pn("");
        $pn("// clang-format on");
    }
    $pa("%s", ";");

    if (!cg$is_valid()) {
        return e$raise(Error.runtime, "Code generation error occured\n");
    }
    return EOK;
}

static Exception
cexy__process_update_code(
    const char* code_file,
    bool only_update,
    sbuf_c cex_h_struct,
    sbuf_c cex_h_var_decl,
    sbuf_c cex_c_var_def
)
{
    mem$scope(tmem$, _)
    {
        char* code = io.file.load(code_file, _);
        e$assert(code && "failed loading code");

        bool is_header = str.ends_with(code_file, ".h");

        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        sbuf_c new_code = sbuf.create(10 * 1024 + (lx.content_end - lx.content), _);
        CexParser.reset(&lx);

        str_s code_buf = { .buf = lx.content, .len = 0 };
        bool has_module_def = false;
        bool has_module_decl = false;
        bool has_module_struct = false;
        arr$(cex_token_s) items = arr$new(items, _);

#define $dump_prev()                                                                               \
    code_buf.len = t.value.buf - code_buf.buf - ((t.value.buf > lx.content) ? 1 : 0);              \
    if (code_buf.buf != NULL)                                                                      \
        e$ret(sbuf.appendf(&new_code, "%S\n", code_buf));                                          \
    code_buf = (str_s){ 0 }
#define $dump_prev_comment()                                                                       \
    for$each(it, items)                                                                            \
    {                                                                                              \
        if (it.type == CexTkn__comment_single || it.type == CexTkn__comment_multi) {               \
            e$ret(sbuf.appendf(&new_code, "%S\n", it.value));                                      \
        } else {                                                                                   \
            break;                                                                                 \
        }                                                                                          \
    }

        while ((t = CexParser.next_entity(&lx, &items)).type) {
            if (t.type == CexTkn__cex_module_def) {
                e$assert(!is_header && "expected in .c file buf got header");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_def) {
                    e$ret(sbuf.appendf(&new_code, "%s\n", cex_c_var_def));
                }
                has_module_def = true;
            } else if (t.type == CexTkn__cex_module_decl) {
                e$assert(is_header && "expected in .h file buf got source");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_decl) {
                    e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_var_decl));
                }
                has_module_decl = true;
            } else if (t.type == CexTkn__cex_module_struct) {
                e$assert(is_header && "expected in .h file buf got source");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_struct) {
                    e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_struct));
                }
                has_module_struct = true;
            } else {
                if (code_buf.buf == NULL) {
                    code_buf.buf = t.value.buf;
                }
                code_buf.len = t.value.buf - code_buf.buf + t.value.len;
            }
        }
        if (code_buf.len) {
            e$ret(sbuf.appendf(&new_code, "%S\n", code_buf));
        }
        if (!is_header) {
            if (!has_module_def) {
                if (only_update) {
                    return EOK;
                }
                e$ret(sbuf.appendf(&new_code, "\n%s\n", cex_c_var_def));
            }
            e$ret(io.file.save(code_file, new_code));
        } else {
            if (!has_module_struct) {
                if (only_update) {
                    return EOK;
                }
                e$ret(sbuf.appendf(&new_code, "\n%s\n", cex_h_struct));
            }
            if (!has_module_decl) {
                if (only_update) {
                    return EOK;
                }
                e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_var_decl));
            }
            e$ret(io.file.save(code_file, new_code));
        }
    }

#undef $dump_prev
#undef $dump_prev_comment
    return EOK;
}

static Exception
cexy__cmd__process(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    const char* process_help = ""
    "process command intended for building CEXy interfaces from your source code\n\n"
    "For example: you can create foo_fun1(), foo_fun2(), foo__bar__fun3(), foo__bar__fun4()\n"
    "   these functions will be processed and wrapped to a `foo` namespace, so you can     \n"
    "   access them via foo.fun1(), foo.fun2(), foo.bar.fun3(), foo.bar.fun4()\n\n" 

    "Requirements / caveats: \n"
    "1. You must have foo.c and foo.h in the same folder\n"
    "2. Filename must start with `foo` - namespace prefix\n"
    "3. Each function in foo.c that you'd like to add to namespace must start with `foo_`\n"
    "4. For adding sub-namespace use `foo__subname__` prefix\n"
    "5. Only one level of sub-namespace is allowed\n"
    "6. You may not declare function signature in header, and ony use .c static functions\n"
    "7. Functions with `static inline` are not included into namespace\n" 
    "8. Functions with prefix `foo__some` are considered internal and not included\n"
    "9. New namespace is created when you use exact foo.c argument, `all` just for updates\n"

    ;
    // clang-format on

    const char* ignore_kw = cexy$process_ignore_kw;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "process [options] all|path/some_file.c",
        .description = process_help,
        .epilog = "Use `all` for updates, and exact path/some_file.c for creating new\n",
        .options =
            (argparse_opt_s[]){
                argparse$opt_group("Options"),
                argparse$opt_help(),
                argparse$opt(
                    &ignore_kw,
                    'i',
                    "ignore",
                    .help = "ignores `keyword` or `keyword()` from processed function signatures\n  uses cexy$process_ignore_kw"
                ),
                { 0 },
            }
    };
    e$ret(argparse.parse(&cmd_args, argc, argv));
    const char* target = argparse.next(&cmd_args);

    if (target == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or path/some_file.c",
            target
        );
    }


    bool only_update = true;
    if (str.eq(target, "all")) {
        target = "*.c";
    } else {
        if (!os.path.exists(target)) {
            return e$raise(Error.not_found, "Target file not exists: '%s'", target);
        }
        only_update = false;
    }


    mem$scope(tmem$, _)
    {
        for$each(src_fn, os.fs.find(target, true, _))
        {
            if (str.starts_with(src_fn, "test")) {
                continue;
            }
            mem$scope(tmem$, _)
            {
                e$assert(str.ends_with(src_fn, ".c") && "file must end with .c");

                char* hdr_fn = str.clone(src_fn, _);
                hdr_fn[str.len(hdr_fn) - 1] = 'h'; // .c -> .h

                str_s ns_prefix = str.sub(os.path.basename(src_fn, _), 0, -2); // src.c -> src
                char* fn_sub_pattern = str.fmt(_, "%S__[a-zA-Z0-9+]__*", ns_prefix);
                char* fn_pattern = str.fmt(_, "%S_*", ns_prefix);
                char* fn_private = str.fmt(_, "%S__*", ns_prefix);

                log$debug(
                    "Cex Processing src: '%s' hdr: '%s' prefix: '%S'\n",
                    src_fn,
                    hdr_fn,
                    ns_prefix
                );
                if (!os.path.exists(hdr_fn)) {
                    return e$raise(Error.not_found, "Header file not exists: '%s'", hdr_fn);
                }
                char* code = io.file.load(src_fn, _);
                e$assert(code && "failed loading code");
                arr$(cex_token_s) items = arr$new(items, _);
                arr$(cex_decl_s*) decls = arr$new(decls, _, .capacity = 128);

                CexParser_c lx = CexParser.create(code, 0, true);
                cex_token_s t;
                while ((t = CexParser.next_entity(&lx, &items)).type) {
                    if (t.type == CexTkn__error) {
                        return e$raise(
                            Error.integrity,
                            "Error parsing file %s, at line: %d, cursor: %d",
                            src_fn,
                            lx.line,
                            (i32)(lx.cur - lx.content)
                        );
                    }
                    cex_decl_s* d = CexParser.decl_parse(t, items, cexy$process_ignore_kw, _);
                    if (d == NULL) {
                        continue;
                    }
                    if (d->type != CexTkn__func_def) {
                        continue;
                    }
                    if (d->is_inline && d->is_static) {
                        continue;
                    }
                    if (str.slice.match(d->name, fn_sub_pattern)) {
                        // OK use it!
                    } else if (str.slice.match(d->name, fn_private) ||
                               !str.slice.match(d->name, fn_pattern)) {
                        continue;
                    }
                    log$trace("FN: %S ret_type: '%s' args: '%s'\n", d->name, d->ret_type, d->args);
                    arr$push(decls, d);
                }
                if (arr$len(decls) == 0) {
                    log$info("CEX process skipped (no cex decls found in : %s)\n", src_fn);
                    continue;
                }

                qsort(decls, arr$len(decls), sizeof(*decls), cexy__decl_comparator);

                sbuf_c cex_h_struct = sbuf.create(10 * 1024, _);
                sbuf_c cex_h_var_decl = sbuf.create(1024, _);
                sbuf_c cex_c_var_def = sbuf.create(10 * 1024, _);

                e$ret(sbuf.appendf(
                    &cex_h_var_decl,
                    "__attribute__((visibility(\"hidden\"))) extern const struct __cex_namespace__%S %S;\n",
                    ns_prefix,
                    ns_prefix
                ));
                e$ret(cexy__process_gen_struct(ns_prefix, decls, &cex_h_struct));
                e$ret(cexy__process_gen_var_def(ns_prefix, decls, &cex_c_var_def));
                e$ret(cexy__process_update_code(
                    src_fn,
                    only_update,
                    cex_h_struct,
                    cex_h_var_decl,
                    cex_c_var_def
                ));
                e$ret(cexy__process_update_code(
                    hdr_fn,
                    only_update,
                    cex_h_struct,
                    cex_h_var_decl,
                    cex_c_var_def
                ));

                // log$info("cex_h_struct: \n%s\n", cex_h_struct);
                log$info("CEX processed: %s\n", src_fn);
            }
        }
    }
    return EOK;
}


const struct __cex_namespace__cexy cexy = {
    // Autogenerated by CEX
    // clang-format off

    .build_self = cexy_build_self,
    .src_changed = cexy_src_changed,
    .src_include_changed = cexy_src_include_changed,
    .target_make = cexy_target_make,

    .cmd = {
        .process = cexy__cmd__process,
    },

    .test = {
        .clean = cexy__test__clean,
        .create = cexy__test__create,
        .make_target_pattern = cexy__test__make_target_pattern,
        .run = cexy__test__run,
    },

    // clang-format on
};
#endif // defined(CEXBUILD)
