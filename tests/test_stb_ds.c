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
    arr$(const char*) array2 = NULL;
    arr$(int) array = { 0 };
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
    arr$(char*) array = NULL;
    arr$put(array, "foo");
    arr$put(array, "bar");
    arr$put(array, "baz");
    for (usize i = 0; i < arr$lenu(array); ++i) {
        printf("%s ", array[i]);
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
    arr$(my_struct) array = NULL;

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

test$case(test_orig_test)
{

    // const int testsize = 100000;
    arr$(int) arr = NULL;
    // const int testsize2 = testsize / 20;
    // struct
    // {
    //     int key;
    //     int value;
    // }* intmap = NULL;
    // struct
    // {
    //     char* key;
    //     int value;
    // } *strmap = NULL, s;
    // struct
    // {
    //     stbds_struct key;
    //     int value;
    // }* map = NULL;
    // stbds_struct* map2 = NULL;
    // stbds_struct2* map3 = NULL;
    // cexds_string_arena sa = { 0 };
    // int key3[2] = { 1, 2 };
    // ptrdiff_t temp;

    int i, j;

    tassert(arr$leni(arr) == 0);
    for (i = 0; i < 20000; i += 50) {
        for (j = 0; j < i; ++j) {
            arr$push(arr, j);
        }
        arr$free(arr);
    }

    for (i = 0; i < 4; ++i) {
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$del(arr, i);
        arr$free(arr);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$delswap(arr, i);
        arr$free(arr);
    }

    for (i = 0; i < 5; ++i) {
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
    }

    // i = 1;
    // tassert(cexds_hmgeti(intmap, i) == -1);
    // cexds_hmdefault(intmap, -2);
    // tassert(cexds_hmgeti(intmap, i) == -1);
    // tassert(cexds_hmget(intmap, i) == -2);
    // for (i = 0; i < testsize; i += 2) {
    //     cexds_hmput(intmap, i, i * 5);
    // }
    // for (i = 0; i < testsize; i += 1) {
    //     if (i & 1) {
    //         tassert(cexds_hmget(intmap, i) == -2);
    //     } else {
    //         tassert(cexds_hmget(intmap, i) == i * 5);
    //     }
    //     if (i & 1) {
    //         tassert(cexds_hmget_ts(intmap, i, temp) == -2);
    //     } else {
    //         tassert(cexds_hmget_ts(intmap, i, temp) == i * 5);
    //     }
    // }
    // for (i = 0; i < testsize; i += 2) {
    //     cexds_hmput(intmap, i, i * 3);
    // }
    // for (i = 0; i < testsize; i += 1) {
    //     if (i & 1) {
    //         tassert(cexds_hmget(intmap, i) == -2);
    //     } else {
    //         tassert(cexds_hmget(intmap, i) == i * 3);
    //     }
    // }
    // for (i = 2; i < testsize; i += 4) {
    //     cexds_hmdel(intmap, i); // delete half the entries
    // }
    // for (i = 0; i < testsize; i += 1) {
    //     if (i & 3) {
    //         tassert(cexds_hmget(intmap, i) == -2);
    //     } else {
    //         tassert(cexds_hmget(intmap, i) == i * 3);
    //     }
    // }
    // for (i = 0; i < testsize; i += 1) {
    //     cexds_hmdel(intmap, i); // delete the rest of the entries
    // }
    // for (i = 0; i < testsize; i += 1) {
    //     tassert(cexds_hmget(intmap, i) == -2);
    // }
    // cexds_hmfree(intmap);
    // for (i = 0; i < testsize; i += 2) {
    //     cexds_hmput(intmap, i, i * 3);
    // }
    // cexds_hmfree(intmap);
    //
    // intmap = NULL;
    // cexds_hmput(intmap, 15, 7);
    // cexds_hmput(intmap, 11, 3);
    // cexds_hmput(intmap, 9, 5);
    // tassert(cexds_hmget(intmap, 9) == 5);
    // tassert(cexds_hmget(intmap, 11) == 3);
    // tassert(cexds_hmget(intmap, 15) == 7);
    //
    // for (i = 0; i < testsize; i += 2) {
    //     stbds_struct s = { i, i * 2, i * 3, i * 4 };
    //     cexds_hmput(map, s, i * 5);
    // }
    //
    // for (i = 0; i < testsize; i += 1) {
    //     stbds_struct s = { i, i * 2, i * 3, i * 4 };
    //     // stbds_struct t = { i, i * 2, i * 3 + 1, i * 4 };
    //     if (i & 1) {
    //         tassert(cexds_hmget(map, s) == 0);
    //     } else {
    //         tassert(cexds_hmget(map, s) == i * 5);
    //     }
    //     if (i & 1) {
    //         tassert(cexds_hmget_ts(map, s, temp) == 0);
    //     } else {
    //         tassert(cexds_hmget_ts(map, s, temp) == i * 5);
    //     }
    //     // tassert(cexds_hmget(map, t.key) == 0);
    // }
    //
    // for (i = 0; i < testsize; i += 2) {
    //     stbds_struct s = { i, i * 2, i * 3, i * 4 };
    //     cexds_hmputs(map2, s);
    // }
    // cexds_hmfree(map);
    //
    // for (i = 0; i < testsize; i += 1) {
    //     stbds_struct s = { i, i * 2, i * 3, i * 4 };
    //     // stbds_struct t = { i, i * 2, i * 3 + 1, i * 4 };
    //     if (i & 1) {
    //         tassert(cexds_hmgets(map2, s.key).d == 0);
    //     } else {
    //         tassert(cexds_hmgets(map2, s.key).d == i * 4);
    //     }
    //     // tassert(cexds_hmgetp(map2, t.key) == 0);
    // }
    // cexds_hmfree(map2);
    //
    // for (i = 0; i < testsize; i += 2) {
    //     stbds_struct2 s = { { i, i * 2 }, i * 3, i * 4, i * 5 };
    //     cexds_hmputs(map3, s);
    // }
    // for (i = 0; i < testsize; i += 1) {
    //     stbds_struct2 s = { { i, i * 2 }, i * 3, i * 4, i * 5 };
    //     // stbds_struct2 t = { { i, i * 2 }, i * 3 + 1, i * 4, i * 5 };
    //     if (i & 1) {
    //         tassert(cexds_hmgets(map3, s.key).d == 0);
    //     } else {
    //         tassert(cexds_hmgets(map3, s.key).d == i * 5);
    //     }
    //     // tassert(cexds_hmgetp(map3, t.key) == 0);
    // }
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
    test$run(test_orig_test);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
