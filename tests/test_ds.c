#include "cex/all.h"
#include "cex/test.h"
#include <cex/all.c>
#include <stdio.h>

/*
 * SUITE INIT / SHUTDOWN
 */
test$teardown()
{
    return EOK;
}

test$setup()
{
    uassert_enable();
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
    arr$(const char*) array2 = arr$new(array2, mem$);
    arr$(int) array = arr$new(array, mem$);
    tassert(array != NULL);
    tassert_eqi(arr$cap(array), 16);

    int z = 200;
    int* zp = &z;
    size_t s = { z };
    size_t s2 = { (*zp) };
    tassert_eqi(s, 200);
    tassert_eqi(s2, 200);

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
    arr$(char*) array = arr$new(array, mem$);
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
    int value;
} my_struct;

test$case(test_array_struct)
{
    arr$(my_struct) array = arr$new(array, mem$, .capacity = 128);
    tassert_eqi(arr$cap(array), 128);

    my_struct s;
    s = (my_struct){ 20, 5.0, "hello ", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 2.5, "failure", 0 };
    arr$push(array, s);
    s = (my_struct){ 40, 1.1, "world!", 0 };
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
        arr$(int) arr = arr$new(arr, mem$);

        for (j = 0; j < i; ++j) {
            arr$push(arr, j);
        }
        arr$free(arr);
        tassert(arr == NULL);
    }

    for (i = 0; i < 4; ++i) {
        arr$(int) arr = arr$new(arr, mem$);
        arr$push(arr, 1);
        arr$push(arr, 2);
        arr$push(arr, 3);
        arr$push(arr, 4);
        arr$del(arr, i);
        arr$free(arr);
        tassert(arr == NULL);

        arr = arr$new(arr, mem$);
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
        arr$(int) arr = arr$new(arr, mem$);
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
    hm$(int, int) intmap = hm$new(intmap, mem$);
    hm$(stbds_struct, int) map = hm$new(map, mem$);
    tassert(intmap != NULL);

    tassert(hm$get(intmap, i) == 0);
    for (i = 0; i < testsize; i += 2) {
        hm$set(intmap, i, i * 5);
    }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i, -2) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 5);
        }
    }
    for (i = 0; i < testsize; i += 2) {
        hm$set(intmap, i, i * 3);
    }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i, -2) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }
    for (i = 2; i < testsize; i += 4) {
        (void)hm$del(intmap, i); // delete half the entries
    }
    for (i = 0; i < testsize; i += 1) {
        if (i & 3) {
            tassert(hm$get(intmap, i) == 0);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }
    for (i = 0; i < testsize; i += 1) {
        (void)hm$del(intmap, i); // delete the rest of the entries
    }
    for (i = 0; i < testsize; i += 1) {
        tassert(hm$get(intmap, i) == 0);
    }
    hm$free(intmap);
    tassert(intmap == NULL);

    intmap = hm$new(intmap, mem$);

    for (i = 0; i < testsize; i += 2) {
        hm$set(intmap, i, i * 3);
    }
    hm$free(intmap);

    intmap = hm$new(intmap, mem$);
    hm$set(intmap, 15, 7);
    hm$set(intmap, 11, 3);
    hm$set(intmap, 9, 5);
    tassert(hm$get(intmap, 9) == 5);
    tassert(hm$get(intmap, 11) == 3);
    tassert(hm$get(intmap, 15) == 7);
    hm$free(intmap);

    intmap = hm$new(intmap, mem$);
    for (i = 0; i < testsize; i += 2) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        hm$set(map, s, i * 5);
        tassert(map != NULL);
    }
    hm$free(intmap);

    for (i = 0; i < testsize; i += 1) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        if (i & 1) {
            tassert(hm$get(map, s) == 0);
        } else {
            tassert(hm$get(map, s) == i * 5);
        }
    }

    stbds_struct* map2 = hm$new(map2, mem$);
    for (i = 0; i < testsize; i += 2) {
        stbds_struct s = { i, i * 2, i * 3, i * 4 };
        hm$sets(map2, s);
    }
    hm$free(map2);
    hm$free(map);

    return EOK;
}

