Symbol found at ./cex.h:1376



- using for$ as unified array iterator

```c

test$case(test_array_iteration)
{
    arr$(int) array = arr$new(array, mem$);
    arr$pushm(array, 1, 2, 3);

    for$each(it, array) {
        io.printf("el=%d\n", it);
    }
    // Prints:
    // el=1
    // el=2
    // el=3

    // NOTE: prefer this when you work with bigger structs to avoid extra memory copying
    for$eachp(it, array) {
        // TIP: making array index out of `it`
        usize i = it - array;

        // NOTE: it now is a pointer
        io.printf("el[%zu]=%d\n", i, *it);
    }
    // Prints:
    // el[0]=1
    // el[1]=2
    // el[2]=3

    // Static arrays work as well (arr$len inferred)
    i32 arr_int[] = {1, 2, 3, 4, 5};
    for$each(it, arr_int) {
        io.printf("static=%d\n", it);
    }
    // Prints:
    // static=1
    // static=2
    // static=3
    // static=4
    // static=5


    // Simple pointer+length also works (let's do a slice)
    i32* slice = &arr_int[2];
    for$each(it, slice, 2) {
        io.printf("slice=%d\n", it);
    }
    // Prints:
    // slice=3
    // slice=4


    // it is type of cex_iterator_s
    // NOTE: run in shell: ➜ ./cex help cex_iterator_s
    s = str.sstr("123,456");
    for$iter (str_s, it, str.slice.iter_split(s, ",", &it.iterator)) {
        io.printf("it.value = %S\n", it.val);
    }
    // Prints:
    // it.value = 123
    // it.value = 456

    arr$free(array);
    return EOK;
}

```



```c
/// Iterates over arrays `it` is iterated **value**, array may be arr$/or static / or pointer,
/// array_len is only required for pointer+len use case
#define for$each(it, array, array_len...)

/// Iterates over arrays `it` is iterated by **pointer**, array may be arr$/or static / or pointer,
/// array_len is only required for pointer+len use case
#define for$eachp(it, array, array_len...)

/// Iterates via iterator function (see usage below)
#define for$iter(it_val_type, it, iter_func)




```
