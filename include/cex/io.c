#pragma once
#include "all.h"
#include "_sprintf.h"
#include <errno.h>
#include <unistd.h>


Exception
io_fopen(FILE** file, const char* filename, const char* mode)
{
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }

    *file = NULL;

    if (filename == NULL) {
        return Error.argument;
    }
    if (mode == NULL) {
        uassert(mode != NULL);
        return Error.argument;
    }

    *file = fopen(filename, mode);
    if (*file == NULL) {
        switch (errno) {
            case ENOENT:
                return Error.not_found;
            default:
                return strerror(errno);
        }
    }
    return Error.ok;
}

int
io_fileno(FILE* file)
{
    uassert(file != NULL);
    return fileno(file);
}

bool
io_isatty(FILE* file)
{
    uassert(file != NULL);
    return isatty(fileno(file)) == 1;
}

Exception
io_fflush(FILE* file)
{
    uassert(file != NULL);

    int ret = fflush(file);
    if (unlikely(ret == -1)) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

Exception
io_fseek(FILE* file, long offset, int whence)
{
    uassert(file != NULL);

    int ret = fseek(file, offset, whence);
    if (unlikely(ret == -1)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
    } else {
        return Error.ok;
    }
}

void
io_rewind(FILE* file)
{
    uassert(file != NULL);
    rewind(file);
}

Exception
io_ftell(FILE* file, usize* size)
{
    uassert(file != NULL);

    long ret = ftell(file);
    if (unlikely(ret < 0)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
        *size = 0;
    } else {
        *size = ret;
        return Error.ok;
    }
}

usize
io__file__size(FILE* file)
{
    uassert(file != NULL);

    usize fsize = 0;
    usize old_pos = 0;

    e$except_silent(err, io_ftell(file, &old_pos))
    {
        return 0;
    }
    e$except_silent(err, io_fseek(file, 0, SEEK_END))
    {
        return 0;
    }
    e$except_silent(err, io_ftell(file, &fsize))
    {
        return 0;
    }
    e$except_silent(err, io_fseek(file, old_pos, SEEK_SET))
    {
        return 0;
    }

    return fsize;
}

Exception
io_fread(FILE* file, void* restrict obj_buffer, usize obj_el_size, usize* obj_count)
{
    if (file == NULL) {
        return Error.argument;
    }
    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == NULL || *obj_count == 0) {
        return Error.argument;
    }

    const usize ret_count = fread(obj_buffer, obj_el_size, *obj_count, file);

    if (ret_count != *obj_count) {
        if (ferror(file)) {
            *obj_count = 0;
            return Error.io;
        } else {
            *obj_count = ret_count;
            return (ret_count == 0) ? Error.eof : Error.ok;
        }
    }

    return Error.ok;
}

