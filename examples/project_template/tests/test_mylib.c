#include "include/cex/all.c"
#include <cex/test/test.h>
#include <cex/test/fff.h>
#include "lib/mylib.c"

IAllocator allocator;

test$teardown()
{
    allocator = AllocatorGeneric.destroy(); // this also nullifies allocator
    return EOK;
}

test$setup()
{
    uassert_enable(); // re-enable if you disabled it in some test case
    allocator = mem$;
    return EOK;
}

test$case(my_test_add)
{
    tassert_eq(4, mylib.add(1, 3));
    tassert_eq(30, mylib.add(1, 29));

    return EOK;
}

test$case(my_test_mul)
{
    // Oops, it looks like there is a bug in mylib.mul() ;) 
    tassert_eq(6, mylib.mul(2, 3));
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header(); // >>> all tests below

    test$run(my_test_add);

    test$print_footer(); // ^^^^^ all tests runs are above
    return test$exit_code();
}
