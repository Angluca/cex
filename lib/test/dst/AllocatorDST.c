#pragma once
#include "AllocatorDST.h"
#include "DST.h"
#include <lib/random/Random.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdlib.h>
#include <unistd.h>


#define ALLOCATOR_DST_MAGIC 0xBAADFEED

static void* _cex_allocator_dst__malloc(usize size);
static void* _cex_allocator_dst__calloc(usize nmemb, usize size);
static void* _cex_allocator_dst__aligned_malloc(usize alignment, usize size);
static void* _cex_allocator_dst__realloc(void* ptr, usize size);
static void* _cex_allocator_dst__aligned_realloc(void* ptr, usize alignment, usize size);
static void _cex_allocator_dst__free(void* ptr);
static FILE* _cex_allocator_dst__fopen(const char* filename, const char* mode);
static int _cex_allocator_dst__fclose(FILE* f);
static int _cex_allocator_dst__open(const char* pathname, int flags, unsigned int mode);
static int _cex_allocator_dst__close(int fd);


static allocator_dst_s
    allocator__dst_data = { .base = {
                                .malloc = _cex_allocator_dst__malloc,
                                .malloc_aligned = _cex_allocator_dst__aligned_malloc,
                                .realloc = _cex_allocator_dst__realloc,
                                .realloc_aligned = _cex_allocator_dst__aligned_realloc,
                                .calloc = _cex_allocator_dst__calloc,
                                .free = _cex_allocator_dst__free,
                                .fopen = _cex_allocator_dst__fopen,
                                .fclose = _cex_allocator_dst__fclose,
                                .open = _cex_allocator_dst__open,
                                .close = _cex_allocator_dst__close,
                            } };
/*
 *                  HEAP ALLOCATOR
 */

/**
 * @brief  heap-based allocator (simple proxy for malloc/free/realloc)
 */
IAllocator
AllocatorDST_create(void)
{
    uassert(allocator__dst_data.magic == 0 && "Already initialized");

    allocator__dst_data.magic = ALLOCATOR_DST_MAGIC;

#ifndef NDEBUG
    memset(&allocator__dst_data.stats, 0, sizeof(allocator__dst_data.stats));
#endif

    return &allocator__dst_data.base;
}

IAllocator
AllocatorDST_destroy(void)
{
    uassert(allocator__dst_data.magic != 0 && "Already destroyed");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");

    allocator__dst_data.magic = 0;

#ifndef NDEBUG
    allocator_dst_s* a = &allocator__dst_data;
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

    memset(&allocator__dst_data.stats, 0, sizeof(allocator__dst_data.stats));
#endif

    return NULL;
}

static void*
_cex_allocator_dst__malloc(usize size)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");

    if (DST.rnd.check(_DST.prm.allocators.malloc_fail_prob)) {
        return NULL;
    }

#ifndef NDEBUG
    allocator__dst_data.stats.n_allocs++;
#endif
    return malloc(size);
}

static void*
_cex_allocator_dst__calloc(usize nmemb, usize size)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");


    if (DST.rnd.check(_DST.prm.allocators.malloc_fail_prob)) {
        return NULL;
    }

#ifndef NDEBUG
    allocator__dst_data.stats.n_allocs++;
#endif
    return calloc(nmemb, size);
}

static void*
_cex_allocator_dst__aligned_malloc(usize alignment, usize size)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(size % alignment == 0 && "size must be rounded to align");

    if (DST.rnd.check(_DST.prm.allocators.malloc_fail_prob)) {
        return NULL;
    }

#ifndef NDEBUG
    allocator__dst_data.stats.n_allocs++;
#endif

#ifdef _WIN32
    return _aligned_malloc(alignment, size);
#else
    return aligned_alloc(alignment, size);
#endif
}

static void*
_cex_allocator_dst__realloc(void* ptr, usize size)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");

    if (DST.rnd.check(_DST.prm.allocators.malloc_fail_prob)) {
        return NULL;
    }

#ifndef NDEBUG
    allocator__dst_data.stats.n_reallocs++;
#endif

    return realloc(ptr, size);
}

static void*
_cex_allocator_dst__aligned_realloc(void* ptr, usize alignment, usize size)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");
    uassert(alignment > 0 && "alignment == 0");
    uassert((alignment & (alignment - 1)) == 0 && "alignment must be power of 2");
    uassert(((usize)ptr % alignment) == 0 && "aligned_realloc existing pointer unaligned");
    uassert(size % alignment == 0 && "size must be rounded to align");

    if (DST.rnd.check(_DST.prm.allocators.malloc_fail_prob)) {
        return NULL;
    }

#ifndef NDEBUG
    allocator__dst_data.stats.n_reallocs++;
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
_cex_allocator_dst__free(void* ptr)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");

#ifndef NDEBUG
    if (ptr != NULL) {
        allocator__dst_data.stats.n_free++;
    }
#endif

    free(ptr);
}

