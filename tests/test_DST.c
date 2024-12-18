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


void postmortem(void* ctx){
    DST.print_state();
    dst_params_s* prm = ctx; 
    printf("malloc_fail_prob: %f\n", prm->allocators.malloc_fail_prob);
}

test$case(test_malloc_dst)
{
    e$ret(DST.create(777999));
    dst_params_s prm = { .allocators = {
                             .malloc_fail_prob = DST.rnd_prob(),
                         } };
    test$set_postmortem(postmortem, &prm);

    e$ret(DST.setup(&prm));
    // tassert(false);
    uassert(false);

    u32 n_failed = 0;
    for (u32 i = 0; i < 1000; i++) {
        void* a = allocator->malloc(100);
        if (a == NULL) {
            n_failed++;
        }
        allocator->free(a);
    }
    tassert(n_failed > 0);

    // WARN: THIS IS DETERMENISTIC:
    // until we may change random gen algo, or add more random calls in code
    tassert_eqf(prm.allocators.malloc_fail_prob, 6.1248767376e-01f);
    tassert_eqi(n_failed, 626);

    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_malloc_dst_zero_prob);
    test$run(test_malloc_dst);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
