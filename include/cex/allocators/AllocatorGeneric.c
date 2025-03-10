#pragma once
#include "AllocatorGeneric.h"
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>

#define ALLOCATOR_HEAP_MAGIC 0xFEED0001U

static void* _cex_allocator_generic__malloc(usize size);
static void* _cex_allocator_generic__calloc(usize nmemb, usize size);
static void* _cex_allocator_generic__aligned_malloc(usize alignment, usize size);
static void* _cex_allocator_generic__realloc(void* ptr, usize size);
static void* _cex_allocator_generic__aligned_realloc(void* ptr, usize alignment, usize size);
static void _cex_allocator_generic__free(void* ptr);
static FILE* _cex_allocator_generic__fopen(const char* filename, const char* mode);
static int _cex_allocator_generic__fclose(FILE* f);
static int _cex_allocator_generic__open(const char* pathname, int flags, unsigned int mode);
static int _cex_allocator_generic__close(int fd);


static allocator_generic_s
    allocator__generic_data = { .base = {
                                    .malloc = _cex_allocator_generic__malloc,
                                    .malloc_aligned = _cex_allocator_generic__aligned_malloc,
                                    .realloc = _cex_allocator_generic__realloc,
                                    .realloc_aligned = _cex_allocator_generic__aligned_realloc,
                                    .calloc = _cex_allocator_generic__calloc,
                                    .free = _cex_allocator_generic__free,
                                    .fopen = _cex_allocator_generic__fopen,
                                    .fclose = _cex_allocator_generic__fclose,
                                    .open = _cex_allocator_generic__open,
                                    .close = _cex_allocator_generic__close,
                                } };
/*
 *                  HEAP ALLOCATOR
 */

/**
 * @brief  heap-based allocator (simple proxy for malloc/free/realloc)
 */
const Allocator_i*
AllocatorGeneric_create(void)
{
    uassert(allocator__generic_data.magic == 0 && "Already initialized");

    allocator__generic_data.magic = ALLOCATOR_HEAP_MAGIC;

#ifndef NDEBUG
    memset(&allocator__generic_data.stats, 0, sizeof(allocator__generic_data.stats));
#endif

    return &allocator__generic_data.base;
}

const Allocator_i*
AllocatorGeneric_destroy(void)
{
    uassert(allocator__generic_data.magic != 0 && "Already destroyed");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    allocator__generic_data.magic = 0;

#ifndef NDEBUG
    allocator_generic_s* a = &allocator__generic_data;
    // NOTE: this message only shown if no DNDEBUG
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

    memset(&allocator__generic_data.stats, 0, sizeof(allocator__generic_data.stats));
#endif

    return NULL;
}

static void*
_cex_allocator_generic__malloc(usize size)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    allocator__generic_data.stats.n_allocs++;
#endif

    return malloc(size);
}

static void*
_cex_allocator_generic__calloc(usize nmemb, usize size)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    allocator__generic_data.stats.n_allocs++;
#endif

    return calloc(nmemb, size);
}

static void*
_cex_allocator_generic__aligned_malloc(usize alignment, usize size)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(size % alignment == 0 && "size must be rounded to align");

#ifndef NDEBUG
    allocator__generic_data.stats.n_allocs++;
#endif

#ifdef _WIN32
    return _aligned_malloc(alignment, size);
#else
    return aligned_alloc(alignment, size);
#endif
}

static void*
_cex_allocator_generic__realloc(void* ptr, usize size)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    if (ptr == NULL) {
        allocator__generic_data.stats.n_allocs++;
    } else {
        allocator__generic_data.stats.n_reallocs++;
    }
#endif

    return realloc(ptr, size);
}

static void*
_cex_allocator_generic__aligned_realloc(void* ptr, usize alignment, usize size)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(((usize)ptr % alignment) == 0 && "aligned_realloc existing pointer unaligned");
    uassert(size % alignment == 0 && "size must be rounded to align");

#ifndef NDEBUG
    if (ptr == NULL) {
        allocator__generic_data.stats.n_allocs++;
    } else {
        allocator__generic_data.stats.n_reallocs++;
    }
#endif

    // TODO: implement #ifdef MSVC it supports _aligned_realloc()

    void* result = NULL;

    // Check if we have available space for realloc'ing new size
    //
#ifdef _WIN32
    usize new_size = _msize(ptr);
#else
    // NOTE: malloc_usable_size() returns a value no less than the size of
    // the block of allocated memory pointed to by ptr.
    usize new_size = malloc_usable_size(ptr);
#endif

    if (new_size >= size) {
        // This should return extended memory
        result = realloc(ptr, size);
        if (result == NULL) {
            // memory error
            return NULL;
        }
        if (result != ptr || (usize)result % alignment != 0) {
            // very rare case, when some thread acquired space or returned unaligned result
            // Pessimistic case
#ifdef _WIN32
            void* aligned = _aligned_malloc(alignment, size);
#else
            void* aligned = aligned_alloc(alignment, size);
#endif
            memcpy(aligned, result, size);
            free(result);
            return aligned;
        }
    } else {
        // No space available, alloc new memory + copy + release old
#ifdef _WIN32
        result = _aligned_malloc(alignment, size);
#else
        result = aligned_alloc(alignment, size);
#endif
        if (result == NULL) {
            return NULL;
        }
        memcpy(result, ptr, new_size);
        free(ptr);
    }

    return result;
}

static void
_cex_allocator_generic__free(void* ptr)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

#ifndef NDEBUG
    if (ptr != NULL) {
        allocator__generic_data.stats.n_free++;
    }
#endif

    free(ptr);
}

static FILE*
_cex_allocator_generic__fopen(const char* filename, const char* mode)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(filename != NULL);
    uassert(mode != NULL);

    FILE* res = fopen(filename, mode);
    (void)res;

#ifndef NDEBUG
    if (res != NULL) {
        allocator__generic_data.stats.n_fopen++;
    }
#endif

    return res;
}

static int
_cex_allocator_generic__open(const char* pathname, int flags, unsigned int mode)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");
    uassert(pathname != NULL);

    int fd = open(pathname, flags, mode);
    (void)fd;

#ifndef NDEBUG
    if (fd != -1) {
        allocator__generic_data.stats.n_open++;
    }
#endif

    return fd;
}

static int
_cex_allocator_generic__close(int fd)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    int ret = close(fd);
    (void)ret;

#ifndef NDEBUG
    if (ret != -1) {
        allocator__generic_data.stats.n_close++;
    }
#endif

    return ret;
}

static int
_cex_allocator_generic__fclose(FILE* f)
{
    uassert(allocator__generic_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__generic_data.magic == ALLOCATOR_HEAP_MAGIC && "Allocator type!");

    uassert(f != NULL);
    uassert(f != stdin && "closing stdin");
    uassert(f != stdout && "closing stdout");
    uassert(f != stderr && "closing stderr");

#ifndef NDEBUG
    allocator__generic_data.stats.n_fclose++;
#endif

    return fclose(f);
}

const struct __class__AllocatorGeneric AllocatorGeneric = {
    // Autogenerated by CEX
    // clang-format off
    .create = AllocatorGeneric_create,
    .destroy = AllocatorGeneric_destroy,
    // clang-format on
};
