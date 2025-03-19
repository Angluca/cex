#pragma once
#include "sbuf.h"
#include "_sprintf.h"
#include "cex.h"
#include "str.h"
#include <stdarg.h>

struct _sbuf__sprintf_ctx
{
    sbuf_head_s* head;
    char* buf;
    Exc err;
    u32 count;
    u32 length;
    char tmp[CEX_SPRINTF_MIN];
};

static inline sbuf_head_s*
sbuf__head(sbuf_c self)
{
    uassert(self != NULL);
    sbuf_head_s* head = (sbuf_head_s*)(self - sizeof(sbuf_head_s));

    uassert(head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");
    uassert(head->capacity > 0 && "zero capacity or memory corruption");
    uassert(head->length <= head->capacity && "count > capacity");
    uassert(head->header.nullterm == 0 && "nullterm != 0");

    return head;
}
static inline usize
sbuf__alloc_capacity(usize capacity)
{
    uassert(capacity < INT32_MAX && "requested capacity exceeds 2gb, maybe overflow?");

    capacity += sizeof(sbuf_head_s) + 1; // also +1 for nullterm

    if (capacity >= 512) {
        return capacity * 1.2;
    } else {
        // Round up to closest pow*2 int
        u64 p = 4;
        while (p < capacity) {
            p *= 2;
        }
        return p;
    }
}
static inline Exception
sbuf__grow_buffer(sbuf_c* self, u32 length)
{
    sbuf_head_s* head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));

    if (unlikely(head->allocator == NULL)) {
        // sbuf is static, bad luck, overflow
        return Error.overflow;
    }

    u32 new_capacity = sbuf__alloc_capacity(length);
    head = mem$realloc(head->allocator, head, new_capacity);
    if (unlikely(head == NULL)) {
        *self = NULL;
        return Error.memory;
    }

    head->capacity = new_capacity - sizeof(sbuf_head_s) - 1,
    *self = (char*)head + sizeof(sbuf_head_s);
    (*self)[head->capacity] = '\0';
    return Error.ok;
}

static sbuf_c
sbuf_create(u32 capacity, IAllocator allocator)
{
    if (unlikely(allocator == NULL)) {
        uassert(allocator != NULL);
        return NULL;
    }

    if (capacity < 512) {
        capacity = sbuf__alloc_capacity(capacity);
    }

    char* buf = mem$malloc(allocator, capacity);
    if (unlikely(buf == NULL)) {
        return NULL;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = capacity - sizeof(sbuf_head_s) - 1,
        .allocator = allocator,
    };

    sbuf_c self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (self)[0] = '\0';
    (self)[head->capacity] = '\0';

    return self;
}

static sbuf_c
sbuf_create_temp(void)
{
    uassert(
        tmem$->scope_depth(tmem$) > 0 && "trying create tmem$ allocator outside mem$scope(tmem$)"
    );
    return sbuf_create(100, tmem$);
}

static sbuf_c
sbuf_create_static(char* buf, usize buf_size)
{
    if (unlikely(buf == NULL)) {
        uassert(buf != NULL);
        return NULL;
    }
    if (unlikely(buf_size == 0)) {
        uassert(buf_size > 0);
        return NULL;
    }
    if (unlikely(buf_size <= sizeof(sbuf_head_s) + 1)) {
        uassert(buf_size > sizeof(sbuf_head_s) + 1);
        return NULL;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = buf_size - sizeof(sbuf_head_s) - 1,
        .allocator = NULL,
    };
    sbuf_c self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (self)[0] = '\0';
    (self)[head->capacity] = '\0';

    return self;
}

static Exception
sbuf_grow(sbuf_c* self, u32 new_capacity)
{
    sbuf_head_s* head = sbuf__head(*self);
    if (new_capacity <= head->capacity) {
        // capacity is enough, no need to grow
        return Error.ok;
    }
    return sbuf__grow_buffer(self, new_capacity);
}

static void
sbuf_update_len(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    uassert((*self)[head->capacity] == '\0' && "capacity null term smashed!");

    head->length = strlen(*self);
    (*self)[head->length] = '\0';
}

