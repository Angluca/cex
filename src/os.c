#pragma once
#include "all.c"
#include "cex_base.h"

#ifndef _WIN32
#    include <dirent.h>
#else // _WIN32
// minirent.h HEADER BEGIN
// Copyright 2021 Alexey Kutepov <reximkut@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining
// a copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be
// included in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
//
// ============================================================

#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"

struct dirent
{
    char d_name[MAX_PATH + 1];
};

typedef struct DIR DIR;

static DIR* opendir(const char* dirpath);
static struct dirent* readdir(DIR* dirp);
static int closedir(DIR* dirp);

struct DIR
{
    HANDLE hFind;
    WIN32_FIND_DATA data;
    struct dirent* dirent;
};

DIR*
opendir(const char* dirpath)
{
    char buffer[MAX_PATH];
    snprintf(buffer, MAX_PATH, "%s\\*", dirpath);

    DIR* dir = (DIR*)realloc(NULL, sizeof(DIR));
    memset(dir, 0, sizeof(DIR));

    dir->hFind = FindFirstFile(buffer, &dir->data);
    if (dir->hFind == INVALID_HANDLE_VALUE) {
        // TODO: opendir should set errno accordingly on FindFirstFile fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        goto fail;
    }

    return dir;

fail:
    if (dir) {
        free(dir);
    }

    return NULL;
}

struct dirent*
readdir(DIR* dirp)
{
    if (dirp->dirent == NULL) {
        dirp->dirent = (struct dirent*)realloc(NULL, sizeof(struct dirent));
        memset(dirp->dirent, 0, sizeof(struct dirent));
    } else {
        if (!FindNextFile(dirp->hFind, &dirp->data)) {
            if (GetLastError() != ERROR_NO_MORE_FILES) {
                // TODO: readdir should set errno accordingly on FindNextFile fail
                // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
                errno = ENOSYS;
            }

            return NULL;
        }
    }

    memset(dirp->dirent->d_name, 0, sizeof(dirp->dirent->d_name));

    strncpy(dirp->dirent->d_name, dirp->data.cFileName, sizeof(dirp->dirent->d_name) - 1);

    return dirp->dirent;
}

int
closedir(DIR* dirp)
{
    if (!FindClose(dirp->hFind)) {
        // TODO: closedir should set errno accordingly on FindClose fail
        // https://docs.microsoft.com/en-us/windows/win32/api/errhandlingapi/nf-errhandlingapi-getlasterror
        errno = ENOSYS;
        return -1;
    }

    if (dirp->dirent) {
        free(dirp->dirent);
    }
    free(dirp);

    return 0;
}
#endif // _WIN32

static void
cex_os_sleep(u32 period_millisec)
{
#ifdef _WIN32
    Sleep(period_millisec);
#else
    usleep(period_millisec * 1000);
#endif
}

static Exc
cex_os_get_last_error(void)
{
#ifdef _WIN32
    DWORD err = GetLastError();
    switch (err) {
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
            return Error.not_found;
        case ERROR_ACCESS_DENIED:
            return Error.permission;
        default:
            break;
    }

    _Thread_local static char win32_err_buf[256] = { 0 };

    DWORD err_msg_size = FormatMessageA(
        FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        NULL,
        err,
        LANG_USER_DEFAULT,
        win32_err_buf,
        arr$len(win32_err_buf),
        NULL
    );

    if (unlikely(err_msg_size == 0)) {
        if (GetLastError() != ERROR_MR_MID_NOT_FOUND) {
            if (sprintf(win32_err_buf, "Generic windows error code: 0x%lX", err) > 0) {
                return (char*)&win32_err_buf;
            } else {
                return NULL;
            }
        } else {
            if (sprintf(win32_err_buf, "Invalid Windows Error code (0x%lX)", err) > 0) {
                return (char*)&win32_err_buf;
            } else {
                return NULL;
            }
        }
    }

    while (err_msg_size > 1 && isspace(win32_err_buf[err_msg_size - 1])) {
        win32_err_buf[--err_msg_size] = '\0';
    }

    return win32_err_buf;
#else
    switch (errno) {
        case 0:
            uassert(errno != 0 && "errno is ok");
            return "Error, but errno is not set";
        case ENOENT:
            return Error.not_found;
        case EPERM:
            return Error.permission;
        case EIO:
            return Error.io;
        case EAGAIN:
            return Error.try_again;
        default:
            return strerror(errno);
    }
#endif
}

