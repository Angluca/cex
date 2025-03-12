#ifndef INCLUDE_STB_DS_H
#define INCLUDE_STB_DS_H

#include "cex.h"
#include "mem.h"
#include <stddef.h>
#include <string.h>

// for security against attackers, seed the library with a random number, at least time() but
// stronger is better
extern void cexds_rand_seed(size_t seed);

// these are the hash functions used internally if you want to test them or use them for other
// purposes
extern size_t cexds_hash_bytes(void* p, size_t len, size_t seed);
extern size_t cexds_hash_string(char* str, size_t seed);

// this is a simple string arena allocator, initialize with e.g. 'cexds_string_arena my_arena={0}'.
typedef struct cexds_string_arena cexds_string_arena;
extern char* cexds_stralloc(cexds_string_arena* a, char* str);
extern void cexds_strreset(cexds_string_arena* a);

///////////////
//
// Everything below here is implementation details
//

struct cexds_hm_new_kwargs_s
{
    usize capacity;
    size_t seed;
    bool copy_keys;
};
struct cexds_arr_new_kwargs_s
{
    usize capacity;
};

// clang-format off
extern void* cexds_arrgrowf(void* a, size_t elemsize, size_t addlen, size_t min_cap, const Allocator_i* allc);
extern void cexds_arrfreef(void* a);
extern void cexds_hmfree_func(void* p, size_t elemsize);
extern void* cexds_hminit(size_t elemsize, const Allocator_i* allc, struct cexds_hm_new_kwargs_s* kwargs);
extern void* cexds_hmget_key(void* a, size_t elemsize, void* key, size_t keysize, int mode);
extern void* cexds_hmget_key_ts(void* a, size_t elemsize, void* key, size_t keysize, ptrdiff_t* temp, int mode);
extern void* cexds_hmput_default(void* a, size_t elemsize);
extern void* cexds_hmput_key(void* a, size_t elemsize, void* key, size_t keysize, int mode);
extern void* cexds_hmdel_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset, int mode);
extern void* cexds_shmode_func(size_t elemsize, int mode);
// clang-format on

#define CEXDS_ARR_MAGIC 0xC001DAAD
#define CEXDS_HM_MAGIC 0xF001C001
#define CEXDS_HDR_PAD 64
_Static_assert(mem$is_power_of2(CEXDS_HDR_PAD), "expected pow of 2");


// cexds array alignment
// v malloc'd pointer              v-element 1
// |..... <cexds_array_header>|====!====!====
//    ^ padding      ^cap^len ^         ^-element 2
//                            ^-- arr$ user space pointer (element 0)
//
typedef struct
{
    void* hash_table;
    ptrdiff_t temp;
    const Allocator_i* allocator;
    u32 magic_num;
    size_t capacity;
    size_t length; // This MUST BE LAST
} cexds_array_header;
_Static_assert(alignof(cexds_array_header) == alignof(void*), "align");
_Static_assert(sizeof(cexds_array_header) <= CEXDS_HDR_PAD, "size too high");
_Static_assert(sizeof(cexds_array_header) % alignof(size_t) == 0, "align size");

_Static_assert(sizeof(cexds_array_header) == 48, "size");


#define cexds_header(t) ((cexds_array_header*)(((char*)(t)) - sizeof(cexds_array_header)))
#define cexds_base(t) ((char*)(t) - CEXDS_HDR_PAD)
#define cexds_temp(t) cexds_header(t)->temp
#define cexds_temp_key(t) (*(char**)cexds_header(t)->hash_table)

#define arr$(T) T*
#define arr$new(a, allocator, kwargs...)                                                           \
    ({                                                                                             \
        _Static_assert(_Alignof(typeof(*a)) <= 64, "array item alignment too high");               \
        uassert(allocator != NULL);                                                                \
        struct cexds_arr_new_kwargs_s _kwargs = { kwargs };                                        \
        (typeof(*a)*)cexds_arrgrowf(NULL, sizeof(*a), _kwargs.capacity, 0, allocator);             \
    })