static Exception
sbuf_replace(sbuf_c* self, const char* oldstr, const char* newstr)
{
    (void)self;
    (void)oldstr;
    (void)newstr;
    return "TODO: str_s api needed";
    /*
    uassert(self != NULL);

    sbuf_head_s* head = sbuf__head(*self);
    uassert(oldstr != newstr && "old and new overlap");
    uassert(*self != newstr && "self and new overlap");
    uassert(*self != oldstr && "self and old overlap");

    if (unlikely(oldstr == NULL || newstr == NULL)) {
        return Error.argument;
    }

    u32 old_len = strlen(oldstr);

    str_s s = str.cbuf(*self, head->length);

    if (unlikely(s.len == 0)) {
        return Error.ok;
    }
        u32 capacity = head->capacity;

        isize idx = -1;
        while ((idx = str.find(s, oldstr, idx + 1, 0)) != -1) {
            // pointer to start of the found `old`

            char* f = &((*self)[idx]);

            if (oldstr.len == newstr.len) {
                // Tokens exact match just replace
                memcpy(f, newstr.buf, newstr.len);
            } else if (newstr.len < oldstr.len) {
                // Move remainder of a string to fill the gap
                memcpy(f, newstr.buf, newstr.len);
                memmove(f + newstr.len, f + oldstr.len, s.len - idx - oldstr.len);
                s.len -= (oldstr.len - newstr.len);
                if (newstr.len == 0) {
                    // NOTE: Edge case: replacing all by empty string, reset index again
                    idx--;
                }
            } else {
                // Try resize
                if (unlikely(s.len + (newstr.len - oldstr.len) > capacity - 1)) {
                    e$except_silent(err, sbuf__grow_buffer(self, s.len + (newstr.len - oldstr.len)))
                    {
                        return err;
                    }
                    // re-fetch head in case of realloc
                    head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
                    s.buf = *self;
                    f = &((*self)[idx]);
                }
                // Move exceeding string to avoid overwriting
                memmove(f + newstr.len, f + oldstr.len, s.len - idx - oldstr.len + 1);
                memcpy(f, newstr.buf, newstr.len);
                s.len += (newstr.len - oldstr.len);
            }
        }

        head->length = s.len;
        // always null terminate
        (*self)[s.len] = '\0';

        return Error.ok;
        */
}


static void
sbuf_clear(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    head->length = 0;
    (*self)[head->length] = '\0';
}

static u32
sbuf_len(const sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    return head->length;
}

static u32
sbuf_capacity(const sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    return head->capacity;
}

static sbuf_c
sbuf_destroy(sbuf_c* self)
{
    uassert(self != NULL);

    if (*self != NULL) {
        sbuf_head_s* head = sbuf__head(*self);

        // NOTE: null-terminate string to avoid future usage,
        // it will appear as empty string if references anywhere else
        ((char*)head)[0] = '\0';
        (*self)[0] = '\0';
        *self = NULL;

        if (head->allocator != NULL) {
            // allocator is NULL for static sbuf
            mem$free(head->allocator, head);
        }
        memset(self, 0, sizeof(*self));
    }
    return NULL;
}

static char*
sbuf__sprintf_callback(const char* buf, void* user, u32 len)
{
    struct _sbuf__sprintf_ctx* ctx = (struct _sbuf__sprintf_ctx*)user;
    sbuf_c sbuf = ((char*)ctx->head + sizeof(sbuf_head_s));

    uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");
    if (unlikely(ctx->err != EOK)) {
        return NULL;
    }
    uassert((buf != ctx->buf) || (sbuf + ctx->length + len <= sbuf + ctx->count && "out of bounds"));

    if (unlikely(ctx->length + len > ctx->count)) {
        bool buf_is_tmp = buf != ctx->buf;

        if (len > INT32_MAX || ctx->length + len > (u32)INT32_MAX) {
            ctx->err = Error.integrity;
            return NULL;
        }

        // sbuf likely changed after realloc
        e$except_silent(err, sbuf__grow_buffer(&sbuf, ctx->length + len + 1))
        {
            ctx->err = err;
            return NULL;
        }
        // re-fetch head in case of realloc
        ctx->head = (sbuf_head_s*)(sbuf - sizeof(sbuf_head_s));
        uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");

        ctx->buf = sbuf + ctx->head->length;
        ctx->count = ctx->head->capacity;
        uassert(ctx->count >= ctx->length);
        if (!buf_is_tmp) {
            buf = ctx->buf;
        }
    }

    ctx->length += len;
    ctx->head->length += len;
    if (len > 0) {
        if (buf != ctx->buf) {
            memcpy(ctx->buf, buf, len); // copy data only if previously tmp buffer used
        }
        ctx->buf += len;
    }
    return ((ctx->count - ctx->length) >= CEX_SPRINTF_MIN) ? ctx->buf : ctx->tmp;
}

