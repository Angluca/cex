#include <cex/all.c>
#include <cex/mem.h>
#include <cex/test.h>

test$teardown()
{
    return EOK;
}

test$setup()
{
    uassert_enable(); // re-enable if you disabled it in some test case
    return EOK;
}

test$case(test_allocator_api)
{
    // mem$->scope_enter = NULL; // GOOD: compiler error
    // mem$ = NULL; // GOOD: compiler error
    void* p = mem$malloc(mem$, 100);
    tassert_eqi(_cex__default_global__allocator_heap.stats.n_allocs, 1);
    tassert_eqi(mem$->scope_depth(mem$), 1); 

    tassert(p != NULL);
    p = mem$->free(mem$, p);
    tassert(p == NULL);
    // double free is checked if NULL
    tassert(mem$->free(mem$, p) == NULL);

    p = mem$malloc(mem$, 100);
    tassert(p != NULL);
    // mem$free always nullifies pointer
    mem$free(mem$, p);
    tassert(p == NULL);

    p = mem$malloc(mem$, 100);
    void** pp = &p;
    tassert(p != NULL);
    // mem$free always nullifies pointer
    mem$free(mem$, *pp);
    tassert(p == NULL);


    return EOK;
}
test$case(test_allocator_temp)
{
    void* p = mem$malloc(tmem$, 100);
    tassert_eqi(_cex__default_global__allocator_temp.stats.bytes_alloc, 100);

    tassert(p != NULL);
    p = mem$free(tmem$, p);

    tassert(mem$ != tmem$);

    mem$scope(tmem$, tal)
    {
        tassert(tal == tmem$);
        printf("we are in mem$scope(tmem$)\n");
    }

    return EOK;
}

test$case(test_allocator_new)
{

    AllocatorHeap_c* a = mem$new(mem$, AllocatorHeap_c);
    tassert(a != NULL);
    tassert(mem$aligned_pointer(a, alignof(AllocatorHeap_c)) == a);

    AllocatorHeap_c b = { 0 };
    tassert_eqi(memcmp(a, &b, sizeof(b)), 0);


    // HACK: this is workaround of allocator const constraints in case if we need heap based item
    memcpy(a, mem$, sizeof(AllocatorHeap_c));
    tassert(&a->alloc != mem$);
    tassert(a->alloc.malloc == mem$->malloc);


    mem$free(mem$, a);
    return EOK;
}


int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_allocator_api);
    test$run(test_allocator_temp);
    test$run(test_allocator_new);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
