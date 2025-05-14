#define mem$asan_enabled() 0
#include "src/all.c"

test$case(test_poison_by_cex)
{
    u8 buf[16] = { 0 };

    for$each (v, buf) { tassert(v == 0); }

#ifndef NDEBUG
    // Poisoning only working when NDEBUG is not set
    mem$asan_poison(buf, arr$len(buf));
    for$each (v, buf) { tassert(v == 0xf7); }
    tassert(mem$asan_poison_check(buf, arr$len(buf)));
    buf[1] = 0;
    tassert(!mem$asan_poison_check(buf, arr$len(buf)));

    mem$asan_poison(buf, arr$len(buf));
    tassert(mem$asan_poison_check(buf, arr$len(buf)));

    mem$asan_unpoison(buf, arr$len(buf));
    for$each (v, buf) { tassert(v == 0x0); }
    tassert(!mem$asan_poison_check(buf, arr$len(buf)));
#else
    // NDEBUG is set, no poisoning
    mem$asan_poison(buf, arr$len(buf));
    for$each (v, buf) { tassert(v == 0); }
#endif

    return EOK;
}


test$main();
