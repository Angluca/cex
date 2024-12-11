#pragma once
#include "cex.h"


/**
 * @brief Dynamic array (list) implementation
 */
typedef struct
{
    // NOTE: do not assign this pointer to local variables, is can become dangling after list
    // operations (e.g. after realloc()). So local pointers can be pointing to invalid area!
    void* arr;
    usize len;
} list_c;

#define list$define(eltype)                                                                        \
    /* NOTE: shadow struct the same as list_c, only used for type safety. const  prevents user to  \
     * overwrite struct arr.arr pointer (elements are ok), and also arr.count */                   \
    union                                                                                          \
    {                                                                                              \
        struct                                                                                     \
        {                                                                                          \
            typeof(eltype)* const arr;                                                             \
            const usize len;                                                                       \
        };                                                                                         \
        list_c base;                                                                               \
    }

#define _list$typedef_extern_0(typename) extern const struct typename##_vtable__ typename
#define _list$typedef_extern_1(typename)
#define _list$typedef_extern(typename, implement) _list$typedef_extern_##implement(typename)

#define _list$typedef_impl_0(typename)
#define _list$typedef_impl_1(typename)                                                             \
    const struct typename##_vtable__ typename = { .append = (void*)list.append,                    \
                                                  .destroy = (void*)list.destroy,                  \
                                                  .insert = (void*)list.insert,\
                                                  .capacity = (void*)list.capacity,\
                                                  .clear = (void*)list.clear,\
                                                  .extend = (void*)list.extend,\
                                                  .del = (void*)list.del,\
                                                  .sort = (void*)list.sort,\
                                                  .len = (void*)list.len,\
                                                  .iter = (void*)list.iter };
#define _list$typedef_impl(typename, implement) _list$typedef_impl_##implement(typename)

#define list$impl(typename) _list$typedef_impl(typename, 1)

#define list$typedef(typename, eltype, implement)                                                  \
    typedef struct typename##_c typename##_c;                                                      \
    struct typename##_vtable__                                                                     \
    {                                                                                              \
        Exception (*append)(typename##_c * self, eltype * item);                                   \
        Exception (*insert)(typename##_c * self, eltype* item, usize index);                         \
        Exception (*extend)(typename##_c * self, eltype* items, usize nitems);                       \
        Exception (*del)(typename##_c * self, usize index);                                        \
        void (*clear)(typename##_c * self);                                                        \
        usize (*len)(typename##_c * self);                                                         \
        usize (*capacity)(typename##_c * self);                                                    \
        void (*sort)(typename##_c * self, int (*comp)(const eltype*, const eltype*));                  \
        eltype* (*iter)(typename##_c * self, cex_iterator_s * iterator);                           \
        void (*destroy)(typename##_c * self);                                                      \
    };                                                                                             \
    _list$typedef_extern(typename, implement);                                                     \
    _list$typedef_impl(typename, implement);                                                       \
    typedef struct typename##_c                                                                    \
    {                                                                                              \
        union                                                                                      \
        {                                                                                          \
            list_c base;                                                                           \
            struct                                                                                 \
            {                                                                                      \
                eltype* arr;                                                                       \
                usize len;                                                                         \
            };                                                                                     \
        };                                                                                         \
    }                                                                                              \
    typename##_c;


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
} __attribute__((packed)) list_head_s;

#define _CEX_LIST_BUF 32
// _Static_assert(sizeof(list_head_s) == 32, "size"); // platform dependent
_Static_assert(sizeof(list_head_s) <= _CEX_LIST_BUF, "size");
_Static_assert(alignof(list_head_s) == 1, "align");

#define list$define_static_buf(var_name, eltype, capacity)                                         \
    alignas(32) char var_name[_CEX_LIST_BUF + sizeof(eltype) * capacity] = { 0 };                  \
    _Static_assert(alignof(eltype) <= 32, "list$define_static_buf alignment is too high");         \
    _Static_assert(capacity > 0, "list$define_static_buf zero capacity")

#define list$new(list_c_ptr, capacity, allocator)                                                  \
    (list.create(                                                                                  \
        &((list_c_ptr)->base),                                                                     \
        capacity,                                                                                  \
        sizeof(typeof(*(((list_c_ptr))->arr))),                                                    \
        alignof(typeof(*(((list_c_ptr))->arr))),                                                   \
        allocator                                                                                  \
    ))

#define list$new_static(list_c_ptr, buf, buf_len)                                                  \
    (list.create_static(                                                                           \
        &((list_c_ptr)->base),                                                                     \
        buf,                                                                                       \
        buf_len,                                                                                   \
        sizeof(typeof(*(((list_c_ptr))->arr))),                                                    \
        alignof(typeof(*(((list_c_ptr))->arr)))                                                    \
    ))

#define _list$head(self) ((list_head_s*)((char*)self->arr - _CEX_LIST_BUF))

#define list$cast(list_base_ptr, cast_to_list)                                                     \
    (void*)(&((list_base_ptr)->arr));                                                              \
    _Static_assert(                                                                                \
        _Generic(list_base_ptr, list_c *: 1, default: 0),                                          \
        "first argument expected list_c* pointer (or try .base ) "                                 \
    );                                                                                             \
    _Static_assert(                                                                                \
        _Generic((&(cast_to_list)->base), list_c *: 1, default: 0),                                \
        "second argument expected to be derivative i.e. list$define()"                             \
    );                                                                                             \
    uassertf(                                                                                      \
        (_list$head((list_base_ptr)))->header.elalign ==                                           \
            alignof(typeof(*(((cast_to_list))->arr))),                                             \
        "list$cast: element align mismatch, incompatible types?"                                   \
    );                                                                                             \
    uassertf(                                                                                      \
        (_list$head((list_base_ptr)))->header.elsize == sizeof(typeof(*(((cast_to_list))->arr))),  \
        "list$cast: element size mismatch, incompatible types?"                                    \
    )

#define _CEX_TYPE_MATCH(X, Y)                                                                      \
    _Generic((Y), __typeof__(X): _Generic((X), __typeof__(Y): 1, default: 0), default: 0)


#define list$insert(self, item, index)                                                             \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        _Static_assert(                                                                            \
            _CEX_TYPE_MATCH(*(self)->arr, *item),                                                  \
            "list$insert incompatible item type, or double pointer passed"                         \
        );                                                                                         \
        list.insert(&(self)->base, (item), (index));                                               \
    })

#define list$del(self, index)                                                                      \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        list.del(&(self)->base, (index));                                                          \
    })

#define list$sort(self, comp)                                                                      \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        list.sort(&(self)->base, (comp));                                                          \
    })

#define list$append(self, item)                                                                    \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        _Static_assert(                                                                            \
            _CEX_TYPE_MATCH(*(self)->arr, *item),                                                  \
            "list$append incompatible item type, or double pointer passed"                         \
        );                                                                                         \
        list.append(&(self)->base, (item));                                                        \
    })

