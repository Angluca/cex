#include "../include/mylib.c"
#include <cex.c>

const Allocator_i* allocator;

test$teardown()
{
    allocator = allocators.heap.destroy(); // this also nullifies allocator
    return EOK;
}

test$setup()
{
    uassert_enable(); // re-enable if you disabled it in some test case
    allocator = allocators.heap.create();
    return EOK;
}

test$case(my_test_add)
{
    tassert_eqi(4, mylib.add(1, 3));
    tassert_eqi(30, mylib.add(1, 29));

    return EOK;
}

// TODO: uncomment this test, it will be automatically added
//       by CEX to the main() at the bottom of this file
// test$case(my_test_mul)
// {
//     tassert_eqi(6, mylib.mul(2, 3));
//     return EOK;
// }

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header(); // >>> all tests below

    test$run(my_test_add);

    test$print_footer(); // ^^^^^ all tests runs are above
    return test$exit_code();
}