static Exception
cex_os__fs__rename(const char* old_path, const char* new_path)
{
    if (old_path == NULL || old_path[0] == '\0') {
        return Error.argument;
    }
    if (new_path == NULL || new_path[0] == '\0') {
        return Error.argument;
    }
#ifdef _WIN32
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
        return Error.os;
    }
    return EOK;
#else
    if (rename(old_path, new_path) < 0) {

        return os.get_last_error();
    }
    return EOK;
#endif // _WIN32
}

static Exception
cex_os__fs__mkdir(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
#ifdef _WIN32
    int result = mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif
    if (result < 0) {
        uassert(errno != 0);
        if (errno == EEXIST) {
            return EOK;
        }
        return strerror(errno);
    }
    return EOK;
}

static Exception
cex_os__fs__mkpath(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
    str_s dir = os.path.split(path, true);
    char dir_path[PATH_MAX] = { 0 };
    e$ret(str.slice.copy(dir_path, dir, sizeof(dir_path)));
    if (os.path.exists(dir_path)) {
        return EOK;
    }
    usize dir_path_len = 0;

    for$iter(str_s, it, str.slice.iter_split(dir, "\\/", &it.iterator))
    {
        if (dir_path_len > 0) {
            uassert(dir_path_len < sizeof(dir_path) - 2);
            dir_path[dir_path_len] = os$PATH_SEP;
            dir_path_len++;
            dir_path[dir_path_len] = '\0';
        }
        e$ret(str.slice.copy(dir_path + dir_path_len, it.val, sizeof(dir_path) - dir_path_len));
        dir_path_len += it.val.len;
        e$ret(os.fs.mkdir(dir_path));
    }
    return EOK;
}

static os_fs_stat_s
cex_os__fs__stat(const char* path)
{
    os_fs_stat_s result = { .error = Error.argument };
    if (path == NULL || path[0] == '\0') {
        return result;
    }

#ifdef _WIN32
    /* FIX: win 32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        nob_log(
            NOB_ERROR,
            "Could not get file attributes of %s: %s",
            path,
            nob_win32_error_message(GetLastError())
        );
        return -1;
    }
    result.is_valid = true;
    result.result = EOK;

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        return NOB_FILE_DIRECTORY;
    }
    return NOB_FILE_REGULAR;
    */
    return result;
#else // _WIN32
    struct stat statbuf;
    if (lstat(path, &statbuf) < 0) {
        if (errno == ENOENT) {
            result.error = Error.not_found;
        } else {
            result.error = strerror(errno);
        }
        return result;
    }
    result.is_valid = true;
    result.error = EOK;
    result.is_other = true;
    result.mtime = statbuf.st_mtime;

    if (!S_ISLNK(statbuf.st_mode)) {
        if (S_ISREG(statbuf.st_mode)) {
            result.is_file = true;
            result.is_other = false;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            result.is_directory = true;
            result.is_other = false;
        }
    } else {
        result.is_symlink = true;
        if (stat(path, &statbuf) < 0) {
            if (errno == ENOENT) {
                result.error = Error.not_found;
            } else {
                result.error = strerror(errno);
            }
            return result;
        }
        if (S_ISREG(statbuf.st_mode)) {
            result.is_file = true;
            result.is_other = false;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            result.is_directory = true;
            result.is_other = false;
        }
    }
    return result;
#endif
}

static Exception
cex_os__fs__remove(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
#ifdef _WIN32
    /* FIX: win32
    if (!DeleteFileA(path)) {
        return nob_win32_error_message(GetLastError());
    }
    */
    return EOK;
#else
    if (remove(path) < 0) {
        return strerror(errno);
    }
    return EOK;
#endif
}

