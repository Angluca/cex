#include <cex/all.c>
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

#define alloc_cmp(alloc_size, align, expected_struct...)                                           \
    ({                                                                                             \
        allocator_arena_rec_s res = _cex_alloc_estimate_alloc_size((alloc_size), (align));         \
        allocator_arena_rec_s exp = (allocator_arena_rec_s){ expected_struct };                    \
        int cmp_res = memcmp(&res, &exp, sizeof(allocator_arena_rec_s));                           \
        if (cmp_res != 0) {                                                                        \
            printf(                                                                                \
                "Wrong result: {.size = %d, .ptr_offset=%d, .ptr_padding=%d, .ptr_alignmet=%d}\n", \
                res.size,                                                                          \
                res.ptr_offset,                                                                    \
                res.ptr_padding,                                                                   \
                res.ptr_alignment                                                                  \
            );                                                                                     \
        }                                                                                          \
        cmp_res == 0;                                                                              \
    })

test$case(test_allocator_arena_alloc_size)
{
    tassert_eqi(sizeof(allocator_arena_rec_s), 8);

    tassert(alloc_cmp(1, 0, .size = 1, .ptr_padding = 7, .ptr_alignment = 8));
    tassert(alloc_cmp(5, 0, .size = 5, .ptr_padding = 3, .ptr_alignment = 8));
    tassert(alloc_cmp(8, 0, .size = 8, .ptr_padding = 8, .ptr_alignment = 8));
    tassert(alloc_cmp(16, 0, .size = 16, .ptr_padding = 8, .ptr_alignment = 8));
    tassert(alloc_cmp(100, 0, .size = 100, .ptr_padding = 4, .ptr_alignment = 8));

    tassert(alloc_cmp(8, 8, .size = 8, .ptr_padding = 8, .ptr_alignment = 8));
    tassert(alloc_cmp(16, 8, .size = 16, .ptr_padding = 8, .ptr_alignment = 8));

    tassert(alloc_cmp(16, 16, .size = 16, .ptr_padding = 8, .ptr_alignment = 16));
    tassert(alloc_cmp(32, 16, .size = 32, .ptr_padding = 8, .ptr_alignment = 16));
    tassert(alloc_cmp(48, 16, .size = 48, .ptr_padding = 8, .ptr_alignment = 16));
    tassert(alloc_cmp(64, 16, .size = 64, .ptr_padding = 8, .ptr_alignment = 16));

    tassert(alloc_cmp(64, 64, .size = 64, .ptr_padding = 8, .ptr_alignment = 64));
    tassert(alloc_cmp(128, 64, .size = 128, .ptr_padding = 8, .ptr_alignment = 64));
    tassert(alloc_cmp(192, 64, .size = 192, .ptr_padding = 8, .ptr_alignment = 64));
    tassert(alloc_cmp(256, 64, .size = 256, .ptr_padding = 8, .ptr_alignment = 64));

    return EOK;
}