#define list$clear(self)                                                                           \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        list.clear(&(self)->base);                                                                 \
    })

#define list$extend(self, items, nitems)                                                           \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        _Static_assert(                                                                            \
            _CEX_TYPE_MATCH(*(self)->arr, *items),                                                 \
            "list$append incompatible item type, or double pointer passed"                         \
        );                                                                                         \
        list.extend(&(self)->base, (items), nitems);                                               \
    })

#define list$len(self)                                                                             \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, const list_c*: 1, default: 0),                  \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        (self)->len; /* NOTE: using direct access instead to list.len, for performance */          \
    })

#define list$capacity(self)                                                                        \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        list.capacity(&(self)->base);                                                              \
    })

#define list$destroy(self)                                                                         \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        list.destroy(&(self)->base);                                                               \
    })

#define list$iter(self, iterator)                                                                  \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((iterator), cex_iterator_s *: 1, default: 0),                                 \
            "expected cex_iterator_s pointer, i.e. `&it.iterator`"                                 \
        );                                                                                         \
        (typeof((self)->arr))list.iter(&(self)->base, iterator);                                   \
    })

#define list$slice(self, start, end)                                                               \
    ({                                                                                             \
        uassert(self != NULL && "list$slice");                                                     \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), list_c *: 1, default: 0),                                    \
            "self argument expected to be derivative i.e. list$define()"                           \
        );                                                                                         \
        slice$define(*((self)->arr)) s = { .arr = NULL, .len = 0 };                                \
        _arr$slice_get(s, (self)->arr, (self)->len, start, end);                                   \
        s;                                                                                         \
    })

struct __module__list
{
    // Autogenerated by CEX
    // clang-format off

Exception
(*create)(list_c* self, usize capacity, usize elsize, usize elalign, const Allocator_i* allocator);

Exception
(*create_static)(list_c* self, void* buf, usize buf_len, usize elsize, usize elalign);

Exception
(*insert)(list_c* self, void* item, usize index);

Exception
(*del)(list_c* self, usize index);

void
(*sort)(list_c* self, int (*comp)(const void*, const void*));

Exception
(*append)(list_c* self, void* item);

void
(*clear)(list_c* self);

Exception
(*extend)(list_c* self, void* items, usize nitems);

usize
(*len)(list_c const * self);

usize
(*capacity)(list_c const * self);

void
(*destroy)(list_c* self);

void*
(*iter)(list_c const * self, cex_iterator_s* iterator);

    // clang-format on
};
extern const struct __module__list list; // CEX Autogen