Exception
cex_os__fs__dir_walk(
    const char* path,
    bool is_recursive,
    os_fs_dir_walk_f callback_fn,
    void* user_ctx
)
{
    (void)user_ctx;
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
    Exc result = Error.os;
    uassert(callback_fn != NULL && "you must provide callback_fn");

    DIR* dp = opendir(path);

    if (unlikely(dp == NULL)) {
        if (errno == ENOENT) {
            result = Error.not_found;
        }
        goto end;
    }

    u32 path_len = strlen(path);
    if (path_len > PATH_MAX - 256) {
        result = Error.overflow;
        goto end;
    }

    char path_buf[PATH_MAX];

    struct dirent* ep;
    while ((ep = readdir(dp)) != NULL) {
        errno = 0;
        if (str.eq(ep->d_name, ".")) {
            continue;
        }
        if (str.eq(ep->d_name, "..")) {
            continue;
        }
        memcpy(path_buf, path, path_len);
        u32 path_offset = 0;
        if (path_buf[path_len - 1] != '/' && path_buf[path_len - 1] != '\\') {
            path_buf[path_len] = os$PATH_SEP;
            path_offset = 1;
        }

        e$except_silent(
            err,
            str.copy(
                path_buf + path_len + path_offset,
                ep->d_name,
                sizeof(path_buf) - path_len - 1 - path_offset
            )
        )
        {
            result = err;
            goto end;
        }

        var ftype = os.fs.stat(path_buf);
        if (!ftype.is_valid) {
            result = ftype.error;
            goto end;
        }

        if (is_recursive && ftype.is_directory && !ftype.is_symlink) {
            e$except_silent(err, cex_os__fs__dir_walk(path_buf, is_recursive, callback_fn, user_ctx))
            {
                result = err;
                goto end;
            }
        }
        // After recursive call make a callback on a directory itself
        e$except_silent(err, callback_fn(path_buf, ftype, user_ctx))
        {
            result = err;
            goto end;
        }
    }

    if (errno != 0) {
        result = strerror(errno);
        goto end;
    }

    result = EOK;
end:
    if (dp != NULL) {
        (void)closedir(dp);
    }
    return result;
}

struct _os_fs_find_ctx_s
{
    const char* pattern;
    arr$(char*) result;
    IAllocator allc;
};

static Exception
_os__fs__remove_tree_walker(const char* path, os_fs_stat_s ftype, void* user_ctx)
{
    (void)user_ctx;
    (void)ftype;
    e$except_silent(err, cex_os__fs__remove(path))
    {
        return err;
    }
    return EOK;
}

static Exception
cex_os__fs__remove_tree(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
    if (!os.path.exists(path)) {
        return EOK;
    }
    e$except_silent(err, cex_os__fs__dir_walk(path, true, _os__fs__remove_tree_walker, NULL))
    {
        return err;
    }
    e$except_silent(err, cex_os__fs__remove(path))
    {
        return err;
    }
    return EOK;
}

static Exception
_os__fs__find_walker(const char* path, os_fs_stat_s ftype, void* user_ctx)
{
    (void)ftype;
    struct _os_fs_find_ctx_s* ctx = (struct _os_fs_find_ctx_s*)user_ctx;
    uassert(ctx->result != NULL);
    if (ftype.is_directory) {
        return EOK; // skip, only supports finding files!
    }
    if (ftype.is_symlink) {
        return EOK; // does not follow symlinks!
    }

    str_s file_part = os.path.split(path, false);
    if (!str.match(file_part.buf, ctx->pattern)) {
        return EOK; // just skip when patten not matched
    }

    // allocate new string because path is stack allocated buffer in os__fs__dir_walk()
    char* new_path = str.clone(path, ctx->allc);
    if (new_path == NULL) {
        return Error.memory;
    }

    // Doing graceful memory check, otherwise arr$push will assert
    if (!arr$grow_check(ctx->result, 1)) {
        return Error.memory;
    }
    arr$push(ctx->result, new_path);
    return EOK;
}