test$case(test_array_len_unified)
{
    char buf[10];
    u32 iarr[20];
    char buf_zero[0];
    arr$(char*) array = arr$new(array, mem$);
    tassert_eqi(0, arr$len(array));
    tassert(mem$aligned_pointer(array, 64) == array);

    arr$push(array, "foo");
    arr$push(array, "bar");
    arr$push(array, "baz");

    tassert_eqi(10, arr$len(buf));
    tassert_eqi(20, arr$len(iarr));
    tassert_eqi(3, arr$len(array));
    tassert_eqi(0, arr$len(buf_zero));
    u32* p = NULL;
    (void)p;
    // tassert_eqi(3, arr$len(NULL));
    // tassert_eqi(3, arr$len(p));

    // u32* p2 = malloc(100);
    // tassert(p2 != NULL);
    // tassert_eqi(3, arr$len(p2 + 10));

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
    arr$(char*) array = arr$new(array, mem$);
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

    arr$(int) array = arr$new(array, mem$);
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

    arr$(char*) array = arr$new(array, mem$);
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
    arr$(my_struct) array = arr$new(array, mem$);
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
    arr$(int) arr = arr$new(arr, mem$);
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
    arr$(int) arr = arr$new(arr, mem$);
    arr$(int) arr2 = arr$new(arr2, mem$);
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

    arr$(struct test32_s) arr = arr$new(arr, mem$);
    struct test32_s f = { .s = 100 };
    tassert(mem$aligned_pointer(arr, 64) == arr);

    for (u32 i = 0; i < 1000; i++) {
        f.s = i;
        arr$push(arr, f);

        tassert_eqi(arr$len(arr), i + 1);
        tassert_eqi(((size_t*)arr)[-2], i + 1);
    }
    tassert_eqi(arr$len(arr), 1000);
    // size is stored -sizeof(size_t) bytes from arr address
    tassert_eqi(((size_t*)arr)[-2], 1000);

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

    arr$(struct test64_s) arr = arr$new(arr, mem$);
    struct test64_s f = { .s = 100 };
    tassert(mem$aligned_pointer(arr, 64) == arr);

    for (u32 i = 0; i < 1000; i++) {
        f.s = i;
        arr$push(arr, f);
    }
    tassert_eqi(arr$len(arr), 1000);
    // size is stored -sizeof(size_t) bytes from arr address
    tassert_eqi(((size_t*)arr)[-2], 1000);

    for (u32 i = 0; i < 1000; i++) {
        tassert_eqi(arr[i].s, i);
        tassert(mem$aligned_pointer(&arr[i], 64) == &arr[i]);
    }

    arr$free(arr);
    return EOK;
}

test$case(test_smallest_alignment)
{
    arr$(char) arr;
    tassert(arr$new(arr, mem$) != NULL);
    tassert(mem$aligned_pointer(arr, 64) == arr);

    for (u32 i = 0; i < 1000; i++) {
        arr$push(arr, i % 10);
    }
    tassert_eqi(arr$len(arr), 1000);
    // size is stored -sizeof(size_t) bytes from arr address
    tassert_eqi(((size_t*)arr)[-2], 1000);

    for (u32 i = 0; i < 1000; i++) {
        tassert_eqi(arr[i], i % 10);
        tassert(mem$aligned_pointer(&arr[i], 1) == &arr[i]);
    }

    arr$free(arr);
    return EOK;
}

test$case(test_hashmap_basic)
{
    hm$(int, int) intmap;
    hm$new(intmap, mem$);

    hm$(i64, int) intmap64 = hm$new(intmap64, mem$);
    tassert(intmap != NULL);
    tassert(intmap64 != NULL);

    tassert_eqi(hm$len(intmap), 0);
    // tassert_eqi(arr$len(intmap), 0);
    int _v = { 1 };

    tassert(hm$set(intmap, 1, 3));
    tassert(hm$set(intmap, 100, _v));
    tassert(hm$set(intmap, 1, 3));

    tassert_eqi(hm$len(intmap), 2);
    tassert_eqi(hm$get(intmap, 1), 3);
    tassert_eqi(*hm$getp(intmap, 1), 3);


    // returns default (all zeros)
    tassert_eqi(hm$get(intmap, -1), 0);
    tassert_eqi(hm$get(intmap64, -1, 999), 999);
    tassert(hm$getp(intmap, -1) == NULL);

    tassert_eqi(hm$len(intmap), 2);
    hm$del(intmap, 1);
    hm$del(intmap, 100);
    tassert_eqi(hm$len(intmap), 0);

    var h = cexds_header(intmap);
    tassert(h->hm_seed != 0);

    hm$free(intmap);
    hm$free(intmap64);
    tassert(intmap == NULL);
    return EOK;
}

