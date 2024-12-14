#pragma once
#include <cex/cex.h>

typedef struct deque_new_kwargs_s
{
    usize max_capacity;
    bool rewrite_overflowed;
} deque_new_kwargs_s;

typedef struct
{
    alignas(64) struct
    {
        u64 magic : 16;             // deque magic number for sanity checks
        u64 elsize : 16;            // element size
        u64 elalign : 8;            // align of element type
        u64 eloffset : 8;           // offset in bytes to the 1st element array
        u64 rewrite_overflowed : 8; // allow rewriting old unread values if que overflowed
    } header;
    usize idx_head;
    usize idx_tail;
    usize max_capacity; // maximum capacity when deque stops growing (must be pow of 2!)
    usize capacity;
    const Allocator_i* allocator; // can be NULL for static deque
} _cex_deque_head_s;
_Static_assert(sizeof(_cex_deque_head_s) == 64, "size");
_Static_assert(alignof(_cex_deque_head_s) == 64, "align");

struct __cex_deque_c
{
    _cex_deque_head_s _head;
    // NOTE: data is hidden, it makes no sense to access it directly
};
typedef struct __cex_deque_c* _cex_deque_c;

#define _deque$typedef_extern_0(typename) extern const struct typename##_vtable__ typename
#define _deque$typedef_extern_1(typename)
#define _deque$typedef_extern(typename, implement) _deque$typedef_extern_##implement(typename)

#define _deque$typedef_impl_0(typename)
#define _deque$typedef_impl_1(typename)                                                            \
    const struct typename##_vtable__ typename = {                                                  \
        .get = (void*)_cex_deque_get,                                                              \
        .pop = (void*)_cex_deque_pop,                                                              \
        .push = (void*)_cex_deque_push,                                                            \
        .enqueue = (void*)_cex_deque_enqueue,                                                      \
        .dequeue = (void*)_cex_deque_dequeue,                                                      \
        .len = (void*)_cex_deque_len,                                                              \
        .clear = (void*)_cex_deque_clear,                                                          \
        .iter_get = (void*)_cex_deque_iter_get,                                                    \
        .iter_fetch = (void*)_cex_deque_iter_fetch,                                                \
        .destroy = (void*)_cex_deque_destroy,                                                      \
    };
#define _deque$typedef_impl(typename, implement) _deque$typedef_impl_##implement(typename)

#define deque$impl(typename) _deque$typedef_impl(typename, 1)


// clang-format off
#define deque$define_static_buf(var_name, dequetype, capacity)                                                  \
    alignas(alignof(_cex_deque_head_s)) char var_name[          \
        sizeof(_cex_deque_head_s) + sizeof(*(dequetype##_c){ 0 }._dtype) * (capacity) \
    ] = {0}; \
    _Static_assert((capacity) > 0, "list$define_static_buf zero capacity");
// clang-format on

#define deque$typedef(typename, eltype, implement)                                                 \
    _Static_assert(alignof(eltype) <= 64, "alignment of element is too high, max is 64");          \
    typedef struct typename##_c typename##_c;                                                      \
    struct typename##_vtable__                                                                     \
    {                                                                                              \
        eltype* (*get)(typename##_c * self, usize index);                                          \
        Exception (*push)(typename##_c * self, eltype * item);                                     \
        eltype* (*pop)(typename##_c * self);                                                       \
        Exception (*enqueue)(typename##_c * self, eltype * item);                                  \
        eltype* (*dequeue)(typename##_c * self);                                                   \
        void (*destroy)(typename##_c * self);                                                      \
        void (*clear)(typename##_c * self);                                                        \
        usize (*len)(typename##_c * self);                                                         \
        eltype* (*iter_get)(typename##_c * self, i32 direction, cex_iterator_s * iterator);        \
        eltype* (*iter_fetch)(typename##_c * self, i32 direction, cex_iterator_s * iterator);      \
    };                                                                                             \
    _deque$typedef_extern(typename, implement);                                                    \
    _deque$typedef_impl(typename, implement);                                                      \
    typedef struct typename##_c                                                                    \
    {                                                                                              \
        union                                                                                      \
        {                                                                                          \
            _cex_deque_c base;                                                                     \
            eltype* const _dtype; /* virtual field, only for type checks, const pointer */         \
        };                                                                                         \
    }                                                                                              \
    typename##_c;

#define deque$new(self, allocator, /* deque_new_kwargs_s */...)                                    \
    (_cex_deque_create(                                                                            \
        &(self)->base,                                                                             \
        sizeof(*((self)->_dtype)),                                                                 \
        alignof(typeof(*((self)->_dtype))),                                                        \
        allocator,                                                                                 \
        &(deque_new_kwargs_s){ __VA_ARGS__ }                                                       \
    ))

#define deque$new_static(self, buf, buf_len, /* deque_new_kwargs_s */...)                          \
    (_cex_deque_create_static(                                                                     \
        &(self)->base,                                                                             \
        buf,                                                                                       \
        buf_len,                                                                                   \
        sizeof(*((self)->_dtype)),                                                                 \
        alignof(typeof(*((self)->_dtype))),                                                        \
        &(deque_new_kwargs_s){ __VA_ARGS__ }                                                       \
    ))

// clang-format off
Exception _cex_deque_validate(_cex_deque_c *self);
Exception _cex_deque_create(_cex_deque_c* self, usize elsize, usize elalign, const Allocator_i* allocator, deque_new_kwargs_s* kwargs);
Exception _cex_deque_create_static(_cex_deque_c* self, void* buf, usize buf_len, usize elsize, usize elalign, deque_new_kwargs_s* kwargs);
Exception _cex_deque_append(_cex_deque_c* self, const void* item);
Exception _cex_deque_enqueue(_cex_deque_c* self, const void* item);
Exception _cex_deque_push(_cex_deque_c* self, const void* item);
void* _cex_deque_dequeue(_cex_deque_c* self);
void* _cex_deque_pop(_cex_deque_c* self);
void* _cex_deque_get(_cex_deque_c* self, usize index);
usize _cex_deque_len(const _cex_deque_c* self);
void _cex_deque_clear(_cex_deque_c* self);
void _cex_deque_destroy(_cex_deque_c* self);
void* _cex_deque_iter_get(_cex_deque_c* self, i32 direction, cex_iterator_s* iterator);
void* _cex_deque_iter_fetch(_cex_deque_c* self, i32 direction, cex_iterator_s* iterator);
// clang-format on
