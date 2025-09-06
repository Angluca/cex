Symbol found at ./cex.h:998


```c
#define arr$(T)

#define arr$at(a, i)

#define arr$cap(a)

#define arr$clear(a)

#define arr$del(a, i)

#define arr$delswap(a, i)

#define arr$free(a)

#define arr$grow(a, add_len, min_cap)

#define arr$grow_check(a, add_extra)

#define arr$ins(a, i, value...)

#define arr$last(a)

#define arr$len(arr)

#define arr$new(a, allocator, kwargs...)

#define arr$pop(a)

#define arr$push(a, value...)

#define arr$pusha(a, array, array_len...)

#define arr$pushm(a, items...)

#define arr$setcap(a, n)

#define arr$sort(a, qsort_cmp)



#define arr$new (a, allocator, kwargs...)                                                           \
    ({                                                                                             \
        static_assert(_Alignof(typeof(*a)) <= 64, "array item alignment too high");               \
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
    });

```
