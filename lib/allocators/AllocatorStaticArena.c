#pragma once
#include "AllocatorStaticArena.h"
#include <fcntl.h>
#include <unistd.h>
#include <cex/cex.h>

#define ALLOCATOR_STATIC_ARENA_MAGIC 0xFEED0003U

static void* _cex_allocator_staticarena__malloc(usize size);
static void* _cex_allocator_staticarena__calloc(usize nmemb, usize size);
static void* _cex_allocator_staticarena__aligned_malloc(usize alignment, usize size);
static void* _cex_allocator_staticarena__realloc(void* ptr, usize size);
static void* _cex_allocator_staticarena__aligned_realloc(void* ptr, usize alignment, usize size);
static void _cex_allocator_staticarena__free(void* ptr);
static FILE* _cex_allocator_staticarena__fopen(const char* filename, const char* mode);
static int _cex_allocator_staticarena__fclose(FILE* f);
static int _cex_allocator_staticarena__open(const char* pathname, int flags, unsigned int mode);
static int _cex_allocator_staticarena__close(int fd);

static allocator_staticarena_s
    allocator__staticarena_data = { .base = {
                                         .malloc = _cex_allocator_staticarena__malloc,
                                         .malloc_aligned = _cex_allocator_staticarena__aligned_malloc,
                                         .calloc = _cex_allocator_staticarena__calloc,
                                         .realloc = _cex_allocator_staticarena__realloc,
                                         .realloc_aligned = _cex_allocator_staticarena__aligned_realloc,
                                         .free = _cex_allocator_staticarena__free,
                                         .fopen = _cex_allocator_staticarena__fopen,
                                         .fclose = _cex_allocator_staticarena__fclose,
                                         .open = _cex_allocator_staticarena__open,
                                         .close = _cex_allocator_staticarena__close,
                                     } };


/*
 *                  STATIC ARENA ALLOCATOR
 */

/**
 * @brief Static arena allocator (can be heap or stack arena)
 *
 * Static allocator should be created at the start of the application (maybe in main()),
 * and freed after app shutdown.
 *
 * Note: memory leaks are not caught by sanitizers, if you forget to call
 * AllocatorStaticArena.destroy() sanitizers will be silent.
 *
 * No realloc() supported by this arena!
 *
 * @param buffer - pointer to memory buffer
 * @param capacity - capacity of a buffer (minimal requires is 1024)
 * @return  allocator instance
 */
IAllocator
AllocatorStaticArena_create(char* buffer, usize capacity)
{
    uassert(allocator__staticarena_data.magic == 0 && "Already initialized");

    uassert(capacity >= 1024 && "capacity is too low");
    uassert(((capacity & (capacity - 1)) == 0) && "must be power of 2");

    allocator_staticarena_s* a = &allocator__staticarena_data;

    a->magic = ALLOCATOR_STATIC_ARENA_MAGIC;

    a->mem = buffer;
    if (a->mem == NULL) {
        memset(a, 0, sizeof(*a));
        return NULL;
    }

    memset(a->mem, 0, capacity);

    a->max = (char*)a->mem + capacity;
    a->next = a->mem;

    usize offset = ((usize)a->next % sizeof(usize));
    a->next = (char*)a->next + (offset ? sizeof(usize) - offset : 0);

    uassert(((usize)a->next % sizeof(usize) == 0) && "alloca/malloc() returned non word aligned ptr");

    return &a->base;
}

IAllocator
AllocatorStaticArena_destroy(void)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");


    allocator_staticarena_s* a = &allocator__staticarena_data;
    a->magic = 0;
    a->mem = NULL;
    a->next = NULL;
    a->max = NULL;

#ifndef NDEBUG
    if (a->stats.n_allocs != a->stats.n_free) {
        log$debug(
            "Allocator: Possible memory leaks/double free: memory allocator->allocs() [%u] != allocator->free() [%u] count! \n",
            a->stats.n_allocs,
            a->stats.n_free
        );
    }
    if (a->stats.n_fopen != a->stats.n_fclose) {
        log$debug(
            "Allocator: Possible FILE* leaks: allocator->fopen() [%u] != allocator->fclose() [%u]!\n",
            a->stats.n_fopen,
            a->stats.n_fclose
        );
    }
    if (a->stats.n_open != a->stats.n_close) {
        log$debug(
            "Allocator: Possible file descriptor leaks: allocator->open() [%u] != allocator->close() [%u]!\n",
            a->stats.n_open,
            a->stats.n_close
        );
    }

    memset(&allocator__staticarena_data.stats, 0, sizeof(allocator__staticarena_data.stats));
#endif

    return NULL;
}


