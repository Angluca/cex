#include "stdlib.h"

//
// This test is simulation of non standard malloc() pointer alignment, typically it's 16 byte
// However, some platforms may return 8-byte aligned pointer. Make sure that we have it working
//   also experiment with custom malloc() allocator.
//

void*
cex_malloc(size_t size)
{
    void* p = malloc(size + 8);
    if (p) { return p + 8; }
    return p;
}
void
cex_free(void* ptr)
{
    if (ptr) { free(ptr - 8); }
}
void*
cex_calloc(size_t nmemb, size_t size)
{
    void* p = calloc(nmemb, size + 8);
    if (p) { return p + 8; }
    return p;
}
void*
cex_realloc(void* ptr, size_t size)
{
    void* p = realloc(ptr - 8, size + 8);
    if (p) { return p + 8; }

    return p;
}

#define cex$platform_malloc cex_malloc
#define cex$platform_calloc cex_calloc
#define cex$platform_free cex_free
#define cex$platform_realloc cex_realloc

#define CEX_IMPLEMENTATION
#define CEX_TEST
#include "cex.h"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}


test$case(aligned_size_malloc_returns_8byte_align)
{
    usize size = 80;
    u8* a = mem$malloc(mem$, size, 16);
    tassert((usize)a % 16 == 0);
    memset(a, 0xaf, size);
    mem$free(mem$, a);
    return EOK;
}

test$case(aligned_malloc)
{
    usize align_arr[] = { 0, 1, 2, 4, 8, 16, 32, 64 };

    for$each (align, align_arr) {
        for (u32 i = 1; i < 100; i++) {
            usize size = align * i;
            if (size == 0) { size = 1; }
            u8* a = mem$malloc(mem$, size, align);
            if (align) { tassert((usize)a % align == 0); }
            memset(a, 0xaf, size);

            u64 hdr = *(u64*)(a - sizeof(u64) * 2);
            u8 _offset = _cex_allocator_heap__hdr_get_offset(hdr);
            u8 _alignment = _cex_allocator_heap__hdr_get_alignment(hdr);
            tassert_ge(_alignment, 8);
            tassert_le(_alignment, 64);
            if (align >= 8) {
                tassert_eq(_alignment, align);
            }
            tassert_ge(_offset, 16);
            tassert_le(_offset, 64 + 16);

            mem$free(mem$, a);
        }
    }
    return EOK;
}


test$case(aligned_calloc)
{
    usize align_arr[] = { 1, 2, 4, 8, 16, 32, 64 };

    for$each (align, align_arr) {
        for (u32 i = 1; i < 100; i++) {
            u8* a = mem$calloc(mem$, i, align, align);
            if (align) { tassertf((usize)a % align == 0, "align: %zu\n", align); }
            memset(a, 0xaf, align*i);
            mem$free(mem$, a);
        }
    }
    return EOK;
}


test$case(aligned_realloc)
{
    usize align_arr[] = { 0, 1, 2, 4, 8, 16, 32, 64 };

    for$each (align, align_arr) {
        for (u32 i = 1; i < 100; i++) {
            usize size = align * i;
            if (size == 0) { size = 1; }
            u8* a = mem$malloc(mem$, size, align);
            if (align) { tassert((usize)a % align == 0); }
            memset(a, 0xaf, size);

            a = mem$realloc(mem$, a, size*2, align);
            tassert(a);
            memset(a, 0xaf, size*2);
            mem$free(mem$, a);
        }
    }
    return EOK;
}
test$main();