static arr$(char*) cex_os__fs__find(const char* path, bool is_recursive, IAllocator allc)
{

    if (unlikely(allc == NULL)) {
        return NULL;
    }

    str_s dir_part = os.path.split(path, true);
    if (dir_part.buf == NULL) {
#if defined(CEX_TEST) || defined(CEX_BUILD)
        (void)e$raise(Error.argument, "Bad path: os.fn.find('%s')", path);
#endif
        return NULL;
    }

    if (!is_recursive) {
        var f = os.fs.stat(path);
        if (f.is_valid && f.is_file) {
            // Find used with exact file path, we still have to return array + allocated path copy
            arr$(char*) result = arr$new(result, allc);
            char* it = str.clone(path, allc);
            arr$push(result, it);
            return result;
        }
    }

    char path_buf[PATH_MAX + 2] = { 0 };
    if (dir_part.len > 0 && str.slice.copy(path_buf, dir_part, sizeof(path_buf))) {
        uassert(dir_part.len < PATH_MAX);
        return NULL;
    }
    if (str.copy(
            path_buf + dir_part.len + 1,
            path + dir_part.len,
            sizeof(path_buf) - dir_part.len - 1
        )) {
        uassert(dir_part.len < PATH_MAX);
        return NULL;
    }
    char* dir_name = (dir_part.len > 0) ? path_buf : ".";
    char* pattern = path_buf + dir_part.len + 1;
    if (*pattern == os$PATH_SEP) {
        pattern++;
    }
    if (*pattern == '\0') {
        pattern = "*";
    }

    struct _os_fs_find_ctx_s ctx = { .result = arr$new(ctx.result, allc),
                                     .pattern = pattern,
                                     .allc = allc };
    if (unlikely(ctx.result == NULL)) {
        return NULL;
    }

    e$except_silent(err, cex_os__fs__dir_walk(dir_name, is_recursive, _os__fs__find_walker, &ctx))
    {
        for$each(it, ctx.result)
        {
            mem$free(allc, it); // each individual item was allocated too
        }
        if (ctx.result != NULL) {
            arr$free(ctx.result);
        }
        ctx.result = NULL;
    }
    return ctx.result;
}

static char*
cex_os__fs__getcwd(IAllocator allc)
{
    char* buf = mem$malloc(allc, PATH_MAX);

    char* result = NULL;
#ifdef _WIN32
    result = _getcwd(buf, PATH_MAX);
#else
    result = getcwd(buf, PATH_MAX);
#endif
    if (result == NULL) {
        mem$free(allc, buf);
    }

    return result;
}

static Exception
cex_os__fs__chdir(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.exists;
    }

    int result;
#ifdef _WIN32
    result = _chdir(path);
#else
    result = chdir(path);
#endif

    if (result == -1) {
        if (errno == ENOENT) {
            return Error.not_found;
        } else {
            return strerror(errno);
        }
    }

    return EOK;
}


static Exception
cex_os__fs__copy(const char* src_path, const char* dst_path)
{
    if (src_path == NULL || src_path[0] == '\0' || dst_path == NULL || dst_path[0] == '\0') {
        return Error.argument;
    }

    log$trace("copying %s -> %s\n", src_path, dst_path);

#ifdef _WIN32
    /* FIX: win32
    if (!CopyFile(src_path, dst_path, FALSE)) {
        nob_log(NOB_ERROR, "Could not copy file: %s", nob_win32_error_message(GetLastError()));
        return Error.io;
    }
    */
    return EOK;
#else
    int src_fd = -1;
    int dst_fd = -1;
    size_t buf_size = 32 * 1024;
    char* buf = mem$malloc(mem$, buf_size);
    if (buf == NULL) {
        return Error.memory;
    }
    Exc result = Error.runtime;

    if ((src_fd = open(src_path, O_RDONLY)) == -1) {
        switch (errno) {
            case ENOENT:
                result = Error.not_found;
                break;
            default:
                result = strerror(errno);
        }
        goto defer;
    }

    struct stat src_stat;
    if (fstat(src_fd, &src_stat) < 0) {
        result = strerror(errno);
        goto defer;
    }

    dst_fd = open(dst_path, O_CREAT | O_TRUNC | O_WRONLY, src_stat.st_mode);
    if (dst_fd < 0) {
        result = strerror(errno);
        goto defer;
    }

    for (;;) {
        ssize_t n = read(src_fd, buf, buf_size);
        if (n == 0) {
            break;
        }
        if (n < 0) {
            result = Error.io;
            goto defer;
        }
        char* buf2 = buf;
        while (n > 0) {
            ssize_t m = write(dst_fd, buf2, n);
            if (m < 0) {
                result = Error.io;
                goto defer;
            }
            n -= m;
            buf2 += m;
        }
    }
    result = EOK;

defer:
    mem$free(mem$, buf);
    close(src_fd);
    close(dst_fd);
    return result;
#endif
}

static const char*
cex_os__env__get(const char* name, const char* deflt)
{
    const char* result = getenv(name);

    if (result == NULL) {
        result = deflt;
    }

    return result;
}