test$case(test_hashmap_basic_struct)
{
    struct test64_s
    {
        usize foo;
        usize bar;
    };

    hm$(int, struct test64_s) intmap = hm$new(intmap, mem$);
    tassert(intmap != NULL);

    tassert_eqi(hm$len(intmap), 0);

    struct test64_s s2 = (struct test64_s){ .foo = 2 };
    tassert(hm$set(intmap, 1, (struct test64_s){ .foo = 1 }));
    tassert(hm$set(intmap, 2, s2));
    // tassert(hm$set(intmap, 3, (struct test64_s){3, 4}));

    var s3 = hm$setp(intmap, 3);
    tassert(s3 != NULL);
    s3->foo = 3;
    s3->bar = 4;

    tassert_eqi(hm$len(intmap), 3);
    tassert_eqi(arr$len(intmap), 3);

    tassert(hm$getp(intmap, -1) == NULL);
    tassert(hm$getp(intmap, 1) != NULL);


    tassert_eqi(hm$get(intmap, 1).foo, 1);
    tassert_eqi(hm$get(intmap, 2).foo, 2);
    tassert_eqi(hm$get(intmap, 3).foo, 3);

    tassert_eqi(hm$getp(intmap, 1)[0].foo, 1);
    tassert_eqi(hm$getp(intmap, 2)[0].foo, 2);
    tassert_eqi(hm$getp(intmap, 3)[0].foo, 3);

    u32 n = 0;
    for$arr(v, intmap)
    {
        tassert_eqi(intmap[n].key, v.key);
        // hasmap also supports bounds checked arr$at
        tassert_eqi(arr$at(intmap, n).value.foo, v.value.foo);
        tassert_eqi(v.key, n + 1);
        n++;
    }
    tassert_eqi(n, arr$len(intmap));

    n = 0;
    for$arrp(v, intmap)
    {
        tassert_eqi(intmap[n].key, v->key);
        tassert_eqi(intmap[n].value.foo, v->value.foo);
        tassert_eqi(v->key, n + 1);
        n++;
    }
    tassert_eqi(n, arr$len(intmap));

    // ZII struct if not found
    tassert_eqi(hm$get(intmap, -1).foo, 0);
    tassert_eqi(hm$get(intmap, -1).bar, 0);
    tassert_eqi(hm$get(intmap, -1, s2).foo, 2);

    hm$free(intmap);
    return EOK;
}

test$case(test_hashmap_struct_full_setget)
{
    struct test64_s
    {
        usize fooa; // WARNING: key will be second to test key field offset
        usize key;
    };

    hm$s(struct test64_s) smap = hm$new(smap, mem$);
    tassert(smap != NULL);

    _Static_assert(offsetof(struct test64_s, key) == sizeof(usize), "unexp");

    tassert_eqi(hm$len(smap), 0);

    tassert(hm$sets(smap, (struct test64_s){ .key = 1, .fooa = 10 }));
    tassert_eqi(hm$len(smap), 1);
    tassert_eqi(smap[0].key, 1);
    tassert_eqi(smap[0].fooa, 10);

    struct test64_s* r = hm$gets(smap, 1);
    tassert(r != NULL);
    tassert(r == &smap[0]);
    tassert_eqi(r->key, 1);
    tassert_eqi(r->fooa, 10);

    struct test64_s s2 = { .key = 2, .fooa = 200 };
    tassert(hm$sets(smap, s2));
    tassert_eqi(hm$len(smap), 2);

    r = hm$gets(smap, 1);
    tassert(r != NULL);
    tassert_eqi(r->key, 1);
    tassert_eqi(r->fooa, 10);

    r = hm$gets(smap, 2);
    tassert(r != NULL);
    tassert_eqi(r->key, 2);
    tassert_eqi(r->fooa, 200);

    tassert(hm$gets(smap, -1) == NULL);

    tassert(hm$del(smap, 2));
    tassert(hm$del(smap, 1));
    tassert_eqi(hm$len(smap), 0);
    tassert(hm$gets(smap, 1) == NULL);
    tassert(hm$gets(smap, 2) == NULL);

    hm$free(smap);
    return EOK;
}