static void*
_cex_allocator_staticarena__aligned_realloc(void* ptr, usize alignment, usize size)
{
    (void)ptr;
    (void)size;
    (void)alignment;
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}
static void*
_cex_allocator_staticarena__realloc(void* ptr, usize size)
{
    (void)ptr;
    (void)size;
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");
    uassert(false && "realloc is not supported by static arena allocator");

    return NULL;
}

static void
_cex_allocator_staticarena__free(void* ptr)
{
    (void)ptr;
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "bad type!");

#ifndef NDEBUG
    allocator_staticarena_s* a = &allocator__staticarena_data;
    if(ptr != NULL){
        a->stats.n_free++;
    }
#endif
}

static void*
_cex_allocator_staticarena__aligned_malloc(usize alignment, usize size)
{
    uassert(allocator__staticarena_data.magic != 0 && "not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");

    allocator_staticarena_s* a = &allocator__staticarena_data;


    if (size == 0) {
        uassert(size > 0 && "zero size");
        return NULL;
    }
    if (size % alignment != 0) {
        uassert(size >= alignment && "size not aligned, must be a rounded to alignment");
        return NULL;
    }

    usize offset = ((usize)a->next % alignment);

    alignment = (offset ? alignment - offset : 0);

    if ((char*)a->next + size + alignment > (char*)a->max) {
        // not enough capacity
        return NULL;
    }

    void* ptr = (char*)a->next + alignment;
    a->next = (char*)ptr + size;

#ifndef NDEBUG
    a->stats.n_allocs++;
#endif

    return ptr;
}

static void*
_cex_allocator_staticarena__malloc(usize size)
{
    uassert(allocator__staticarena_data.magic != 0 && "not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    allocator_staticarena_s* a = &allocator__staticarena_data;

    if (size == 0) {
        uassert(size > 0 && "zero size");
        return NULL;
    }

    void* ptr = a->next;
    if ((char*)a->next + size > (char*)a->max) {
        // not enough capacity
        return NULL;
    }

    u32 offset = (size % sizeof(usize));
    a->next = (char*)a->next + size + (offset ? sizeof(usize) - offset : 0);

#ifndef NDEBUG
    a->stats.n_allocs++;
#endif

    return ptr;
}

static void*
_cex_allocator_staticarena__calloc(usize nmemb, usize size)
{
    uassert(allocator__staticarena_data.magic != 0 && "not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    allocator_staticarena_s* a = &allocator__staticarena_data;

    usize alloc_size = nmemb * size;
    if (nmemb != 0 && alloc_size / nmemb != size) {
        // overflow handling
        return NULL;
    }

    if (alloc_size == 0) {
        uassert(alloc_size > 0 && "zero size");
        return NULL;
    }

    void* ptr = a->next;
    if ((char*)a->next + alloc_size > (char*)a->max) {
        // not enough capacity
        return NULL;
    }

    u32 offset = (alloc_size % sizeof(usize));
    a->next = (char*)a->next + alloc_size + (offset ? sizeof(usize) - offset : 0);

    memset(ptr, 0, alloc_size);

#ifndef NDEBUG
    a->stats.n_allocs++;
#endif

    return ptr;
}

static FILE*
_cex_allocator_staticarena__fopen(const char* filename, const char* mode)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");
    uassert(filename != NULL);
    uassert(mode != NULL);

    FILE* res = fopen(filename, mode);

#ifndef NDEBUG
    if (res != NULL) {
        allocator_staticarena_s* a = &allocator__staticarena_data;
        a->stats.n_fopen++;
    }
#endif

    return res;
}

static int
_cex_allocator_staticarena__fclose(FILE* f)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    uassert(f != NULL);
    uassert(f != stdin && "closing stdin");
    uassert(f != stdout && "closing stdout");
    uassert(f != stderr && "closing stderr");

#ifndef NDEBUG
    allocator_staticarena_s* a = &allocator__staticarena_data;
    a->stats.n_fclose++;
#endif

    return fclose(f);
}
static int
_cex_allocator_staticarena__open(const char* pathname, int flags, unsigned int mode)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");
    uassert(pathname != NULL);

    int fd = open(pathname, flags, mode);

#ifndef NDEBUG
    if (fd != -1) {
        allocator__staticarena_data.stats.n_open++;
    }
#endif
    return fd;
}

static int
_cex_allocator_staticarena__close(int fd)
{
    uassert(allocator__staticarena_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__staticarena_data.magic == ALLOCATOR_STATIC_ARENA_MAGIC && "Allocator type!");

    int ret = close(fd);

#ifndef NDEBUG
    if (ret != -1) {
        allocator__staticarena_data.stats.n_close++;
    }
#endif

    return ret;
}

const struct __cex_namespace__AllocatorStaticArena AllocatorStaticArena = {
    // Autogenerated by CEX
    // clang-format off

    .create = AllocatorStaticArena_create,
    .destroy = AllocatorStaticArena_destroy,

    // clang-format on
};
