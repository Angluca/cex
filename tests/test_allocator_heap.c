#include <cex/all.c>
#include <cex/mem.h>
#include <cex/test.h>
#include <stdlib.h>

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

    mem$scope(tmem$, tal)
    {
        void* p = mem$malloc(tmem$, 100);
        tassert(_cex__default_global__allocator_temp.used >= 100);

        tassert(p != NULL);
        p = mem$free(tmem$, p);

        tassert(mem$ != tmem$);
        tassert(tal == tmem$);
    }

    return EOK;
}

test$case(test_allocator_new)
{

    AllocatorHeap_c* a = mem$malloc(mem$, sizeof(AllocatorHeap_c), alignof(AllocatorHeap_c));
    tassert(a != NULL);
    tassert(mem$aligned_pointer(a, alignof(AllocatorHeap_c)) == a);

    mem$free(mem$, a);
    return EOK;
}

test$case(test_allocator_heap_default_alignment)
{

    AllocatorHeap_c* allc = (AllocatorHeap_c*)mem$;
    allc->stats.n_free = 0;
    allc->stats.n_allocs = 0;

    for (u32 i = 1; i < 1000; i++) {
        usize len = i * 8 + 1;
        u8* a = mem$malloc(mem$, len);
        tassert(a != NULL);
        tassert(mem$aligned_pointer(a, 8) == a);
        // filled with 'poisoned' pattern
        for$arr(v, a, len)
        {
            tassert(v == 0xf7);
        }
        mem$free(mem$, a);
    }
    tassert_eqi(allc->stats.n_allocs, 999);
    tassert_eqi(allc->stats.n_allocs, allc->stats.n_free);

    return EOK;
}

test$case(test_allocator_heap_alloc_header)
{

    tassert_eqi(alignof(u64), 8);
    tassert_eqi(sizeof(u64), 8);
    u64 hdr = _cex_allocator_heap__hdr_set(0x123456789ABC, 0xCD, 0xEF);
    tassert_eqi(hdr, 0xefcd123456789abc);
    tassert_eqi(0x123456789ABC, _cex_allocator_heap__hdr_get_size(hdr));
    tassert_eqi(0xCD, _cex_allocator_heap__hdr_get_offset(hdr));
    tassert_eqi(0xEF, _cex_allocator_heap__hdr_get_alignment(hdr));

    return EOK;
}

test$case(test_allocator_heap_malloc_random_align)
{

    usize align_arr[] = {0, 1, 2, 4, 8, 16, 32, 64};
    srand(20250317);

    for(u32 i = 0; i < 1000; i++) {
        usize al = align_arr[i % arr$len(align_arr)];
        usize size = (rand() % 15 + 1) * al; 

        if (al <= 1) {
            size = rand() % 1000 + 1;
        }

        u8* a = mem$->malloc(mem$, size, al);
        tassert(a != NULL);

        if (al < 8) {
            tassert((usize)a % 8 == 0 && "expected aligned to 8");
        } else {
            tassert((usize)a % al == 0 && "expected aligned to al");
        }

        for$arr(v, a, size)
        {
            tassert(v == 0xf7);
        }

        memset(a, 'Z', size);

        // printf("size: %ld align: %ld p: %p\n", size, al, a);
        mem$free(mem$, a);
    }

    return EOK;
}

test$case(test_allocator_heap_calloc_random_align)
{

    usize align_arr[] = {0, 1, 2, 4, 8, 16, 32, 64};
    srand(20250317);

    for(u32 i = 0; i < 1000; i++) {
        usize al = align_arr[i % arr$len(align_arr)];
        usize size = (rand() % 15 + 1) * al; 

        if (al <= 1) {
            size = rand() % 1000 + 1;
        }

        usize nmemb = i % 5 + 1;

        u8* a = mem$->calloc(mem$, nmemb, size, al);
        tassert(a != NULL);

        if (al < 8) {
            tassert((usize)a % 8 == 0 && "expected aligned to 8");
        } else {
            tassert((usize)a % al == 0 && "expected aligned to al");
        }

        for$arr(v, a, size*nmemb)
        {
            tassert(v == 0);
        }

        memset(a, 'Z', size*nmemb);

        // printf("size: %ld align: %ld p: %p\n", size, al, a);
        mem$free(mem$, a);
    }

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
    test$run(test_allocator_heap_default_alignment);
    test$run(test_allocator_heap_alloc_header);
    test$run(test_allocator_heap_malloc_random_align);
    test$run(test_allocator_heap_calloc_random_align);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