static Exception
cex_os__env__set(const char* name, const char* value)
{
#ifdef _WIN32
    _putenv_s(name, value);
#else
    setenv(name, value, true);
#endif
    // TODO: add error reporting
    return EOK;
}

static bool
cex_os__path__exists(const char* file_path)
{
    var ftype = os.fs.stat(file_path);
    return ftype.is_valid;
}

static char*
cex_os__path__join(const char** parts, u32 parts_len, IAllocator allc)
{
    char sep[2] = { os$PATH_SEP, '\0' };
    return str.join(parts, parts_len, sep, allc);
}

static str_s
cex_os__path__split(const char* path, bool return_dir)
{
    if (path == NULL) {
        return (str_s){ 0 };
    }
    usize pathlen = strlen(path);
    if (pathlen == 0) {
        return str$s("");
    }

    isize last_slash_idx = -1;

    for (usize i = pathlen; i-- > 0;) {
        if (path[i] == '/' || path[i] == '\\') {
            last_slash_idx = i;
            break;
        }
    }
    if (last_slash_idx != -1) {
        if (return_dir) {
            return str.sub(path, 0, last_slash_idx == 0 ? 1 : last_slash_idx);
        } else {
            if ((usize)last_slash_idx == pathlen - 1) {
                return str$s("");
            } else {
                return str.sub(path, last_slash_idx + 1, 0);
            }
        }

    } else {
        if (return_dir) {
            return str$s("");
        } else {
            return str.sstr(path);
        }
    }
}

static char*
cex_os__path__basename(const char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    str_s fname = cex_os__path__split(path, false);
    return str.slice.clone(fname, allc);
}

static char*
cex_os__path__dirname(const char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    str_s fname = cex_os__path__split(path, true);
    return str.slice.clone(fname, allc);
}

static Exception
cex_os__cmd__create(os_cmd_c* self, arr$(char*) args, arr$(char*) env, os_cmd_flags_s* flags)
{
    (void)env;
    uassert(self != NULL);
    if (args == NULL || arr$len(args) == 0) {
        return "`args` is empty or null";
    }
    usize args_len = arr$len(args);
    if (args_len == 1 || args[args_len - 1] != NULL) {
        return "`args` last item must be a NULL";
    }
    for (u32 i = 0; i < args_len - 1; i++) {
        if (args[i] == NULL) {
            return "one of `args` items is NULL, which may indicate string operation failure";
        }
    }

    *self = (os_cmd_c){
        ._is_subprocess = true,
        ._flags = (flags) ? *flags : (os_cmd_flags_s){ 0 },
    };

    int sub_flags = 0;
    if (!self->_flags.no_inherit_env) {
        sub_flags |= subprocess_option_inherit_environment;
    }
    if (self->_flags.no_window) {
        sub_flags |= subprocess_option_no_window;
    }
    if (!self->_flags.no_search_path) {
        sub_flags |= subprocess_option_search_user_path;
    }
    if (self->_flags.combine_stdouterr) {
        sub_flags |= subprocess_option_combined_stdout_stderr;
    }

    int result = subprocess_create((const char* const*)args, sub_flags, &self->_subpr);
    if (result != 0) {
        return os.get_last_error();
    }

    return EOK;
}

static bool
cex_os__cmd__is_alive(os_cmd_c* self)
{
    return subprocess_alive(&self->_subpr);
}

static Exception
cex_os__cmd__kill(os_cmd_c* self)
{
    if (subprocess_alive(&self->_subpr)) {
        if (subprocess_terminate(&self->_subpr) != 0) {
            return Error.os;
        }
    }
    return EOK;
}

