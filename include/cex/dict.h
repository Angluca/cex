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


#define dict$define(eltype)                                                                        \
    union                                                                                          \
    {                                                                                              \
        dict_c base;                                                                               \
        typeof(eltype)* const _dtype; /* virtual field, only for type checks, const pointer */     \
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
        _Static_assert(                                                                            \
            offsetof(typeof(*((self)->_dtype)), key) == 0,                                         \
            "dict _dtype struct must have `key` member at first place"                             \
        );                                                                                         \
        dict_new_kwargs_s kwargs = { __VA_ARGS__ };                                                \
        if (kwargs.cmp_func == NULL) {                                                             \
            kwargs.cmp_func = _dict$cmpfunc_field(((typeof(*((self)->_dtype))){ 0 }).key);         \
        }                                                                                          \
        if (kwargs.hash_func == NULL) {                                                            \
            kwargs.hash_func = _dict$hashfunc_field(((typeof(*((self)->_dtype))){ 0 }).key);       \
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

#define dict$get(self, dictkey)                                                                    \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic(((self)), dict_c *: 0, default: 1),                                           \
            "self argument expected to be dict$define(eltype) 1"                                   \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), dict_c *: 1, default: 0),                                    \
            "self argument expected to be dict$define(eltype)"                                     \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic(/* Validate if `dictkey` match dict.key type */                               \
                     (&(((typeof(*((self)->_dtype))){ 0 }).key)),                                  \
                u64 *: _Generic(/* STANDARD u64 keys, we may pass literals and type of _dtype */   \
                                (dictkey),                                                         \
                        i8: 1,                                                                     \
                        u8: 1,                                                                     \
                        i16: 1,                                                                    \
                        u16: 1,                                                                    \
                        i32: 1,                                                                    \
                        u32: 1,                                                                    \
                        u64: 1,                                                                    \
                        i64: 1,                                                                    \
                        typeof((*(self)->_dtype))*: 1, /* only full record type*/                  \
                        default: 0                     /* assert failure */                        \
                     ),                                                                            \
                char(*)[]: _Generic(/* STANDARD char array keys */                                 \
                                    (dictkey),                                                     \
                    const char*: 1,                                                                \
                    char*: 1,                                                                      \
                    typeof((*(self)->_dtype))*: 1, /* only full record type*/                      \
                    default: 0                     /* assert failure */                            \
                ),                                                                                 \
                default: _Generic(/* CUSTOM keys must be exact key type or full record pointer*/   \
                                  (dictkey),                                                       \
                    typeof((*(self)->_dtype))*: 1,                    /* only full record type*/   \
                    typeof(((typeof(*(self)->_dtype)){ 0 }).key)*: 1, /* only full key type */     \
                    default: 0                                        /* assert failure */         \
                )                                                                                  \
            ),                                                                                     \
            "dict$get() `dictkey` arg and expected dict `key` field type mismatch"                 \
        );                                                                                         \
        /* clang-format off */ \
        (typeof(*((self)->_dtype))* /* return as dict type */ )_Generic(                           \
            /* NOTE: picking appropriate function based on dictkey type (supporting literals) */   \
            (dictkey), \
            /* number literals */ i8: dict.geti, u8: dict.geti,i16: dict.geti, u16: dict.geti, i32: dict.geti, u32: dict.geti, i64: dict.geti, u64: dict.geti, \
            /* all other cases */ default: dict.get                           \
         )(&((self)->base), (dictkey));                                                                   \
        /* clang-format on */                                                                      \
    })

#define dict$del(self, dictkey)                                                                    \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic(((self)), dict_c *: 0, default: 1),                                           \
            "self argument expected to be dict$define(eltype) 1"                                   \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), dict_c *: 1, default: 0),                                    \
            "self argument expected to be dict$define(eltype)"                                     \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic(/* Validate if `dictkey` match dict.key type */                               \
                     (&(((typeof(*((self)->_dtype))){ 0 }).key)),                                  \
                u64 *: _Generic(/* STANDARD u64 keys, we may pass literals and type of _dtype */   \
                                (dictkey),                                                         \
                        i8: 1,                                                                     \
                        u8: 1,                                                                     \
                        i16: 1,                                                                    \
                        u16: 1,                                                                    \
                        i32: 1,                                                                    \
                        u32: 1,                                                                    \
                        u64: 1,                                                                    \
                        i64: 1,                                                                    \
                        typeof((*(self)->_dtype))*: 1, /* only full record type*/                  \
                        default: 0                     /* assert failure */                        \
                     ),                                                                            \
                char(*)[]: _Generic(/* STANDARD char array keys */                                 \
                                    (dictkey),                                                     \
                    const char*: 1,                                                                \
                    char*: 1,                                                                      \
                    typeof((*(self)->_dtype))*: 1, /* only full record type*/                      \
                    default: 0                     /* assert failure */                            \
                ),                                                                                 \
                default: _Generic(/* CUSTOM keys must be exact key type or full record pointer*/   \
                                  (dictkey),                                                       \
                    typeof((*(self)->_dtype))*: 1,                    /* only full record type*/   \
                    typeof(((typeof(*(self)->_dtype)){ 0 }).key)*: 1, /* only full key type */     \
                    default: 0                                        /* assert failure */         \
                )                                                                                  \
            ),                                                                                     \
            "dict$get() `dictkey` arg and expected dict `key` field type mismatch"                 \
        );                                                                                         \
        /* clang-format off */ \
        (typeof(*((self)->_dtype))* /* return as dict type */ )_Generic(                           \
            /* NOTE: picking appropriate function based on dictkey type (supporting literals) */   \
            (dictkey), \
            /* number literals */ i8: dict.deli, u8: dict.deli,i16: dict.deli, u16: dict.deli, i32: dict.deli, u32: dict.deli, i64: dict.deli, u64: dict.deli, \
            /* all other cases */ default: dict.del                           \
         )(&((self)->base), (dictkey));                                                                   \
        /* clang-format on */                                                                      \
    })


#define dict$destroy(self)                                                                         \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic(((self)), dict_c *: 0, default: 1),                                           \
            "self argument expected to be dict$define(eltype) 1"                                   \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), dict_c *: 1, default: 0),                                    \
            "self argument expected to be dict$define(eltype)"                                     \
        );                                                                                         \
        dict.destroy(&((self)->base));                                                  \
    })

#define dict$iter(self, it)                                                                  \
    ({                                                                                             \
        _Static_assert(                                                                            \
            _Generic(((self)), dict_c *: 0, default: 1),                                           \
            "self argument expected to be dict$define(eltype) 1"                                   \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((&(self)->base), dict_c *: 1, default: 0),                                    \
            "self argument xpected to be dict$define(eltype)"                                      \
        );                                                                                         \
        _Static_assert(                                                                            \
            _Generic((it), cex_iterator_s *: 1, default: 0),                                    \
            "expected cex_iterator_s pointer, i.e. `&it.iterator`"                                      \
        );                                                                                         \
        (typeof(((self)->_dtype)))dict.iter(&((self)->base), (it));                          \
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
