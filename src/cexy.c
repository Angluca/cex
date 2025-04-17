#include "all.h"
#if defined(CEX_BUILD)

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
        // arr$pushm(args, cexy$cc, "-fsanitize-address-use-after-scope", "-fsanitize=address",
        // "-fsanitize=undefined", "-DCEX_SELF_BUILD", "-g", "-o", bin_path, cex_source, NULL);
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
cexy__test__create(const char* target, bool include_sample)
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
    e$ret(os.fs.mkpath(target));

    mem$scope(tmem$, _)
    {
        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        $pn("#define CEX_IMPLEMENTATION");
        $pn("#include \"cex.h\"");
        if (include_sample) {
            $pn("#include \"src/mylib.c\"");
        }
        $pn("");
        $pn("//test$setup_case() {return EOK;}");
        $pn("//test$teardown_case() {return EOK;}");
        $pn("//test$setup_suite() {return EOK;}");
        $pn("//test$teardown_suite() {return EOK;}");
        $pn("");
        $scope("test$case(%s)", (include_sample) ? "mylib_test_case" : "my_test_case")
        {
            if (include_sample) {
                $pn("tassert_eq(mylib_add(1, 2), 3);");
                $pn("// Next will be available after calling `cex process src/mylib.c`");
                $pn("// tassert_eq(mylib.add(1, 2), 3);");
            } else {
                $pn("tassert_eq(1, 0);");
            }
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
_cexy__decl_comparator(const void* a, const void* b)
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

    return str.slice.qscmp(&_a[0]->name, &_b[0]->name);
}

static str_s
_cexy__process_make_brief_docs(cex_decl_s* decl)
{
    str_s brief_str = { 0 };
    if (!decl->docs.buf) {
        return brief_str;
    }

    if (str.slice.starts_with(decl->docs, str$s("///"))) {
        // doxygen style ///
        brief_str = str.slice.strip(str.slice.sub(decl->docs, 3, 0));
    } else if (str.slice.match(decl->docs, "/\\*[\\*!]*")) {
        // doxygen style /** or /*!
        for$iter(
            str_s,
            it,
            str.slice.iter_split(str.slice.sub(decl->docs, 3, -2), "\n", &it.iterator)
        )
        {
            str_s line = str.slice.strip(it.val);
            if (line.len == 0) {
                continue;
            }
            brief_str = line;
            break;
        }
    }
    isize bridx = (str.slice.index_of(brief_str, str$s("@brief")));
    if (bridx == -1) {
        bridx = str.slice.index_of(brief_str, str$s("\\brief"));
    }
    if (bridx != -1) {
        brief_str = str.slice.strip(str.slice.sub(brief_str, bridx + strlen("@brief"), 0));
    }

    return brief_str;
}

static Exception
_cexy__process_gen_struct(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
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
            str_s clean_name = it->name;
            if (str.slice.starts_with(clean_name, str$s("cex_"))) {
                clean_name = str.slice.sub(clean_name, 4, 0);
            }
            clean_name = str.slice.sub(clean_name, ns_prefix.len + 1, 0);

            if (str.slice.starts_with(clean_name, str$s("_"))) {
                // sub-namespace
                isize ns_end = str.slice.index_of(clean_name, str$s("__"));
                uassert(ns_end >= 0);
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
            str_s brief_str = _cexy__process_make_brief_docs(it);
            if (brief_str.len) {
                $pf("/// %S", brief_str);
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
_cexy__process_gen_var_def(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
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
            str_s clean_name = it->name;
            if (str.slice.starts_with(clean_name, str$s("cex_"))) {
                clean_name = str.slice.sub(clean_name, 4, 0);
            }
            clean_name = str.slice.sub(clean_name, ns_prefix.len + 1, 0);

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
_cexy__process_update_code(
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

    #define $dump_prev()                                                                           \
        code_buf.len = t.value.buf - code_buf.buf - ((t.value.buf > lx.content) ? 1 : 0);          \
        if (code_buf.buf != NULL)                                                                  \
            e$ret(sbuf.appendf(&new_code, "%S\n", code_buf));                                      \
        code_buf = (str_s){ 0 }
    #define $dump_prev_comment()                                                                   \
        for$each(it, items)                                                                        \
        {                                                                                          \
            if (it.type == CexTkn__comment_single || it.type == CexTkn__comment_multi) {           \
                e$ret(sbuf.appendf(&new_code, "%S\n", it.value));                                  \
            } else {                                                                               \
                break;                                                                             \
            }                                                                                      \
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

static bool
_cexy__fn_match(str_s fn_name, str_s ns_prefix)
{

    mem$scope(tmem$, _)
    {
        char* fn_sub_pattern = str.fmt(_, "%S__[a-zA-Z0-9+]__*", ns_prefix);
        char* fn_sub_pattern_cex = str.fmt(_, "cex_%S__[a-zA-Z0-9+]__*", ns_prefix);
        char* fn_pattern = str.fmt(_, "%S_*", ns_prefix);
        char* fn_pattern_cex = str.fmt(_, "cex_%S_*", ns_prefix);
        char* fn_private = str.fmt(_, "%S__*", ns_prefix);
        char* fn_private_cex = str.fmt(_, "cex_%S__*", ns_prefix);

        if (str.slice.starts_with(fn_name, str$s("_"))) {
            continue;
        }
        if (str.slice.match(fn_name, fn_sub_pattern) ||
            str.slice.match(fn_name, fn_sub_pattern_cex)) {
            return true;
        } else if ((str.slice.match(fn_name, fn_private) || str.slice.match(fn_name, fn_private_cex)
                   ) ||
                   (!str.slice.match(fn_name, fn_pattern_cex) &&
                    !str.slice.match(fn_name, fn_pattern))) {
            return false;
        }
    }

    return true;
}

static str_s
_cexy__fn_dotted(str_s fn_name, const char* expected_ns, IAllocator alloc)
{
    str_s clean_name = fn_name;
    if (str.slice.starts_with(clean_name, str$s("cex_"))) {
        clean_name = str.slice.sub(clean_name, 4, 0);
    }
    if (!str.eq(expected_ns, "cex") && !str.slice.starts_with(clean_name, str.sstr(expected_ns))) {
        return (str_s){ 0 };
    }

    isize ns_idx = str.slice.index_of(clean_name, str$s("_"));
    if (ns_idx < 0) {
        return (str_s){ 0 };
    }
    str_s ns = str.slice.sub(clean_name, 0, ns_idx);
    clean_name = str.slice.sub(clean_name, ns.len + 1, 0);

    str_s subn = { 0 };
    if (str.slice.starts_with(clean_name, str$s("_"))) {
        // sub-namespace
        isize ns_end = str.slice.index_of(clean_name, str$s("__"));
        if (ns_end < 0) {
            // looks like a private func, str__something
            return (str_s){ 0 };
        }
        subn = str.slice.sub(clean_name, 1, ns_end);
        clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
    }
    if (subn.len) {
        return str.sstr(str.fmt(alloc, "%S.%S.%S", ns, subn, clean_name));
    } else {
        return str.sstr(str.fmt(alloc, "%S.%S", ns, clean_name));
    }
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
        .epilog = "\nUse `all` for updates, and exact path/some_file.c for creating new\n",
        argparse$opt_list(
            argparse$opt_group("Options"),
            argparse$opt_help(),
            argparse$opt(
                &ignore_kw,
                'i',
                "ignore",
                .help = "ignores `keyword` or `keyword()` from processed function signatures\n  uses cexy$process_ignore_kw"
            ),
        ),
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
            char* basename = os.path.basename(src_fn, _);
            if (str.starts_with(basename, "test") || str.eq(basename, "cex.c")) {
                continue;
            }
            mem$scope(tmem$, _)
            {
                e$assert(str.ends_with(src_fn, ".c") && "file must end with .c");

                char* hdr_fn = str.clone(src_fn, _);
                hdr_fn[str.len(hdr_fn) - 1] = 'h'; // .c -> .h

                str_s ns_prefix = str.sub(os.path.basename(src_fn, _), 0, -2); // src.c -> src
                log$debug(
                    "Cex Processing src: '%s' hdr: '%s' prefix: '%S'\n",
                    src_fn,
                    hdr_fn,
                    ns_prefix
                );
                if (!os.path.exists(hdr_fn)) {
                    if (only_update) {
                        log$debug("CEX skipped (no .h file for: %s)\n", src_fn);
                        continue;
                    } else {
                        return e$raise(Error.not_found, "Header file not exists: '%s'", hdr_fn);
                    }
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
                    cex_decl_s* d = CexParser.decl_parse(&lx, t, items, cexy$process_ignore_kw, _);
                    if (d == NULL) {
                        continue;
                    }
                    if (d->type != CexTkn__func_def) {
                        continue;
                    }
                    if (d->is_inline && d->is_static) {
                        continue;
                    }
                    if (!_cexy__fn_match(d->name, ns_prefix)) {
                        continue;
                    }
                    log$trace("FN: %S ret_type: '%s' args: '%s'\n", d->name, d->ret_type, d->args);
                    arr$push(decls, d);
                }
                if (arr$len(decls) == 0) {
                    log$info("CEX skipped (no cex decls found in : %s)\n", src_fn);
                    continue;
                }

                arr$sort(decls, _cexy__decl_comparator);

                sbuf_c cex_h_struct = sbuf.create(10 * 1024, _);
                sbuf_c cex_h_var_decl = sbuf.create(1024, _);
                sbuf_c cex_c_var_def = sbuf.create(10 * 1024, _);

                e$ret(sbuf.appendf(
                    &cex_h_var_decl,
                    "__attribute__((visibility(\"hidden\"))) extern const struct __cex_namespace__%S %S;\n",
                    ns_prefix,
                    ns_prefix
                ));
                e$ret(_cexy__process_gen_struct(ns_prefix, decls, &cex_h_struct));
                e$ret(_cexy__process_gen_var_def(ns_prefix, decls, &cex_c_var_def));
                e$ret(_cexy__process_update_code(
                    src_fn,
                    only_update,
                    cex_h_struct,
                    cex_h_var_decl,
                    cex_c_var_def
                ));
                e$ret(_cexy__process_update_code(
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

static bool
_cexy__is_str_pattern(const char* s)
{
    if (s == NULL) {
        return false;
    }
    char pat[] = { '*', '?', '(', '[' };

    while (*s) {
        for (u32 i = 0; i < arr$len(pat); i++) {
            if (unlikely(pat[i] == *s)) {
                return true;
            }
        }
        s++;
    }
    return false;
}

static int
_cexy__help_qscmp_decls_type(const void* a, const void* b)
{
    // this struct fields must match to cexy.cmd.help() hm$
    const struct
    {
        str_s key;
        cex_decl_s* value;
    }* _a = a;
    typeof(_a) _b = b;
    if (_a->value->type != _b->value->type) {
        return _b->value->type - _a->value->type;
    } else {
        return str.slice.qscmp(&_a->key, &_b->key);
    }
}

static Exception
_cexy__display_full_info(cex_decl_s* d, char* base_ns, bool show_source)
{
    str_s name = d->name;
    mem$scope(tmem$, _)
    {
        io.printf("Symbol found at %s:%d\n", d->file, d->line + 1);
        if (d->type == CexTkn__func_def) {
            name = _cexy__fn_dotted(d->name, base_ns, _);
            if (!name.buf) {
                // error converting to dotted name (fallback)
                name = d->name;
            }
        }
        if (d->docs.buf) {
            io.printf("%S\n", d->docs);
        }
        if (d->type == CexTkn__macro_const || d->type == CexTkn__macro_func) {
            io.printf("#define ");
        }

        if (sbuf.len(&d->ret_type)) {
            io.printf("%s ", d->ret_type);
        }

        io.printf("%S ", name);

        if (sbuf.len(&d->args)) {
            io.printf("(%s)", d->args);
        }
        if (!show_source && d->type == CexTkn__func_def) {
            io.printf(";");
        } else if (d->body.buf) {
            io.printf("%S;", d->body);
        }
        io.printf("\n");
    }
    return EOK;
}

static Exception
cexy__cmd__help(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    const char* process_help = "Symbol / documentation search tool for C projects";
    const char* epilog_help = 
        "\nQuery examples: \n"
        "cex help                - list all namespaces in project directory\n"
        "cex help foo            - find any symbol containing 'foo' (case sensitive)\n"
        "cex help foo.           - find namespace prefix: foo$, Foo_func(), FOO_CONST, etc\n"
        "cex help 'foo_*_bar'    - find using pattern search for symbols (see 'cex help str.match')\n"
        "cex help '*_(bar|foo)'  - find any symbol ending with '_bar' or '_foo'\n"
        "cex help os             - display all functions of 'os' namespace (for CEX or user project)\n"
        "cex help str.find       - display function documentation if exactly matched\n"
        "cex help 'os$PATH_SEP'  - display macro constant value if exactly matched\n"
        "cex help str_s          - display type info and documentation if exactly matched\n"
    ;
    const char* filter = "./*.[hc]";
    bool show_source = false;

    // clang-format on
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "help [options] [query]",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(
            argparse$opt_group("Options"),
            argparse$opt_help(),
            argparse$opt(&filter, 'f', "filter", .help = "file pattern for searching"),
            argparse$opt(&show_source, 's', "source", .help = "show full source on match"),
        ),
    };
    if (argparse.parse(&cmd_args, argc, argv)) {
        return Error.argsparse;
    }
    const char* query = argparse.next(&cmd_args);
    str_s query_s = str.sstr(query);

    mem$arena(1024 * 100, arena)
    {

        arr$(char*) sources = os.fs.find("./*.[hc]", true, arena);
        arr$sort(sources, str.qscmp);

        const char* query_pattern = NULL;
        bool is_namespace_filter = false;
        if (str.match(query, "[a-zA-Z0-9+].")) {
            query_pattern = str.fmt(arena, "%S[._$]*", str.sub(query, 0, -1));
            is_namespace_filter = true;
        } else if (_cexy__is_str_pattern(query)) {
            query_pattern = query;
        } else {
            query_pattern = str.fmt(arena, "*%s*", query);
        }

        hm$(str_s, cex_decl_s*) names = hm$new(names, arena, .capacity = 1024);
        hm$(char*, char*) cex_ns_map = hm$new(cex_ns_map, arena, .capacity = 256);
        hm$set(cex_ns_map, "./cexy.h", "cex");

        for$each(src_fn, sources)
        {
            mem$scope(tmem$, _)
            {
                var basename = os.path.basename(src_fn, _);
                if (str.starts_with(basename, "_") || str.starts_with(basename, "test_")) {
                    continue;
                }
                char* base_ns = str.fmt(_, "%S", str.sub(basename, 0, -2));
                log$trace("Loading: %s (namespace: %s)\n", src_fn, base_ns);

                char* code = io.file.load(src_fn, arena);
                if (code == NULL) {
                    return e$raise(Error.not_found, "Error loading: %s\n", src_fn);
                }
                arr$(cex_token_s) items = arr$new(items, _);

                CexParser_c lx = CexParser.create(code, 0, true);
                cex_token_s t;
                while ((t = CexParser.next_entity(&lx, &items)).type) {
                    if (t.type == CexTkn__error) {
                        return e$raise(Error.integrity, "Error parsing: %s\n", src_fn);
                    }
                    cex_decl_s* d = CexParser.decl_parse(&lx, t, items, NULL, arena);
                    if (d == NULL) {
                        continue;
                    }

                    d->file = src_fn;
                    if (d->type == CexTkn__cex_module_struct || d->type == CexTkn__cex_module_def) {
                        log$trace("Found cex namespace: %s (namespace: %s)\n", src_fn, base_ns);
                        hm$set(cex_ns_map, src_fn, str.clone(base_ns, arena));
                    }

                    if (query == NULL) {
                        if (d->type == CexTkn__macro_const || d->type == CexTkn__macro_func) {
                            isize dollar = str.slice.index_of(d->name, str$s("$"));
                            str_s macro_ns = str.slice.sub(d->name, 0, dollar + 1);
                            if (dollar > 0 && !hm$getp(names, macro_ns)) {
                                hm$set(names, macro_ns, d);
                            }
                        } else if (d->type == CexTkn__typedef ||
                                   d->type == CexTkn__cex_module_struct) {
                            if (!hm$getp(names, d->name)) {
                                hm$set(names, d->name, d);
                            }
                        }
                    } else {
                        if (d->type == CexTkn__func_def) {
                            if (str.eq(query, "cex.") &&
                                str.slice.starts_with(d->name, str$s("cex_"))) {
                                continue;
                            }
                        }
                        str_s fndotted = (d->type == CexTkn__func_def)
                                           ? _cexy__fn_dotted(d->name, base_ns, _)
                                           : d->name;

                        if (str.slice.eq(d->name, query_s) || str.slice.eq(fndotted, query_s)) {
                            // We have full match display full help
                            return _cexy__display_full_info(d, base_ns, show_source);
                        }

                        bool has_match = false;
                        if (str.slice.match(d->name, query_pattern)) {
                            has_match = true;
                        }
                        if (str.slice.match(fndotted, query_pattern)) {
                            has_match = true;
                        }
                        if (is_namespace_filter) {
                            str_s prefix = str.sub(query, 0, -1);
                            str_s sub_name = str.slice.sub(d->name, 0, prefix.len);
                            if (str.slice.eqi(sub_name, prefix) &&
                                sub_name.buf[prefix.len] == '_') {
                                has_match = true;
                            }
                        }
                        if (has_match) {
                            hm$set(names, d->name, d);
                        }
                    }
                }
                if (arr$len(names) == 0) {
                    continue;
                }
            }
        }

        // WARNING: sorting of hashmap items is a dead end, hash indexes get invalidated
        qsort(names, hm$len(names), sizeof(*names), _cexy__help_qscmp_decls_type);

        for$each(it, names)
        {
            if (query == NULL) {
                switch (it.value->type) {
                    case CexTkn__cex_module_struct:
                        io.printf("%-20s", "cexy namespace");
                        break;
                    case CexTkn__macro_func:
                    case CexTkn__macro_const:
                        io.printf("%-20s", "macro namespace");
                        break;
                    default:
                        io.printf("%-20s", CexTkn_str[it.value->type]);
                }
                io.printf(" %-30S %s:%d\n", it.key, it.value->file, it.value->line + 1);
            } else {
                str_s name = it.key;
                const char* cex_ns = hm$get(cex_ns_map, (char*)it.value->file);
                if (cex_ns && it.value->type == CexTkn__func_def) {
                    name = _cexy__fn_dotted(name, cex_ns, arena);
                    if (!name.buf) {
                        // something weird happened, fallback
                        log$trace(
                            "Failed to make dotted name from %S, cex_ns: %s\n",
                            it.key,
                            cex_ns
                        );
                        name = it.key;
                    }
                }

                io.printf(
                    "%-20s %-30S %s:%d\n",
                    CexTkn_str[it.value->type],
                    name,
                    it.value->file,
                    it.value->line + 1
                );
            }
        }
    }

    return EOK;
}

static Exception
cexy__cmd__config(int argc, char** argv, void* user_ctx)
{
    (void)argc;
    (void)argv;
    (void)user_ctx;

    // clang-format off
    const char* process_help = "Check project and system environment";
    const char* epilog_help = 
        "\nProject setup examples: \n"
    ;

    // clang-format on
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "check [options]",
        .description = process_help,
        .epilog = epilog_help,
        argparse$opt_list(argparse$opt_help(), ),
    };
    if (argparse.parse(&cmd_args, argc, argv)) {
        return Error.argsparse;
    }

    // clang-format off
#define $env                                                                                \
    "\ncexy$* variables used in build system, see `cex check --help` for more info\n"            \
    "* cexy$build_dir            " cexy$build_dir "\n"                                             \
    "* cexy$cc                   " cexy$cc "\n"                                                    \
    "* cexy$cc_include           " cex$stringize(cexy$cc_include) "\n"                             \
    "* cexy$cc_args              " cex$stringize(cexy$cc_args) "\n"                                \
    "* cexy$cc_args_test         " cex$stringize(cexy$cc_args_test) "\n"                           \
    "* cexy$ld_args              " cex$stringize(cexy$ld_args) "\n"                                \
    "* cexy$ld_libs              " cex$stringize(cexy$ld_libs) "\n"                                \
    "* cexy$debug_cmd            " cex$stringize(cexy$debug_cmd) "\n"                              \
    "* cexy$process_ignore_kw    " cex$stringize(cexy$process_ignore_kw) "\n"
    // clang-format on

    io.printf("%s", $env);

    #undef $env

    return EOK;
}

static Exception
cexy__cmd__simple_test(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "test [options] {run,build,create,clean,debug} all|tests/test_file.c [--test-options]",
        .description = _cexy$cmd_test_help,
        .epilog = _cexy$cmd_test_epilog,
        argparse$opt_list(argparse$opt_help(), ),
    };

    e$ret(argparse.parse(&cmd_args, argc, argv));
    const char* cmd = argparse.next(&cmd_args);
    const char* target = argparse.next(&cmd_args);

    if (!str.match(cmd, "(run|build|create|clean|debug)") || target == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(Error.argsparse, "Invalid command: '%s' or target: '%s'", cmd, target);
    }

    if (str.eq(cmd, "create")) {
        e$ret(cexy.test.create(target, false));
        return EOK;
    } else if (str.eq(cmd, "clean")) {
        e$ret(cexy.test.clean(target));
        return EOK;
    }
    e$ret(cexy.test.make_target_pattern(&target)); // validation + convert 'all' -> "tests/test_*.c"

    log$info("Tests building: %s\n", target);
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
            arr$pushm(args, cexy$cc, );
            // NOTE: reconstructing char*[] because some cexy$ variables might be empty
            char* cc_args_test[] = { cexy$cc_args_test };
            char* cc_include[] = { cexy$cc_include };
            char* cc_ld_args[] = { cexy$ld_args };
            char* cc_ld_libs[] = { cexy$ld_libs };
            arr$pusha(args, cc_args_test);
            arr$pusha(args, cc_include);
            arr$pusha(args, cc_ld_args);
            arr$push(args, test_src);
            arr$pusha(args, cc_ld_libs);
            arr$pushm(args, "-o", test_target);


            arr$push(args, NULL);
            e$ret(os$cmda(args));
            n_built++;
        }
    }

    log$info("Tests building: %d tests processed, %d tests built\n", n_tests, n_built);

    if (str.match(cmd, "(run|debug)")) {
        e$ret(cexy.test.run(target, str.eq(cmd, "debug"), cmd_args.argc, cmd_args.argv));
    }
    return EOK;
}

static Exception
cexy__utils__make_new_project(const char* proj_dir)
{
    log$info("Creating new boilerplate CEX project in '%s'\n", proj_dir);
    mem$scope(tmem$, _)
    {
        if (!str.eq(proj_dir, ".")) {
            if (os.path.exists(proj_dir)) {
                return e$raise(Error.exists, "New project dir already exists: %s", proj_dir);
            }
            e$ret(os.fs.mkdir(proj_dir));
            log$info("Copying '%s/cex.h'\n", proj_dir);
            e$ret(os.fs.copy("./cex.h", os$path_join(_, proj_dir, "cex.h")));
        } else {
            // Creating in the current dir
            if (os.path.exists("./cex.c")) {
                return e$raise(
                    Error.exists,
                    "New project seems to de already initialized, cex.c exists"
                );
            }
        }

        sbuf_c buf = sbuf.create(1024 * 10, _);
        {
            sbuf.clear(&buf);
            cg$init(&buf);
            $pn("#define CEX_IMPLEMENTATION");
            $pn("#include \"cex.h\"");
            // e$ret(io.file.save(target, buf));
            log$debug("Src: %s\n", buf);
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
        .config = cexy__cmd__config,
        .help = cexy__cmd__help,
        .process = cexy__cmd__process,
        .simple_test = cexy__cmd__simple_test,
    },

    .test = {
        .clean = cexy__test__clean,
        .create = cexy__test__create,
        .make_target_pattern = cexy__test__make_target_pattern,
        .run = cexy__test__run,
    },

    .utils = {
        .make_new_project = cexy__utils__make_new_project,
    },

    // clang-format on
};
#endif // defined(CEX_BUILD)
