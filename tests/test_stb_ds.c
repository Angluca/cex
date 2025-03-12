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


static void
add_to_arr(arr$(int) * arr)
{
    // arrput(*arr, 2);
    // arrput(*arr, 3);
    // arrput(*arr, 5);
    int z = 200;
    arr$pushm(*arr, z, 3, 5);
}

static void
add_to_str(arr$(const char*) * arr)
{
    char buf[10] = { "buf" };

    arr$push(*arr, "foooo");
    arr$push(*arr, "barrr");
    arr$push(*arr, "bazzz");
    arr$pushm(*arr, "one", "two", "three", buf);
}

test$case(test_array)
{
    arr$(const char*) array2 = arr$new(array2, allocator);
    arr$(int) array = arr$new(array, allocator);
    tassert(array != NULL);
    tassert_eqi(arr$cap(array), 16);


    add_to_arr(&array);
    add_to_str(&array2);
    for (usize i = 0; i < arr$len(array); ++i) {
        printf("%d \n", array[i]);
    }

    for (usize i = 0; i < arr$len(array2); ++i) {
        printf("%s \n", array2[i]);
    }
    arr$free(array);
    arr$free(array2);

    return EOK;
}

test$case(test_array_char_ptr)
{
    arr$(char*) array = arr$new(array,  allocator);
    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$push(array, "baz");
    for (usize i = 0; i < arr$len(array); ++i) {
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
    arr$(my_struct) array = arr$new(array,  allocator, .capacity = 128);
    tassert_eqi(arr$cap(array), 128);

    my_struct s;
    s = (my_struct){ 20, 5.0, "hello " };
    arr$push(array, s);
    s = (my_struct){ 40, 2.5, "failure" };
    arr$push(array, s);
    s = (my_struct){ 40, 1.1, "world!" };
    arr$push(array, s);

    for (usize i = 0; i < arr$len(array); ++i) {
        printf("key: %d str: %s\n", array[i].key, array[i].my_string);
    }
    arr$free(array);

    return EOK;
}


typedef struct
{
    int key, b, c, d;
} stbds_struct;
typedef struct
{
    int key[2], b, c, d;
} stbds_struct2;

static char buffer[256];
char*
strkey(int n)
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
        arr$(int) arr = arr$new(arr,  allocator);

        for (j = 0; j < i; ++j) {
            arr$push(arr, j);
        }
        arr$free(arr);
        tassert(arr == NULL);
    }

    for (i = 0; i < 4; ++i) {
        arr$(int) arr = arr$new(arr,  allocator);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$del(arr, i);
        arr$free(arr);
        tassert(arr == NULL);

        arr = arr$new(arr,  allocator);
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
        arr$(int) arr = arr$new(arr,  allocator);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$ins(arr, i, 5);
        tassert(arr[i] == 5);
        tassert_eqi(arr$len(arr), 5);
        if (i < 4) {
            tassert_eqi(arr[4], 4);
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
    hm$(int, int) intmap = hm$new(intmap,  allocator);
    hm$(stbds_struct, int) map = hm$new(map,  allocator);
    tassert(intmap != NULL);

    ptrdiff_t temp;

    tassert(hm$geti(intmap, i) == -1);
    hm$default(intmap, -2);
    tassert(hm$geti(intmap, i) == -1);
    tassert(hm$get(intmap, i) == -2);
    for (i = 0; i < testsize; i += 2) {
        hm$set(intmap, i, i * 5);
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
        hm$set(intmap, i, i * 3);
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

    intmap = hm$new(intmap,  allocator);

    for (i = 0; i < testsize; i += 2) {
        hm$set(intmap, i, i * 3);
    }
    hm$free(intmap);

    intmap = hm$new(intmap,  allocator);
    hm$set(intmap, 15, 7);
    hm$set(intmap, 11, 3);
    hm$set(intmap, 9, 5);
    tassert(hm$get(intmap, 9) == 5);
    tassert(hm$get(intmap, 11) == 3);
    tassert(hm$get(intmap, 15) == 7);
    hm$free(intmap);

    intmap = hm$new(intmap,  allocator);
    for (i = 0; i < testsize; i += 2) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        hm$set(map, s, i * 5);
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

    stbds_struct* map2 = hm$new(map2,  allocator);
    for (i = 0; i < testsize; i += 2) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        hm$sets(map2, s);
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
    //     hm$sets(map3, s);
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
    arr$(char*) array = arr$new(array,  allocator);
    tassert_eqi(0, arr$len(array));
    tassert(mem$aligned_pointer(array, 64) == array);

    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$push(array, "baz");

    tassert_eqi(10, arr$len(buf));
    tassert_eqi(20, arr$len(iarr));
    tassert_eqi(3, arr$len(array));
    tassert_eqi(0, arr$len(buf_zero));
    u32* p = &iarr[10];
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
    char buf[] = { 'a', 'b', 'c' };
    char buf_zero[] = {};
    for$arr(v, buf_zero)
    {
        (void)v;
        tassert(false && "must not happen!");
    }
    arr$(char*) array = arr$new(array,  allocator);
    tassert(mem$aligned_pointer(array, 64) == array);
    for$arr(v, array)
    {
        (void)v;
        tassert(false && "must not happen!");
    }
    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$push(array, "baz");

    tassert_eqi(3, arr$len(buf));
    tassert_eqi(3, arr$len(array));

    for$arr(v, array)
    {
        printf("v: %s\n", v);
    }

    typeof(buf[0])* pbuf = &buf[0];
    typeof(array[0])* pbuf2 = &array[0];

    tassert_eqi(pbuf[0], 'a');
    tassert_eqs(pbuf2[0], "foo");

    for$arr(c, buf)
    {
        printf("c: %c\n", c);
    }

    u32 n = 0;
    for$arrp(v, array)
    {
        isize i = v - array;
        tassert_eqi(n, i);
        printf("v: %s, i: %ld\n", *v, i);
        n++;
    }

    n = 0;
    for$arrp(v, buf)
    {
        isize i = v - buf;
        tassert_eqi(n, i);
        printf("v: %c, i: %ld\n", *v, i);
        n++;
    }

    // TODO: check if possible for$arr(*v, &arr)??
    // for$arr(c, &buf) {
    //     printf("c: %c\n", *c);
    // }
    typeof((&buf)[0])* pbuf_ptr = &((&buf)[0]);
    // typeof((&buf)[0]) p = pbuf_ptr[0];
    var p = pbuf_ptr[0];
    tassert_eqi(*p, 'a');

    typeof((array)[0])* array_ptr = &((array)[0]);
    // typeof((&array)[0]) p_array = array_ptr[0];
    var p_array = array_ptr;
    // tassert_eqs(*p_array, "foo");
    printf("p_array: %p\n", p_array);

    arr$free(array);
    return EOK;
}

test$case(test_slice)
{
    char buf[] = { 'a', 'b', 'c' };

    var b = arr$slice(buf, 1);
    tassert_eqi(b.arr[0], 'b');
    tassert_eqi(b.len, 2);

    arr$(int) array = arr$new(array,  allocator);
    arr$push(array, 1);
    arr$push(array, 2);
    arr$push(array, 3);

    var i = arr$slice(array, 1);
    tassert_eqi(i.arr[0], 2);
    tassert_eqi(i.len, 2);
    arr$free(array);
    return EOK;
}

test$case(test_for_arr_custom_size)
{

    arr$(char*) array = arr$new(array,  allocator);
    for$arr(v, array)
    {
        (void)v;
        tassert(false && "must not happen!");
    }
    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$push(array, "baz");

    u32 n = 0;
    tassert_eqi(3, arr$len(array));
    for$arr(v, array, arr$len(array) - 1)
    {
        printf("v: %s\n", v);
        n++;
    }
    tassert_eqi(n, 2);

    char buf[] = { 'a', 'b', 'c' };
    n = 0;
    tassert_eqi(3, arr$len(buf));
    for$arr(c, buf, 2)
    {
        printf("c: %c\n", c);
        n++;
    }
    tassert_eqi(n, 2);

    arr$free(array);
    return EOK;
}

test$case(test_for_arr_for_struct)
{
    arr$(my_struct) array = arr$new(array, allocator);
    tassert_eqi(arr$cap(array), 16);
    for$arr(v, array)
    {
        (void)v;
        tassert(false && "must not happen!");
    }
    my_struct f = { .my_string = "far", .key = 200 };
    arr$push(array, (my_struct){ .my_string = "foo", .key = 200, .my_val = 31.2 });
    arr$push(array, f);
    tassert_eqi(2, arr$len(array));

    u32 n = 0;
    // v - is a copy of the array element
    for$arr(v, array)
    {
        printf("v: %s\n", v.my_string);
        n++;
        v.key = 77;
    }
    tassert_eqi(n, 2);
    // NOT changed
    tassert_eqi(array[0].key, 200);
    tassert_eqi(array[1].key, 200);

    n = 0;
    for$arrp(v, array, arr$len(array))
    {
        isize i = v - array;
        printf("v: %s\n", v->my_string);
        tassert_eqi(n, i);
        n++;
        v->key = 77;
    }
    tassert_eqi(n, 2);
    // array has been changed by pointer
    tassert_eqi(array[0].key, 77);
    tassert_eqi(array[1].key, 77);
    arr$free(array);
    return EOK;
}

test$case(test_arr_insert_pop)
{
    arr$(int) arr = arr$new(arr,  allocator);
    arr$ins(arr, 0, 5);
    arr$ins(arr, 0, 4);
    arr$ins(arr, 0, 3);
    arr$ins(arr, 0, 2);
    arr$ins(arr, 0, 1);

    tassert_eqi(arr$len(arr), 5);
    tassert_eqi(arr$at(arr, 0), 1);
    tassert_eqi(arr$at(arr, 1), 2);
    tassert_eqi(arr$at(arr, 2), 3);
    tassert_eqi(arr$at(arr, 3), 4);
    tassert_eqi(arr$at(arr, 4), 5);

    tassert_eqi(arr$pop(arr), 5);
    tassert_eqi(arr$pop(arr), 4);
    tassert_eqi(arr$pop(arr), 3);
    tassert_eqi(arr$pop(arr), 2);
    tassert_eqi(arr$pop(arr), 1);

    tassert_eqi(arr$len(arr), 0);

    arr$free(arr);
    return EOK;
}

test$case(test_arr_pushm)
{
    arr$(int) arr = arr$new(arr,  allocator);
    arr$(int) arr2 = arr$new(arr,  allocator);
    tassert(mem$aligned_pointer(arr, 64) == arr);

    arr$pushm(arr2, 10, 6, 7, 8, 9, 0);
    tassert_eqi(arr$len(arr2), 6);

    int sarr[] = { 4, 5 };

    for (u32 i = 0; i < 100; i++) {
        arr$pushm(arr, 1, 2, 3);
        arr$pusha(arr, sarr);
        // NOTE: overriding arr2 size + using it as a raw pointer
        // arr$pusha(arr, &arr2[1]); // THIS IS WRONG, and fail sanitize/arr$validate - needs len!

        // We may use any pointer as an array, but length become mandatory
        arr$pusha(arr, &arr2[1], 5);
    }
    tassert_eqi(1000, arr$len(arr));

    for$arr(v, arr)
    {
        tassert(v < 10);
    }

    arr$free(arr);
    arr$free(arr2);
    return EOK;
}

test$case(test_overaligned_struct)
{
    struct test32_s
    {
        alignas(32) usize s;
    };

    arr$(struct test32_s) arr = arr$new(arr,  allocator);
    struct test32_s f = { .s = 100 };
    tassert(mem$aligned_pointer(arr, 64) == arr);

    for (u32 i = 0; i < 1000; i++) {
        f.s = i;
        arr$push(arr, f);

        tassert_eqi(arr$len(arr), i + 1);
        tassert_eqi(((size_t*)arr)[-1], i + 1);
    }
    tassert_eqi(arr$len(arr), 1000);
    // size is stored -sizeof(size_t) bytes from arr address
    tassert_eqi(((size_t*)arr)[-1], 1000);

    for (u32 i = 0; i < 1000; i++) {
        tassert_eqi(arr[i].s, i);
        tassert(mem$aligned_pointer(&arr[i], 32) == &arr[i]);
    }

    arr$free(arr);
    return EOK;
}

test$case(test_overaligned_struct64)
{
    struct test64_s
    {
        alignas(64) usize s;
    };

    arr$(struct test64_s) arr = arr$new(arr,  allocator);
    struct test64_s f = { .s = 100 };
    tassert(mem$aligned_pointer(arr, 64) == arr);

    for (u32 i = 0; i < 1000; i++) {
        f.s = i;
        arr$push(arr, f);
    }
    tassert_eqi(arr$len(arr), 1000);
    // size is stored -sizeof(size_t) bytes from arr address
    tassert_eqi(((size_t*)arr)[-1], 1000);

    for (u32 i = 0; i < 1000; i++) {
        tassert_eqi(arr[i].s, i);
        tassert(mem$aligned_pointer(&arr[i], 64) == &arr[i]);
    }

    arr$free(arr);
    return EOK;
}

test$case(test_smallest_alignment)
{
    arr$(char) arr = arr$new(arr,  allocator);
    tassert(mem$aligned_pointer(arr, 64) == arr);

    for (u32 i = 0; i < 1000; i++) {
        arr$push(arr, i % 10);
    }
    tassert_eqi(arr$len(arr), 1000);
    // size is stored -sizeof(size_t) bytes from arr address
    tassert_eqi(((size_t*)arr)[-1], 1000);

    for (u32 i = 0; i < 1000; i++) {
        tassert_eqi(arr[i], i % 10);
        tassert(mem$aligned_pointer(&arr[i], 1) == &arr[i]);
    }

    arr$free(arr);
    return EOK;
}

test$case(test_hashmap_basic)
{
    hm$(int, int) intmap = hm$new(intmap,  allocator);
    hm$(i64, int) intmap64 = hm$new(intmap64,  allocator);
    tassert(intmap != NULL);
    tassert(intmap64 != NULL);

    hm$validate(intmap);
    tassert_eqi(hm$len(intmap), 0);

    hm$set(intmap, 1, 3);
    tassert_eqi(hm$len(intmap), 1);
    tassert_eqi(hm$get(intmap, 1), 3);

    // returns default (all zeros)
    tassert_eqi(hm$get(intmap, -1), 0);
    tassert_eqi(hm$get(intmap64, -1), 0);

    tassert_eqi(hm$len(intmap), 1);
    hm$del(intmap, 1);
    tassert_eqi(hm$len(intmap), 0);

    hm$default(intmap, 2);
    tassert_eqi(hm$get(intmap, -1), 2);



    hm$free(intmap);
    hm$free(intmap64);
    tassert(intmap == NULL);
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
    test$run(test_for_arr_for_struct);
    test$run(test_arr_insert_pop);
    test$run(test_arr_pushm);
    test$run(test_overaligned_struct);
    test$run(test_overaligned_struct64);
    test$run(test_smallest_alignment);
    test$run(test_hashmap_basic);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