test$case(test_hashmap_keytype)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);
    hm$(i64, int) intmap64 = hm$new(intmap64, mem$);
    hm$(char, int) map1 = hm$new(map1, mem$);
    hm$(char*, int) map2 = hm$new(map2, mem$);
    hm$(const char*, int) map3 = hm$new(map3, mem$);

    struct
    {
        char key[10];
        int value;
    }* map4 = hm$new(map4, mem$);

    hm$(str_c, int) map5 = hm$new(map5, mem$);

    tassert_eqi(cexds_header(intmap)->hm_key_type, _CexDsKeyType__generic);
    tassert_eqi(cexds_header(intmap64)->hm_key_type, _CexDsKeyType__generic);
    tassert_eqi(cexds_header(map1)->hm_key_type, _CexDsKeyType__generic);
    tassert_eqi(cexds_header(map2)->hm_key_type, _CexDsKeyType__charptr);
    tassert_eqi(cexds_header(map3)->hm_key_type, _CexDsKeyType__charptr);
    tassert_eqi(cexds_header(map4)->hm_key_type, _CexDsKeyType__charbuf);
    tassert_eqi(cexds_header(map5)->hm_key_type, _CexDsKeyType__cexstr);

    hm$free(intmap);
    hm$free(intmap64);
    hm$free(map1);
    hm$free(map2);
    hm$free(map3);
    hm$free(map4);
    hm$free(map5);

    return EOK;
}

test$case(test_hashmap_hash)
{
    // We created the same string, but stored in different places
    const char* key = "foobar";
    char key_buf[10] = "foobar";
    char key_buf2[6] = "foobar";
    str_c key_str = str.cbuf(key_buf2, sizeof(key_buf2));

    // Make sure pointers are different
    tassert(str.cmp(key_str, str$("foobar")) == 0);
    tassert_eqi(sizeof(key_buf2), 6);
    tassert(key_str.buf != key);
    tassert(key_buf != key);
    tassert(key_buf != key_str.buf);

    // mimic how hm$get macro produces initial pointers
    const char* keyvar[1] = { key };

    size_t seed = 27361;
    size_t hash_key = cexds_hash(_CexDsKeyType__charptr, keyvar, 10000, seed);
    tassert(hash_key > 0);

    tassert_eqi(cexds_hash(_CexDsKeyType__charbuf, key_buf, sizeof(key_buf), seed), hash_key);
    tassert_eqi(cexds_hash(_CexDsKeyType__cexstr, &key_str, sizeof(key_str), seed), hash_key);
    // NOTE: This case is non \0 terminated buffer!
    tassert_eqi(cexds_hash(_CexDsKeyType__charbuf, key_buf2, sizeof(key_buf2), seed), hash_key);

    return EOK;
}

test$case(test_hashmap_compare_strings_bounded)
{
    // We created the same string, but stored in different places
    char key_buf[10] = "foobar";
    char key_buf2[6] = "foobar";
    char key_buf3[6] = "foobar";
    char key_buf4[5] = "fooba";
    char key_buf5[] = "";
    tassert_eqi(sizeof(key_buf2), 6);
    tassert_eqi(sizeof(key_buf), 10);
    tassert(cexds_compare_strings_bounded(key_buf, key_buf2, sizeof(key_buf), sizeof(key_buf2)));
    tassert(cexds_compare_strings_bounded(key_buf2, key_buf, sizeof(key_buf2), sizeof(key_buf)));

    tassert(cexds_compare_strings_bounded(key_buf2, key_buf3, sizeof(key_buf2), sizeof(key_buf3)));
    tassert(!cexds_compare_strings_bounded(key_buf3, key_buf4, sizeof(key_buf3), sizeof(key_buf4)));
    tassert(!cexds_compare_strings_bounded(key_buf3, key_buf5, sizeof(key_buf3), sizeof(key_buf5)));
    tassert(cexds_compare_strings_bounded(key_buf5, key_buf5, sizeof(key_buf5), sizeof(key_buf5)));
    return EOK;
}

test$case(test_hashmap_string)
{
    char key_buf[10] = "foobar";

    hm$(const char*, int) smap = hm$new(smap, mem$);

    char* k = "foo";
    char* k2 = "baz";

    char key_buf2[10] = "foo";
    char* k3 = key_buf2;
    hm$set(smap, "foo", 3);
    tassert_eqi(hm$len(smap), 1);
    tassert_eqi(hm$get(smap, "foo"), 3);
    tassert_eqi(hm$get(smap, k), 3);
    tassert_eqi(hm$get(smap, key_buf2), 3);
    tassert_eqi(hm$get(smap, k3), 3);

    tassert_eqi(hm$get(smap, "bar"), 0);
    tassert_eqi(hm$get(smap, k2), 0);
    tassert_eqi(hm$get(smap, key_buf), 0);

    tassert_eqi(hm$del(smap, key_buf2), 1);
    tassert_eqi(hm$len(smap), 0);

    hm$free(smap);
    return EOK;
}

