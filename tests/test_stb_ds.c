#include "_generic_defs.h"
#include "cex/cex.h"
#include <cex/all.c>
#include <cex/list.c>
#include <cex/ds.c>
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

#define arr$(T) T*

#define arr$putm(arr, args...)                                                                      \
    ({                                                                                             \
        typeof(*arr) _args[] = { args };                                                           \
        for (usize i = 0; i < sizeof(_args) / sizeof(_args[0]); i++)                               \
            arr$put(arr, _args[i]);                                                                 \
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
// test$case(test_array_bad_ptr)
// {
//     int a = 0;
//     int* ap = &a;
//     arr$(int) array = NULL;
//     arrput(array, 2);
//     arrput(array, 3);
//     arrput(ap, 5);
//     for (int i = 0; i < arrlen(array); ++i) {
//         printf("%d ", array[i]);
//     }
//
//     return EOK;
// }

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_array);
    test$run(test_array_char_ptr);
    test$run(test_array_struct);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
