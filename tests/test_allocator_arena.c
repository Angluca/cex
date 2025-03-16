#include <cex/all.c>
#include <cex/allocator2.c>
#include <cex/allocators/AllocatorArena.c>
#include <cex/test/fff.h>
#include <cex/test/test.h>

const Allocator_i* allocator;

test$teardown()
{
    allocator = AllocatorGeneric.destroy(); // this also nullifies allocator
    return EOK;
}

test$setup()
{
    uassert_enable(); // re-enable if you disabled it in some test case
    allocator = AllocatorGeneric.create();
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
        char* p = mem$malloc(arena, 100);
        tassert(p != NULL);
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
    tassert(arena != NULL);

    mem$scope(arena, _)
    {
        for (int i = 0; i < 100; i++) {
            char* p = mem$malloc(arena, i % 32 + 1);
            tassert(p != NULL);
            tassert(mem$aligned_pointer(p, 8) == p);
        }
        u32 align[] = { 8, 16, 32, 64 };

        for (int i = 0; i < 100; i++) {
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
    }

    AllocatorArena_sanitize(arena);
    AllocatorArena_destroy(arena);
    return EOK;
}

test$case(test_allocator_arena_scope_sanitization)
{

    IAllocator arena = AllocatorArena_create(4096 * 100);
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
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
