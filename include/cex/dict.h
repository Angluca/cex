#pragma once
#include "cex.h"
#include <stddef.h>
#include <string.h>

typedef u64 (*dict_hash_func_f)(const void* item, u64 seed0, u64 seed1);
typedef int (*dict_compare_func_f)(const void* a, const void* b, void* udata);
typedef void (*dict_elfree_func_f)(void* item);

typedef struct dict_new_kwargs_s
{
    usize capacity;
    dict_hash_func_f hash_func;
    dict_compare_func_f cmp_func;
    dict_elfree_func_f elfree_func;
    void* cmp_func_udata;
    u64 seed0;
    u64 seed1;
} dict_new_kwargs_s;


// Hack for getting hash/cmp functions by a type of key field
// https://gustedt.wordpress.com/2015/05/11/the-controlling-expression-of-_generic/
#define _dict$hashfunc(strucfield)                                                                 \
    _Generic(                                                                                      \
        &(strucfield),                                                                             \
        const u64*: _cex_dict_u64_hash,                                                            \
        u64*: _cex_dict_u64_hash,                                                                  \
        char(*)[]: _cex_dict_str_hash,                                                             \
        const char(*)[]: _cex_dict_str_hash,                                                       \
        default: NULL /* This will force dict.create() to raise assert */                          \
    )

#define _dict$cmpfunc(strucfield)                                                                  \
    _Generic(                                                                                      \
        &(strucfield),                                                                             \
        const u64*: _cex_dict_u64_cmp,                                                             \
        u64*: _cex_dict_u64_cmp,                                                                   \
        char(*)[]: _cex_dict_str_cmp,                                                              \
        const char(*)[]: _cex_dict_str_cmp,                                                        \
        default: NULL /* This will force dict.create() to raise assert */                          \
    )

#define _dict$typedef_extern_0(typename) extern const struct typename##_vtable__ typename
#define _dict$typedef_extern_1(typename)
#define _dict$typedef_extern(typename, implement) _dict$typedef_extern_##implement(typename)


#define _dict$typedef_impl_0(typename, keytype)
#define _dict$typedef_impl_1(typename, keytype)                                                                               \
    _Static_assert(/* Validate if `keytype` match <typename>_c.key type */                                                    \
                   _Generic(                                                                                                  \
                       (&(typename##_c){ 0 }._dtype->key),                                                                    \
                       u64 *: _Generic((keytype){ 0 }, u64: 1, default: 0),                                                   \
                       char(*)[]: _Generic((keytype){ 0 }, char*: 1, default: 0),                                             \
                       default: _Generic(                                                                                     \
                           (keytype){ 0 },                                                                                    \
                           typeof(((typename##_c){ 0 }._dtype->key))*: 1, /* expected keytype* */                             \
                           default: 0                                                                                         \
                       )                                                                                                      \
                   ),                                                                                                         \
                   "dict$impl() `keytype` arg does not match <typename>_c.key (check types, keytype must be u64 or keytype*)" \
    );                                                                                                                        \
    const struct typename##_vtable__ typename = {                                                                             \
        .set = (void*)_cex_dict_set,                                                                                          \
        .get = (void*)(_Generic((keytype){ 0 }, u64: _cex_dict_geti, default: _cex_dict_get)),                                \
        .del = (void*)(_Generic((keytype){ 0 }, u64: _cex_dict_deli, default: _cex_dict_del)),                                \
        .len = (void*)_cex_dict_len,                                                                                          \
        .iter = (void*)_cex_dict_iter,                                                                                        \
        .destroy = (void*)_cex_dict_destroy,                                                                                  \
        .clear = (void*)_cex_dict_clear,                                                                                      \
    };

#define _dict$typedef_impl(typename, keytype, implement)                                           \
    _dict$typedef_impl_##implement(typename, keytype)

typedef struct _cex_dict_c
{
    void* hashmap; // any generic hashmap implementation
    const Allocator_i* allc;
} _cex_dict_c;

#define dict$impl(typename, keytype) _dict$typedef_impl(typename, keytype, 1)
#define dict$typedef(typename, eltype, keytype, implement)                                         \
    typedef struct typename##_c                                                                    \
    {                                                                                              \
        union                                                                                      \
        {                                                                                          \
            _cex_dict_c base;                                                                      \
            eltype* const _dtype; /* virtual field, only for type checks, const pointer */         \
        };                                                                                         \
    }                                                                                              \
    typename##_c;                                                                                  \
    struct typename##_vtable__                                                                     \
    {                                                                                              \
        Exception (*set)(typename##_c * self, const eltype* item);                                 \
        eltype* (*get)(typename##_c * self, const keytype item);                                   \
        usize (*len)(typename##_c * self);                                                         \
        eltype* (*del)(typename##_c * self, const keytype item);                                   \
        void (*clear)(typename##_c * self);                                                        \
        eltype* (*iter)(typename##_c * self, cex_iterator_s * iterator);                           \
        void (*destroy)(typename##_c * self);                                                      \
    };                                                                                             \
    _dict$typedef_extern(typename, implement);                                                     \
    _dict$typedef_impl(typename, keytype, implement);

#define dict$new(self, allocator, ...)                                                             \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic(((self)), _cex_dict_c *: 0, default: 1),                                      \
            "self argument expected to be dict$typedef() class"                                           \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), _cex_dict_c *: 1, default: 0),                               \
            "self argument expected to be dict$typedef() class"                                           \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Alignof(typeof(*((self)->_dtype))) <= _Alignof(void*),                                \
            "dict item alignment must be not greater than generic pointer"                         \
        );                                                                                         \
        _Static_assert(                                                                            \
            offsetof(typeof(*((self)->_dtype)), key) == 0,                                         \
            "dict _dtype struct must have `key` member at first place"                             \
        );                                                                                         \
        dict_new_kwargs_s kwargs = { __VA_ARGS__ };                                                \
        if (kwargs.cmp_func == NULL) {                                                             \
            kwargs.cmp_func = _dict$cmpfunc(((typeof(*((self)->_dtype))){ 0 }).key);               \
        }                                                                                          \
        if (kwargs.hash_func == NULL) {                                                            \
            kwargs.hash_func = _dict$hashfunc(((typeof(*((self)->_dtype))){ 0 }).key);             \
        }                                                                                          \
        _cex_dict_create(&((self)->base), sizeof(*((self)->_dtype)), allocator, &kwargs);          \
    })


// clang-format off
int _cex_dict_u64_cmp(const void* a, const void* b, void* udata);
u64 _cex_dict_u64_hash(const void* item, u64 seed0, u64 seed1);
int _cex_dict_str_cmp(const void* a, const void* b, void* udata);
u64 _cex_dict_str_hash(const void* item, u64 seed0, u64 seed1);
Exception _cex_dict_create(_cex_dict_c* self, usize item_size, const Allocator_i* allocator, dict_new_kwargs_s* kwargs);
Exception _cex_dict_set(_cex_dict_c* self, const void* item);
void* _cex_dict_geti(_cex_dict_c* self, u64 key);
void* _cex_dict_get(_cex_dict_c* self, const void* key);
usize _cex_dict_len(_cex_dict_c* self);
void _cex_dict_destroy(_cex_dict_c* self);
void _cex__cex_dict_clear(_cex_dict_c* self);
void* _cex_dict_deli(_cex_dict_c* self, u64 key);
void* _cex_dict_del(_cex_dict_c* self, const void* key);
void* _cex_dict_iter(_cex_dict_c* self, cex_iterator_s* iterator);
// clang-format on
