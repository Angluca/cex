#pragma once
#include "os.h"

#ifdef _WIN32
#define os$PATH_SEP '\\'
#else
#define os$PATH_SEP '/'
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

void
os_sleep(u32 period_millisec)
{
#ifdef _WIN32
    Sleep(period_millisec);
#else
    usleep(period_millisec * 1000);
#endif
}

Exception
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

Exception
os__fs__remove(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
#ifdef _WIN32
    if (!DeleteFileA(path)) {
        return  nob_win32_error_message(GetLastError());
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
os__fs__listdir(const char* path, sbuf_c* out_buf)
{

    if (unlikely(path == NULL)) {
        return Error.argument;
    }
    if (out_buf == NULL) {
        return Error.argument;
    }
#ifdef _WIN32
#error "TODO: add minirent.h for windows use"
#endif

    // use own as temp outfer for dir name (because path, may not be a valid null-term string)
    DIR* dp = NULL;
    Exc result = Error.ok;

    uassert(sbuf.isvalid(out_buf) && "buf is not valid sbuf_c (or missing initialization)");

    sbuf.clear(out_buf);

    e$goto(result = sbuf.append(out_buf, path), fail);

    dp = opendir(*out_buf);

    if (unlikely(dp == NULL)) {
        result = Error.not_found;
        goto fail;
    }
    sbuf.clear(out_buf);

    struct dirent* ep;
    while ((ep = readdir(dp)) != NULL) {
        if (str.eq(ep->d_name, ".")) {
            continue;
        }
        if (str.eq(ep->d_name, "..")) {
            continue;
        }
        e$goto(result = sbuf.appendf(out_buf, "%s\n", ep->d_name), fail);
    }

end:
    if (dp != NULL) {
        (void)closedir(dp);
    }
    return result;

fail:
    sbuf.clear(out_buf);
    goto end;
    return os__fs__listdir(path, out_buf);
}

Exception
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

const char*
os__env__get(const char* name, const char* deflt)
{
    const char* result = getenv(name);

    if (result == NULL) {
        result = deflt;
    }

    return result;
}

void
os__env__set(const char* name, const char* value, bool overwrite)
{
    setenv(name, value, overwrite);
}

void
os__env__unset(const char* name)
{
    unsetenv(name);
}

Exception
os__path__exists(const char* file_path)
{

    if (file_path == NULL) {
        return Error.argument;
    }

    usize path_len = strlen(file_path);
    if (path_len == 0) return Error.argument;

    if(path_len >= PATH_MAX) {
        return Error.argument;
    }

#if _WIN32
    errno = 0;
    DWORD dwAttrib = GetFileAttributesA(file_path);
    if(dwAttrib != INVALID_FILE_ATTRIBUTES) {
        
        return EOK;
    } else {
        /*
        Here are some common file-related functions and the types of errors they might set:
CreateFile: Used to open or create a file.
ERROR_FILE_NOT_FOUND (2): The file does not exist.
ERROR_PATH_NOT_FOUND (3): The specified path does not exist.
ERROR_ACCESS_DENIED (5): The file cannot be accessed due to permissions.
ERROR_SHARING_VIOLATION (32): The file is in use by another process.
ReadFile: Used to read data from a file.
ERROR_HANDLE_EOF (38): Attempted to read past the end of the file.
ERROR_IO_DEVICE (1117): An I/O error occurred.
WriteFile: Used to write data to a file.
ERROR_DISK_FULL (112): The disk is full.
ERROR_ACCESS_DENIED (5): The file is read-only or locked.
CloseHandle: Used to close a file handle.
ERROR_INVALID_HANDLE (6): The handle is invalid.
DeleteFile: Used to delete a file.
ERROR_FILE_NOT_FOUND (2): The file does not exist.
ERROR_ACCESS_DENIED (5): The file is read-only or in use.
MoveFile: Used to move or rename a file.
ERROR_FILE_EXISTS (80): The destination file already exists.
ERROR_ACCESS_DENIED (5): The file is in use or locked.
Example: Using GetLastError() with File Operations
    */
        // TODO: handle other win32 errors!
        uassert(false && "TODO handle other win32 errors!");
        return Error.not_found;
    }
#else
    struct stat statbuf;
    if (stat(file_path, &statbuf) < 0) {
        if (errno == ENOENT) return Error.not_found;
        return strerror(errno);
    }
    return EOK;
#endif
}

char*
os__path__join(arr$(char*) parts, IAllocator allc)
{
    char sep[2] = { os$PATH_SEP, '\0' };
    return str.join(parts, sep, allc);
}

str_s
os__path__splitext(const char* path, bool return_ext)
{
    if (path == NULL) {
        return (str_s){ 0 };
    }
    usize pathlen = strlen(path);
    if (pathlen == 0) {
        return str$s("");
    }

    usize last_char_idx = pathlen;
    usize last_dot_idx = pathlen;

    for (usize i = pathlen; i-- > 0;) {
        if (path[i] == '.') {
            if (last_dot_idx == pathlen) {
                last_dot_idx = i;
            }
        } else if (path[i] == '/' || path[i] == '\\') {
            break;
        } else if (last_dot_idx != pathlen) {
            last_char_idx = i;
            break;
        }
    }
    if (last_char_idx < last_dot_idx) {
        if (return_ext) {
            return str.sub(path, last_dot_idx, 0);
        } else {
            return str.sub(path, 0, last_dot_idx);
        }

    } else {
        if (return_ext) {
            return str$s("");
        } else {
            return str.sstr(path);
        }
    }
}

#define os$cmd(args...)                                                                            \
    ({                                                                                             \
        const char* _args[] = { args, NULL };                                                      \
        usize _args_len = arr$len(_args);                                                          \
        os_cmd_c _cmd = { 0 };                                                                     \
        Exc result = EOK;                                                                          \
        e$except(err, os.cmd.run(_args, _args_len, &_cmd))                                         \
        {                                                                                          \
            result = err;                                                                          \
        }                                                                                          \
        if (result == EOK) {                                                                       \
            e$except(err, os.cmd.join(&_cmd, 0, NULL))                                             \
            {                                                                                      \
                result = err;                                                                      \
            }                                                                                      \
        }                                                                                          \
        result;                                                                                    \
    })

Exception
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

bool
os__cmd__is_alive(os_cmd_c* self)
{
    return subprocess_alive(&self->_subpr);
}

Exception
os__cmd__kill(os_cmd_c* self)
{
    if (subprocess_alive(&self->_subpr)) {
        if (subprocess_terminate(&self->_subpr) != 0) {
            return Error.os;
        }
    }
    return EOK;
}

Exception
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

FILE*
os__cmd__stdout(os_cmd_c* self)
{
    return self->_subpr.stdout_file;
}


FILE*
os__cmd__stderr(os_cmd_c* self)
{
    return self->_subpr.stderr_file;
}

FILE*
os__cmd__stdin(os_cmd_c* self)
{
    return self->_subpr.stdin_file;
}

char*
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

char*
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

Exception
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


Exception
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
            log$error("Could not exec child process: %s", strerror(errno));
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
        .mkdir = os__fs__mkdir,
        .remove = os__fs__remove,
        .listdir = os__fs__listdir,
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
        .splitext = os__path__splitext,
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
