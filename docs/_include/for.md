Symbol found at ./cex.h:647


 * @brief Iterates via iterator function (see usage below)
 *
 * Iterator function signature:
 * u32* array_iterator(u32 array[], u32 len, cex_iterator_s* iter)
 *
 * for$iter(u32, it, array_iterator(arr2, arr$len(arr2), &it.iterator))
 

```c
#define for$each(v, array, array_len...)

#define for$eachp(v, array, array_len...)

#define for$iter(it_val_type, it, iter_func)



#define for$iter (it_val_type, it, iter_func)                                                       \
    struct cex$tmpname(__cex_iter_)                                                                \
    {                                                                                              \
        it_val_type val;                                                                           \
        union /* NOTE:  iterator above and this struct shadow each other */                        \
        {                                                                                          \
            cex_iterator_s iterator;                                                               \
            struct                                                                                 \
            {                                                                                      \
                union                                                                              \
                {                                                                                  \
                    usize i;                                                                       \
                    char* skey;                                                                    \
                    void* pkey;                                                                    \
                };                                                                                 \
            } idx;                                                                                 \
        };                                                                                         \
    };                                                                                             \
                                                                                                   \
    for (struct cex$tmpname(__cex_iter_) it = { .val = (iter_func) }; !it.iterator.stopped;        \
         it.val = (iter_func));

```
