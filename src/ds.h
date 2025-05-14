#pragma once
#include "all.h"

// this is a simple string arena allocator, initialize with e.g. 'cexds_string_arena my_arena={0}'.
typedef struct _cexds__string_arena _cexds__string_arena;
extern char* _cexds__stralloc(_cexds__string_arena* a, char* str);
extern void _cexds__strreset(_cexds__string_arena* a);

///////////////
//
// Everything below here is implementation details
//


// clang-format off
struct _cexds__hm_new_kwargs_s;
struct _cexds__arr_new_kwargs_s;
struct _cexds__hash_index;
enum _CexDsKeyType_e
{
    _CexDsKeyType__generic,
    _CexDsKeyType__charptr,
    _CexDsKeyType__charbuf,
    _CexDsKeyType__cexstr,
};
extern void* _cexds__arrgrowf(void* a, usize elemsize, usize addlen, usize min_cap, u16 el_align, IAllocator allc);
extern void _cexds__arrfreef(void* a);
extern bool _cexds__arr_integrity(const void* arr, usize magic_num);
extern usize _cexds__arr_len(const void* arr);
extern void _cexds__hmfree_func(void* p, usize elemsize, usize keyoffset);
extern void _cexds__hmfree_keys_func(void* a, usize elemsize, usize keyoffset);
extern void _cexds__hmclear_func(struct _cexds__hash_index* t, struct _cexds__hash_index* old_table);
extern void* _cexds__hminit(usize elemsize, IAllocator allc, enum _CexDsKeyType_e key_type, u16 el_align, struct _cexds__hm_new_kwargs_s* kwargs);
extern void* _cexds__hmget_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset);
extern void* _cexds__hmput_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset, void* full_elem, void* result);
extern bool _cexds__hmdel_key(void* a, usize elemsize, void* key, usize keysize, usize keyoffset);
// clang-format on

#define _CEXDS_ARR_MAGIC 0xC001DAAD
#define _CEXDS_HM_MAGIC 0xF001C001


// cexds array alignment
// v malloc'd pointer                v-element 1
// |..... <_cexds__array_header>|====!====!====
//      ^ padding      ^cap^len ^         ^-element 2
//                              ^-- arr$ user space pointer (element 0)
//
typedef struct
{
    struct _cexds__hash_index* _hash_table;
    IAllocator allocator;
    u32 magic_num;
    u16 allocator_scope_depth;
    u16 el_align;
    usize capacity;
    usize length; // This MUST BE LAST before __poison_area
    u8 __poison_area[8];
} _cexds__array_header;
_Static_assert(alignof(_cexds__array_header) == alignof(usize), "align");
_Static_assert(
    sizeof(usize) == 8 ? sizeof(_cexds__array_header) == 48 : sizeof(_cexds__array_header) == 32,
    "size for x64 is 48 / for x32 is 32"
);

#define _cexds__header(t) ((_cexds__array_header*)(((char*)(t)) - sizeof(_cexds__array_header)))

#define arr$(T) T*

struct _cexds__arr_new_kwargs_s
{
    usize capacity;
};
#define arr$new(a, allocator, kwargs...)                                                           \
    ({                                                                                             \
        _Static_assert(_Alignof(typeof(*a)) <= 64, "array item alignment too high");               \
        uassert(allocator != NULL);                                                                \
        struct _cexds__arr_new_kwargs_s _kwargs = { kwargs };                                      \
        (a) = (typeof(*a)*)_cexds__arrgrowf(                                                       \
            NULL,                                                                                  \
            sizeof(*a),                                                                            \
            _kwargs.capacity,                                                                      \
            0,                                                                                     \
            alignof(typeof(*a)),                                                                   \
            allocator                                                                              \
        );                                                                                         \
    })

#define arr$free(a) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), _cexds__arrfreef((a)), (a) = NULL)

#define arr$setcap(a, n) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), arr$grow(a, 0, n))
#define arr$clear(a) (_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC), _cexds__header(a)->length = 0)
#define arr$cap(a) ((a) ? (_cexds__header(a)->capacity) : 0)

#define arr$del(a, i)                                                                              \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        memmove(&(a)[i], &(a)[(i) + 1], sizeof *(a) * (_cexds__header(a)->length - 1 - (i)));      \
        _cexds__header(a)->length--;                                                               \
    })
#define arr$delswap(a, i)                                                                          \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        (a)[i] = arr$last(a);                                                                      \
        _cexds__header(a)->length -= 1;                                                            \
    })

#define arr$last(a)                                                                                \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassert(_cexds__header(a)->length > 0 && "empty array");                                   \
        (a)[_cexds__header(a)->length - 1];                                                        \
    })
#define arr$at(a, i)                                                                               \
    ({                                                                                             \
        _cexds__arr_integrity(a, 0); /* may work also on hm$ */                                    \
        uassert((usize)i < _cexds__header(a)->length && "out of bounds");                          \
        (a)[i];                                                                                    \
    })

