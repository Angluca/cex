#ifndef INCLUDE_STB_DS_H
#define INCLUDE_STB_DS_H

#include "cex.h"
#include "mem.h"
#include <stddef.h>
#include <string.h>

// this is a simple string arena allocator, initialize with e.g. 'cexds_string_arena my_arena={0}'.
typedef struct cexds_string_arena cexds_string_arena;
extern char* cexds_stralloc(cexds_string_arena* a, char* str);
extern void cexds_strreset(cexds_string_arena* a);

///////////////
//
// Everything below here is implementation details
//


// clang-format off
struct cexds_hm_new_kwargs_s;
struct cexds_arr_new_kwargs_s;
enum _CexDsKeyType_e
{
    _CexDsKeyType__generic,
    _CexDsKeyType__charptr,
    _CexDsKeyType__charbuf,
    _CexDsKeyType__cexstr,
};
extern void* cexds_arrgrowf(void* a, size_t elemsize, size_t addlen, size_t min_cap, const Allocator_i* allc);
extern void cexds_arrfreef(void* a);
extern bool cexds_arr_integrity(void* arr, size_t magic_num);
extern void cexds_hmfree_func(void* p, size_t elemsize);
extern void* cexds_hminit(size_t elemsize, const Allocator_i* allc, enum _CexDsKeyType_e key_type, struct cexds_hm_new_kwargs_s* kwargs);
extern void* cexds_hmget_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset);
extern void* cexds_hmput_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset, void* full_elem, void* result);
extern void* cexds_hmdel_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset, int mode);
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
    const Allocator_i* allocator;
    u32 magic_num;
    enum _CexDsKeyType_e hm_key_type;
    size_t hm_seed;
    size_t capacity;
    size_t length; // This MUST BE LAST
} cexds_array_header;
_Static_assert(alignof(cexds_array_header) == alignof(void*), "align");
_Static_assert(sizeof(cexds_array_header) <= CEXDS_HDR_PAD, "size too high");
_Static_assert(sizeof(cexds_array_header) % alignof(size_t) == 0, "align size");

_Static_assert(sizeof(cexds_array_header) == 48, "size");


#define cexds_header(t) ((cexds_array_header*)(((char*)(t)) - sizeof(cexds_array_header)))
#define cexds_base(t) ((char*)(t) - CEXDS_HDR_PAD)
#define cexds_item_ptr(t, i, elemsize) ((char*)a + elemsize * i)

#define arr$(T) T*

struct cexds_arr_new_kwargs_s
{
    usize capacity;
};
#define arr$new(a, allocator, kwargs...)                                                           \
    ({                                                                                             \
        _Static_assert(_Alignof(typeof(*a)) <= 64, "array item alignment too high");               \
        uassert(allocator != NULL);                                                                \
        struct cexds_arr_new_kwargs_s _kwargs = { kwargs };                                        \
        (a) = (typeof(*a)*)cexds_arrgrowf(NULL, sizeof(*a), _kwargs.capacity, 0, allocator);       \
    })

#define arr$free(a) (cexds_arr_integrity(a, CEXDS_ARR_MAGIC), cexds_arrfreef((a)), (a) = NULL)

#define arr$setcap(a, n) (cexds_arr_integrity(a, CEXDS_ARR_MAGIC), arr$grow(a, 0, n))
#define arr$clear(a) (cexds_arr_integrity(a, CEXDS_ARR_MAGIC), cexds_header(a)->length = 0)
#define arr$cap(a) ((a) ? (cexds_header(a)->capacity) : 0)

#define arr$del(a, i)                                                                              \
    ({                                                                                             \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
        uassert((usize)i < cexds_header(a)->length && "out of bounds");                            \
        memmove(&(a)[i], &(a)[(i) + 1], sizeof *(a) * (cexds_header(a)->length - 1 - (i)));        \
        cexds_header(a)->length--;                                                                 \
    })
#define arr$delswap(a, i)                                                                          \
    ({                                                                                             \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
        uassert((usize)i < cexds_header(a)->length && "out of bounds");                            \
        (a)[i] = arr$last(a);                                                                      \
        cexds_header(a)->length -= 1;                                                              \
    })

#define arr$last(a)                                                                                \
    ({                                                                                             \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
        uassert(cexds_header(a)->length > 0 && "empty array");                                     \
        (a)[cexds_header(a)->length - 1];                                                          \
    })
#define arr$at(a, i)                                                                               \
    ({                                                                                             \
        cexds_arr_integrity(a, 0); /* may work also on hm$ */                                      \
        uassert((usize)i < cexds_header(a)->length && "out of bounds");                            \
        (a)[i];                                                                                    \
    })

#define arr$pop(a)                                                                                 \
    ({                                                                                             \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
        cexds_header(a)->length--;                                                                 \
        (a)[cexds_header(a)->length];                                                              \
    })