#define arr$free(a) (arr$validate(a), cexds_arrfreef((a)), (a) = NULL)

#define arr$setcap(a, n) (arr$validate(a), arr$grow(a, 0, n))
#define arr$clear(a) (arr$validate(a), cexds_header(a)->length = 0)
#define arr$cap(a) ((a) ? (cexds_header(a)->capacity) : 0)

#define arr$del(a, i)                                                                              \
    ({                                                                                             \
        arr$validate(a);                                                                           \
        uassert((usize)i < cexds_header(a)->length && "out of bounds");                            \
        memmove(&(a)[i], &(a)[(i) + 1], sizeof *(a) * (cexds_header(a)->length - 1 - (i)));        \
        cexds_header(a)->length--;                                                                 \
    })
#define arr$delswap(a, i)                                                                          \
    ({                                                                                             \
        arr$validate(a);                                                                           \
        uassert((usize)i < cexds_header(a)->length && "out of bounds");                            \
        (a)[i] = arr$last(a);                                                                      \
        cexds_header(a)->length -= 1;                                                              \
    })

#define arr$last(a)                                                                                \
    ({                                                                                             \
        arr$validate(a);                                                                           \
        uassert(cexds_header(a)->length > 0 && "empty array");                                     \
        (a)[cexds_header(a)->length - 1];                                                          \
    })
#define arr$at(a, i)                                                                               \
    ({                                                                                             \
        arr$validate(a);                                                                           \
        uassert((usize)i < cexds_header(a)->length && "out of bounds");                            \
        (a)[i];                                                                                    \
    })

#define arr$pop(a)                                                                                 \
    ({                                                                                             \
        arr$validate(a);                                                                           \
        cexds_header(a)->length--;                                                                 \
        (a)[cexds_header(a)->length];                                                              \
    })

