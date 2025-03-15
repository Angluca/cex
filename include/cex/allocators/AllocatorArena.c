#include "AllocatorArena.h"
#include <cex/mem.h>

#define CEX_ARENA_MAX_ALLOC UINT32_MAX - 1000

static void
_cex_allocator_arena__validate(IAllocator self)
{
    (void)self;
#ifndef NDEBUG
    uassert(self != NULL);
    uassert(
        self->meta.magic_id == CEX_ALLOCATOR_ARENA_MAGIC &&
        "bad allocator pointer or mem corruption"
    );
#endif
}

static inline usize
_cex_alloc_estimate_page_size(usize page_size, usize alloc_size)
{
    uassert(alloc_size < CEX_ARENA_MAX_ALLOC && "allocation is to big");
    usize base_page_size = page_size + sizeof(allocator_arena_page_s) + 64;

    if (alloc_size > 0.7 * base_page_size) {
        if (alloc_size > 1024 * 1024) {
            return alloc_size * 1.1 + sizeof(allocator_arena_page_s) + 64;
        } else {
            usize result = mem$next_pow2(alloc_size + sizeof(allocator_arena_page_s) + 64);
            uassert(mem$is_power_of2(result));
            return result;
        }
    } else {
        return base_page_size;
    }
}
static allocator_arena_rec_s
_cex_alloc_estimate_alloc_size(usize alloc_size, usize alignment)
{
    if (alloc_size == 0 || alloc_size > CEX_ARENA_MAX_ALLOC || alignment > 64) {
        uassert(alloc_size > 0);
        uassert(alloc_size <= CEX_ARENA_MAX_ALLOC && "allocation size is too high");
        uassert(alignment <= 64);
        return (allocator_arena_rec_s){ 0 };
    }
    usize size = alloc_size;

    if (alignment < 8) {
        _Static_assert(sizeof(allocator_arena_rec_s) == 8, "unexpected size");
        _Static_assert(alignof(void*) <= 8, "unexpected ptr alignment");
        alignment = 8;
        size = mem$aligned_round(alloc_size, 8);
    } else {
        if (alloc_size % alignment != 0) {
            uassert(mem$is_power_of2(alignment) && "must be pow2");
            uassert(alloc_size % alignment == 0 && "requested size is not aligned");
            return (allocator_arena_rec_s){ 0 };
        }
    }
    size += sizeof(allocator_arena_rec_s);

#if mem$asan_enabled()
    if (size - alloc_size == sizeof(allocator_arena_rec_s)) {
        size += sizeof(allocator_arena_rec_s); // adding extra bytes for ASAN poison
    }
#endif
    uassert(size - alloc_size >= sizeof(allocator_arena_rec_s));
    uassert(size - alloc_size <= 255 - sizeof(allocator_arena_rec_s) && "ptr_offset oveflow");

    return (allocator_arena_rec_s){
        .size = alloc_size, // original size of allocation
        // offset from end allocator_arena_rec_s struct end
        .ptr_offset = alignment,
        // offset from end of allocated data to the next possible allocator_arena_rec_s
        .ptr_padding = size - alloc_size - sizeof(allocator_arena_rec_s),
    };
}

static allocator_arena_page_s*
_cex_allocator_arena__new_page(AllocatorArena_c* self, usize page_size)
{
    allocator_arena_page_s* page = mem$->calloc(mem$, 1, page_size, alignof(allocator_arena_page_s));
    if (page == NULL) {
        return NULL; // memory error
    }
    uassert(mem$aligned_pointer(page, alignof(allocator_arena_page_s)) == page);

    page->prev_page = self->last_page;
    page->used_start = self->used;
    page->capacity = page_size - sizeof(allocator_arena_page_s);
    mem$asan_poison(page->__poison_area, sizeof(page->__poison_area));

    return page;
}

static void*
_cex_allocator_arena__malloc(IAllocator allc, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    uassert(self->scope_depth > 0 && "arena allocation must be performed in mem$scope() block!");

    allocator_arena_rec_s rec = _cex_alloc_estimate_alloc_size(size, alignment);
    if (rec.size == 0) {
        return NULL;
    }

    // TODO: check if existing page has enough capacity
    if (self->last_page == NULL) {

        usize _page_size = _cex_alloc_estimate_page_size(self->page_size, size);
        if (_page_size > CEX_ARENA_MAX_ALLOC) {
            uassert(_page_size <= CEX_ARENA_MAX_ALLOC && "_page_size is to big");
            return NULL;
        }

        self->last_page = _cex_allocator_arena__new_page(self, _page_size);
        if (self->last_page == NULL) {
            return NULL; // memory error
        }
    }
    return malloc(size);
}
static void*
_cex_allocator_arena__calloc(IAllocator allc, usize nmemb, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);

    if (alignment <= alignof(usize)) {
        return calloc(nmemb, size);
    }

#ifdef _WIN32
    void* result = _aligned_malloc(alignment, size * nmemb);
#else
    void* result = aligned_alloc(alignment, size * nmemb);
#endif

    memset(result, 0, size * nmemb);
    return result;
}
static void*
_cex_allocator_arena__realloc(IAllocator allc, void* ptr, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    uassert(alignment <= alignof(size_t) && "TODO: implement aligned version");
    return realloc(ptr, size);
}

static void*
_cex_allocator_arena__free(IAllocator allc, void* ptr)
{
    (void)ptr;
    _cex_allocator_arena__validate(allc);
    return NULL;
}

static const struct Allocator2_i*
_cex_allocator_arena__scope_enter(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    self->scope_depth++;
    return allc;
}
static void
_cex_allocator_arena__scope_exit(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    uassert(self->scope_depth > 0);
    self->scope_depth--;
}
static u32
_cex_allocator_arena__scope_depth(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    return self->scope_depth;
}

const Allocator2_i*
AllocatorArena_create(usize page_size)
{
    if (page_size < 1024 || page_size >= UINT32_MAX) {
        uassert(page_size > 1024 && "page size is too small");
        uassert(page_size < UINT32_MAX && "page size is too big");
        return NULL;
    }

    AllocatorArena_c template = {
        .alloc = {
            .malloc = _cex_allocator_arena__malloc,
            .realloc = _cex_allocator_arena__realloc,
            .calloc = _cex_allocator_arena__calloc,
            .free = _cex_allocator_arena__free,
            .scope_enter = _cex_allocator_arena__scope_enter,
            .scope_exit = _cex_allocator_arena__scope_exit,
            .scope_depth = _cex_allocator_arena__scope_depth,
            .meta = {
                .magic_id = CEX_ALLOCATOR_ARENA_MAGIC,
                .is_arena = true, 
                .is_temp = false, 
            }
        },
        .page_size = page_size,
    };

    AllocatorArena_c* self = mem$new(mem$, AllocatorArena_c);
    if (self == NULL) {
        return NULL; // memory error
    }

    memcpy(self, &template, sizeof(AllocatorArena_c));
    uassert(self->alloc.meta.magic_id == CEX_ALLOCATOR_ARENA_MAGIC);
    uassert(self->alloc.malloc == _cex_allocator_arena__malloc);

    return &self->alloc;
}

void
AllocatorArena_destroy(IAllocator self)
{
    _cex_allocator_arena__validate(self);
    AllocatorArena_c* allc = (AllocatorArena_c*)self;
    mem$free(mem$, allc);
}
