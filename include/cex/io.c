#pragma once
#include "io.h"
#include "_sprintf.h"
#include "cex.h"
#include <errno.h>
#include <unistd.h>

Exception
io_open(io_c* self, const char* filename, const char* mode, IAllocator allocator)
{
    if (self == NULL) {
        uassert(self != NULL);
        return Error.argument;
    }
    if (filename == NULL) {
        uassert(filename != NULL);
        return Error.argument;
    }
    if (mode == NULL) {
        uassert(mode != NULL);
        return Error.argument;
    }
    if (allocator == NULL) {
        uassert(allocator != NULL);
        return Error.argument;
    }


    *self = (io_c){
        .file = fopen(filename, mode),
        ._allocator = allocator,
    };

    if (self->file == NULL) {
        *self = (io_c){ 0 };
        switch (errno) {
            case ENOENT:
                return Error.not_found;
            default:
                return strerror(errno);
        }
    } else {
        return Error.ok;
    }
}

io_c
io_fattach(FILE* fh, IAllocator allocator)
{
    if (allocator == NULL) {
        uassert(allocator != NULL);
    }
    if (fh == NULL) {
        uassert(fh != NULL);
    }

    return (io_c){ .file = fh,
                   ._allocator = allocator,
                   ._flags = {
                       .is_attached = true,
                   } };
}

int
io_fileno(io_c* self)
{
    uassert(self != NULL);
    uassert(self->file != NULL);

    return fileno(self->file);
}

bool
io_isatty(io_c* self)
{
    uassert(self != NULL);
    uassert(self->file != NULL);

    return isatty(fileno(self->file)) == 1;
}

