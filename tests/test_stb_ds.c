#include "_generic_defs.h"
#include "cex/cex.h"
#include "cex/ds.h"
#include <cex/all.c>
#include <cex/ds.c>
#include <cex/list.c>
#include <cex/test/fff.h>
#include <cex/test/test.h>
#include <stdio.h>

const Allocator_i* allocator;
/*
 * SUITE INIT / SHUTDOWN
 */
test$teardown()
{
    allocator = AllocatorGeneric.destroy();
    return EOK;
}

test$setup()
{
    uassert_enable();
    allocator = AllocatorGeneric.create();
    return EOK;
}


#define arr$putm(arr, args...)                                                                     \
    ({                                                                                             \
        typeof(*arr) _args[] = { args };                                                           \
        for (usize i = 0; i < sizeof(_args) / sizeof(_args[0]); i++)                               \
            arr$put(arr, _args[i]);                                                                \
    })

static void
add_to_arr(arr$(int) * arr)
{
    // arrput(*arr, 2);
    // arrput(*arr, 3);
    // arrput(*arr, 5);
    int z = 200;
    arr$putm(*arr, z, 3, 5);
}

static void
add_to_str(arr$(const char*) * arr)
{
    char buf[10] = { "buf" };

    arr$put(*arr, "foooo");
    arr$put(*arr, "barrr");
    arr$put(*arr, "bazzz");
    arr$putm(*arr, "one", "two", "three", buf);
}

test$case(test_array)
{
    arr$(const char*) array2 = arr$new(array2, 10, allocator);
    arr$(int) array = arr$new(array, 10, allocator);
    tassert(array != NULL);
    tassert_eqi(arr$cap(array), 10);


    add_to_arr(&array);
    add_to_str(&array2);
    for (usize i = 0; i < arr$lenu(array); ++i) {
        printf("%d \n", array[i]);
    }

    for (usize i = 0; i < arr$lenu(array2); ++i) {
        printf("%s \n", array2[i]);
    }
    arr$free(array);
    arr$free(array2);

    return EOK;
}

test$case(test_array_char_ptr)
{
    arr$(char*) array = arr$new(array, 10, allocator);
    arr$put(array, "foo");
    arr$put(array, "bar");
    arr$put(array, "baz");
    for (usize i = 0; i < arr$lenu(array); ++i) {
        printf("%s \n", array[i]);
    }
    arr$free(array);

    return EOK;
}


typedef struct
{
    int key;
    float my_val;
    char* my_string;
} my_struct;

test$case(test_array_struct)
{
    arr$(my_struct) array = arr$new(array, 10, allocator);

    my_struct s;
    s = (my_struct){ 20, 5.0, "hello " };
    arr$put(array, s);
    s = (my_struct){ 40, 2.5, "failure" };
    arr$put(array, s);
    s = (my_struct){ 40, 1.1, "world!" };
    arr$put(array, s);

    for (int i = 0; i < arr$leni(array); ++i) {
        printf("key: %d str: %s\n", array[i].key, array[i].my_string);
    }
    arr$free(array);

    return EOK;
}
 

typedef struct { int key,b,c,d; } stbds_struct;
typedef struct { int key[2],b,c,d; } stbds_struct2;

static char buffer[256];
char *strkey(int n)
{
#if defined(_WIN32) && defined(__STDC_WANT_SECURE_LIB__)
   sprintf_s(buffer, sizeof(buffer), "test_%d", n);
#else
   sprintf(buffer, "test_%d", n);
#endif
   return buffer;
}

test$case(test_orig_arr)
{
    int i, j;

    for (i = 0; i < 20000; i += 50) {
        arr$(int) arr = arr$new(arr, 10, allocator);

        for (j = 0; j < i; ++j) {
            arr$push(arr, j);
        }
        arr$free(arr);
        tassert(arr == NULL);
    }

    for (i = 0; i < 4; ++i) {
        arr$(int) arr = arr$new(arr, 10, allocator);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$del(arr, i);
        arr$free(arr);
        tassert(arr == NULL);

        arr = arr$new(arr, 10, allocator);
        tassert(arr != NULL);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$delswap(arr, i);
        arr$free(arr);
        tassert(arr == NULL);
    }

    for (i = 0; i < 5; ++i) {
        arr$(int) arr = arr$new(arr, 10, allocator);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$ins(arr, i, 5);
        tassert(arr[i] == 5);
        if (i < 4) {
            tassert(arr[4] == 4);
        }
        arr$free(arr);
        tassert(arr == NULL);
    }

    return EOK;
}

