#pragma once
#include "all.h"

#ifdef _WIN32
#define os$PATH_SEP '\\'
#else
#define os$PATH_SEP '/'
#endif

static void
os_sleep(u32 period_millisec)
{
#ifdef _WIN32
    Sleep(period_millisec);
#else
    usleep(period_millisec * 1000);
#endif
}

static Exception
os__fs__rename(const char* old_path, const char* new_path)
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

        if (errno == ENOENT) {
            return Error.not_found;
        } else {
            return strerror(errno);
        }
    }
    return EOK;
#endif // _WIN32
}

static Exception
os__fs__mkdir(const char* path)
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

static os_fs_stat_s
os__fs__stat(const char* path)
{
    os_fs_stat_s result = { .error = Error.argument };
    if (path == NULL || path[0] == '\0') {
        return result;
    }

#ifdef _WIN32
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
    // TODO: detect symlinks on Windows (whatever that means on Windows anyway)
    return NOB_FILE_REGULAR;
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
os__fs__remove(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
#ifdef _WIN32
    if (!DeleteFileA(path)) {
        return nob_win32_error_message(GetLastError());
    }
    return EOK;
#else
    if (remove(path) < 0) {
        return strerror(errno);
    }
    return EOK;
#endif
}

Exception
os__fs__dir_walk(const char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx)
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

        if (ftype.is_symlink) {
            continue; // does not follow symlinks!
        }

        if (is_recursive && ftype.is_directory) {
            e$except_silent(err, os__fs__dir_walk(path_buf, is_recursive, callback_fn, user_ctx))
            {
                result = err;
                goto end;
            }

        } else {
            e$except_silent(err, callback_fn(path_buf, ftype, user_ctx))
            {
                result = err;
                goto end;
            }
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
_os__fs__find_walker(const char* path, os_fs_stat_s ftype, void* user_ctx)
{
    (void)ftype;
    struct _os_fs_find_ctx_s* ctx = (struct _os_fs_find_ctx_s*)user_ctx;
    uassert(ctx->result != NULL);
    if (ftype.is_directory) {
        return EOK; // skip, only supports finding files!
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

static arr$(char*) os__fs__find(const char* path, bool is_recursive, IAllocator allc)
{

    if (unlikely(allc == NULL)) {
        return NULL;
    }


    str_s dir_part = dir_part = os.path.split(path, true);
    if (dir_part.buf == NULL) {
#if defined(CEXTEST) || defined(CEXBUILD)
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
    char* pattern = (dir_part.len > 0) ? path_buf + dir_part.len + 1 : path_buf;
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

    e$except_silent(err, os__fs__dir_walk(dir_name, is_recursive, _os__fs__find_walker, &ctx))
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

static Exception
os__fs__getcwd(sbuf_c* out)
{

    uassert(sbuf.isvalid(out) && "out is not valid sbuf_c (or missing initialization)");

    e$except_silent(err, sbuf.grow(out, PATH_MAX + 1))
    {
        return err;
    }
    sbuf.clear(out);

    errno = 0;
    if (unlikely(getcwd(*out, sbuf.capacity(out)) == NULL)) {
        return strerror(errno);
    }

    sbuf.update_len(out);

    return EOK;
}

static const char*
os__env__get(const char* name, const char* deflt)
{
    const char* result = getenv(name);

    if (result == NULL) {
        result = deflt;
    }

    return result;
}

static void
os__env__set(const char* name, const char* value, bool overwrite)
{
    setenv(name, value, overwrite);
}

static void
os__env__unset(const char* name)
{
    unsetenv(name);
}

static bool
os__path__exists(const char* file_path)
{
    var ftype = os.fs.stat(file_path);
    return ftype.is_valid;
}

static char*
os__path__join(const char** parts, u32 parts_len, IAllocator allc)
{
    char sep[2] = { os$PATH_SEP, '\0' };
    return str.join(parts, parts_len, sep, allc);
}

static str_s
os__path__split(const char* path, bool return_dir)
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
os__path__basename(const char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    str_s fname = os__path__split(path, false);
    return str.slice.clone(fname, allc);
}

static char*
os__path__dirname(const char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    str_s fname = os__path__split(path, true);
    return str.slice.clone(fname, allc);
}


static Exception
os__cmd__create(os_cmd_c* self, arr$(char*) args, arr$(char*) env, os_cmd_flags_s* flags)
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
        uassert(errno != 0);
        return strerror(errno);
    }

    return EOK;
}

static bool
os__cmd__is_alive(os_cmd_c* self)
{
    return subprocess_alive(&self->_subpr);
}

static Exception
os__cmd__kill(os_cmd_c* self)
{
    if (subprocess_alive(&self->_subpr)) {
        if (subprocess_terminate(&self->_subpr) != 0) {
            return Error.os;
        }
    }
    return EOK;
}

static Exception
os__cmd__join(os_cmd_c* self, u32 timeout_sec, i32* out_ret_code)
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
            if (os__cmd__is_alive(self)) {
                os_sleep(100); // 100 ms sleep
            } else {
                subprocess_join(&self->_subpr, &ret_code);
                break;
            }
            timeout_elapsed_ms += 100;
        } while (timeout_elapsed_ms < timeout_ms);

        if (timeout_elapsed_ms >= timeout_ms) {
            result = Error.timeout;
            if (os__cmd__kill(self)) { // discard
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
os__cmd__stdout(os_cmd_c* self)
{
    return self->_subpr.stdout_file;
}


static FILE*
os__cmd__stderr(os_cmd_c* self)
{
    return self->_subpr.stderr_file;
}

static FILE*
os__cmd__stdin(os_cmd_c* self)
{
    return self->_subpr.stdin_file;
}

static char*
os__cmd__read_all(os_cmd_c* self, IAllocator allc)
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
os__cmd__read_line(os_cmd_c* self, IAllocator allc)
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
os__cmd__write_line(os_cmd_c* self, char* line)
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
os__cmd__run(const char** args, usize args_len, os_cmd_c* out_cmd)
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
        if (args[i] == NULL) {
            return e$raise(
                Error.argument,
                "`args` item[%d] is NULL, which may indicate string operation failure",
                i
            );
        }
    }


#ifdef _WIN32
    // FIX:  WIN32 uncompilable

    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
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


const struct __module__os os = {
    // Autogenerated by CEX
    // clang-format off
    .sleep = os_sleep,

    .fs = {  // sub-module .fs >>>
        .rename = os__fs__rename,
        .mkdir = os__fs__mkdir,
        .stat = os__fs__stat,
        .remove = os__fs__remove,
        .dir_walk = os__fs__dir_walk,
        .find = os__fs__find,
        .getcwd = os__fs__getcwd,
    },  // sub-module .fs <<<

    .env = {  // sub-module .env >>>
        .get = os__env__get,
        .set = os__env__set,
        .unset = os__env__unset,
    },  // sub-module .env <<<

    .path = {  // sub-module .path >>>
        .exists = os__path__exists,
        .join = os__path__join,
        .split = os__path__split,
        .basename = os__path__basename,
        .dirname = os__path__dirname,
    },  // sub-module .path <<<

    .cmd = {  // sub-module .cmd >>>
        .create = os__cmd__create,
        .is_alive = os__cmd__is_alive,
        .kill = os__cmd__kill,
        .join = os__cmd__join,
        .stdout = os__cmd__stdout,
        .stderr = os__cmd__stderr,
        .stdin = os__cmd__stdin,
        .read_all = os__cmd__read_all,
        .read_line = os__cmd__read_line,
        .write_line = os__cmd__write_line,
        .run = os__cmd__run,
    },  // sub-module .cmd <<<
    // clang-format on
};