static Exception
cex_os__cmd__join(os_cmd_c* self, u32 timeout_sec, i32* out_ret_code)
{
    uassert(self != NULL);
    Exc result = Error.os;
    int ret_code = 1;

    if (timeout_sec == 0) {
        // timeout_sec == 0 -> infinite wait
        int join_result = subprocess_join(&self->_subpr, &ret_code);
        if (join_result != 0) {
            ret_code = -1;
            result = Error.os;
            goto end;
        }
    } else {
        uassert(timeout_sec < INT32_MAX && "timeout is negative or too high");
        u64 timeout_elapsed_ms = 0;
        u64 timeout_ms = timeout_sec * 1000;
        do {
            if (cex_os__cmd__is_alive(self)) {
                cex_os_sleep(100); // 100 ms sleep
            } else {
                subprocess_join(&self->_subpr, &ret_code);
                break;
            }
            timeout_elapsed_ms += 100;
        } while (timeout_elapsed_ms < timeout_ms);

        if (timeout_elapsed_ms >= timeout_ms) {
            result = Error.timeout;
            if (cex_os__cmd__kill(self)) { // discard
            }
            subprocess_join(&self->_subpr, NULL);
            ret_code = -1;
            goto end;
        }
    }

    if (ret_code != 0) {
        result = Error.runtime;
        goto end;
    }

    result = Error.ok;

end:
    if (out_ret_code) {
        *out_ret_code = ret_code;
    }
    subprocess_destroy(&self->_subpr);
    memset(self, 0, sizeof(os_cmd_c));
    return result;
}

static FILE*
cex_os__cmd__fstdout(os_cmd_c* self)
{
    return self->_subpr.stdout_file;
}

static FILE*
cex_os__cmd__fstderr(os_cmd_c* self)
{
    return self->_subpr.stderr_file;
}

static FILE*
cex_os__cmd__fstdin(os_cmd_c* self)
{
    return self->_subpr.stdin_file;
}

static char*
cex_os__cmd__read_all(os_cmd_c* self, IAllocator allc)
{
    uassert(self != NULL);
    uassert(allc != NULL);
    if (self->_subpr.stdout_file) {
        str_s out = { 0 };
        if (io.fread_all(self->_subpr.stdout_file, &out, allc)) {
            return NULL;
        }
        return out.buf;
    }
    return NULL;
}

static char*
cex_os__cmd__read_line(os_cmd_c* self, IAllocator allc)
{
    uassert(self != NULL);
    uassert(allc != NULL);
    if (self->_subpr.stdout_file) {
        str_s out = { 0 };
        if (io.fread_line(self->_subpr.stdout_file, &out, allc)) {
            return NULL;
        }
        return out.buf;
    }
    return NULL;
}

static Exception
cex_os__cmd__write_line(os_cmd_c* self, char* line)
{
    uassert(self != NULL);
    if (line == NULL) {
        return Error.argument;
    }

    if (self->_subpr.stdin_file == NULL) {
        return Error.not_found;
    }

    e$except_silent(err, io.file.writeln(self->_subpr.stdin_file, line))
    {
        return err;
    }
    fflush(self->_subpr.stdin_file);

    return EOK;
}

static Exception
cex_os__cmd__run(const char** args, usize args_len, os_cmd_c* out_cmd)
{
    uassert(out_cmd != NULL);
    memset(out_cmd, 0, sizeof(os_cmd_c));

    if (args == NULL || args_len == 0) {
        return e$raise(Error.argument, "`args` argument is empty or null");
    }
    if (args_len == 1 || args[args_len - 1] != NULL) {
        return e$raise(Error.argument, "`args` last item must be a NULL");
    }

    for (u32 i = 0; i < args_len - 1; i++) {
        if (args[i] == NULL || args[i][0] == '\0') {
            return e$raise(
                Error.argument,
                "`args` item[%d] is NULL/empty, which may indicate string operation failure",
                i
            );
        }
    }

#ifdef _WIN32
    // FIX:  WIN32 uncompilable
    /*

    //
    https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    //
    https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError = redirect.fderr ? *redirect.fderr : GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = redirect.fdout ? *redirect.fdout : GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = redirect.fdin ? *redirect.fdin : GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // TODO: use a more reliable rendering of the command instead of cmd_render
    // cmd_render is for logging primarily
    nob_cmd_render(cmd, &sb);
    nob_sb_append_null(&sb);
    BOOL bSuccess = CreateProcessA(
        NULL,
        sb.items,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &siStartInfo,
        &piProcInfo
    );
    nob_sb_free(sb);

    if (!bSuccess) {
        nob_log(
            NOB_ERROR,
            "Could not create child process: %s",
            nob_win32_error_message(GetLastError())
        );
        return NOB_INVALID_PROC;
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
    */
    return "TODO";
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        return e$raise(Error.os, "Could not fork child process: %s", strerror(errno));
    }

    if (cpid == 0) {
        if (execvp(args[0], (char* const*)args) < 0) {
            log$error("Could not exec child process: %s\n", strerror(errno));
            exit(1);
        }
        uassert(false && "unreachable");
    }

    *out_cmd = (os_cmd_c){ ._is_subprocess = false,
                           ._subpr = {
                               .child = cpid,
                               .alive = 1,
                           } };
    return EOK;

