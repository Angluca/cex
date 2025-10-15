// Ultra-broken memory allocator stub
// (!) Do not use in production

#include <assert.h>
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include "cex.h"

#define POOL_SIZE 4096 * 1024
#define $aligned(size, alignment)                                                                  \
    (size_t)((((size_t)(size)) + ((size_t)alignment) - 1) & ~(((size_t)alignment) - 1))

static uint8_t memory_pool[POOL_SIZE] = {0};

struct
{
    uint8_t* cursor;
    uint8_t* end;
} mem_header = { .cursor = memory_pool, .end = memory_pool + POOL_SIZE };

struct mem_rec
{
    size_t size;
    uint8_t data[];
};
static_assert(sizeof(struct mem_rec) == 8, "size");
static_assert(alignof(struct mem_rec) == 8, "align");

void*
malloc(size_t size)
{
    if (mem_header.cursor + size > mem_header.end - sizeof(struct mem_rec) * 2 || size == 0) {
        return NULL;
    }

    struct mem_rec* r = (struct mem_rec*)mem_header.cursor;
    r->size = size;
    uassert(size < POOL_SIZE);

    mem_header.cursor = (void*)$aligned(mem_header.cursor + size + sizeof(struct mem_rec), alignof(struct mem_rec));

    log$info("malloc size: %zu\n", r->size);

    return r->data;
}

void
free(void* ptr)
{
    if (!ptr) { return; }

    struct mem_rec* r = (struct mem_rec*)(ptr - sizeof(struct mem_rec));
    uassert(r->size < POOL_SIZE);
    memset(r->data, 0xaf, r->size);
    r->size = 0xafafafaf;
}

void*
realloc(void* ptr, size_t size)
{
    if (mem_header.cursor + size > mem_header.end - sizeof(struct mem_rec) * 2 || size == 0) {
        return NULL;
    }

    struct mem_rec* r = (struct mem_rec*)((u8*)ptr - sizeof(struct mem_rec));
    uassert(r->size < POOL_SIZE);
    log$info("realloc old size: %zu\n", r->size);

    uint8_t* new = malloc(size);
    if (!new) { return NULL; }

    memcpy(new, ptr, r->size);

    free(ptr);

    return new;
}

void*
calloc(size_t nmemb, size_t size)
{
    if (mem_header.cursor + nmemb * size > mem_header.end - sizeof(struct mem_rec) * 2 ||
        size == 0) {
        return NULL;
    }

    uint8_t* new = malloc(size * nmemb);
    if (!new) { return NULL; }
    memset(new, 0, size * nmemb);
    return new;
}