test$case(test_allocator_arena_create_destroy)
{

    IAllocator arena = AllocatorArena_create(4096);
    tassert(arena != NULL);
    tassert(arena != tmem$);
    tassert(arena != mem$);
    tassert(mem$aligned_pointer(arena, alignof(AllocatorArena_c)) == arena);


    tassert_eqi(mem$->scope_depth(mem$), 1);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, tal)
    {
        tassert_eqi(allc->scope_depth, 1);
        tassert_eqi(arena->scope_depth(arena), 1);
        tassert_eqi(mem$->scope_depth(mem$), 1);

        mem$scope(arena, tal)
        {
            tassert_eqi(allc->scope_depth, 2);
            tassert_eqi(arena->scope_depth(arena), 2);
            tassert_eqi(mem$->scope_depth(mem$), 1);
            mem$scope(arena, tal)
            {
                tassert_eqi(allc->scope_depth, 3);
                tassert_eqi(mem$->scope_depth(mem$), 1);
                tassert_eqi(arena->scope_depth(arena), 3);
            }
            tassert_eqi(allc->scope_depth, 2);
        }
        tassert_eqi(allc->scope_depth, 1);
    }
    tassert_eqi(allc->scope_depth, 0);

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_malloc)
{

    IAllocator arena = AllocatorArena_create(4096);
    tassert(arena != NULL);

    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, _)
    {
        u8* p = mem$malloc(arena, 100);
        tassert(p != NULL);
        tassert(mem$asan_enabled());
        // p[-2] = 1;
        // tassert_eqi(p[100], 1); //   GOOD ASAN poison!
        // tassert_eqi(p[-3], 0xf7);   //GOOD ASAN poison!


        // NOTE: includes size + alignment offset + padding + allocator_arena_rec_s
        tassert_eqi(allc->stats.bytes_alloc, 112);
        tassert_eqi(allc->used, 112);

        tassert_eqi(allc->scope_depth, 1);
        tassert_eqi(allc->scope_stack[0], 0);
        tassert_eqi(allc->scope_stack[1], 0);
        tassert_eqi(allc->scope_stack[2], 0);
        tassert_eqi(allc->last_page->used_start, 0);
        tassert_eqi(allc->last_page->cursor, 112);
        tassert(allc->last_page->last_alloc == p);

        mem$scope(arena, _)
        {
            tassert_eqi(allc->scope_depth, 2);
            tassert_eqi(allc->scope_stack[0], 0);
            tassert_eqi(allc->scope_stack[1], 112);
            tassert_eqi(allc->scope_stack[2], 0);

            char* p2 = mem$malloc(arena, 4);
            tassert(p2 != NULL);
            tassert_eqi(allc->stats.bytes_alloc, 112 + 16);
            tassert_eqi(allc->used, 112 + 16);
            tassert_eqi(allc->last_page->cursor, 112 + 16);
            tassert(allc->last_page->last_alloc == p2);

            mem$scope(arena, _)
            {
                char* p3 = mem$malloc(arena, 4);
                tassert(p3 != NULL);
                tassert_eqi(allc->stats.bytes_alloc, 112 + 16 + 16);
                tassert_eqi(allc->used, 112 + 16 + 16);
                tassert_eqi(allc->last_page->cursor, 112 + 16 + 16);

                tassert_eqi(allc->scope_depth, 3);
                tassert_eqi(allc->scope_stack[0], 0);
                tassert_eqi(allc->scope_stack[1], 112);
                tassert_eqi(allc->scope_stack[2], 112 + 16);
                tassert(allc->last_page->last_alloc == p3);
                AllocatorArena_sanitize(arena);
            }
            // Unwinding arena
            tassert_eqi(allc->used, 112 + 16);
            tassert_eqi(allc->last_page->cursor, 112 + 16);
        }
        // Unwinding arena
        tassert_eqi(allc->used, 112);
        tassert_eqi(allc->last_page->cursor, 112);
    }
    // Unwinding arena
    tassert_eqi(allc->used, 0);
    tassert_eqi(allc->last_page->cursor, 0);

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_malloc_pointer_alignment)
{

    IAllocator arena = AllocatorArena_create(4096 * 100);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;
    tassert(arena != NULL);

    mem$scope(arena, _)
    {
        char* _p = mem$malloc(arena, 100);
        tassert(_p != NULL);
        allocator_arena_page_s* page = allc->last_page;
        tassert(page != NULL);

        u32 align[] = { 8, 16, 32, 64 };

        for (int i = 0; i < 100; i++) {
            char* p = mem$malloc(arena, i % 32 + 1);
            tassert(p != NULL);
            tassert(mem$aligned_pointer(p, 8) == p);

            for$arr(alignment, align)
            {
                tassert(page == allc->last_page && "this test case expected to use one page");
                tassert(alignment >= 8);
                tassert(alignment <= 64);
                tassert(mem$is_power_of2(alignment));
                usize alloc_size = alignment * (i % 4 + 1);
                char* ptr_algn = arena->calloc(arena, 1, alloc_size, alignment);
                for (u32 j = 0; j < alloc_size; j++) {
                    tassert(ptr_algn[j] == 0);
                }
                memset(ptr_algn, 0xAA, alloc_size);
                tassert(ptr_algn != NULL);
                // ensure returned pointers are aligned

                uassert(((usize)(ptr_algn) & ((alignment)-1)) == 0);
                allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(ptr_algn);
                tassert_eqi(rec->ptr_alignment, alignment);
                tassert_eqi(rec->size, alloc_size);
                tassert_eqi(rec->is_free, 0);

                if (i % 2 == 0) {
                    tassert(arena->free(arena, ptr_algn) == NULL);
                    tassert(mem$asan_poison_check(ptr_algn, alloc_size));
                    tassert_eqi(rec->is_free, 1);
                } else {
                    usize alloc_size2 = alignment * (i % 4 + 2);
                    tassert(alloc_size2 > alloc_size);

                    char* ptr_algn2 = arena->realloc(arena, ptr_algn, alloc_size2, alignment);
                    tassert(ptr_algn2);
                    tassert_eqi(rec->ptr_alignment, alignment);
                    tassert_eqi(rec->size, alloc_size2);
                    tassert_eqi(rec->is_free, 0);
                }

                AllocatorArena_sanitize(arena);
            }

            char* p2 = mem$realloc(arena, p, i % 32 + 100);
            tassert(p2 != NULL);
            tassert(mem$aligned_pointer(p2, 8) == p2);
            AllocatorArena_sanitize(arena);
        }
    }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_scope_sanitization)
{

    IAllocator arena = AllocatorArena_create(4096);
    tassert(arena != NULL);

    mem$scope(arena, _)
    {
        u32 align[] = { 8, 16, 32, 64 };


        for (int i = 0; i < 10; i++) {
            mem$scope(arena, _)
            {
                char* p = mem$malloc(arena, i % 32 + 1);
                tassert(p != NULL);
                tassert(mem$aligned_pointer(p, 8) == p);

                for$arr(alignment, align)
                {
                    tassert(alignment >= 8);
                    tassert(alignment <= 64);
                    tassert(mem$is_power_of2(alignment));
                    usize alloc_size = alignment * (i % 4 + 1);
                    char* p = arena->malloc(arena, alloc_size, alignment);
                    memset(p, 0xAA, alloc_size);
                    tassert(p != NULL);
                    // ensure returned pointers are aligned

                    uassert(((usize)(p) & ((alignment)-1)) == 0);
                    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
                    tassert_eqi(rec->ptr_alignment, alignment);
                    tassert_eqi(rec->size, alloc_size);
                    tassert_eqi(rec->is_free, 0);

                    tassert(arena->free(arena, p) == NULL);
                    tassert(mem$asan_poison_check(p, alloc_size));
                    tassert_eqi(rec->is_free, 1);

                    AllocatorArena_sanitize(arena);
                }
            }
            AllocatorArena_sanitize(arena);
        }
        AllocatorArena_sanitize(arena);
    }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_realloc)
{

    IAllocator arena = AllocatorArena_create(4096);
    tassert(arena != NULL);

    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, _)
    {
        char* p = mem$malloc(arena, 100);
        tassert(p != NULL);
        memset(p, 0xAA, 100);
        // NOTE: includes size + alignment offset + padding + allocator_arena_rec_s
        tassert_eqi(allc->stats.bytes_alloc, 112);
        tassert_eqi(allc->used, 112);
        AllocatorArena_sanitize(arena);

        char* p2 = mem$malloc(arena, 100);
        tassert(p2 != NULL);
        memset(p2, 0xAA, 100);
        tassert_eqi(allc->stats.bytes_alloc, 112 + 112);
        tassert_eqi(allc->used, 112 + 112);
        AllocatorArena_sanitize(arena);

        char* p3 = mem$realloc(arena, p, 200);
        tassert(p3 != NULL);
        tassert(p3 != p);
        memset(p3, 0xAA, 100);
        tassert_eqi(allc->stats.bytes_alloc, 112 + 112 + 216);
        tassert_eqi(allc->used, 112 + 112 + 216);
        AllocatorArena_sanitize(arena);

        allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
        tassert_eqi(rec->ptr_alignment, 8);
        tassert_eqi(rec->size, 100);
        tassert_eqi(rec->is_free, 1);

        rec = _cex_alloc_arena__get_rec(p3);
        tassert_eqi(rec->ptr_alignment, 8);
        tassert_eqi(rec->size, 200);
        tassert_eqi(rec->is_free, 0);
        tassert(allc->last_page->last_alloc == p3);

        // Extending last pointer!
        char* p4 = mem$realloc(arena, p3, 300);
        tassert(p4 != NULL);
        tassert(p3 == p4);
        memset(p3, 0xAA, 300);
        AllocatorArena_sanitize(arena);
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_page_size)
{
    usize page_size = 1024;
    for (u32 i = 0; i < 10000; i += 10) {
        usize est_page_size = _cex_alloc_estimate_page_size(page_size, i);
        usize base_page_size = mem$aligned_round(
            page_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN,
            alignof(allocator_arena_page_s)
        );

        tassert(est_page_size >= page_size);
        if (i > 0.7 * base_page_size) {
            if (i > 1024 * 1024) {
                uassert(est_page_size <= i * 1.2 + CEX_ARENA_MAX_ALIGN * 3);
            } else {
                uassert(est_page_size <= i * 2 + CEX_ARENA_MAX_ALIGN * 3);
            }
        } else {
            uassert(est_page_size == base_page_size);
        }
        tassert(est_page_size % alignof(allocator_arena_page_s) == 0);
    }
    return EOK;
}

