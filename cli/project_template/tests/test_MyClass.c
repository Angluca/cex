
#include "../include/MyClass.c"
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

test$case(my_test)
{
    MyClass_c* a;
    MyClass_c* b;
    tassert_eqe(EOK, MyClass.create(&a, 1, 2, allocator));
    tassert_eqe(EOK, MyClass.create(&b, 4, 9, allocator));

    tassert_eqe(EOK, MyClass.add(a, b));
    tassert_eqi(a->x, 1 + 4);
    tassert_eqi(a->y, 2 + 9);

    b->x = 0;
    tassert_eqe(Error.assert, MyClass.add(a, b));

    b->y = 0;
    tassert_eqe("y is zero", MyClass.add(a, b));


    MyClass.destroy(&a, allocator);
    MyClass.destroy(&b, allocator);

    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header(); // >>> all tests below

    test$run(my_test);

    test$print_footer(); // ^^^^^ all tests runs are above
    return test$exit_code();
}
