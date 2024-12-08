#pragma once
#include "cex.h"
#include <stddef.h>
#include <string.h>

typedef struct dict_c
{
    void* hashmap; // any generic hashmap implementation
} dict_c;

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
#define _dict$hashfunc_field(strucfield)                                                           \
    _Generic(                                                                                      \
        &(strucfield),                                                                             \
        const u64*: dict.hashfunc.u64_hash,                                                        \
        u64*: dict.hashfunc.u64_hash,                                                              \
        char(*)[]: dict.hashfunc.str_hash,                                                         \
        const char(*)[]: dict.hashfunc.str_hash,                                                   \
        default: NULL /* This will force dict.create() to raise assert */                          \
    )

#define _dict$cmpfunc_field(strucfield)                                                            \
    _Generic(                                                                                      \
        &(strucfield),                                                                             \
        const u64*: dict.hashfunc.u64_cmp,                                                         \
        u64*: dict.hashfunc.u64_cmp,                                                               \
        char(*)[]: dict.hashfunc.str_cmp,                                                          \
        const char(*)[]: dict.hashfunc.str_cmp,                                                    \
        default: NULL /* This will force dict.create() to raise assert */                          \
    )

#define _dict$hashfunc(struct, field) _dict$hashfunc_field(((struct){ 0 }.field))

#define _dict$cmpfunc(struct, field) _dict$cmpfunc_field(((struct){ 0 }.field))


#define dict$define(eltype, key_field_name)                                                        \
    union                                                                                          \
    {                                                                                              \
        dict_c base;                                                                               \
        typeof(eltype) const* const _dtype; /* virtual field, only for type checks*/               \
        typeof(((eltype){ 0 }.key_field_name)) const* const _ktype; /* virtual field */            \
    }

#define dict$new(self, allocator, ...)                                                             \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic(((self)), dict_c *: 0, default: 1),                                           \
            "self argument expected to be dict$define()"                                           \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), dict_c *: 1, default: 0),                                    \
            "self argument expected to be dict$define()"                                           \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Alignof(typeof(*((self)->_dtype))) <= _Alignof(void*),                                \
            "dict item alignment must be not greater than generic pointer"                         \
        );                                                                                         \
        dict_new_kwargs_s kwargs = { __VA_ARGS__ };                                                \
        if (kwargs.cmp_func == NULL) {                                                             \
            kwargs.cmp_func = _dict$cmpfunc_field(*((self)->_ktype));                              \
        }                                                                                          \
        if (kwargs.hash_func == NULL) {                                                            \
            kwargs.hash_func = _dict$hashfunc_field(*((self)->_ktype));                            \
        }                                                                                          \
        dict.create(&((self)->base), sizeof(*((self)->_dtype)), allocator, &kwargs);               \
    })


#define dict$set(self, ...)                                                                        \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic(((self)), dict_c *: 0, default: 1),                                           \
            "self argument expected to be dict$define()"                                           \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), dict_c *: 1, default: 0),                                    \
            "self argument xpected to be dict$define()"                                            \
        );                                                                                         \
        /*__VA_ARGS__ to support inline struct initialization &(struct s){.key = 1}*/              \
        typeof(((self)->_dtype)) _value = (__VA_ARGS__); /*dict type safety check */               \
        dict.set(&((self)->base), _value);                                                         \
    })

#define dict$get(self, key)                                                                        \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic(((self)), dict_c *: 0, default: 1),                                           \
            "self argument expected to be dict$define(eltype) 1"                                   \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), dict_c *: 1, default: 0),                                    \
            "self argument xpected to be dict$define(eltype)"                                      \
        );                                                                                         \
        /* NOTE: init only key's type variable, instead of full struct to limit memory usage */    \
        typeof(*((self)->_ktype)) _key = (key); /* dict key type safety check */                   \
        (typeof(((self)->_dtype)))dict.get(&((self)->base), &_key);                                \
    })

struct __module__dict
{
    // Autogenerated by CEX
    // clang-format off


struct {  // sub-module .hashfunc >>>
    /**
     * @brief Hashmap: int compare function
     *
     * @param a as u64
     * @param b as u64
     * @param udata (unused)
     * @return
     */
    int
    (*u64_cmp)(const void* a, const void* b, void* udata);

    /**
     * @brief Hashmap: int hash function, for making ticker_id -> hash (uniformly distrib)
     *
     * @param item (u64) item (ticker id)
     * @param seed0 (unused)
     * @param seed1 (unused)
     * @return hash value
     */
    u64
    (*u64_hash)(const void* item, u64 seed0, u64 seed1);

    /**
     * @brief Compares static char[] buffer keys **must be null terminated**
     *
     * @param a  char[N] string
     * @param b  char[N] string
     * @param udata  (unused)
     * @return compared int value
     */
    int
    (*str_cmp)(const void* a, const void* b, void* udata);

    /**
     * @brief Compares static char[] buffer keys **must be null terminated**
     *
     * @param item
     * @param seed0
     * @param seed1
     * @return
     */
    u64
    (*str_hash)(const void* item, u64 seed0, u64 seed1);

} hashfunc;  // sub-module .hashfunc <<<
Exception
(*create)(dict_c* self, usize item_size, const Allocator_i* allocator, dict_new_kwargs_s* kwargs);

/**
 * @brief Set or replace dict item
 *
 * @param self dict() instance
 * @param item  item key/value struct
 * @return error code, EOK (0!) on success, positive on failure
 */
Exception
(*set)(dict_c* self, const void* item);

/**
 * @brief Get item by integer key
 *
 * @param self dict() instance
 * @param key u64 key
 */
void*
(*geti)(dict_c* self, u64 key);

/**
 * @brief Get item by generic key pointer (including strings)
 *
 * @param self dict() instance
 * @param key generic pointer key
 */
void*
(*get)(dict_c* self, const void* key);

/**
 * @brief Number elements in dict()
 *
 * @param self  dict() instance
 * @return number
 */
usize
(*len)(dict_c* self);

/**
 * @brief Free dict() instance
 *
 * @param self  dict() instance
 * @return always NULL
 */
void
(*destroy)(dict_c* self);

/**
 * @brief Clear all elements in dict (but allocated capacity unchanged)
 *
 * @param self dict() instane
 */
void
(*clear)(dict_c* self);

/**
 * @brief Delete item by integer key
 *
 * @param self dict() instance
 * @param key u64 key
 */
void*
(*deli)(dict_c* self, u64 key);

/**
 * @brief Delete item by generic key pointer (including strings)
 *
 * @param self dict() instance
 * @param key generic pointer key
 */
void*
(*del)(dict_c* self, const void* key);

void*
(*iter)(dict_c* self, cex_iterator_s* iterator);

Exception
(*tolist)(dict_c* self, void* listptr, const Allocator_i* allocator);

    // clang-format on
};
extern const struct __module__dict dict; // CEX Autogen