#define arr$push(a, value...)                                                                      \
    ({                                                                                             \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$push memory error");                                             \
            abort();                                                                               \
        }                                                                                          \
        (a)[cexds_header(a)->length++] = (value);                                                  \
    })

#define arr$pushm(a, items...)                                                                     \
    ({                                                                                             \
        typeof(*a) _args[] = { items };                                                            \
        _Static_assert(sizeof(_args) > 0, "You must pass at least one item");                      \
        arr$pusha(a, _args, arr$len(_args));                                                       \
    })

#define arr$pusha(a, array, array_len...)                                                          \
    ({                                                                                             \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
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
    ((cexds_arr_integrity(a, CEXDS_ARR_MAGIC) &&                                                   \
      cexds_header(a)->length + (add_extra) > cexds_header(a)->capacity)                           \
         ? (arr$grow(a, add_extra, 0), a != NULL)                                                  \
         : true)

#define arr$grow(a, add_len, min_cap)                                                              \
    ((a) = cexds_arrgrowf((a), sizeof *(a), (add_len), (min_cap), NULL))

#define arr$len(arr)                                                                               \
    ({                                                                                             \
        _Pragma("GCC diagnostic push");                                                            \
        /* NOTE: temporary disable syntax error to support both static array length and arr$(T) */ \
        _Pragma("GCC diagnostic ignored \"-Wsizeof-pointer-div\"");                                \
        __builtin_types_compatible_p(typeof(arr), typeof(&(arr)[0]))   /* check if array of ptr */ \
            ? (cexds_arr_integrity(arr, 0), cexds_header(arr)->length) /* some pointer to array */ \
            : (sizeof(arr) / sizeof((arr)[0])                          /* static array[] */        \
              );                                                                                   \
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

// #define CEXDS_HASH_TO_ARR(x, elemsize) ((char*)(x) - (elemsize))
// #define CEXDS_ARR_TO_HASH(x, elemsize) ((char*)(x) + (elemsize))
#define cexds_hash_table(a) ((cexds_hash_index*)cexds_header(a)->hash_table)

#define hm$(_KeyType, _ValType)                                                                    \
    struct                                                                                         \
    {                                                                                              \
        _KeyType key;                                                                              \
        _ValType value;                                                                            \
    }*

#define hm$s(_StructType) _StructType*

struct cexds_hm_new_kwargs_s
{
    usize capacity;
    size_t seed;
    bool copy_keys;
};


#define hm$new(t, allocator, kwargs...)                                                            \
    ({                                                                                             \
        _Static_assert(_Alignof(typeof(*t)) <= 64, "hashmap record alignment too high");           \
        uassert(allocator != NULL);                                                                \
        enum _CexDsKeyType_e _key_type = _Generic(                                                 \
            &((t)->key),                                                                           \
            str_c *: _CexDsKeyType__cexstr,                                                        \
            char(**): _CexDsKeyType__charptr,                                                      \
            const char(**): _CexDsKeyType__charptr,                                                \
            char(*)[]: _CexDsKeyType__charbuf,                                                     \
            const char(*)[]: _CexDsKeyType__charbuf,                                               \
            default: _CexDsKeyType__generic                                                        \
        );                                                                                         \
        struct cexds_hm_new_kwargs_s _kwargs = { kwargs };                                         \
        (t) = (typeof(*t)*)cexds_hminit(sizeof(*t), (allocator), _key_type, &_kwargs);             \
    })


#define hm$set(t, k, v...)                                                                         \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = cexds_hmput_key(                                                                     \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key),      /* offset of key in hm struct */                       \
            NULL,                           /* no full element set */                              \
            &result                         /* NULL on memory error */                             \
        );                                                                                         \
        if (result)                                                                                \
            result->value = (v);                                                                   \
        result;                                                                                    \
    })

#define hm$setp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = cexds_hmput_key(                                                                     \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key),      /* offset of key in hm struct */                       \
            NULL,                           /* no full element set */                              \
            &result                         /* NULL on memory error */                             \
        );                                                                                         \
        (result ? &result->value : NULL);                                                          \
    })

#define hm$sets(t, v...)                                                                           \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        typeof(*t) _val = (v);                                                                     \
        (t) = cexds_hmput_key(                                                                     \
            (t),                                                                                   \
            sizeof(*t),                /* size of hashmap item */                                  \
            &_val.key,                 /* temp on stack pointer to (k) value */                    \
            sizeof((t)->key),          /* size of key */                                           \
            offsetof(typeof(*t), key), /* offset of key in hm struct */                            \
            &(_val),                   /* full element write */                                    \
            &result                    /* NULL on memory error */                                  \
        );                                                                                         \
        result;                                                                                    \
    })