static FILE*
_cex_allocator_dst__fopen(const char* filename, const char* mode)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");
    uassert(filename != NULL);
    uassert(mode != NULL);

    if (DST.rnd.check(_DST.prm.allocators.open_fail_prob)) {
        errno = 0xBAAD; // some non-existing errno, but always not zero

        if (_DST.prm.allocators.open_fail_errnos_len > 0) {
            uassert(
                _DST.prm.allocators.open_fail_errnos_len <=
                arr$len(_DST.prm.allocators.open_fail_errnos)
            );

            if (DST.rnd.check(0.95)) {
                // With 95% probability replace errno 0xBAAD by something else in error list
                usize ierr = DST.rnd.range(0, _DST.prm.allocators.open_fail_errnos_len);
                i32 e = _DST.prm.allocators.open_fail_errnos[ierr];
                uassertf(
                    e != 0,
                    "DST_STATE.allocators.open_fail_errnos[%zu] is zero, EOK (not allowed) or wrong len",
                    ierr
                );
                errno = e;
            }
        }

        return NULL;
    }

    FILE* res = fopen(filename, mode);
    (void)res;

#ifndef NDEBUG
    if (res != NULL) {
        allocator__dst_data.stats.n_fopen++;
    }
#endif

    return res;
}

static int
_cex_allocator_dst__open(const char* pathname, int flags, unsigned int mode)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");
    uassert(pathname != NULL);

    if (DST.rnd.check(_DST.prm.allocators.open_fail_prob)) {
        errno = 0xBAAD; // some non-existing errno, but always not zero

        if (_DST.prm.allocators.open_fail_errnos_len > 0) {
            uassert(
                _DST.prm.allocators.open_fail_errnos_len <=
                arr$len(_DST.prm.allocators.open_fail_errnos)
            );

            if (DST.rnd.check(0.95)) {
                // With 95% probability replace errno 0xBAAD by something else in error list
                usize ierr = DST.rnd.range(0, _DST.prm.allocators.open_fail_errnos_len);
                i32 e = _DST.prm.allocators.open_fail_errnos[ierr];
                uassertf(
                    e != 0,
                    "DST_STATE.allocators.open_fail_errnos[%zu] is zero, EOK (not allowed) or wrong len",
                    ierr
                );
                errno = e;
            }
        }

        return -1;
    }
    int fd = open(pathname, flags, mode);
    (void)fd;

#ifndef NDEBUG
    if (fd != -1) {
        allocator__dst_data.stats.n_open++;
    }
#endif

    return fd;
}

static int
_cex_allocator_dst__close(int fd)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");

    if (DST.rnd.check(_DST.prm.allocators.close_fail_prob)) {
        errno = 0xBAAD; // some non-existing errno, but always not zero

        if (_DST.prm.allocators.close_fail_errnos_len > 0) {
            uassert(
                _DST.prm.allocators.close_fail_errnos_len <=
                arr$len(_DST.prm.allocators.close_fail_errnos)
            );

            if (DST.rnd.check(0.95)) {
                // With 95% probability replace errno 0xBAAD by something else in error list
                usize ierr = DST.rnd.range(0, _DST.prm.allocators.close_fail_errnos_len);
                i32 e = _DST.prm.allocators.close_fail_errnos[ierr];
                uassertf(
                    e != 0,
                    "DST_STATE.allocators.close_fail_errnos[%zu] is zero, EOK (not allowed) or wrong len",
                    ierr
                );
                errno = e;
            }
        }

        return -1;
    }
    int ret = close(fd);
    (void)ret;

#ifndef NDEBUG
    if (ret != -1) {
        allocator__dst_data.stats.n_close++;
    }
#endif

    return ret;
}

static int
_cex_allocator_dst__fclose(FILE* f)
{
    uassert(allocator__dst_data.magic != 0 && "Allocator not initialized");
    uassert(allocator__dst_data.magic == ALLOCATOR_DST_MAGIC && "Allocator type!");

    uassert(f != NULL);
    uassert(f != stdin && "closing stdin");
    uassert(f != stdout && "closing stdout");
    uassert(f != stderr && "closing stderr");

    if (DST.rnd.check(_DST.prm.allocators.close_fail_prob)) {
        errno = 0xBAAD; // some non-existing errno, but always not zero

        if (_DST.prm.allocators.close_fail_errnos_len > 0) {
            uassert(
                _DST.prm.allocators.close_fail_errnos_len <=
                arr$len(_DST.prm.allocators.close_fail_errnos)
            );

            if (DST.rnd.check(0.95)) {
                // With 95% probability replace errno 0xBAAD by something else in error list
                usize ierr = DST.rnd.range(0, _DST.prm.allocators.close_fail_errnos_len);
                i32 e = _DST.prm.allocators.close_fail_errnos[ierr];
                uassertf(
                    e != 0,
                    "DST_STATE.allocators.close_fail_errnos[%zu] is zero, EOK (not allowed) or wrong len",
                    ierr
                );
                errno = e;
            }
        }

        return -1;
    }
    int ret = fclose(f);
    (void)ret;

#ifndef NDEBUG
    if (ret != -1) {
        allocator__dst_data.stats.n_fclose++;
    }
#endif

    return ret;
}

const struct __class__AllocatorDST AllocatorDST = {
    // Autogenerated by CEX
    // clang-format off
    .create = AllocatorDST_create,
    .destroy = AllocatorDST_destroy,
    // clang-format on
};
