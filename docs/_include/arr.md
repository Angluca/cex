Symbol found at ./cex.h:608


 * @brief Gets array generic slice (typed as array)
 *
 * Example:
 * var s = arr$slice(arr, .start = 1, .end = -2);
 * var s = arr$slice(arr, 1, -2);
 * var s = arr$slice(arr, .start = -2);
 * var s = arr$slice(arr, .end = 3);
 *
 * Note: arr$slice creates a temporary type, and it's preferable to use var keyword
 *
 * @param array - generic array
 * @param .start - start index, may be negative to get item from the end of array
 * @param .end - end index, 0 - means all, or may be negative to get item from the end of array
 * @return struct {eltype* arr, usize len}, or {NULL, 0} if bad slice index/not found/NULL array
 *
 * @warning returns {.arr = NULL, .len = 0} if bad indexes or array
 

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

#define arr$slice(array, ...)

#define arr$sort(a, qsort_cmp)



#define arr$slice (array, ...)                                                                      \
    ({                                                                                             \
        slice$define(*array) s = { .arr = NULL, .len = 0 };                                        \
        _arr$slice_get(s, array, arr$len(array), __VA_ARGS__);                                     \
        s;                                                                                         \
    });

```