Exception
io_fread_all(FILE* file, str_s* s, IAllocator allc)
{
    Exc result = Error.runtime;
    usize buf_size = 0;
    char* buf = NULL;


    if (file == NULL) {
        result = Error.argument;
        goto fail;
    }
    uassert(s != NULL);
    uassert(allc != NULL);

    // Forbid console and stdin
    if (unlikely(io_isatty(file))) {
        result = "io.fread_all() not allowed for pipe/socket/std[in/out/err]";
        goto fail;
    }

    usize _fsize = io__file__size(file);
    if (unlikely(_fsize > INT32_MAX)) {
        result = "io.fread_all() file is too big.";
        goto fail;
    }
    bool is_stream = false;
    if (unlikely(_fsize == 0)) {
        if (feof(file)) {
            result = EOK; // return just empty data
            goto fail;
        } else {
            is_stream = true;
        }
    }
    buf_size = (is_stream ? 4096 : _fsize) + 1 + 15;
    buf = mem$malloc(allc, buf_size);

    if (unlikely(buf == NULL)) {
        buf_size = 0;
        result = Error.memory;
        goto fail;
    }

    size_t total_read = 0;
    while (1) {
        if (total_read == buf_size) {
            if (unlikely(total_read > INT32_MAX)) {
                result = "io.fread_all() stream output is too big.";
                goto fail;
            }
            if (buf_size > 100 * 1024 * 1024) {
                buf_size *= 1.2;
            } else {
                buf_size *= 2;
            }
            char* new_buf = mem$realloc(allc, buf, buf_size);
            if (!new_buf) {
                result = Error.memory;
                goto fail;
            }
            buf = new_buf;
        }

        // Read data into the buf
        size_t bytes_read = fread(buf + total_read, 1, buf_size - total_read, file);
        if (bytes_read == 0) {
            if (feof(file)) {
                break;
            } else if (ferror(file)) {
                result = Error.io;
                goto fail;
            }
        }
        total_read += bytes_read;
    }
    char* final_buffer = mem$realloc(allc, buf, total_read + 1);
    if (!final_buffer) {
        result = Error.memory;
        goto fail;
    }
    buf = final_buffer;

    *s = (str_s){
        .buf = buf,
        .len = total_read,
    };
    buf[total_read] = '\0';
    return EOK;

fail:
    *s = (str_s){
        .buf = NULL,
        .len = 0,
    };

    if (io_fseek(file, 0, SEEK_END)) {
        // unused result
    }

    if (buf) {
        mem$free(allc, buf);
    }
    return result;
}

Exception
io_fread_line(FILE* file, str_s* s, IAllocator allc)
{
    Exc result = Error.runtime;
    usize cursor = 0;
    FILE* fh = file;
    char* buf = NULL;
    usize buf_size = 0;

    if (unlikely(file == NULL)) {
        result = Error.argument;
        goto fail;
    }
    uassert(s != NULL);

    int c = EOF;
    while ((c = fgetc(fh)) != EOF) {
        if (unlikely(c == '\n')) {
            // Handle windows \r\n new lines also
            if (cursor > 0 && buf[cursor - 1] == '\r') {
                cursor--;
            }
            break;
        }
        if (unlikely(c == '\0')) {
            // plain text file should not have any zero bytes in there
            result = Error.integrity;
            goto fail;
        }

        if (unlikely(cursor >= buf_size)) {
            if (buf == NULL) {
                uassert(cursor == 0 && "no buf, cursor expected 0");

                buf = mem$malloc(allc, 256);
                if (buf == NULL) {
                    result = Error.memory;
                    goto fail;
                }
                buf_size = 256 - 1; // keep extra for null
                buf[buf_size] = '\0';
                buf[cursor] = '\0';
            } else {
                uassert(cursor > 0 && "no buf, cursor expected 0");
                uassert(buf_size > 0 && "empty buffer, weird");

                if (buf_size + 1 < 256) {
                    // Cap minimal buf size
                    buf_size = 256 - 1;
                }

                // Grow initial size by factor of 2
                buf = mem$realloc(allc, buf, (buf_size + 1) * 2);
                if (buf == NULL) {
                    buf_size = 0;
                    result = Error.memory;
                    goto fail;
                }
                buf_size = (buf_size + 1) * 2 - 1;
                buf[buf_size] = '\0';
            }
        }
        buf[cursor] = c;
        cursor++;
    }


    if (unlikely(ferror(file))) {
        result = Error.io;
        goto fail;
    }

    if (unlikely(cursor == 0 && feof(file))) {
        result = Error.eof;
        goto fail;
    }

    if (buf != NULL) {
        buf[cursor] = '\0';
    } else {
        buf = mem$malloc(allc, 1);
        buf[0] = '\0';
        cursor = 0;
    }
    *s = (str_s){
        .buf = buf,
        .len = cursor,
    };
    return Error.ok;

fail:
    if (buf) {
        mem$free(allc, buf);
    }
    *s = (str_s){
        .buf = NULL,
        .len = 0,
    };
    return result;
}