test$case(test_hashmap_bufkey)
{
    struct
    {
        char key[10];
        int value;
    }* smap = hm$new(smap, mem$);

    hm$set(smap, "foo", 3);
    // hm$set(smap, "foobarbazfoo", 4);

    // Doesn't work
    // char* f = "soo";
    // tassert(hm$set(smap, f, 9));

    tassert_eqi(hm$len(smap), 1);
    tassert_eqi(hm$get(smap, "foo"), 3);
    // tassert_eqi(hm$get(smap, "barbazfooo0"), 0);

    tassert_eqi(hm$del(smap, "foo"), 1);
    tassert_eqi(hm$len(smap), 0);
    hm$free(smap);
    return EOK;
}

#define _hm$test(t, k)                                                                             \
    ({                                                                                             \
        void* key = (typeof((t)->key)[1]){ k };                                                    \
        size_t key_len = 0;                                                                        \
        size_t key_size = sizeof((t)->key);                                                        \
        _Generic(                                                                                  \
            &((t)->key),                                                                           \
            char(**): _Generic(                                                                    \
                (k),                                                                               \
                char*: (key_len = 0),                                                              \
                str_c: (key = *(((size_t**)key) + 1)),                                             \
                default: (key = NULL)                                                              \
            ),                                                                                     \
            str_c*: (key_len = 0),                                                                 \
            default: (key_len = 0)                                                                 \
        );                                                                                         \
        printf("key: %p, key_len: %ld, key_size: %ld\n", key, key_len, key_size);                  \
    })

test$case(test_hashmap_cex_string)
{
    hm$(str_c, int) smap = hm$new(smap, mem$);

    char key_buf[10] = "bar";
    char key_buf2[10] = "baz";

    hm$set(smap, str$("foo"), 1);
    hm$set(smap, str.cstr(key_buf), 2);
    hm$set(smap, str.cstr(key_buf2), 3);

    tassert_eqi(hm$len(smap), 3);
    tassert_eqi(hm$get(smap, str$("foo")), 1);
    tassert_eqi(hm$get(smap, str.cstr("bar")), 2);
    tassert_eqi(hm$get(smap, str$("baz")), 3);

    tassert_eqi(hm$del(smap, str.cstr("foo")), 1);
    tassert_eqi(hm$del(smap, str$("bar")), 1);
    tassert_eqi(hm$del(smap, str$("baz")), 1);
    tassert_eqi(hm$len(smap), 0);

    hm$(int, int) imap;
    hm$(char*, int) cmap;
    (void)imap;

    char* c = "foobar";
    str_c s = (str_c){ .buf = c, .len = 6 };


    _hm$test(imap, 2);

    _hm$test(smap, str.cstr("bar"));
    _hm$test(smap, str$("bar"));
    _hm$test(smap, s);
    // _hm$test(smap, "foo");

    _hm$test(cmap, "foobar");
    _hm$test(cmap, c);
    // _hm$test(cmap, s);
    // _hm$test(cmap, str$("xxxyyyzzz").buf);


    hm$free(smap);
    return EOK;
}

test$case(test_hashmap_basic_delete)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);
    tassert(intmap != NULL);
    tassert_eqi(hm$del(intmap, -1), 0);

    tassert_eqi(hm$len(intmap), 0);

    tassert(hm$set(intmap, 1, 10));
    tassert(hm$set(intmap, 2, 20));
    tassert(hm$set(intmap, 3, 30));

    tassert_eqi(hm$len(intmap), 3);

    // check order
    tassert_eqi(intmap[0].key, 1);
    tassert_eqi(intmap[1].key, 2);
    tassert_eqi(intmap[2].key, 3);

    // not deleted/found
    tassert_eqi(hm$del(intmap, -1), 0);

    tassert_eqi(hm$del(intmap, 3), 1);
    tassert_eqi(2, hm$len(intmap));
    tassert_eqi(intmap[0].key, 1);
    tassert_eqi(intmap[1].key, 2);

    tassert_eqi(hm$del(intmap, 2), 1);
    tassert_eqi(1, hm$len(intmap));
    tassert_eqi(intmap[0].key, 1);

    tassert_eqi(hm$del(intmap, 1), 1);
    tassert_eqi(0, hm$len(intmap));
    // NOTE: this key is not nullified typically out-of-bounds access
    tassert_eqi(intmap[0].key, 1);

    tassert_eqi(hm$del(intmap, 1), 0);


    tassert(hm$set(intmap, 1, 10));
    tassert(hm$set(intmap, 2, 20));
    tassert(hm$set(intmap, 3, 30));
    tassert_eqi(hm$del(intmap, 1), 1);

    // NOTE: swap delete
    tassert_eqi(hm$len(intmap), 2);
    tassert_eqi(intmap[0].key, 3);
    tassert_eqi(intmap[1].key, 2);

    hm$free(intmap);
    return EOK;
}