test$case(test_orig_hashmap)
{
    int i = 1;
    const int testsize = 100000;
    hm$(int, int) intmap = hm$new(intmap, 10, allocator);
    hm$(stbds_struct, int) map = hm$new(map, 10, allocator);
    tassert(intmap != NULL);

    ptrdiff_t temp;

    tassert(hm$geti(intmap, i) == -1);
    hm$default(intmap, -2);
    tassert(hm$geti(intmap, i) == -1);
    tassert(hm$get(intmap, i) == -2);
    for (i = 0; i < testsize; i += 2) {
        hm$put(intmap, i, i * 5);
    }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 5);
        }
        if (i & 1) {
            tassert(hm$get_ts(intmap, i, temp) == -2);
        } else {
            tassert(hm$get_ts(intmap, i, temp) == i * 5);
        }
    }
    for (i = 0; i < testsize; i += 2) {
        hm$put(intmap, i, i * 3);
    }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }
    for (i = 2; i < testsize; i += 4) {
        (void)hm$del(intmap, i); // delete half the entries
    }
    for (i = 0; i < testsize; i += 1) {
        if (i & 3) {
            tassert(hm$get(intmap, i) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }
    for (i = 0; i < testsize; i += 1) {
        (void)hm$del(intmap, i); // delete the rest of the entries
    }
    for (i = 0; i < testsize; i += 1) {
        tassert(hm$get(intmap, i) == -2);
    }
    hm$free(intmap);
    tassert(intmap == NULL);

    intmap = hm$new(intmap, 10, allocator);

    for (i = 0; i < testsize; i += 2) {
        hm$put(intmap, i, i * 3);
    }
    hm$free(intmap);

    intmap = hm$new(intmap, 10, allocator);
    hm$put(intmap, 15, 7);
    hm$put(intmap, 11, 3);
    hm$put(intmap, 9, 5);
    tassert(hm$get(intmap, 9) == 5);
    tassert(hm$get(intmap, 11) == 3);
    tassert(hm$get(intmap, 15) == 7);
    hm$free(intmap);

    intmap = hm$new(intmap, 10, allocator);
    for (i = 0; i < testsize; i += 2) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        hm$put(map, s, i * 5);
        tassert(map != NULL);
    }
    hm$free(intmap);

    for (i = 0; i < testsize; i += 1) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        // stbds_struct t = { i, i * 2, i * 3 + 1, i * 4 };
        if (i & 1) {
            tassert(hm$get(map, s) == 0);
        } else {
            tassert(hm$get(map, s) == i * 5);
        }
        if (i & 1) {
            tassert(hm$get_ts(map, s, temp) == 0);
        } else {
            tassert(hm$get_ts(map, s, temp) == i * 5);
        }
        // tassert(hm$get(map, t.key) == 0);
    }

    stbds_struct* map2 = hm$new(map2, 10, allocator);
    for (i = 0; i < testsize; i += 2) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        hm$puts(map2, s);
    }
    hm$free(map2);
    hm$free(map);
    //
    // for (i = 0; i < testsize; i += 1) {
    //     stbds_struct s = { i, i * 2, i * 3, i * 4 };
    //     // stbds_struct t = { i, i * 2, i * 3 + 1, i * 4 };
    //     if (i & 1) {
    //         tassert(hm$gets(map2, s.key).d == 0);
    //     } else {
    //         tassert(hm$gets(map2, s.key).d == i * 4);
    //     }
    //     // tassert(hm$getp(map2, t.key) == 0);
    // }
    // hm$free(map2);
    //
    // for (i = 0; i < testsize; i += 2) {
    //     stbds_struct2 s = { { i, i * 2 }, i * 3, i * 4, i * 5 };
    //     hm$puts(map3, s);
    // }
    // for (i = 0; i < testsize; i += 1) {
    //     stbds_struct2 s = { { i, i * 2 }, i * 3, i * 4, i * 5 };
    //     // stbds_struct2 t = { { i, i * 2 }, i * 3 + 1, i * 4, i * 5 };
    //     if (i & 1) {
    //         tassert(hm$gets(map3, s.key).d == 0);
    //     } else {
    //         tassert(hm$gets(map3, s.key).d == i * 5);
    //     }
    //     // tassert(hm$getp(map3, t.key) == 0);
    // }
    return EOK;
}

