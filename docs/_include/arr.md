Symbol found at ./cex.h:1045



* Creating array
```c
    // Using heap allocator (need to free later!)
    arr$(i32) array = arr$new(array, mem$);

    // adding elements
    arr$pushm(array, 1, 2, 3); // multiple at once
    arr$push(array, 4); // single element

    // length of array
    arr$len(array);

    // getting i-th elements
    array[1];

    // iterating array (by value)
    for$each(it, array) {
        io.printf("el=%d\n", it);
    }

    // iterating array (by pointer - prefer for bigger structs to avoid copying)
    for$each(it, array) {
        // TIP: making array index out of `it`
        usize i = it - array;

        // NOTE: 'it' now is a pointer
        io.printf("el[%zu]=%d\n", i, *it);
    }

    // free resources
    arr$free(array);
```

* Array of structs
```c

typedef struct
{
    int key;
    float my_val;
    char* my_string;
    int value;
} my_struct;

void somefunc(void)
{
    arr$(my_struct) array = arr$new(array, mem$, .capacity = 128);
    uassert(arr$cap(array), 128);

    my_struct s;
    s = (my_struct){ 20, 5.0, "hello ", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 2.5, "failure", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 1.1, "world!", 0 };
    arr$push(array, s);

    for (usize i = 0; i < arr$len(array); ++i) {
        io.printf("key: %d str: %s\n", array[i].key, array[i].my_string);
    }
    arr$free(array);

    return EOK;
}
```



```c
/// Generic array type definition. Use arr$(int) myarr - defines new myarr variable, as int array
#define arr$(T)

/// Get element at index (bounds checking with uassert())
#define arr$at(a, i)

/// Returns current array capacity
#define arr$cap(a)

/// Clear array contents
#define arr$clear(a)

/// Delete array elements by index (memory will be shifted, order preserved)
#define arr$del(a, i)

/// Delete element by swapping with last one (no memory overhear, element order changes)
#define arr$delswap(a, i)

/// Free resources for dynamic array (only needed if mem$ allocator was used)
#define arr$free(a)

/// Grows array capacity
#define arr$grow(a, add_len, min_cap)

/// Check array capacity and return false on memory error
#define arr$grow_check(a, add_extra)

/// Inserts element into array at index `i`
#define arr$ins(a, i, value...)

/// Return last element of array
#define arr$last(a)

/// Versatile array length, works with dynamic (arr$) and static compile time arrays
#define arr$len(arr)

/// Array initialization: use arr$(int) arr = arr$new(arr, mem$, .capacity = , ...)
#define arr$new(a, allocator, kwargs...)

/// Pop element from the end
#define arr$pop(a)

/// Push element to the end
#define arr$push(a, value...)

/// Push another array into a. array can be dynamic or static or pointer+len
#define arr$pusha(a, array, array_len...)

/// Push many elements to the end
#define arr$pushm(a, items...)

/// Set array capacity and resize if needed
#define arr$setcap(a, n)

/// Sort array with qsort() libc function
#define arr$sort(a, qsort_cmp)




```