test$case(test_hashmap_basic_clear)
{
    hm$(int, int) intmap = hm$new(intmap, mem$);
    tassert(intmap != NULL);

    tassert(hm$set(intmap, 1, 10));
    tassert(hm$set(intmap, 2, 20));
    tassert(hm$set(intmap, 3, 30));

    tassert_eqi(hm$len(intmap), 3);
    tassert(hm$getp(intmap, 1) != NULL);
    tassert(hm$getp(intmap, 2) != NULL);
    tassert(hm$getp(intmap, 3) != NULL);

    tassert(hm$clear(intmap));
    tassert_eqi(hm$len(intmap), 0);
    tassert(hm$getp(intmap, 1) == NULL);
    tassert(hm$getp(intmap, 2) == NULL);
    tassert(hm$getp(intmap, 3) == NULL);

    hm$free(intmap);
    return EOK;
}

test$case(test_orig_del_add_clear)
{
    int i = 1;
    const int testsize = 100000;
    hm$(int, int) intmap = hm$new(intmap, mem$);
    tassert(intmap != NULL);

    tassert(hm$get(intmap, i) == 0);
    for (i = 0; i < testsize; i += 2) {
        hm$set(intmap, i, i * 5);
    }
    hm$clear(intmap);

    for (i = 0; i < testsize; i += 2) {
        hm$set(intmap, i, i * 3);
    }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i, -2) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }
    for (i = 2; i < testsize; i += 4) {
        (void)hm$del(intmap, i); // delete half the entries
    }
    hm$clear(intmap);

    for (i = 0; i < testsize; i += 2) {
        hm$set(intmap, i, i * 3);
    }
    for (i = 0; i < testsize; i += 1) {
        if (i & 1) {
            tassert(hm$get(intmap, i, -2) == -2);
        } else {
            tassert(hm$get(intmap, i) == i * 3);
        }
    }

    hm$free(intmap);
    return EOK;
}

test$case(test_mem_scope_lifetime_test)
{
    // NOTE: no mem leak! Temp allocator auto cleanup!
    mem$scope(tmem$, ta)
    {
        arr$(u32) arr = arr$new(arr, ta);
        void* old_arr = arr;
        tassert(old_arr != NULL);
        tassert_eqi(arr$cap(arr), 16);
        for (u32 i = 0; i < 16; i++) {
            arr$push(arr, i);
        }
        tassert_eqi(arr$len(arr), 16);
        tassert_eqi(arr$cap(arr), 16);
        mem$scope(tmem$, ta)
        {
            uassert_disable();
            arr$pushm(arr, 170, 180, 190);
            // tassert(old_arr = arr);

            tassert_eqi(arr$len(arr), 19);
            tassert_eqi(arr$cap(arr), 32);

            tassert_eqi(arr[16], 170);
            tassert_eqi(arr[17], 180);
            tassert_eqi(arr[18], 190);
        }
        // WARNING: mem$scope exited => new allocation for arr$pushm marked as poisoned
        //          you'll trigger weird use-after-poison ASAN failure
        tassert(mem$asan_poison_check(&arr[18], sizeof(u32)*3));
    }
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
    test$run(test_hashmap_basic_struct);
    test$run(test_hashmap_struct_full_setget);
    test$run(test_hashmap_keytype);
    test$run(test_hashmap_hash);
    test$run(test_hashmap_compare_strings_bounded);
    test$run(test_hashmap_string);
    test$run(test_hashmap_bufkey);
    test$run(test_hashmap_cex_string);
    test$run(test_hashmap_basic_delete);
    test$run(test_hashmap_basic_clear);
    test$run(test_orig_del_add_clear);
    test$run(test_mem_scope_lifetime_test);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