test$case(test_array_len_unified)
{
    char buf[10];
    u32 iarr[20];
    char buf_zero[0];
    arr$(char*) array = arr$new(array, 10, allocator);
    tassert_eqi(0, arr$len(array));

    arr$put(array, "foo");
    arr$put(array, "bar");
    arr$put(array, "baz");

    tassert_eqi(10, arr$len(buf));
    tassert_eqi(20, arr$len(iarr));
    tassert_eqi(3, arr$len(array));
    tassert_eqi(0, arr$len(buf_zero));
    u32 *p = &iarr[10];
    (void)p;
    // u32 *p = NULL;
    // tassert_eqi(3, arr$len(NULL));
    // tassert_eqi(3, arr$len(p));

    // u32* p2 = malloc(100);
    // tassert(p2 != NULL);
    // tassert_eqi(3, arr$len(p2));

    arr$free(array);
    return EOK;
}

test$case(test_for_arr)
{
    char buf[] = {'a', 'b', 'c'};
    char buf_zero[] = {};
    for$arr(v, buf_zero) {
        (void)v;
        tassert(false && "must not happen!");
    }
    arr$(char*) array = arr$new(array, 10, allocator);
    for$arr(v, array) {
        (void)v;
        tassert(false && "must not happen!");
    }
    arr$put(array, "foo");
    arr$put(array, "bar");
    arr$put(array, "baz");

    tassert_eqi(3, arr$len(buf));
    tassert_eqi(3, arr$len(array));

    for$arr(v, array) {
        printf("v: %s\n", v);
    }

    typeof(buf[0])* pbuf = &buf[0];
    typeof(array[0])* pbuf2 = &array[0];

    tassert_eqi(pbuf[0], 'a');
    tassert_eqs(pbuf2[0], "foo");

    for$arr(c, buf) {
        printf("c: %c\n", c);
    }

    int j = 0; 
    int cnt = 3; 
    for(int i; j < cnt && set_true(i, j); j++) {
        printf("i: %d\n", i);
    }

    for$arrp(v, array) {
        printf("v: %s\n", *v);
    }

    arr$free(array);

    // TODO: check if possible for$arr(*v, &arr)??
    // for$arr(c, &buf) {
    //     printf("c: %c\n", *c);
    // }
    // typeof((&buf)[0])* pbuf_ptr = &((&buf)[0]);
    // // typeof((&buf)[0]) p = pbuf_ptr[0]; 
    // var p = pbuf_ptr[0]; 
    // tassert_eqi(*p, 'a');

    // typeof((&array)[0])* array_ptr = &((&array)[0]);
    // typeof((&array)[0]) p_array = array_ptr[0]; 
    // char*** array_ptr = &array;
    // char** p_array = array_ptr[0]; 
    // tassert_eqs(*p_array, "foo");
    // printf("p_array: %s\n", *p_array);

    return EOK;
}

test$case(test_slice)
{
    char buf[] = {'a', 'b', 'c'};

    var b = arr$slice(buf, 1);
    tassert_eqi(b.arr[0], 'b');
    tassert_eqi(b.len, 2);

    arr$(int) array = arr$new(array, 10, allocator);
    arr$put(array, 1);
    arr$put(array, 2);
    arr$put(array, 3);

    var i = arr$slice(array, 1);
    tassert_eqi(i.arr[0], 2);
    tassert_eqi(i.len, 2);
    arr$free(array);
    return EOK;
}

test$case(test_for_arr_custom_size)
{

    arr$(char*) array = arr$new(array, 10, allocator);
    for$arr(v, array) {
        (void)v;
        tassert(false && "must not happen!");
    }
    arr$put(array, "foo");
    arr$put(array, "bar");
    arr$put(array, "baz");

    u32 n = 0;
    tassert_eqi(3, arr$len(array));
    for$arr(v, array, arr$len(array)-1) {
        printf("v: %s\n", v);
        n++;
    }
    tassert_eqi(n, 2);

    char buf[] = {'a', 'b', 'c'};
    n = 0;
    tassert_eqi(3, arr$len(buf));
    for$arr(c, buf, 2) {
        printf("c: %c\n", c);
        n++;
    }
    tassert_eqi(n, 2);

    arr$free(array);
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_array);
    test$run(test_array_char_ptr);
    test$run(test_array_struct);
    test$run(test_orig_arr);
    test$run(test_orig_hashmap);
    test$run(test_array_len_unified);
    test$run(test_for_arr);
    test$run(test_slice);
    test$run(test_for_arr_custom_size);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
