#ifndef NDEBUG
#    define NDEBUG
#endif
#include "src/all.c"

test$case(test_poison_by_cex)
{
    u8 buf[16] = { 0 };

    for$each (v, buf) { tassert(v == 0); }

    mem$asan_poison(buf, arr$len(buf));
    for$each (v, buf) { tassert(v == 0); }
    tassert(mem$asan_poison_check(buf, arr$len(buf)));
    buf[1] = 0;
    tassert(mem$asan_poison_check(buf, arr$len(buf)));

    mem$asan_poison(buf, arr$len(buf));
    tassert(mem$asan_poison_check(buf, arr$len(buf)));

    mem$asan_unpoison(buf, arr$len(buf));
    for$each (v, buf) { tassert(v == 0x0); }
    tassert(mem$asan_poison_check(buf, arr$len(buf)));


    return EOK;
}

test$main();
