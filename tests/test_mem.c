#include <cex/all.c>

test$case(test_is_power_of2)
{
    _Static_assert(!mem$is_power_of2(0), "fail");
    _Static_assert(mem$is_power_of2(1), "fail");
    _Static_assert(mem$is_power_of2(2), "fail");
    _Static_assert(mem$is_power_of2(64), "fail");
    _Static_assert(mem$is_power_of2(128), "fail");
    return EOK;
}

test$case(test_aligned_size)
{
    tassert_eqi(4, mem$aligned_round(3, 4));
    tassert_eqi(64, mem$aligned_round(3, 64));
    tassert_eqi(0, mem$aligned_round(0, 64));
    tassert_eqi(64, mem$aligned_round(1, 64));
    tassert_eqi(63, mem$aligned_round(63, 1));

    alignas(64) char buf[128];

    tassert(buf == mem$aligned_pointer(buf, 64));
    tassert(&buf[64] == mem$aligned_pointer(&buf[1], 64));
    
    return EOK;
}

test$main();
