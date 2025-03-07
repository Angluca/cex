#include <cex/all.c>
#include <cex/random/Random.c>
#include <cex/test/dst/AllocatorDST.c>
#include <cex/test/dst/DST.c>
#include <cex/test/fff.h>
#include <cex/test/test.h>

const Allocator_i* allocator;

test$teardown()
{
    DST.destroy();
    allocator = AllocatorDST.destroy(); // this also nullifies allocator
    return EOK;
}

test$setup()
{
    uassert_enable(); // re-enable if you disabled it in some test case
    allocator = AllocatorDST.create();
    return EOK;
}

test$case(test_malloc_dst_zero_prob)
{
    e$ret(DST.create(777999));
    dst_params_s prm = { .allocators = {
                             .malloc_fail_prob = 0,
                         } };
    e$ret(DST.setup(&prm));

    u32 n_failed = 0;
    for (u32 i = 0; i < 1000; i++) {
        void* a = allocator->malloc(100);
        if (a == NULL) {
            n_failed++;
        }
        allocator->free(a);
    }
    tassert_eqi(n_failed, 0);

    return EOK;
}

#define DST$fuzz(var, ...)                                                                         \
    _Generic((var), u64 *: DST.fuzz.u64)(                                                          \
        var,                                                                                       \
        (typeof(*(var))[]){ __VA_ARGS__ },                                                         \
        sizeof((typeof(*(var))[]){ __VA_ARGS__ })/sizeof(typeof(*(var)))                                                 \
    )

Exception
fuzz_proto(void)
{
    printf("Fork succeeded: %u\n", Random.next(&_DST._rng));
    char* foo = malloc(200);

    // e$ret(DST.simulate(fuzz_proto));

    (void)foo;
    // foo[0] = 1;
    if (DST.rnd.check(1.0)) {
        free(foo);
    }
    u64 v = 0;

    // foo[2] = 108;
    // e$assert(false);
    while (DST.tick(1000)) {
        fprintf(stdout, "#%ld: stdout test\n", _DST._tick_current);
        fprintf(stderr, "#%ld: stderr test\n", _DST._tick_current);

        DST.fuzz.u64(&v, NULL, 0);
        tassert(v <= 100);

        // DST.fuzz.u64(&v, (u64[]){1, 2, 3}, 3);
        DST$fuzz(&v, 1, 2, 3);
        tassert(v == 1 || v == 2 || v == 3);

        // DST$fuzz(&v2, 1, 2, 3);
    }
    fprintf(stdout, "sim pos: %zd\n", ftell(_DST._sim_log));
    // uassert(false);
    // foo[209123] = 108;

    // return Error.io;
    return EOK;
}
test$case(test_fuzz)
{

    e$ret(DST.create(999));
    dst_params_s prm = { 
        .allocators = {
                        .malloc_fail_prob = DST.rnd.prob(),
                         },
        .simulator = {
            .keep_tick_log = true,
        } };

    tassert_eqe(DST.setup(&prm), EOK);
    tassert_eqe(DST.simulate(fuzz_proto), EOK);

    return EOK;
}

test$case(test_malloc_dst)
{
    // test$set_postmortem(DST.print_postmortem, NULL);

    e$ret(DST.create(777999));
    dst_params_s prm = { .allocators = {
                             .malloc_fail_prob = DST.rnd.prob(),
                         } };

    e$ret(DST.setup(&prm));

    u32 n_failed = 0;
    u32 n_ticked = 0;
    while (DST.tick(1000)) {
        void* a = allocator->malloc(100);
        if (a == NULL) {
            n_failed++;
        }
        allocator->free(a);
        n_ticked++;
    }
    tassert(n_failed > 0);
    tassert_eqi(n_ticked, 1000);
    tassert_eqi(1001, _DST._tick_current);
    tassert_eqi(1000, _DST._tick_max);
    // WARN: THIS IS DETERMENISTIC:
    // until we may change random gen algo, or add more random calls in code
    tassert_eqf(prm.allocators.malloc_fail_prob, 6.1248767376e-01f);
    tassert_eqi(n_failed, 637);

    e$ret(DST.setup(&(dst_params_s){ .allocators = {
                                         .malloc_fail_prob = 0.3,
                                     } }));
    tassert_eqi(0, _DST._tick_current);
    tassert_eqi(0, _DST._tick_max);
    tassert_eqf(0.3, _DST.prm.allocators.malloc_fail_prob);

    e$ret(DST.setup(NULL));
    tassert_eqi(0, _DST._tick_current);
    tassert_eqi(0, _DST._tick_max);
    tassert_eqf(0, _DST.prm.allocators.malloc_fail_prob);


    return EOK;
}
test$case(test_seed)
{
    e$ret(DST.create(777999));
    tassert_eqi(_DST._initial_seed, 777999);
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_malloc_dst_zero_prob);
    test$run(test_fuzz);
    test$run(test_malloc_dst);
    test$run(test_seed);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
