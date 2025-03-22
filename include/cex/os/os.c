#pragma once
#include "os.h"

#ifdef _WIN32
#include "_os_win.c"
#define os$PATH_SEP '\\'
#else
#include "_os_posix.c"
#define os$PATH_SEP '/'
#endif

Exception
os_listdir(const char* path, sbuf_c* out)
{
    return os__listdir_(path, out);
}

Exception
os_getcwd(sbuf_c* out)
{
    return os__getcwd_(out);
}

const char*
os_getenv(const char* name, const char* deflt)
{
    const char* result = getenv(name);

    if (result == NULL) {
        result = deflt;
    }

    return result;
}

void
os_setenv(const char* name, const char* value, bool overwrite)
{
    setenv(name, value, overwrite);
}

void
os_unsetenv(const char* name)
{
    unsetenv(name);
}

Exception
os__path__exists(const char* path)
{
    return os__path__exists_(path);
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
            e$except(err, os.cmd.wait(&_cmd))                                                      \
            {                                                                                      \
                result = err;                                                                      \
            }                                                                                      \
            if (result == EOK && _cmd._subpr.return_status != 0) {                                 \
                result = Error.runtime;                                                            \
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
        .is_subprocess = true,
        ._flags = (flags) ? *flags : (os_cmd_flags_s){ 0 },
    };

    // TODO: set flags

    int result = subprocess_create(
        (const char* const*)args,
        subprocess_option_inherit_environment | subprocess_option_search_user_path,
        &self->_subpr
    );
    if (0 != result) {
        uassert(errno != 0);
        return strerror(errno);
    }

    return EOK;
}
Exception
os__cmd__join(os_cmd_c* self, i32* out_ret_code)
{
    uassert(self != NULL);
    int ret_code = 0;

    int result = subprocess_join(&self->_subpr, &ret_code);
    if (result != 0) {
        if (out_ret_code) {
            *out_ret_code = -1;
        }
        return Error.os;
    }
    if (out_ret_code) {
        *out_ret_code = ret_code;
    }
    if (ret_code != 0) {
        return Error.runtime;
    }
    return EOK;
}

Exception
os__cmd__ret_code(os_cmd_c* self)
{
    uassert(self != NULL);
    int ret_code = 0;
    int result = subprocess_join(&self->_subpr, &ret_code);
    if (result != 0) {
        return Error.os;
    }
    if (ret_code != 0) {
        return Error.runtime;
    }
    return EOK;
}

char*
os__cmd__read_all(os_cmd_c* self, IAllocator allc)
{
    uassert(self != NULL);
    uassert(allc != NULL);
    if (self->_subpr.stdout_file) {
        str_s out = { 0 };
        if(io.fread_all(self->_subpr.stdout_file, &out, allc))
        {
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
        if(io.fread_line(self->_subpr.stdout_file, &out, allc))
        {
            return NULL;
        }
        return out.buf;
    }
    return NULL;
}

Exception
os__cmd__destroy(os_cmd_c* self)
{
    if (self == NULL) {
        return EOK;
    }

    if (self->_subpr.alive) {
        return Error.exists;
    }

    subprocess_destroy(&self->_subpr);
    memset(self, 0, sizeof(os_cmd_c));
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

    *out_cmd = (os_cmd_c){ .is_subprocess = false,
                           ._subpr = {
                               .child = cpid,
                               .alive = 1,
                           } };
    return EOK;

#endif
}

Exception
os__cmd__wait(os_cmd_c* self)
{
    uassert(self != NULL);
    if (!self->_subpr.alive) {
        return Error.empty;
    }

    if (subprocess_join(&self->_subpr, &self->_subpr.return_status)) {
        uassert(errno != 0);
        return strerror(errno);
    }
    return EOK;
}


const struct __module__os os = {
    // Autogenerated by CEX
    // clang-format off
    .listdir = os_listdir,
    .getcwd = os_getcwd,
    .getenv = os_getenv,
    .setenv = os_setenv,
    .unsetenv = os_unsetenv,

    .path = {  // sub-module .path >>>
        .exists = os__path__exists,
        .join = os__path__join,
        .splitext = os__path__splitext,
    },  // sub-module .path <<<

    .cmd = {  // sub-module .cmd >>>
        .create = os__cmd__create,
        .join = os__cmd__join,
        .ret_code = os__cmd__ret_code,
        .read_all = os__cmd__read_all,
        .read_line = os__cmd__read_line,
        .destroy = os__cmd__destroy,
        .run = os__cmd__run,
        .wait = os__cmd__wait,
    },  // sub-module .cmd <<<
    // clang-format on
};