#define arr$pop(a)                                                                                 \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        _cexds__header(a)->length--;                                                               \
        (a)[_cexds__header(a)->length];                                                            \
    })

#define arr$push(a, value...)                                                                      \
    ({                                                                                             \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$push memory error");                                             \
            abort();                                                                               \
        }                                                                                          \
        (a)[_cexds__header(a)->length++] = (value);                                                \
    })

#define arr$pushm(a, items...)                                                                     \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        typeof(*a) _args[] = { items };                                                            \
        _Static_assert(sizeof(_args) > 0, "You must pass at least one item");                      \
        arr$pusha(a, _args, arr$len(_args));                                                       \
        /* NOLINTEND */                                                                            \
    })

#define arr$pusha(a, array, array_len...)                                                          \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        uassertf(array != NULL, "arr$pusha: array is NULL");                                       \
        usize _arr_len_va[] = { array_len };                                                       \
        (void)_arr_len_va;                                                                         \
        usize arr_len = (sizeof(_arr_len_va) > 0) ? _arr_len_va[0] : arr$len(array);               \
        uassert(arr_len < PTRDIFF_MAX && "negative length or overflow");                           \
        if (unlikely(!arr$grow_check(a, arr_len))) {                                               \
            uassert(false && "arr$pusha memory error");                                            \
            abort();                                                                               \
        }                                                                                          \
        /* NOLINTEND */                                                                            \
        for (usize i = 0; i < arr_len; i++) { (a)[_cexds__header(a)->length++] = ((array)[i]); }   \
    })

#define arr$sort(a, qsort_cmp)                                                                     \
    ({                                                                                             \
        _cexds__arr_integrity(a, _CEXDS_ARR_MAGIC);                                                \
        qsort((a), arr$len(a), sizeof(*a), qsort_cmp);                                             \
    })


#define arr$ins(a, i, value...)                                                                    \
    do {                                                                                           \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$ins memory error");                                              \
            abort();                                                                               \
        }                                                                                          \
        _cexds__header(a)->length++;                                                               \
        uassert((usize)i < _cexds__header(a)->length && "i out of bounds");                        \
        memmove(&(a)[(i) + 1], &(a)[i], sizeof(*(a)) * (_cexds__header(a)->length - 1 - (i)));     \
        (a)[i] = (value);                                                                          \
    } while (0)

#define arr$grow_check(a, add_extra)                                                               \
    ((_cexds__arr_integrity(a, _CEXDS_ARR_MAGIC) &&                                                \
      _cexds__header(a)->length + (add_extra) > _cexds__header(a)->capacity)                       \
         ? (arr$grow(a, add_extra, 0), a != NULL)                                                  \
         : true)

#define arr$grow(a, add_len, min_cap)                                                              \
    ((a) = _cexds__arrgrowf((a), sizeof *(a), (add_len), (min_cap), alignof(typeof(*a)), NULL))


#if defined(__GNUC__) && !defined(__clang__) && (__GNUC__ < 12)
#    define arr$len(arr)                                                                           \
        ({                                                                                         \
            __builtin_types_compatible_p(                                                          \
                typeof(arr),                                                                       \
                typeof(&(arr)[0])                                                                  \
            )                          /* check if array or ptr */                                 \
                ? _cexds__arr_len(arr) /* some pointer or arr$ */                                  \
                : (                                                                                \
                      sizeof(arr) / sizeof((arr)[0]) /* static array[] */                          \
                  );                                                                               \
        })
#else
#    define arr$len(arr)                                                                           \
        ({                                                                                         \
            _Pragma("GCC diagnostic push");                                                        \
            /* NOTE: temporary disable syntax error to support both static array length and        \
             * arr$(T) */                                                                          \
            _Pragma("GCC diagnostic ignored \"-Wsizeof-pointer-div\"");                            \
            /* NOLINTBEGIN */                                                                      \
            __builtin_types_compatible_p(                                                          \
                typeof(arr),                                                                       \
                typeof(&(arr)[0])                                                                  \
            )                          /* check if array or ptr */                                 \
                ? _cexds__arr_len(arr) /* some pointer or arr$ */                                  \
                : (                                                                                \
                      sizeof(arr) / sizeof((arr)[0]) /* static array[] */                          \
                  );                                                                               \
            /* NOLINTEND */                                                                        \
            _Pragma("GCC diagnostic pop");                                                         \
        })
#endif

static inline void*
_cex__get_buf_addr(void* a)
{
    return (a != NULL) ? &((char*)a)[0] : NULL;
}

#define for$each(v, array, array_len...)                                                           \
    /* NOLINTBEGIN*/                                                                               \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    typeof((array)[0])* cex$tmpname(arr_arrp) = _cex__get_buf_addr(array);                         \
    usize cex$tmpname(arr_index) = 0;                                                              \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    /* NOLINTEND */                                                                                \
    for (typeof((array)[0]) v = { 0 };                                                             \
         (cex$tmpname(arr_index) < cex$tmpname(arr_length) &&                                      \
          ((v) = cex$tmpname(arr_arrp)[cex$tmpname(arr_index)], 1));                               \
         cex$tmpname(arr_index)++)


#define for$eachp(v, array, array_len...)                                                          \
    /* NOLINTBEGIN*/                                                                               \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    typeof((array)[0])* cex$tmpname(arr_arrp) = _cex__get_buf_addr(array);                         \
    usize cex$tmpname(arr_index) = 0;                                                              \
    /* NOLINTEND */                                                                                \
    for (typeof((array)[0])* v = cex$tmpname(arr_arrp);                                            \
         cex$tmpname(arr_index) < cex$tmpname(arr_length);                                         \
         cex$tmpname(arr_index)++, v++)

/*
 *                 HASH MAP
 */

#define hm$(_KeyType, _ValType)                                                                    \
    struct                                                                                         \
    {                                                                                              \
        _KeyType key;                                                                              \
        _ValType value;                                                                            \
    }*

#define hm$s(_StructType) _StructType*

struct _cexds__hm_new_kwargs_s
{
    usize capacity;
    usize seed;
    u32 copy_keys_arena_pgsize;
    bool copy_keys;
};


#define hm$new(t, allocator, kwargs...)                                                            \
    ({                                                                                             \
        _Static_assert(_Alignof(typeof(*t)) <= 64, "hashmap record alignment too high");           \
        uassert(allocator != NULL);                                                                \
        enum _CexDsKeyType_e _key_type = _Generic(                                                 \
            &((t)->key),                                                                           \
            str_s *: _CexDsKeyType__cexstr,                                                        \
            char(**): _CexDsKeyType__charptr,                                                      \
            const char(**): _CexDsKeyType__charptr,                                                \
            char (*)[]: _CexDsKeyType__charbuf,                                                    \
            const char (*)[]: _CexDsKeyType__charbuf,                                              \
            default: _CexDsKeyType__generic                                                        \
        );                                                                                         \
        struct _cexds__hm_new_kwargs_s _kwargs = { kwargs };                                       \
        (t) = (typeof(*t)*)                                                                        \
            _cexds__hminit(sizeof(*t), (allocator), _key_type, alignof(typeof(*t)), &_kwargs);     \
    })


#define hm$set(t, k, v...)                                                                         \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = _cexds__hmput_key(                                                                   \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key),      /* offset of key in hm struct */                       \
            NULL,                           /* no full element set */                              \
            &result                         /* NULL on memory error */                             \
        );                                                                                         \
        if (result) result->value = (v);                                                           \
        result;                                                                                    \
    })

