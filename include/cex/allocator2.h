#pragma once
#include "cex.h"
#include <stddef.h>
#include <threads.h>

#define CEX_ALLOCATOR_HEAP_MAGIC 0xF00dBa01
#define CEX_ALLOCATOR_TEMP_MAGIC 0xF00dBeef
#define CEX_ALLOCATOR_ARENA_MAGIC 0xFeedF001

// clang-format off
#define IAllocator const struct Allocator2_i* const 
typedef struct Allocator2_i
{
    // >>> cacheline
    alignas(64) void* (*const malloc)(IAllocator self, usize size, usize alignment);
    void* (*const calloc)(IAllocator self, usize nmemb, usize size, usize alignment);
    void* (*const realloc)(IAllocator self, void* ptr, usize new_size, usize alignment);
    void* (*const free)(IAllocator self, void* ptr);
    const struct Allocator2_i* (*const scope_enter)(IAllocator self);   /* Only for arenas/temp alloc! */
    void (*const scope_exit)(IAllocator self);    /* Only for arenas/temp alloc! */
    u32 (*const scope_depth)(IAllocator self);  /* Current mem$scope depth */
    struct {
        u32 magic_id;
        bool is_arena;
        bool is_temp;
    } meta;
    //<<< 64 byte cacheline
} Allocator2_i;
// clang-format on
_Static_assert(alignof(Allocator2_i) == 64, "size");
_Static_assert(sizeof(Allocator2_i) == 64, "size");
_Static_assert(sizeof((Allocator2_i){ 0 }.meta) == 8, "size");

typedef struct
{
    alignas(64) const Allocator2_i alloc;
    // below goes sanity check stuff
    struct
    {
        u32 n_allocs;
        u32 n_reallocs;
        u32 n_free;
    } stats;
} AllocatorHeap_c;

_Static_assert(sizeof(AllocatorHeap_c) == 128, "size!");
_Static_assert(offsetof(AllocatorHeap_c, alloc) == 0, "base must be the 1st struct member");

extern AllocatorHeap_c _cex__default_global__allocator_heap;
extern IAllocator _cex__default_global__allocator_heap__allc;

void
_cex_allocator_memscope_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    (*allc)->scope_exit(*allc);
}

extern thread_local AllocatorHeap_c _cex__default_global__allocator_temp;
#define tmem$ ((IAllocator)(&_cex__default_global__allocator_temp.alloc))

#define mem$ _cex__default_global__allocator_heap__allc
#define mem$malloc(alloc, size) (alloc)->malloc((alloc), size, 0)
#define mem$free(alloc, ptr)                                                                       \
    ({                                                                                             \
        (ptr) = (alloc)->free((alloc), ptr);                                                       \
        (ptr) = NULL;                                                                              \
        (ptr);                                                                                     \
    })

// TODO: IDEA -- add optional argument (count)
#define mem$new(alloc, T) (typeof(T)*)(alloc)->calloc((alloc), 1, sizeof(T), _Alignof(T))
/*
 */

// clang-format off
#define mem$scope(alloc, allc_var)                                                                                                                                           \
    u32 cex$tmpname(tallc_cnt) = 0;                                                                                                                                \
    for (IAllocator allc_var  \
        __attribute__ ((__cleanup__(_cex_allocator_memscope_cleanup))) =  \
                                                        (alloc)->scope_enter(alloc); \
        cex$tmpname(tallc_cnt) < 1; \
        cex$tmpname(tallc_cnt)++)
// clang-format on
