#undef __SANITIZE_ADDRESS__ // disables sanitizer manual poison
#include <cex/all.c>
#include <cex/test.h>
#include <unistd.h>
#include <x86intrin.h>

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

test$case(test_poison_by_cex)
{
    u8 buf[16] = {0};

    for$arr(v, buf){
        tassert(v == 0);
    }

    mem$asan_poison(buf, arr$len(buf));
    for$arr(v, buf){
        tassert(v == 0xf7);
    }
    tassert(mem$asan_poison_check(buf, arr$len(buf)));
    buf[1] = 0;
    tassert(!mem$asan_poison_check(buf, arr$len(buf)));

    mem$asan_poison(buf, arr$len(buf));
    tassert(mem$asan_poison_check(buf, arr$len(buf)));

    mem$asan_unpoison(buf, arr$len(buf));
    for$arr(v, buf){
        tassert(v == 0x0);
    }
    tassert(!mem$asan_poison_check(buf, arr$len(buf)));

    
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