#define hm$setp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = _cexds__hmput_key(                                                                   \
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
        (t) = _cexds__hmput_key(                                                                   \
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
        typeof(t) result = _cexds__hmget_key(                                                      \
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
        typeof(t) result = _cexds__hmget_key(                                                      \
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
        typeof(t) result = _cexds__hmget_key(                                                      \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
        result;                                                                                    \
    })

#define hm$clear(t)                                                                                \
    ({                                                                                             \
        _cexds__arr_integrity(t, _CEXDS_HM_MAGIC);                                                 \
        _cexds__hmfree_keys_func((t), sizeof(*t), offsetof(typeof(*t), key));                      \
        _cexds__hmclear_func(_cexds__header((t))->_hash_table, NULL);                              \
        _cexds__header(t)->length = 0;                                                             \
        true;                                                                                      \
    })

#define hm$del(t, k)                                                                               \
    ({                                                                                             \
        _cexds__hmdel_key(                                                                         \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
    })


#define hm$free(t) (_cexds__hmfree_func((t), sizeof *(t), offsetof(typeof(*t), key)), (t) = NULL)

#define hm$len(t)                                                                                  \
    ({                                                                                             \
        if (t != NULL) { _cexds__arr_integrity(t, _CEXDS_HM_MAGIC); }                              \
        (t) ? _cexds__header((t))->length : 0;                                                     \
    })

typedef struct _cexds__string_block
{
    struct _cexds__string_block* next;
    char storage[8];
} _cexds__string_block;

struct _cexds__string_arena
{
    _cexds__string_block* storage;
    usize remaining;
    unsigned char block;
    unsigned char mode; // this isn't used by the string arena itself
};

enum
{
    _CEXDS_SH_NONE,
    _CEXDS_SH_DEFAULT,
    _CEXDS_SH_STRDUP,
    _CEXDS_SH_ARENA
};

#define _cexds__shmode_func_wrapper(t, e, m) _cexds__shmode_func(e, m)