#define hm$get(t, k, def...)                                                                       \
    ({                                                                                             \
        typeof(t) result = cexds_hmget_key(                                                        \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key)       /* offset of key in hm struct */                       \
        );                                                                                         \
        typeof((t)->value) _def[1] = { def }; /* default value, always 0 if def... is empty! */    \
        result ? result->value : _def[0];                                                          \
    })

#define hm$getp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = cexds_hmget_key(                                                        \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
        result ? &result->value : NULL;                                                            \
    })

#define hm$gets(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = cexds_hmget_key(                                                        \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
        result;                                                                                    \
    })

#define hm$del(t, k)                                                                               \
    ({                                                                                             \
        (t) = cexds_hmdel_key(                                                                     \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key),                                                             \
            0                                                                                      \
        );                                                                                         \
        /* TODO: implement return bool flag if deleted key existed*/                               \
    })


#define hm$free(p) (cexds_hmfree_func((p), sizeof *(p)), (p) = NULL)

#define hm$len(t)                                                                                  \
    ({                                                                                             \
        uassert(t != NULL && "hashmap uninitialized or out-of-mem");                               \
        /* IMPORTANT: next can trigger sanitizer with "stack/heap-buffer-underflow on              \
         * address" */                                                                             \
        uassert((cexds_header(t)->magic_num == CEXDS_HM_MAGIC) && "bad hashmap pointer");          \
        cexds_header((t))->length;                                                                 \
    })

/*
#define cexds_shput(t, k, v) \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*)(k), sizeof(t)->key, CEXDS_HM_STRING), \
     (t)[cexds_temp((t) - 1)].value = (v))

#define cexds_shputi(t, k, v) \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*)(k), sizeof(t)->key, CEXDS_HM_STRING), \
     (t)[cexds_temp((t) - 1)].value = (v), \ cexds_temp((t) - 1))

#define cexds_shputs(t, s) \
    ((t) = cexds_hmput_key((t), sizeof *(t), (void*)(s).key, sizeof(s).key,
CEXDS_HM_STRING),      \
     (t)[cexds_temp((t) - 1)] = (s), \
     (t)[cexds_temp((t) - 1)].key = cexds_temp_key((t) - 1) \ ) // above line overwrites
whole structure, so must rewrite key here if it was allocated
      // internally

#define cexds_pshput(t, p) \
    ((t \
     ) = cexds_hmput_key((t), sizeof *(t), (void*)(p)->key, sizeof(p)->key,
CEXDS_HM_PTR_TO_STRING),
\ (t)[cexds_temp((t) - 1)] = (p))

#define cexds_shgeti(t, k) \
    ((t) = cexds_hmget_key((t), sizeof *(t), (void*)(k), sizeof(t)->key, CEXDS_HM_STRING), \
     cexds_temp((t) - 1))

#define cexds_pshgeti(t, k) \
    ((t \
     ) = cexds_hmget_key((t), sizeof *(t), (void*)(k), sizeof(*(t))->key,
CEXDS_HM_PTR_TO_STRING), \ cexds_temp((t) - 1))

#define cexds_shgetp(t, k) ((void)cexds_shgeti(t, k), &(t)[cexds_temp((t) - 1)])

#define cexds_pshget(t, k) ((void)cexds_pshgeti(t, k), (t)[cexds_temp((t) - 1)])

#define cexds_shdel(t, k) \
    (((t) = cexds_hmdel_key( \
          (t), \
          sizeof *(t), \
          (void*)(k), \
          sizeof(t)->key, \
          mem$offsetof((t), key), \
          CEXDS_HM_STRING \
      )), \ (t) ? cexds_temp((t) - 1) : 0)
#define cexds_pshdel(t, k) \
    (((t) = cexds_hmdel_key( \
          (t), \
          sizeof *(t), \
          (void*)(k), \
          sizeof(*(t))->key, \
          mem$offsetof(*(t), key), \
          CEXDS_HM_PTR_TO_STRING \
      )), \ (t) ? cexds_temp((t) - 1) : 0)

#define cexds_sh_new_arena(t) ((t) = cexds_shmode_func_wrapper(t, sizeof *(t),
CEXDS_SH_ARENA)) #define cexds_sh_new_strdup(t) ((t) = cexds_shmode_func_wrapper(t, sizeof
*(t), CEXDS_SH_STRDUP))

#define cexds_shdefault(t, v) hm$default(t, v)
#define cexds_shdefaults(t, s) hm$defaults(t, s)

#define cexds_shfree hm$free
#define cexds_shlenu hm$lenu

#define cexds_shgets(t, k) (*cexds_shgetp(t, k))
#define cexds_shget(t, k) (cexds_shgetp(t, k)->value)
#define cexds_shgetp_null(t, k) (cexds_shgeti(t, k) == -1 ? NULL : &(t)[cexds_temp((t) -
1)]) #define cexds_shlen hm$len

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
