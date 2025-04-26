#include "src/all.c"

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
    tassert_eq(4, mem$aligned_round(3, 4));
    tassert_eq(64, mem$aligned_round(3, 64));
    tassert_eq(0, mem$aligned_round(0, 64));
    tassert_eq(64, mem$aligned_round(1, 64));
    tassert_eq(63, mem$aligned_round(63, 1));

    // alignas(64) char buf[127] = {0};
    char* buf = mem$malloc(mem$, 128, 64);

    log$debug("Initial buf addr: %p buf %%64: %zu\n", buf, ((u64)buf) % (u64)64L);

    tassert(((u64)&buf[0]) % 64 == 0);
    tassert(((u64)buf) % 64 == 0);
    tassert(((u64)mem$aligned_pointer(&buf[0], 64)) % 64 == 0);

    tassert(((u64)&buf[0]) % 64 == 0);
    tassert(((u64)buf) % 64 == 0);
    tassert(((u64)mem$aligned_pointer(buf, 64)) % 64 == 0);
    log$debug("After buf addr: %p add2: %p\n", buf, &buf[0]);

    tassertf(
        buf == mem$aligned_pointer(buf, 64),
        "buf addr: %p aligned addr: %p ptr_diff: %zd",
        buf,
        mem$aligned_pointer(buf, 64),
        (char*)buf - (char*)mem$aligned_pointer(buf, 64)
    );
    tassert(&buf[64] == mem$aligned_pointer(&buf[1], 64));
    mem$free(mem$, buf);

    return EOK;
}

test$main();
