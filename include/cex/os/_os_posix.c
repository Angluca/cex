#pragma once
#include <cex/all.h>
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

static Exception
os__listdir_(const char* path, sbuf_c* buf)
{
    if (unlikely(path == NULL)){
        return Error.argument;
    }
    if (buf == NULL) {
        return Error.argument;
    }

    // use own as temp buffer for dir name (because path, may not be a valid null-term string)
    DIR* dp = NULL;
    Exc result = Error.ok;

    uassert(sbuf.isvalid(buf) && "buf is not valid sbuf_c (or missing initialization)");

    sbuf.clear(buf);

    e$goto(result = sbuf.append(buf, path), fail);

    dp = opendir(*buf);

    if (unlikely(dp == NULL)) {
        result = Error.not_found;
        goto fail;
    }
    sbuf.clear(buf);

    struct dirent* ep;
    while ((ep = readdir(dp)) != NULL) {
        if (str.eq(ep->d_name, ".")) {
            continue;
        }
        if (str.eq(ep->d_name, "..")) {
            continue;
        }
        e$goto(result = sbuf.appendf(buf, "%s\n", ep->d_name), fail);
    }

end:
    if (dp != NULL) {
        (void)closedir(dp);
    }
    return result;

fail:
    sbuf.clear(buf);
    goto end;
}

static Exception
os__getcwd_(sbuf_c* out)
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

static Exception
os__path__exists_(const char* path)
{
    if (path == NULL) {
        return Error.argument;
    }
    usize path_len = strlen(path);
    if (path_len == 0) return Error.argument;

    if(path_len >= PATH_MAX) {
        uassert(path_len < PATH_MAX && "Path is too long");
        return Error.argument;
    }

    int ret_code = access(path, F_OK);
    if (ret_code < 0){
        if(errno == ENOENT){
            return Error.not_found;
        } else {
            return strerror(errno);
        }
    }

    return Error.ok;
}
