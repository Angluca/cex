#pragma once
#include "mem.h"

typedef struct
{
    alignas(64) const Allocator_i alloc;
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
extern IAllocator const _cex__default_global__allocator_heap__allc;