#endif
}

static OSPlatform_e
cex_os__platform__current(void)
{
#ifdef _WIN32
    return OSPlatform__win;
#elif __linux__
    return OSPlatform__linux;
#elif __APPLE__ && __MACH__
    return OSPlatform__macos;
#elif __unix__
#    ifdef __FreeBSD__
    return OSPlatform__freebsd;
#    elif __NetBSD__
    return OSPlatform__netbsd;
#    elif __OpenBSD__
    return OSPlatform__openbsd;
#    elif __ANDROID__
    return OSPlatform__android;
#    else
#        error "Untested platform. Need more?"
#    endif
#elif __wasm__
    return OSPlatform__wasm;
#else
#    error "Untested platform. Need more?"
#endif
}

static const char*
cex_os__platform__current_str(void)
{
    return os.platform.to_str(os.platform.current());
}

static OSPlatform_e
cex_os__platform__from_str(const char* name)
{
    if (name == NULL || name[0] == '\0') {
        return OSPlatform__unknown;
    }
    for (u32 i = 1; i < OSPlatform__count; i++) {
        if (str.eq(OSPlatform_str[i], name)) {
            return (OSPlatform_e)i;
        }
    }
    return OSPlatform__unknown;
    ;
}

static const char*
cex_os__platform__to_str(OSPlatform_e platform)
{
    if (unlikely(platform <= OSPlatform__unknown || platform >= OSPlatform__count)) {
        return NULL;
    }
    return OSPlatform_str[platform];
}

static OSArch_e
cex_os__platform__arch_from_str(const char* name)
{
    if (name == NULL || name[0] == '\0') {
        return OSArch__unknown;
    }
    for (u32 i = 1; i < OSArch__count; i++) {
        if (str.eq(OSArch_str[i], name)) {
            return (OSArch_e)i;
        }
    }
    return OSArch__unknown;
    ;
}

static const char*
cex_os__platform__arch_to_str(OSArch_e platform)
{
    if (unlikely(platform <= OSArch__unknown || platform >= OSArch__count)) {
        return NULL;
    }
    return OSArch_str[platform];
}

const struct __cex_namespace__os os = {
    // Autogenerated by CEX
    // clang-format off

    .get_last_error = cex_os_get_last_error,
    .sleep = cex_os_sleep,

    .cmd = {
        .create = cex_os__cmd__create,
        .fstderr = cex_os__cmd__fstderr,
        .fstdin = cex_os__cmd__fstdin,
        .fstdout = cex_os__cmd__fstdout,
        .is_alive = cex_os__cmd__is_alive,
        .join = cex_os__cmd__join,
        .kill = cex_os__cmd__kill,
        .read_all = cex_os__cmd__read_all,
        .read_line = cex_os__cmd__read_line,
        .run = cex_os__cmd__run,
        .write_line = cex_os__cmd__write_line,
    },

    .env = {
        .get = cex_os__env__get,
        .set = cex_os__env__set,
    },

    .fs = {
        .chdir = cex_os__fs__chdir,
        .copy = cex_os__fs__copy,
        .dir_walk = cex_os__fs__dir_walk,
        .find = cex_os__fs__find,
        .getcwd = cex_os__fs__getcwd,
        .mkdir = cex_os__fs__mkdir,
        .mkpath = cex_os__fs__mkpath,
        .remove = cex_os__fs__remove,
        .remove_tree = cex_os__fs__remove_tree,
        .rename = cex_os__fs__rename,
        .stat = cex_os__fs__stat,
    },

    .path = {
        .basename = cex_os__path__basename,
        .dirname = cex_os__path__dirname,
        .exists = cex_os__path__exists,
        .join = cex_os__path__join,
        .split = cex_os__path__split,
    },

    .platform = {
        .arch_from_str = cex_os__platform__arch_from_str,
        .arch_to_str = cex_os__platform__arch_to_str,
        .current = cex_os__platform__current,
        .current_str = cex_os__platform__current_str,
        .from_str = cex_os__platform__from_str,
        .to_str = cex_os__platform__to_str,
    },

    // clang-format on
};
