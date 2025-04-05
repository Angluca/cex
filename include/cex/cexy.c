#include "cexy.h"
#if defined(CEXBUILD)

static bool
_cexy_needs_build_by_include(const char* path, time_t target_mtime)
{
    (void)path;
    (void)target_mtime;
    return false;
}

static bool
cexy_needs_build(
    const char* target_path,
    const char** src_array,
    usize src_array_len,
    bool inspect_includes
)
{
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
                return true;
            }
            if (inspect_includes && (str.ends_with(p, ".c") || str.ends_with(p, ".h"))) {
                if (_cexy_needs_build_by_include(p, target_ftype.mtime)) {
                    return true;
                }
            }
        }
    }

    return false;
}
#endif // #if defined(CEXBUILD)
