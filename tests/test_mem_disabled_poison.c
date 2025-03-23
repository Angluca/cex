#define CEX_DISABLE_POISON 1
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

test$case(test_poison_totally_disabled)
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
    
    test$run(test_poison_totally_disabled);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