Exception
io_flush(io_c* self)
{
    uassert(self != NULL);
    uassert(self->file != NULL);

    int ret = fflush(self->file);
    if (unlikely(ret == -1)) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

Exception
io_seek(io_c* self, long offset, int whence)
{
    uassert(self != NULL);
    uassert(self->file != NULL);

    int ret = fseek(self->file, offset, whence);
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
io_rewind(io_c* self)
{
    uassert(self != NULL);
    uassert(self->file != NULL);

    rewind(self->file);
}

Exception
io_tell(io_c* self, usize* size)
{
    uassert(self != NULL);
    uassert(self->file != NULL);

    long ret = ftell(self->file);
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
io_size(io_c* self)
{
    uassert(self != NULL);
    uassert(self->file != NULL);

    if (self->_fsize == 0) {
        // Do some caching
        usize old_pos = 0;
        e$except_silent(err, io_tell(self, &old_pos))
        {
            return 0;
        }
        e$except_silent(err, io_seek(self, 0, SEEK_END))
        {
            return 0;
        }
        e$except_silent(err, io_tell(self, &self->_fsize))
        {
            return 0;
        }
        e$except_silent(err, io_seek(self, old_pos, SEEK_SET))
        {
            return 0;
        }
    }

    return self->_fsize;
}

Exception
io_read(io_c* self, void* restrict obj_buffer, usize obj_el_size, usize* obj_count)
{
    uassert(self != NULL);
    uassert(self->file != NULL);

    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == NULL || *obj_count == 0) {
        return Error.argument;
    }

    const usize ret_count = fread(obj_buffer, obj_el_size, *obj_count, self->file);

    if (ret_count != *obj_count) {
        if (ferror(self->file)) {
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
io_readall(io_c* self, str_c* s)
{
    uassert(self != NULL);
    uassert(self->file != NULL);
    uassert(s != NULL);

    // invalidate result if early exit
    *s = (str_c){
        .buf = NULL,
        .len = 0,
    };

    // Forbid console and stdin
    if (io_isatty(self)) {
        return "io.readall() not allowed for pipe/socket/std[in/out/err]";
    }

    self->_fsize = io_size(self);

    if (unlikely(self->_fsize == 0)) {

        *s = (str_c){
            .buf = "",
            .len = 0,
        };
        e$except(err, io_seek(self, 0, SEEK_END))
        {
            ;
        }
        return Error.eof;
    }
    // allocate extra 16 bytes, to catch condition when file size grows
    // this may be indication we are trying to read stream
    usize exp_size = self->_fsize + 1 + 15;

    if (self->_fbuf == NULL) {
        self->_fbuf = mem$malloc(self->_allocator, exp_size);
        self->_fbuf_size = exp_size;
    } else {
        if (self->_fbuf_size < exp_size) {
            self->_fbuf = mem$realloc(self->_allocator, self->_fbuf, exp_size);
            self->_fbuf_size = exp_size;
        }
    }
    if (unlikely(self->_fbuf == NULL)) {
        self->_fbuf_size = 0;
        return Error.memory;
    }

    usize read_size = self->_fbuf_size;
    e$except_silent(err, io_read(self, self->_fbuf, sizeof(char), &read_size))
    {
        return err;
    }

    if (read_size != self->_fsize) {
        return "File size changed";
    }

    *s = (str_c){
        .buf = self->_fbuf,
        .len = read_size,
    };

    // Always null terminate
    self->_fbuf[read_size] = '\0';

    return read_size == 0 ? Error.eof : Error.ok;
}

Exception
io_readline(io_c* self, str_c* s)
{
    uassert(self != NULL);
    uassert(self->file != NULL);
    uassert(s != NULL);


    Exc result = Error.ok;
    usize cursor = 0;
    FILE* fh = self->file;
    char* buf = self->_fbuf;
    usize buf_size = self->_fbuf_size;

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
            if (self->_fbuf == NULL) {
                uassert(cursor == 0 && "no buf, cursor expected 0");

                self->_fbuf = buf = mem$malloc(self->_allocator, 4096);
                if (self->_fbuf == NULL) {
                    result = Error.memory;
                    goto fail;
                }
                self->_fbuf_size = buf_size = 4096 - 1; // keep extra for null
                self->_fbuf[self->_fbuf_size] = '\0';
                self->_fbuf[cursor] = '\0';
            } else {
                uassert(cursor > 0 && "no buf, cursor expected 0");
                uassert(self->_fbuf_size > 0 && "empty buffer, weird");

                if (self->_fbuf_size + 1 < 4096) {
                    // Cap minimal buf size
                    self->_fbuf_size = 4095;
                }

                // Grow initial size by factor of 2
                self->_fbuf = buf = mem$realloc(
                    self->_allocator,
                    self->_fbuf,
                    (self->_fbuf_size + 1) * 2
                );
                if (self->_fbuf == NULL) {
                    self->_fbuf_size = 0;
                    result = Error.memory;
                    goto fail;
                }
                self->_fbuf_size = buf_size = (self->_fbuf_size + 1) * 2 - 1;
                self->_fbuf[self->_fbuf_size] = '\0';
            }
        }
        buf[cursor] = c;
        cursor++;
    }

    if (self->_fbuf != NULL) {
        self->_fbuf[cursor] = '\0';
    }

    if (ferror(self->file)) {
        result = Error.io;
        goto fail;
    }

    if (cursor == 0) {
        // return valid str_c, but empty string
        *s = (str_c){
            .buf = "",
            .len = cursor,
        };
        return (feof(self->file) ? Error.eof : Error.ok);
    } else {
        *s = (str_c){
            .buf = self->_fbuf,
            .len = cursor,
        };
        return Error.ok;
    }

fail:
    *s = (str_c){
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
io_write(io_c* self, void* restrict obj_buffer, usize obj_el_size, usize obj_count)
{
    uassert(self != NULL);
    uassert(self->file != NULL);

    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == 0) {
        return Error.argument;
    }

    const usize ret_count = fwrite(obj_buffer, obj_el_size, obj_count, self->file);

    if (ret_count != obj_count) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

void
io_close(io_c* self)
{
    if (self != NULL) {

        if (self->file != NULL && !self->_flags.is_attached) {
            uassert(self->_allocator != NULL && "allocator not set");
            // prevent closing attached FILE* (i.e. stdin/out or other)
            fclose(self->file);
        }

        if (self->_fbuf != NULL) {
            uassert(self->_allocator != NULL && "allocator not set");
            mem$free(self->_allocator, self->_fbuf);
        }

        memset(self, 0, sizeof(*self));
    }
}


Exception
io_fload(const char* path, str_c* out_content, IAllocator allc)
{
    // invalidate result if early exit
    *out_content = (str_c){
        .buf = NULL,
        .len = 0,
    };
    io_c self = { 0 };
    e$except_silent(err, io_open(&self, path, "r", allc))
    {
        return err;
    }

    e$except_silent(err, io_readall(&self, out_content))
    {
        if (err == Error.eof && out_content->buf != NULL) {
            return EOK;
        }
        return err;
    }

    return EOK;
}


const struct __module__io io = {
    // Autogenerated by CEX
    // clang-format off
    .open = io_open,
    .fattach = io_fattach,
    .fileno = io_fileno,
    .isatty = io_isatty,
    .flush = io_flush,
    .seek = io_seek,
    .rewind = io_rewind,
    .tell = io_tell,
    .size = io_size,
    .read = io_read,
    .readall = io_readall,
    .readline = io_readline,
    .fprintf = io_fprintf,
    .printf = io_printf,
    .write = io_write,
    .close = io_close,
    .fload = io_fload,
    // clang-format on
};
