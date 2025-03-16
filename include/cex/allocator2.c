#pragma once
#include "allocator2.h"


// clang-format off
static void* _cex_allocator_heap__malloc(IAllocator self,usize size, usize alignment);
static void* _cex_allocator_heap__calloc(IAllocator self,usize nmemb, usize size, usize alignment);
static void* _cex_allocator_heap__realloc(IAllocator self,void* ptr, usize size, usize alignment);
static void* _cex_allocator_heap__free(IAllocator self,void* ptr);
static const struct Allocator2_i*  _cex_allocator_heap__scope_enter(IAllocator self);
static void _cex_allocator_heap__scope_exit(IAllocator self);
static u32 _cex_allocator_heap__scope_depth(IAllocator self);
// clang-format on

AllocatorHeap_c _cex__default_global__allocator_heap = {
    .alloc = {
        .malloc = _cex_allocator_heap__malloc,
        .realloc = _cex_allocator_heap__realloc,
        .calloc = _cex_allocator_heap__calloc,
        .free = _cex_allocator_heap__free,
        .scope_enter = _cex_allocator_heap__scope_enter,
        .scope_exit = _cex_allocator_heap__scope_exit,
        .scope_depth = _cex_allocator_heap__scope_depth,
        .meta = {
            .magic_id =CEX_ALLOCATOR_HEAP_MAGIC,
            .is_arena = false,
            .is_temp = false, 
        }
    },
};
IAllocator _cex__default_global__allocator_heap__allc = &_cex__default_global__allocator_heap.alloc;

thread_local AllocatorHeap_c _cex__default_global__allocator_temp = {
    .alloc = {
        .malloc = _cex_allocator_heap__malloc,
        .realloc = _cex_allocator_heap__realloc,
        .calloc = _cex_allocator_heap__calloc,
        .free = _cex_allocator_heap__free,
        .scope_enter = _cex_allocator_heap__scope_enter,
        .scope_exit = _cex_allocator_heap__scope_exit,
        .scope_depth = _cex_allocator_heap__scope_depth,
        .meta = {
            // .magic_id = CEX_ALLOCATOR_TEMP_MAGIC, // TODO: another magic for temp
            .magic_id = CEX_ALLOCATOR_HEAP_MAGIC,
            .is_arena = true,  // coming... soon
            .is_temp = true, 
        }
    },
};

static void
_cex_allocator_heap__validate(IAllocator self)
{
    (void)self;
#ifndef NDEBUG
    uassert(self != NULL);
    uassert(
        self->meta.magic_id == CEX_ALLOCATOR_HEAP_MAGIC &&
        "bad allocator pointer or mem corruption"
    );
#endif
}

static void*
_cex_allocator_heap__malloc(IAllocator self, usize size, usize alignment)
{
    _cex_allocator_heap__validate(self);
    uassert(alignment <= alignof(size_t) && "TODO: implement aligned version");
    AllocatorHeap_c* a = (AllocatorHeap_c*)self;
    a->stats.n_allocs++;
    return malloc(size);
}
static void*
_cex_allocator_heap__calloc(IAllocator self, usize nmemb, usize size, usize alignment)
{
    _cex_allocator_heap__validate(self);

    if (alignment <= alignof(usize)) {
        return calloc(nmemb, size);
    }

#ifdef _WIN32
    void* result = _aligned_malloc(alignment, size*nmemb);
#else
    void* result = aligned_alloc(alignment, size*nmemb);
#endif

    memset(result, 0, size*nmemb);    
    return result;
}
static void*
_cex_allocator_heap__realloc(IAllocator self, void* ptr, usize size, usize alignment)
{
    _cex_allocator_heap__validate(self);
    uassert(ptr != NULL);
    uassert(alignment <= alignof(size_t) && "TODO: implement aligned version");
    return realloc(ptr, size);
}

static void*
_cex_allocator_heap__free(IAllocator self, void* ptr)
{
    _cex_allocator_heap__validate(self);
    if (ptr != NULL) {
        free(ptr);
    }
    return NULL;
}

static const struct Allocator2_i*  
_cex_allocator_heap__scope_enter(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    printf("_cex_allocator_heap__scope_enter\n");
    // uassert(false && "this only supported by arenas");
    // abort();
    return self;
}
static void
_cex_allocator_heap__scope_exit(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    printf("_cex_allocator_heap__scope_exit\n");
    // uassert(false && "this only supported by arenas");
    // abort();
}
static u32
_cex_allocator_heap__scope_depth(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    return 1; // always 1
}