#define arr$push(a, value...)                                                                      \
    do {                                                                                           \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$push memory error");                                             \
            abort();                                                                               \
        }                                                                                          \
        (a)[cexds_header(a)->length++] = (value);                                                  \
    } while (0)

#define arr$pushm(a, items...)                                                                     \
    ({                                                                                             \
        typeof(*a) _args[] = { items };                                                            \
        _Static_assert(sizeof(_args) > 0, "You must pass at least one item");                      \
        arr$pusha(a, _args, arr$len(_args));                                                       \
    })

#define arr$pusha(a, array, array_len...)                                                          \
    ({                                                                                             \
        arr$validate(a);                                                                           \
        uassert(array != NULL);                                                                    \
        usize _arr_len_va[] = { array_len };                                                       \
        (void)_arr_len_va;                                                                         \
        usize arr_len = (sizeof(_arr_len_va) > 0) ? _arr_len_va[0] : arr$len(array);               \
        uassert(arr_len < PTRDIFF_MAX && "negative length or overflow");                           \
        if (unlikely(!arr$grow_check(a, arr_len))) {                                               \
            uassert(false && "arr$pusha memory error");                                            \
            abort();                                                                               \
        }                                                                                          \
        for (usize i = 0; i < arr_len; i++) {                                                      \
            (a)[cexds_header(a)->length++] = ((array)[i]);                                         \
        }                                                                                          \
    })


#define arr$ins(a, i, value...)                                                                    \
    do {                                                                                           \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$ins memory error");                                              \
            abort();                                                                               \
        }                                                                                          \
        cexds_header(a)->length++;                                                                 \
        uassert((usize)i < cexds_header(a)->length && "i out of bounds");                          \
        memmove(&(a)[(i) + 1], &(a)[i], sizeof(*(a)) * (cexds_header(a)->length - 1 - (i)));       \
        (a)[i] = (value);                                                                          \
    } while (0)

#define arr$grow_check(a, add_extra)                                                               \
    ((arr$validate(a) && cexds_header(a)->length + (add_extra) > cexds_header(a)->capacity)        \
         ? (arr$grow(a, add_extra, 0), a != NULL)                                                  \
         : true)

#define arr$grow(a, add_len, min_cap)                                                              \
    ((a) = cexds_arrgrowf((a), sizeof *(a), (add_len), (min_cap), NULL))

#define arr$validate(arr)                                                                          \
    ({                                                                                             \
        uassert(arr != NULL && "array uninitialized or out-of-mem");                               \
        /* WARNING: next can trigger sanitizer with "stack/heap-buffer-underflow on address" */    \
        /*          when arr pointer is invalid arr$ type pointer                            */    \
        uassert((cexds_header(arr)->magic_num == CEXDS_ARR_MAGIC) && "bad array pointer");         \
        true;                                                                                      \
    })

#define arr$len(arr)                                                                               \
    ({                                                                                             \
        _Pragma("GCC diagnostic push");                                                            \
        /* NOTE: temporary disable syntax error to support both static array length and arr$(T) */ \
        _Pragma("GCC diagnostic ignored \"-Wsizeof-pointer-div\"");                                \
        __builtin_types_compatible_p(typeof(arr), typeof(&(arr)[0]))                               \
            ? (arr$validate(arr), cexds_header(arr)->length)                                       \
            : (sizeof(arr) / sizeof((arr)[0]));                                                    \
        _Pragma("GCC diagnostic pop");                                                             \
    })

#define for$arr(v, array, array_len...)                                                            \
    usize cex$tmpname(arr_length_opt)[] = { array_len };                                           \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    typeof((array)[0])* cex$tmpname(arr_arrp) = &(array)[0];                                       \
    usize cex$tmpname(arr_index) = 0;                                                              \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    for (typeof((array)[0]) v = { 0 }; cex$tmpname(arr_index) < cex$tmpname(arr_length) &&         \
                                       ((v) = cex$tmpname(arr_arrp)[cex$tmpname(arr_index)], 1);   \
         cex$tmpname(arr_index)++)

#define for$arrp(v, array, array_len...)                                                           \
    usize cex$tmpname(arr_length_opt)[] = { array_len };                                           \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    typeof((array)[0])* cex$tmpname(arr_arrp) = &(array)[0];                                       \
    usize cex$tmpname(arr_index) = 0;                                                              \
    for (typeof((array)[0])* v = cex$tmpname(arr_arrp);                                            \
         cex$tmpname(arr_index) < cex$tmpname(arr_length);                                         \
         cex$tmpname(arr_index)++, v++)
/*
 *                 HASH MAP
 *
 */

#define CEXDS_HASH_TO_ARR(x, elemsize) ((char*)(x) - (elemsize))
#define CEXDS_ARR_TO_HASH(x, elemsize) ((char*)(x) + (elemsize))
#define cexds_hash_table(a) ((cexds_hash_index*)cexds_header(a)->hash_table)

#define hm$(K, V)                                                                                  \
    struct                                                                                         \
    {                                                                                              \
        K key;                                                                                     \
        V value;                                                                                   \
    }*


#define hm$new(t, allocator, kwargs...)                                                            \
    ({                                                                                             \
        _Static_assert(_Alignof(typeof(*t)) <= 64, "hashmap record alignment too high");           \
        uassert(allocator != NULL);                                                                \
        struct cexds_hm_new_kwargs_s _kwargs = { kwargs };                                         \
        (typeof(*t)*)cexds_hminit(sizeof(*t), (allocator), &_kwargs);                              \
    })

#define hm$validate(hm)                                                                                                   \
    ({                                                                                                                    \
        uassert(hm != NULL && "hashmap uninitialized or out-of-mem");                                                       \
        /* WARNING: next can trigger sanitizer with "stack/heap-buffer-underflow on address" */                           \
        /*          when arr pointer is invalid arr$ type pointer                            */                           \
        uassert((cexds_header(CEXDS_HASH_TO_ARR(hm, sizeof(*hm)))->magic_num == CEXDS_HM_MAGIC) && "bad hashmap pointer"); \
        true;                                                                                                             \
    })

#define hm$set(t, k, v)                                                                             \
    ((t                                                                                             \
     ) = cexds_hmput_key((t), sizeof *(t), (void*)mem$addressof((t)->key, (k)), sizeof(t)->key, 0), \
     (t)[cexds_temp((t) - 1)].key = (k),                                                            \
     (t)[cexds_temp((t) - 1)].value = (v))

#define hm$sets(t, s)                                                                              \
    ((t) = cexds_hmput_key((t), sizeof *(t), &(s).key, sizeof(s).key, CEXDS_HM_BINARY),            \
     (t)[cexds_temp((t) - 1)] = (s))


#define hm$geti(t, k)                                                                              \
    ((t) = cexds_hmget_key(                                                                        \
         (t),                                                                                      \
         sizeof *(t),                                                                              \
         (void*)mem$addressof((t)->key, (k)),                                                      \
         sizeof(t)->key,                                                                           \
         CEXDS_HM_BINARY                                                                           \
     ),                                                                                            \
     cexds_temp((t) - 1))

#define hm$geti_ts(t, k, temp)                                                                     \
    ((t) = cexds_hmget_key_ts(                                                                     \
         (t),                                                                                      \
         sizeof *(t),                                                                              \
         (void*)mem$addressof((t)->key, (k)),                                                      \
         sizeof(t)->key,                                                                           \
         &(temp),                                                                                  \
         CEXDS_HM_BINARY                                                                           \
     ),                                                                                            \
     (temp))

#define hm$getp(t, k) ((void)hm$geti(t, k), &(t)[cexds_temp((t) - 1)])

#define hm$getp_ts(t, k, temp) ((void)hm$geti_ts(t, k, temp), &(t)[temp])

#define hm$del(t, k)                                                                               \
    (((t) = cexds_hmdel_key(                                                                       \
          (t),                                                                                     \
          sizeof *(t),                                                                             \
          (void*)mem$addressof((t)->key, (k)),                                                     \
          sizeof(t)->key,                                                                          \
          mem$offsetof((t), key),                                                                  \
          CEXDS_HM_BINARY                                                                          \
      )),                                                                                          \
     (t) ? cexds_temp((t) - 1) : 0)

#define hm$default(t, v) ((t) = cexds_hmput_default((t), sizeof *(t)), (t)[-1].value = (v))

#define hm$defaults(t, s) ((t) = cexds_hmput_default((t), sizeof *(t)), (t)[-1] = (s))

#define hm$free(p) (cexds_hmfree_func((p) - 1, sizeof *(p)), (p) = NULL)

#define hm$gets(t, k) (*hm$getp(t, k))
#define hm$get(t, k) (hm$getp(t, k)->value)
#define hm$get_ts(t, k, temp) (hm$getp_ts(t, k, temp)->value)
#define hm$len(t) ((t) ? (ptrdiff_t)cexds_header((t) - 1)->length - 1 : 0)
#define hm$lenu(t) ((t) ? cexds_header((t) - 1)->length - 1 : 0)
#define hm$getp_null(t, k) (hm$geti(t, k) == -1 ? NULL : &(t)[cexds_temp((t) - 1)])

/*
#define cexds_shput(t, k, v)                                                                       \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*)(k), sizeof(t)->key, CEXDS_HM_STRING),         \
     (t)[cexds_temp((t) - 1)].value = (v))

#define cexds_shputi(t, k, v)                                                                      \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*)(k), sizeof(t)->key, CEXDS_HM_STRING),         \
     (t)[cexds_temp((t) - 1)].value = (v),                                                         \
     cexds_temp((t) - 1))

#define cexds_shputs(t, s)                                                                         \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*)(s).key, sizeof(s).key, CEXDS_HM_STRING),      \
     (t)[cexds_temp((t) - 1)] = (s),                                                               \
     (t)[cexds_temp((t) - 1)].key = cexds_temp_key((t) - 1)                                        \
    ) // above line overwrites whole structure, so must rewrite key here if it was allocated
      // internally

#define cexds_pshput(t, p) \
    ((t \
     ) = cexds_hmput_key((t), sizeof *(t), (void*)(p)->key, sizeof(p)->key, CEXDS_HM_PTR_TO_STRING),
\ (t)[cexds_temp((t) - 1)] = (p))

#define cexds_shgeti(t, k)                                                                         \
    ((t) = cexds_hmget_key((t), sizeof *(t), (void*)(k), sizeof(t)->key, CEXDS_HM_STRING),         \
     cexds_temp((t) - 1))

#define cexds_pshgeti(t, k)                                                                        \
    ((t                                                                                            \
     ) = cexds_hmget_key((t), sizeof *(t), (void*)(k), sizeof(*(t))->key, CEXDS_HM_PTR_TO_STRING), \
     cexds_temp((t) - 1))

#define cexds_shgetp(t, k) ((void)cexds_shgeti(t, k), &(t)[cexds_temp((t) - 1)])

#define cexds_pshget(t, k) ((void)cexds_pshgeti(t, k), (t)[cexds_temp((t) - 1)])

#define cexds_shdel(t, k)                                                                          \
    (((t) = cexds_hmdel_key(                                                                       \
          (t),                                                                                     \
          sizeof *(t),                                                                             \
          (void*)(k),                                                                              \
          sizeof(t)->key,                                                                          \
          mem$offsetof((t), key),                                                                  \
          CEXDS_HM_STRING                                                                          \
      )),                                                                                          \
     (t) ? cexds_temp((t) - 1) : 0)
#define cexds_pshdel(t, k)                                                                         \
    (((t) = cexds_hmdel_key(                                                                       \
          (t),                                                                                     \
          sizeof *(t),                                                                             \
          (void*)(k),                                                                              \
          sizeof(*(t))->key,                                                                       \
          mem$offsetof(*(t), key),                                                                 \
          CEXDS_HM_PTR_TO_STRING                                                                   \
      )),                                                                                          \
     (t) ? cexds_temp((t) - 1) : 0)

#define cexds_sh_new_arena(t) ((t) = cexds_shmode_func_wrapper(t, sizeof *(t), CEXDS_SH_ARENA))
#define cexds_sh_new_strdup(t) ((t) = cexds_shmode_func_wrapper(t, sizeof *(t), CEXDS_SH_STRDUP))

#define cexds_shdefault(t, v) hm$default(t, v)
#define cexds_shdefaults(t, s) hm$defaults(t, s)

#define cexds_shfree hm$free
#define cexds_shlenu hm$lenu

#define cexds_shgets(t, k) (*cexds_shgetp(t, k))
#define cexds_shget(t, k) (cexds_shgetp(t, k)->value)
#define cexds_shgetp_null(t, k) (cexds_shgeti(t, k) == -1 ? NULL : &(t)[cexds_temp((t) - 1)])
#define cexds_shlen hm$len

*/

typedef struct cexds_string_block
{
    struct cexds_string_block* next;
    char storage[8];
} cexds_string_block;

struct cexds_string_arena
{
    cexds_string_block* storage;
    size_t remaining;
    unsigned char block;
    unsigned char mode; // this isn't used by the string arena itself
};

#define CEXDS_HM_BINARY 0
#define CEXDS_HM_STRING 1

enum
{
    CEXDS_SH_NONE,
    CEXDS_SH_DEFAULT,
    CEXDS_SH_STRDUP,
    CEXDS_SH_ARENA
};

#define cexds_arrgrowf cexds_arrgrowf
#define cexds_shmode_func_wrapper(t, e, m) cexds_shmode_func(e, m)

#endif // INCLUDE_STB_DS_H
