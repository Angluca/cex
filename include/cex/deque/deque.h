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
        usize magic : 16;             // deque magic number for sanity checks
        usize elsize : 16;            // element size
        usize elalign : 8;            // align of element type
        usize eloffset : 8;           // offset in bytes to the 1st element array
        usize rewrite_overflowed : 8; // allow rewriting old unread values if que overflowed
    } header;
    usize idx_head;
    usize idx_tail;
    usize max_capacity; // maximum capacity when deque stops growing (must be pow of 2!)
    usize capacity;
    const Allocator_i* allocator; // can be NULL for static deque
} deque_head_s;
_Static_assert(sizeof(deque_head_s) == 64, "size");
_Static_assert(alignof(deque_head_s) == 64, "align");

struct _deque_c
{
    deque_head_s _head;
    // NOTE: data is hidden, it makes no sense to access it directly
};
typedef struct _deque_c* deque_c;

#define _deque$typedef_extern_0(typename) extern const struct typename##_vtable__ typename
#define _deque$typedef_extern_1(typename)
#define _deque$typedef_extern(typename, implement) _deque$typedef_extern_##implement(typename)

#define _deque$typedef_impl_0(typename)
#define _deque$typedef_impl_1(typename)                                                            \
    const struct typename##_vtable__ typename = {                                                  \
        .append = (void*)deque.append,                                                             \
        .pop = (void*)deque.pop,                                                                   \
        .len = (void*)deque.len,                                                                   \
        .enqueue = (void*)deque.enqueue,                                                           \
        .dequeue = (void*)deque.dequeue,                                                           \
        .destroy = (void*)deque.destroy,                                                           \
    };
#define _deque$typedef_impl(typename, implement) _deque$typedef_impl_##implement(typename)

#define deque$impl(typename) _deque$typedef_impl(typename, 1)

#define deque$typedef(typename, eltype, implement)                                                 \
    typedef struct typename##_c typename##_c;                                                      \
    struct typename##_vtable__                                                                     \
    {                                                                                              \
        Exception (*append)(typename##_c * self, eltype * item);                                   \
        eltype* (*pop)(typename##_c * self);                                                       \
        usize (*len)(typename##_c * self);                                                         \
        Exception (*enqueue)(typename##_c * self, eltype * item);                                  \
        eltype* (*dequeue)(typename##_c * self);                                                   \
        void (*destroy)(typename##_c * self);                                                      \
    };                                                                                             \
    _deque$typedef_extern(typename, implement);                                                    \
    _deque$typedef_impl(typename, implement);                                                      \
    typedef struct typename##_c                                                                    \
    {                                                                                              \
        union                                                                                      \
        {                                                                                          \
            deque_c base;                                                                          \
            eltype* const _dtype; /* virtual field, only for type checks, const pointer */         \
        };                                                                                         \
    }                                                                                              \
    typename##_c;

#define deque$new(self, allocator, ...)                                                            \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), deque_c *: 1, default: 0),                               \
            "self argument expected to be deque$typedef() class"                                           \
        );                                                                                         \
        deque_new_kwargs_s kwargs = { __VA_ARGS__ };                                               \
        deque.create(self, sizeof(eltype), alignof(eltype), allocator, &kwargs);                   \
    })

#define deque$new_static(self, eltype, buf, buf_len, rewrite_overflowed)                           \
    ({                                                                                             \
        deque_new_kwargs_s kwargs = { __VA_ARGS__ };                                               \
        deque.create_static(self, buf, buf_len, sizeof(eltype), alignof(eltype), &kwargs);         \
    })

struct __module__deque
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*validate)(deque_c *self);

Exception
(*create)(deque_c* self, usize elsize, usize elalign, const Allocator_i* allocator, deque_new_kwargs_s* kwargs);

Exception
(*create_static)(deque_c* self, void* buf, usize buf_len, bool rewrite_overflowed, usize elsize, usize elalign);

Exception
(*append)(deque_c* self, const void* item);

Exception
(*enqueue)(deque_c* self, const void* item);

Exception
(*push)(deque_c* self, const void* item);

void*
(*dequeue)(deque_c* self);

void*
(*pop)(deque_c* self);

void*
(*get)(deque_c* self, usize index);

usize
(*len)(const deque_c* self);

void
(*clear)(deque_c* self);

void*
(*destroy)(deque_c* self);

void*
(*iter_get)(deque_c* self, i32 direction, cex_iterator_s* iterator);

void*
(*iter_fetch)(deque_c* self, i32 direction, cex_iterator_s* iterator);

    // clang-format on
};
extern const struct __module__deque deque; // CEX Autogen
