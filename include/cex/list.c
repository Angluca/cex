#pragma once
#include "list.h"

static inline _cex_list_head_s*
_cex_list__head(_cex_list_c const * self)
{
    uassert(self != NULL);
    uassert(self->arr != NULL && "array is not initialized");
    _cex_list_head_s* head = _list$head(self);
    if (head->header.magic != 0x1eed) {
        uassertf(false, "list: bad header - not a list / used on slice / bad pointer");
        return NULL;
    }
    uassert(head->capacity > 0 && "zero capacity or memory corruption");
    uassert(head->count <= head->capacity && "count > capacity");

    return head;
}
static inline usize
_cex_list__alloc_capacity(usize capacity)
{
    uassert(capacity > 0);

    if (capacity < 4) {
        return 4;
    } else if (capacity >= 1024) {
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

static inline void*
_cex_list__elidx(_cex_list_head_s const * head, usize idx)
{
    // Memory alignment
    // |-----|head|--el1--|--el2--|--elN--|
    //  ^^^^ - head can moved to el1, because of el1 alignment
    void* result = (char*)head + _CEX_LIST_BUF + (head->header.elsize * idx);

    uassert((usize)result % head->header.elalign == 0 && "misaligned array index pointer");

    return result;
}

static inline _cex_list_head_s*
_cex_list__realloc(_cex_list_head_s* head, usize alloc_size)
{
    // Memory alignment
    // |-----|head|--el1--|--el2--|--elN--|
    //  ^^^^ - head can moved to el1, because of el1 alignment
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    usize offset = 0;
    usize align = alignof(_cex_list_head_s);
    if (head->header.elalign > _CEX_LIST_BUF) {
        align = head->header.elalign;
        offset = head->header.elalign - _CEX_LIST_BUF;
    }
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    void* mptr = (char*)head - offset;
    uassert((usize)mptr % alignof(usize) == 0 && "misaligned malloc pointer");
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    uassert(alloc_size % head->header.elalign == 0 && "misaligned size");

    void* result = head->allocator->realloc_aligned(mptr, align, alloc_size);
    uassert((usize)result % align == 0 && "misaligned after realloc");

    head = (_cex_list_head_s*)((char*)result + offset);
    uassert(head->header.magic == 0x1eed && "not a dlist / bad pointer");
    return head;
}

static inline usize
_cex_list__alloc_size(usize capacity, usize elsize, usize elalign)
{
    uassert(capacity > 0 && "zero capacity");
    uassert(elsize > 0 && "zero element size");
    uassert(elalign > 0 && "zero element align");
    uassert(elsize % elalign == 0 && "element size has to be rounded to elalign");

    usize result = (capacity * elsize) + (elalign > _CEX_LIST_BUF ? elalign : _CEX_LIST_BUF);
    uassert(result % elalign == 0 && "alloc_size is unaligned");
    return result;
}

Exception
_cex_list_create(_cex_list_c* self, usize capacity, usize elsize, usize elalign, const Allocator_i* allocator)
{
    if (self == NULL) {
        uassert(self != NULL && "must not be NULL");
        return Error.argument;
    }

    if (allocator == NULL) {
        uassert(allocator != NULL && "allocator invalid");
        return Error.argument;
    }

    if (elsize == 0 || elsize > INT16_MAX) {
        uassert(elsize > 0 && "zero elsize");
        uassert(elsize < INT16_MAX && "element size if too high");
        return Error.argument;
    }
    if (elalign == 0 || (elalign & (elalign - 1)) != 0) {
        uassert((elalign & (elalign - 1)) == 0 && "elalign must be power of 2");
        uassert(elsize > 0 && "zero elsize");
        uassert(elalign > 0 && "zero elalign");
        return Error.argument;
    }

    // Clear dlist pointer
    memset(self, 0, sizeof(_cex_list_c));

    capacity = _cex_list__alloc_capacity(capacity);
    usize alloc_size = _cex_list__alloc_size(capacity, elsize, elalign);
    char* buf = allocator->malloc_aligned(elalign, alloc_size);

    if (buf == NULL) {
        return Error.memory;
    }

    if (elalign > _CEX_LIST_BUF) {
        buf += elalign - _CEX_LIST_BUF;
    }

    _cex_list_head_s* head = (_cex_list_head_s*)buf;
    *head = (_cex_list_head_s){
        .header = {
            .magic = 0x1eed,
            .elsize = elsize,
            .elalign = elalign,
        }, 
        .count = 0, 
        .capacity = capacity,
        .allocator = allocator,
    };

    _cex_list_c* d = (_cex_list_c*)self;
    d->len = 0;
    d->arr = _cex_list__elidx(head, 0);

    return Error.ok;
}

Exception
_cex_list_create_static(_cex_list_c* self, void* buf, usize buf_len, usize elsize, usize elalign)
{
    if (self == NULL) {
        uassert(self != NULL && "must not be NULL");
        return Error.argument;
    }

    if (elsize == 0 || elsize > INT16_MAX) {
        uassert(elsize > 0 && "zero elsize");
        uassert(elsize < INT16_MAX && "element size if too high");
        return Error.argument;
    }
    if (elalign == 0 || elalign > 64 || (elalign & (elalign - 1)) != 0) {
        uassert(elalign <= 64 && "el align is too high");
        uassert((elalign & (elalign - 1)) == 0 && "elalign must be power of 2");
        uassert(elsize > 0 && "zero elsize");
        uassert(elalign > 0 && "zero elalign");
        return Error.argument;
    }

    // Clear dlist pointer
    memset(self, 0, sizeof(_cex_list_c));

    usize offset = ((usize)buf + _CEX_LIST_BUF) % elalign;
    if (offset != 0) {
        offset = elalign - offset;
    }

    if (buf_len < _CEX_LIST_BUF + offset + elsize) {
        return Error.overflow;
    }
    usize max_capacity = (buf_len - _CEX_LIST_BUF - offset) / elsize;

    _cex_list_head_s* head = (_cex_list_head_s*)((char*)buf + offset);
    *head = (_cex_list_head_s){
        .header = {
            .magic = 0x1eed,
            .elsize = elsize,
            .elalign = elalign,
        }, 
        .count = 0, 
        .capacity = max_capacity,
        .allocator = NULL,
    };

    _cex_list_c* d = (_cex_list_c*)self;
    d->len = 0;
    d->arr = _cex_list__elidx(head, 0);

    return Error.ok;
}


Exception
_cex_list_insert(_cex_list_c* self, void* item, usize index)
{
    uassert(self != NULL);
    _cex_list_c* d = (_cex_list_c*)self;
    _cex_list_head_s* head = _cex_list__head(self);
    if (unlikely(head == NULL)) {
        return Error.integrity;
    }

    if (unlikely(index > head->count)) {
        return Error.argument;
    }

    if (unlikely(head->count == head->capacity)) {
        if (head->allocator == NULL) {
            return Error.overflow;
        }
        usize new_cap = _cex_list__alloc_capacity(head->capacity + 1);
        usize alloc_size = _cex_list__alloc_size(new_cap, head->header.elsize, head->header.elalign);
        head = _cex_list__realloc(head, alloc_size);
        uassert(head->header.magic == 0x1eed && "head missing after realloc");

        if (head == NULL) {
            d->len = 0;
            return Error.memory;
        }

        d->arr = _cex_list__elidx(head, 0);
        head->capacity = new_cap;
    }

    if (unlikely(index < head->count)) {
        memmove(
            _cex_list__elidx(head, index + 1),
            _cex_list__elidx(head, index),
            head->header.elsize * (head->count - index)
        );
    }

    memcpy(_cex_list__elidx(head, index), item, head->header.elsize);

    head->count++;
    d->len = head->count;

    return Error.ok;
}

Exception
_cex_list_del(_cex_list_c* self, usize index)
{
    uassert(self != NULL);
    _cex_list_c* d = (_cex_list_c*)self;
    _cex_list_head_s* head = _cex_list__head(self);
    if (unlikely(head == NULL)) {
        return Error.integrity;
    }

    if (unlikely(index >= head->count)) {
        return Error.argument;
    }

    if (unlikely(index < head->count)) {
        memmove(
            _cex_list__elidx(head, index),
            _cex_list__elidx(head, index + 1),
            head->header.elsize * (head->count - index - 1)
        );
    }

    head->count--;
    d->len = head->count;

    return Error.ok;
}

void
_cex_list_sort(_cex_list_c* self, int (*comp)(const void*, const void*))
{
    uassert(self != NULL);
    uassert(comp != NULL);
    _cex_list_c* d = (_cex_list_c*)self;
    _cex_list_head_s* head = _cex_list__head(self);
    if (head != NULL) {
        qsort(d->arr, head->count, head->header.elsize, comp);
    }
}


Exception
_cex_list_append(_cex_list_c* self, void* item)
{
    uassert(self != NULL);
    _cex_list_head_s* head = _cex_list__head(self);
    return _cex_list_insert(self, item, head->count);
}

void
_cex_list_clear(_cex_list_c* self)
{
    uassert(self != NULL);

    _cex_list_c* d = (_cex_list_c*)self;
    _cex_list_head_s* head = _cex_list__head(self);
    if (head != NULL) {
        head->count = 0;
        d->len = 0;
    }
}

Exception
_cex_list_extend(_cex_list_c* self, void* items, usize nitems)
{
    uassert(self != NULL);

    _cex_list_c* d = (_cex_list_c*)self;
    _cex_list_head_s* head = _cex_list__head(self);
    if (unlikely(head == NULL)) {
        return Error.integrity;
    }

    if (unlikely(items == NULL || nitems == 0)) {
        return Error.argument;
    }

    if (unlikely(head->count + nitems > head->capacity)) {
        if (unlikely(head->allocator == NULL)) {
            return Error.overflow;
        }

        usize new_cap = _cex_list__alloc_capacity(head->capacity + nitems);

        usize alloc_size = _cex_list__alloc_size(new_cap, head->header.elsize, head->header.elalign);
        head = _cex_list__realloc(head, alloc_size);
        uassert(head->header.magic == 0x1eed && "head missing after realloc");

        if (unlikely(d->arr == NULL)) {
            d->len = 0;
            return Error.memory;
        }

        d->arr = _cex_list__elidx(head, 0);
        head->capacity = new_cap;
    }
    memcpy(_cex_list__elidx(head, d->len), items, head->header.elsize * nitems);
    head->count += nitems;
    d->len = head->count;

    return Error.ok;
}

usize
_cex_list_capacity(_cex_list_c const * self)
{
    _cex_list_head_s* head = _cex_list__head(self);
    if (unlikely(head == NULL)) {
        return 0;                
    }
    return head->capacity;
}

void
_cex_list_destroy(_cex_list_c* self)
{
    _cex_list_c* d = (_cex_list_c*)self;

    if (self != NULL) {
        if (d->arr != NULL) {
            _cex_list_head_s* head = _cex_list__head(self);
            if (unlikely(head == NULL)) {
                goto end;
            }

            usize offset = 0;
            if (head->header.elalign > _CEX_LIST_BUF) {
                offset = head->header.elalign - _CEX_LIST_BUF;
            }

            void* mptr = (char*)head - offset;
            if (head->allocator != NULL) {
                // free only if it's a dynamic array
                head->allocator->free(mptr);
            } else {
                // in static list reset head
                memset(head, 0, sizeof(*head));
            }
        }
    }

end:
    d->arr = NULL;
    d->len = 0;
}

