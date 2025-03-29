#define CEX_DISABLE_POISON 1
#include <cex/all.c>

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

test$main();
