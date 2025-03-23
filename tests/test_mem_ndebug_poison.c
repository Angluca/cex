#define NDEBUG
#include <cex/all.c>

/*
 * SUITE INIT / SHUTDOWN
 */
test$teardown()
{
    return EOK;
}

test$setup()
{
    return EOK;
}

test$case(test_poison_by_cex)
{
    u8 buf[16] = {0};

    for$each(v, buf){
        tassert(v == 0);
    }

    mem$asan_poison(buf, arr$len(buf));
    for$each(v, buf){
        tassert(v == 0);
    }
    tassert(mem$asan_poison_check(buf, arr$len(buf)));
    buf[1] = 0;
    tassert(mem$asan_poison_check(buf, arr$len(buf)));

    mem$asan_poison(buf, arr$len(buf));
    tassert(mem$asan_poison_check(buf, arr$len(buf)));

    mem$asan_unpoison(buf, arr$len(buf));
    for$each(v, buf){
        tassert(v == 0x0);
    }
    tassert(mem$asan_poison_check(buf, arr$len(buf)));

    
    return EOK;
}


/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_poison_by_cex);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