test$case(test_allocator_arena_multiple_pages)
{

    IAllocator arena = AllocatorArena_create(1024);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;
    tassert(arena != NULL);
    allocator_arena_page_s* page = NULL;

    mem$scope(arena, _)
    {
        char* p = mem$malloc(arena, 1000);
        tassert(p != NULL);

        page = allc->last_page;
        tassert(page != NULL);
        tassert(page->prev_page == NULL);
        tassert_eqi(allc->used, 1016);
        tassert_eqi(page->cursor, 1016);


        // next allocated on next page
        char* p2 = mem$malloc(arena, 1200);
        tassert(p2 != NULL);
        tassert(page != NULL);
        tassert(allc->last_page != NULL);
        tassert(allc->last_page != page);
        tassert(page->prev_page == NULL);
        tassert(allc->last_page->prev_page == page);
        tassert_eqi(allc->used, 2232);
    }

    tassert(allc->last_page != NULL);
    tassert(allc->last_page == page); // replaced by first!
    tassert(allc->last_page->prev_page == NULL);
    tassert(allc->last_page->cursor == 0);
    tassert_eqi(allc->used, 0);
    tassert_eqi(allc->stats.pages_created, 2);
    tassert_eqi(allc->stats.pages_free, 1); // last page should be still active

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_realloc_shrink)
{

    IAllocator arena = AllocatorArena_create(4096);
    tassert(arena != NULL);

    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, _)
    {
        char* p = mem$malloc(arena, 100);
        tassert(p != NULL);
        memset(p, 0xAA, 100);
        // NOTE: includes size + alignment offset + padding + allocator_arena_rec_s
        tassert_eqi(allc->stats.bytes_alloc, 112);
        tassert_eqi(allc->used, 112);
        AllocatorArena_sanitize(arena);

        allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
        tassert_eqi(rec->ptr_alignment, 8);
        tassert_eqi(rec->size, 100);
        tassert_eqi(rec->ptr_padding, 4);
        tassert_eqi(rec->is_free, 0);
        tassert(mem$asan_poison_check(p + rec->size, rec->ptr_padding));

        // same size just ignored
        char* p2 = mem$realloc(arena, p, 100);
        tassert(p2 != NULL);
        tassert(p2 == p);
        tassert_eqi(allc->stats.bytes_alloc, 112);
        tassert_eqi(allc->used, 112);
        tassert_eqi(rec->ptr_alignment, 8);
        tassert_eqi(rec->size, 100);
        tassert_eqi(rec->ptr_padding, 4);
        tassert_eqi(rec->is_free, 0);
        tassert(mem$asan_poison_check(p + rec->size, rec->ptr_padding));
        AllocatorArena_sanitize(arena);

        char* p3 = mem$realloc(arena, p, 50);
        tassert(p3 != NULL);
        tassert(p3 == p);
        tassert(p3 == p2);
        tassert_eqi(allc->stats.bytes_alloc, 112);
        tassert_eqi(allc->used, 112);
        // only poisoning, other fields untouched
        tassert(mem$asan_poison_check(p + 50, rec->size - 50 + rec->ptr_padding));
        tassert_eqi(rec->size, 100);
        tassert_eqi(rec->ptr_padding, 4);
        tassert_eqi(rec->is_free, 0);
        AllocatorArena_sanitize(arena);
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_malloc_mem_pattern)
{

    IAllocator arena = AllocatorArena_create(4096);
    tassert(arena != NULL);

    mem$scope(arena, _)
    {
        u8* p = mem$malloc(arena, 100);
        tassert(p != NULL);
        for (u32 i = 0; i < 100; i++) {
            tassert(p[i] == 0xf7);
        }
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_pointer_lifetime)
{

    IAllocator arena = AllocatorArena_create(4096);
    tassert(arena != NULL);
    AllocatorArena_c* allc = (AllocatorArena_c*)arena;

    mem$scope(arena, _)
    {
        u8* p = mem$malloc(arena, 100);
        allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(p);
        tassert(p != NULL);
        tassert((void*)rec > (void*)allc->last_page);
        tassert(
            (void*)rec <
            (void*)allc->last_page + sizeof(allocator_arena_page_s) + allc->last_page->capacity
        );
        mem$scope(arena, _)
        {
            u8* p2 = mem$malloc(arena, 100);
            tassert(p2 != NULL);
            allocator_arena_rec_s* rec2 = _cex_alloc_arena__get_rec(p2);
            tassert((void*)rec2 > (void*)allc->last_page);
            tassert(
                (void*)rec2 <
                (void*)allc->last_page + sizeof(allocator_arena_page_s) + allc->last_page->capacity
            );
            tassert(_cex_allocator_arena__check_pointer_valid(allc, p2));
            uassert_disable();
            tassert(!_cex_allocator_arena__check_pointer_valid(allc, p));
        }
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_realloc_last_pointer)
{

    IAllocator arena = AllocatorArena_create(1024);
    tassert(arena != NULL);

    mem$scope(arena, _)
    {
        for (u32 z = 0; z < 1000; z++) {
            char* p = mem$malloc(arena, 1);
            tassert(p != NULL);
            *p = 0;
            for (u32 i = 1; i < 200; i++) {
                char* new_p = mem$realloc(arena, p, i+1);
                tassert(new_p != NULL);
                tassert(new_p == p);
                tassert_eqi(p[i-1], i-1);
                p[i] = i;
            }
            for (u32 i = 0; i < 200; i++) {
                tassert_eqi(p[i], i);
            }
        }
        AllocatorArena_sanitize(arena);
    }

    AllocatorArena_destroy(arena);
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_allocator_arena_alloc_size);
    test$run(test_allocator_arena_create_destroy);
    test$run(test_allocator_arena_malloc);
    test$run(test_allocator_arena_malloc_pointer_alignment);
    test$run(test_allocator_arena_scope_sanitization);
    test$run(test_allocator_arena_realloc);
    test$run(test_allocator_arena_page_size);
    test$run(test_allocator_arena_multiple_pages);
    test$run(test_allocator_arena_realloc_shrink);
    test$run(test_allocator_arena_malloc_mem_pattern);
    test$run(test_allocator_arena_pointer_lifetime);
    test$run(test_allocator_arena_realloc_last_pointer);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