static Exception
sbuf__appendfva(sbuf_c* self, const char* format, va_list va)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    struct _sbuf__sprintf_ctx ctx = {
        .head = head,
        .err = EOK,
        .buf = *self + head->length,
        .length = head->length,
        .count = head->capacity,
    };

    cexsp__vsprintfcb(
        sbuf__sprintf_callback,
        &ctx,
        sbuf__sprintf_callback(NULL, &ctx, 0),
        format,
        va
    );

    // re-fetch self in case of realloc in sbuf__sprintf_callback
    *self = ((char*)ctx.head + sizeof(sbuf_head_s));

    // always null terminate
    (*self)[ctx.head->length] = '\0';
    (*self)[ctx.head->capacity] = '\0';

    return ctx.err;
}

static Exception
sbuf_appendf(sbuf_c* self, const char* format, ...)
{

    va_list va;
    va_start(va, format);
    Exc result = sbuf__appendfva(self, format, va);
    va_end(va);
    return result;
}

static Exception
sbuf_append(sbuf_c* self, const char* s)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    if (s == NULL) {
        return Error.argument;
    }

    u32 length = head->length;
    u32 capacity = head->capacity;
    u32 slen = strlen(s);

    uassert(*self != s && "buffer overlap");

    // Try resize
    if (length + slen > capacity - 1) {
        e$except_silent(err, sbuf__grow_buffer(self, length + slen))
        {
            return err;
        }
    }
    memcpy((*self + length), s, slen);
    length += slen;

    // always null terminate
    (*self)[length] = '\0';

    // re-fetch head in case of realloc
    head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
    head->length = length;

    return Error.ok;
}

static bool
sbuf_isvalid(sbuf_c* self)
{
    if (self == NULL) {
        return false;
    }
    if (*self == NULL) {
        return false;
    }

    sbuf_head_s* head = (sbuf_head_s*)((char*)(*self) - sizeof(sbuf_head_s));

    if (head->header.magic != 0xf00e) {
        return false;
    }
    if (head->capacity == 0) {
        return false;
    }
    if (head->length > head->capacity) {
        return false;
    }
    if (head->header.nullterm != 0) {
        return false;
    }

    return true;
}

// static inline isize
// sbuf__index(const char* self, usize self_len, const char* c, u8 clen)
// {
//     isize result = -1;
//
//     u8 split_by_idx[UINT8_MAX] = { 0 };
//     for (u8 i = 0; i < clen; i++) {
//         split_by_idx[(u8)c[i]] = 1;
//     }
//
//     for (usize i = 0; i < self_len; i++) {
//         if (split_by_idx[(u8)self[i]]) {
//             result = i;
//             break;
//         }
//     }
//
//     return result;
// }

const struct __module__sbuf sbuf = {
    // Autogenerated by CEX
    // clang-format off
    .create = sbuf_create,
    .create_temp = sbuf_create_temp,
    .create_static = sbuf_create_static,
    .grow = sbuf_grow,
    .update_len = sbuf_update_len,
    .replace = sbuf_replace,
    .clear = sbuf_clear,
    .len = sbuf_len,
    .capacity = sbuf_capacity,
    .destroy = sbuf_destroy,
    .appendf = sbuf_appendf,
    .append = sbuf_append,
    .isvalid = sbuf_isvalid,
    // clang-format on
};