Exception
io_fprintf(FILE* stream, const char* format, ...)
{
    uassert(stream != NULL);

    va_list va;
    va_start(va, format);
    int result = cexsp__vfprintf(stream, format, va);
    va_end(va);

    if (result == -1) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

void
io_printf(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    cexsp__vfprintf(stdout, format, va);
    va_end(va);
}

Exception
io_fwrite(FILE* file, const void* restrict obj_buffer, usize obj_el_size, usize obj_count)
{
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }

    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == 0) {
        return Error.argument;
    }

    const usize ret_count = fwrite(obj_buffer, obj_el_size, obj_count, file);

    if (ret_count != obj_count) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

Exception
io__file__writeln(FILE* file, char* line)
{
    errno = 0;
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }
    if (line == NULL) {
        return Error.argument;
    }
    usize line_len = strlen(line);
    usize ret_count = fwrite(line, 1, line_len, file);
    if (ret_count != line_len) {
        return Error.io;
    }

    char new_line[] = { '\n' };
    ret_count = fwrite(new_line, 1, sizeof(new_line), file);

    if (ret_count != sizeof(new_line)) {
        return Error.io;
    }
    return Error.ok;
}

void
io_fclose(FILE** file)
{
    uassert(file != NULL);

    if (*file != NULL) {
        fclose(*file);
    }
    *file = NULL;
}


Exception
io__file__save(const char* path, const char* contents)
{
    if (path == NULL) {
        return Error.argument;
    }

    if (contents == NULL) {
        return Error.argument;
    }

    FILE* file;
    e$except_silent(err, io_fopen(&file, path, "w"))
    {
        return err;
    }

    usize contents_len = strlen(contents);
    e$except_silent(err, io_fwrite(file, contents, 1, contents_len))
    {
        io_fclose(&file);
        return err;
    }

    io_fclose(&file);
    return EOK;
}

char*
io__file__load(const char* path, IAllocator allc)
{
    errno = 0;
    uassert(allc != NULL);
    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    FILE* file;
    e$except_silent(err, io_fopen(&file, path, "r"))
    {
        return NULL;
    }

    str_s out_content = (str_s){ 0 };
    e$except_silent(err, io_fread_all(file, &out_content, allc))
    {
        if (err == Error.eof) {
            uassert(out_content.buf == NULL);
            out_content.buf = mem$malloc(allc, 1);
            if (out_content.buf) {
                out_content.buf[0] = '\0';
            }
            return out_content.buf;
        }
        if (out_content.buf) {
            mem$free(allc, out_content.buf);
        }
        return NULL;
    }
    return out_content.buf;
}

char*
io__file__readln(FILE* file, IAllocator allc)
{
    errno = 0;
    uassert(allc != NULL);
    if (file == NULL) {
        errno = EINVAL;
        return NULL;
    }

    str_s out_content = (str_s){ 0 };
    e$except_silent(err, io_fread_line(file, &out_content, allc))
    {
        if (err == Error.eof) {
            return NULL;
        }

        if (out_content.buf) {
            mem$free(allc, out_content.buf);
        }
        return NULL;
    }
    return out_content.buf;
}

const struct __module__io io = {
    // Autogenerated by CEX
    // clang-format off
    .fopen = io_fopen,
    .fileno = io_fileno,
    .isatty = io_isatty,
    .fflush = io_fflush,
    .fseek = io_fseek,
    .rewind = io_rewind,
    .ftell = io_ftell,
    .fread = io_fread,
    .fread_all = io_fread_all,
    .fread_line = io_fread_line,
    .fprintf = io_fprintf,
    .printf = io_printf,
    .fwrite = io_fwrite,
    .fclose = io_fclose,

    .file = {  // sub-module .file >>>
        .size = io__file__size,
        .writeln = io__file__writeln,
        .save = io__file__save,
        .load = io__file__load,
        .readln = io__file__readln,
    },  // sub-module .file <<<
    // clang-format on
};
