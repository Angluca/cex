#pragma once
#include "cex.h"


/**
 * @brief Dynamic array (list) implementation
 */


#define _list$typedef_extern_0(typename) extern const struct typename##_vtable__ typename
#define _list$typedef_extern_1(typename)
#define _list$typedef_extern(typename, implement) _list$typedef_extern_##implement(typename)

#define _list$typedef_impl_0(typename)
#define _list$typedef_impl_1(typename)                                                             \
    const struct typename##_vtable__ typename = { .append = (void*)_cex_list_append,               \
                                                  .destroy = (void*)_cex_list_destroy,             \
                                                  .insert = (void*)_cex_list_insert,               \
                                                  .capacity = (void*)_cex_list_capacity,           \
                                                  .clear = (void*)_cex_list_clear,                 \
                                                  .extend = (void*)_cex_list_extend,               \
                                                  .del = (void*)_cex_list_del,                     \
                                                  .sort = (void*)_cex_list_sort,                   \
                                                  .iter = (void*)_cex_list_iter };
#define _list$typedef_impl(typename, implement) _list$typedef_impl_##implement(typename)

#define list$impl(typename) _list$typedef_impl(typename, 1)

typedef struct
{
    // NOTE: do not assign this pointer to local variables, is can become dangling after list
    // operations (e.g. after realloc()). So local pointers can be pointing to invalid area!
    void* arr;
    usize len;
} _cex_list_c;

#define list$typedef(typename, eltype, implement)                                                  \
    typedef struct typename##_c typename##_c;                                                      \
    struct typename##_vtable__                                                                     \
    {                                                                                              \
        Exception (*append)(typename##_c * self, eltype * item);                                   \
        Exception (*insert)(typename##_c * self, eltype * item, usize index);                      \
        Exception (*extend)(typename##_c * self, eltype * items, usize nitems);                    \
        Exception (*del)(typename##_c * self, usize index);                                        \
        void (*clear)(typename##_c * self);                                                        \
        usize (*capacity)(typename##_c * self);                                                    \
        void (*sort)(typename##_c * self, int (*comp)(const eltype*, const eltype*));              \
        eltype* (*iter)(typename##_c * self, cex_iterator_s * iterator);                           \
        void (*destroy)(typename##_c * self);                                                      \
    };                                                                                             \
    _list$typedef_extern(typename, implement);                                                     \
    _list$typedef_impl(typename, implement);                                                       \
    typedef struct typename##_c                                                                    \
    {                                                                                              \
        union                                                                                      \
        {                                                                                          \
            _cex_list_c base;                                                                      \
            struct                                                                                 \
            {                                                                                      \
                eltype* arr;                                                                       \
                usize len;                                                                         \
            };                                                                                     \
        };                                                                                         \
    }                                                                                              \
    typename##_c;

#define _CEX_LIST_BUF 32

typedef struct
{
    struct
    {
        u16 magic;
        u16 elsize;
        u16 elalign;
    } header;
    usize count;
    usize capacity;
    const Allocator_i* allocator;
    // NOTE: we must use packed struct, because elements of the list my have absolutely different
    // alignment, so the list_head_s takes place at (void*)1st_el - sizeof(list_head_s)
} __attribute__((packed)) _cex_list_head_s;
// _Static_assert(sizeof(_cex_list_head_s) == 30, "size"); // platform dependent
_Static_assert(sizeof(_cex_list_head_s) <= _CEX_LIST_BUF, "size");
_Static_assert(alignof(_cex_list_head_s) == 1, "align");

#define list$define_static_buf(var_name, listtype, capacity)                                       \
    alignas(alignof(typeof(*((((listtype##_c){ 0 }).arr))))                                        \
    ) char var_name[_CEX_LIST_BUF + sizeof(typeof(*(((listtype##_c){ 0 }).arr))) * (capacity)] = { \
        0                                                                                          \
    };                                                                                             \
    _Static_assert((capacity) > 0, "list$define_static_buf zero capacity");

/*
    _Static_assert(                                                                                \
        _Generic((&(listtype)->base), _cex_list_c *: 1, default: 0),                               \
        "self argument expected to be list$typedef() class"                               \
    );
*/

#define list$new(self, capacity, allocator)                                                        \
    (_cex_list_create(                                                                             \
        &((self)->base),                                                                           \
        capacity,                                                                                  \
        sizeof(typeof(*(((self))->arr))),                                                          \
        alignof(typeof(*(((self))->arr))),                                                         \
        allocator                                                                                  \
    ))

#define list$new_static(self, buf, buf_len)                                                        \
    (_cex_list_create_static(                                                                      \
        &((self)->base),                                                                           \
        buf,                                                                                       \
        buf_len,                                                                                   \
        sizeof(typeof(*(((self))->arr))),                                                          \
        alignof(typeof(*(((self))->arr)))                                                          \
    ))

#define _list$head(self) ((_cex_list_head_s*)((char*)self->arr - _CEX_LIST_BUF))

// TODO: decide how to reconcile it with generic lists
#define list$slice(self, start, end)                                                               \
    ({                                                                                             \
        uassert(self != NULL && "list$slice");                                                     \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), _cex_list_c *: 1, default: 0),                               \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        slice$define(*((self)->arr)) s = { .arr = NULL, .len = 0 };                                \
        _arr$slice_get(s, (self)->arr, (self)->len, start, end);                                   \
        s;                                                                                         \
    })

// clang-format off
Exception _cex_list_create(_cex_list_c* self, usize capacity, usize elsize, usize elalign, const Allocator_i* allocator);
Exception _cex_list_create_static(_cex_list_c* self, void* buf, usize buf_len, usize elsize, usize elalign);
Exception _cex_list_insert(_cex_list_c* self, void* item, usize index);
Exception _cex_list_del(_cex_list_c* self, usize index);
void _cex_list_sort(_cex_list_c* self, int (*comp)(const void*, const void*));
Exception _cex_list_append(_cex_list_c* self, void* item);
void _cex_list_clear(_cex_list_c* self);
Exception _cex_list_extend(_cex_list_c* self, void* items, usize nitems);
usize _cex_list_capacity(_cex_list_c const* self);
void _cex_list_destroy(_cex_list_c* self);
void* _cex_list_iter(_cex_list_c const* self, cex_iterator_s* iterator);
// clang-format on
