#include "cexy.h"
#if defined(CEXBUILD)

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
            (void)e$raise("IOError", "src is not a file: %s", src_path);
            return false;
        }

        CexLexer_c lx = CexLexer_create(code, 0, true);
        cex_token_s t;
        while ((t = CexLexer_next_token(&lx)).type) {
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
                if (!str.slice.starts_with(incf, str$s("\"")) ||
                    !str.slice.ends_with(incf, str$s("\""))) {
                    // system includes skipped
                    continue;
                }

                incf = str.slice.sub(incf, 1, -1);
                log$trace("Processing include: '%S'\n", incf);
                if (!str.slice.ends_with(incf, str$s(".c")) &&
                    !str.slice.ends_with(incf, str$s(".h"))) {
                    continue;
                }

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
cexy_src_changed(const char* target_path, const arr$(char*) src_array)
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
#endif // #if defined(CEXBUILD)
const struct __module__cexy cexy = {
    // Autogenerated by CEX
    // clang-format off
    .src_include_changed = cexy_src_include_changed,
    .src_changed = cexy_src_changed,
    // clang-format on
};
