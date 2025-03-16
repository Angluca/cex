#include "AllocatorArena.h"

#define CEX_ARENA_MAX_ALLOC UINT32_MAX - 1000
#define CEX_ARENA_MAX_ALIGN 64

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
    usize base_page_size = page_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN;

    if (alloc_size > 0.7 * base_page_size) {
        if (alloc_size > 1024 * 1024) {
            // TODO: align to closest system page size ? 4096 maybe?
            return alloc_size * 1.1 + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN;
        } else {
            usize result = mem$next_pow2(
                alloc_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN
            );
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
    if (alloc_size == 0 || alloc_size > CEX_ARENA_MAX_ALLOC || alignment > CEX_ARENA_MAX_ALIGN) {
        uassert(alloc_size > 0);
        uassert(alloc_size <= CEX_ARENA_MAX_ALLOC && "allocation size is too high");
        uassert(alignment <= CEX_ARENA_MAX_ALIGN);
        return (allocator_arena_rec_s){ 0 };
    }
    usize size = alloc_size;

    if (alignment < 8) {
        _Static_assert(sizeof(allocator_arena_rec_s) == 8, "unexpected size");
        _Static_assert(alignof(void*) <= 8, "unexpected ptr alignment");
        alignment = 8;
        size = mem$aligned_round(alloc_size, 8);
    } else {
        uassert(mem$is_power_of2(alignment) && "must be pow2");
        if ((alloc_size & (alignment - 1)) != 0) {
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
    uassert(size < alloc_size + 128 && "weird overflow");

    return (allocator_arena_rec_s){
        .size = alloc_size, // original size of allocation
        .ptr_offset = 0,    // offset from allocator_arena_rec_s to pointer (will be set later!)
        .ptr_alignment = alignment, // expected pointer alignment
        .ptr_padding = size - alloc_size - sizeof(allocator_arena_rec_s),
    };
}

static inline allocator_arena_rec_s*
_cex_alloc_arena__get_rec(void* alloc_pointer)
{
    uassert(alloc_pointer != NULL);
    u8 offset = *((u8*)alloc_pointer - 1);
    uassert(offset <= CEX_ARENA_MAX_ALIGN);
    return (allocator_arena_rec_s*)((char*)alloc_pointer - offset);
}

static allocator_arena_page_s*
_cex_allocator_arena__request_page_size(
    AllocatorArena_c* self,
    allocator_arena_rec_s new_rec,
    bool* out_is_allocated
)
{
    usize req_size = new_rec.size + new_rec.ptr_alignment + new_rec.ptr_padding;
    if (out_is_allocated) {
        *out_is_allocated = false;
    }

    if (self->last_page == NULL || self->last_page->capacity - self->last_page->cursor < req_size) {
        usize page_size = _cex_alloc_estimate_page_size(self->page_size, req_size);

        if (page_size == 0 || page_size > CEX_ARENA_MAX_ALLOC) {
            uassert(page_size > 0 && "page_size is zero");
            uassert(page_size <= CEX_ARENA_MAX_ALLOC && "page_size is to big");
            return false;
        }
        allocator_arena_page_s*
            page = mem$->calloc(mem$, 1, page_size, alignof(allocator_arena_page_s));
        if (page == NULL) {
            return NULL; // memory error
        }
        uassert(mem$aligned_pointer(page, 64) == page);

        page->prev_page = self->last_page;
        page->used_start = self->used;
        page->capacity = page_size - sizeof(allocator_arena_page_s);
        self->last_page = page;
        mem$asan_poison(page->__poison_area, sizeof(page->__poison_area));
        mem$asan_poison(&page->data, page->capacity);

        if (out_is_allocated) {
            *out_is_allocated = true;
        }
    }

    return self->last_page;
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

    // TODO: check if existing page has enough capacity + adding extra pages
    allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(self, rec, NULL);
    if (page == NULL) {
        return NULL;
    }
    uassert(page->capacity - page->cursor >= rec.size + rec.ptr_padding + rec.ptr_alignment);
    uassert(page->cursor % 8 == 0);

    allocator_arena_rec_s* page_rec = (allocator_arena_rec_s*)&page->data[page->cursor];
    uassert((((usize)(page_rec) & ((8) - 1)) == 0) && "unaligned pointer");
    _Static_assert(sizeof(allocator_arena_rec_s) == 8, "unexpected size");
    _Static_assert(alignof(allocator_arena_rec_s) <= 8, "unexpected alignment");

    mem$asan_unpoison(page_rec, sizeof(allocator_arena_rec_s));
    *page_rec = rec;

    void* result = mem$aligned_pointer(
        (char*)page_rec + sizeof(allocator_arena_rec_s),
        rec.ptr_alignment
    );

    uassert((char*)result >= ((char*)page_rec) + sizeof(allocator_arena_rec_s));
    rec.ptr_offset = (char*)result - (char*)page_rec;
    uassert(rec.ptr_offset <= rec.ptr_alignment);

    page_rec->ptr_offset = rec.ptr_offset;
    uassert(rec.ptr_alignment <= CEX_ARENA_MAX_ALIGN);
    uassert(rec.ptr_padding <= 8);

    mem$asan_unpoison(((char*)result) - 1, rec.size + 1);
    *(((char*)result) - 1) = rec.ptr_offset;

    usize bytes_alloc = rec.ptr_offset + rec.size + rec.ptr_padding;
    self->used += bytes_alloc;
    self->stats.bytes_alloc += bytes_alloc;
    page->cursor += bytes_alloc;
    page->last_alloc = result;
    uassert(page->cursor % 8 == 0);
    uassert(self->used % 8 == 0);
    uassert(((usize)(result) & ((rec.ptr_alignment) - 1)) == 0);

    return result;
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
_cex_allocator_arena__free(IAllocator allc, void* ptr)
{
    (void)ptr;
    // NOTE: this intentionally does nothing, all memory releasing in scope_exit()
    _cex_allocator_arena__validate(allc);

    if (ptr == NULL) {
        return NULL;
    }

    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(ptr);
    rec->is_free = true;
    mem$asan_poison(ptr, rec->size);

    return NULL;
}

static void*
_cex_allocator_arena__realloc(IAllocator allc, void* old_ptr, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    uassert(old_ptr != NULL);
    uassert(size > 0);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    uassert(self->scope_depth > 0 && "arena allocation must be performed in mem$scope() block!");

    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(old_ptr);
    uassert(!rec->is_free && "trying to realloc() already freed pointer");
    if (alignment < 8) {
        uassert(rec->ptr_alignment == 8);
    } else {
        uassert(alignment == rec->ptr_alignment && "realloc alignment mismatch with old_ptr");
        uassert(((usize)(old_ptr) & ((alignment)-1)) == 0 && "weird old_ptr not aligned");
        uassert(((usize)(size) & ((alignment)-1)) == 0 && "size is not aligned as expected");
    }

    if (size <= rec->size) {
        uassert(size >= rec->ptr_alignment);
        uassert(false && "TODO: implement shrinking");
        return old_ptr;
    }

    if (unlikely(self->last_page && self->last_page->last_alloc == old_ptr)) {
        allocator_arena_rec_s nrec = _cex_alloc_estimate_alloc_size(size, alignment);
        if (nrec.size == 0) {
            return NULL;
        }
        bool is_created = false;
        allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(
            self,
            nrec,
            &is_created
        );
        if (page == NULL) {
            return NULL;
        }
        if (!is_created) {
            // If new page was created, fall back to malloc/copy/free method
            //   but currently we have spare capacity for growth
            u32 extra_bytes = size - rec->size;
            mem$asan_unpoison((char*)old_ptr + rec->size, extra_bytes);
            memset((char*)old_ptr + rec->size, 0, extra_bytes);
            page->cursor += extra_bytes;
            self->used += extra_bytes;
            self->stats.bytes_alloc += extra_bytes;
            rec->size = size;
            mem$asan_poison((char*)old_ptr + size, rec->ptr_padding);
            return old_ptr;
        }
        // NOTE: fall through to default way
    }

    void* new_ptr = _cex_allocator_arena__malloc(allc, size, alignment);
    if (new_ptr == NULL) {
        return NULL;
    }
    memcpy(new_ptr, old_ptr, rec->size);
    memset((char*)new_ptr + rec->size, 0, size - rec->size);
    _cex_allocator_arena__free(allc, old_ptr);
    return new_ptr;
}


static const struct Allocator2_i*
_cex_allocator_arena__scope_enter(IAllocator allc)
{
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    // NOTE: If scope_depth is higher CEX_ALLOCATOR_MAX_SCOPE_STACK, we stop marking
    //  all memory will be released after exiting scope_depth == CEX_ALLOCATOR_MAX_SCOPE_STACK
    if (self->scope_depth < arr$len(self->scope_stack)) {
        self->scope_stack[self->scope_depth] = self->used;
    }
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
    if (self->scope_depth >= arr$len(self->scope_stack)) {
        // Scope overflow, wait until we reach CEX_ALLOCATOR_MAX_SCOPE_STACK
        return;
    }
    usize used_mark = self->scope_stack[self->scope_depth];

    allocator_arena_page_s* page = self->last_page;
    while (page) {
        var tpage = page->prev_page;
        if (page->used_start <= used_mark) {
            // last page, just set mark and poison
            usize free_offset = (used_mark - page->used_start);
            uassert(page->cursor >= free_offset);

            usize free_len = page->cursor - free_offset;
            page->cursor = used_mark;
            mem$asan_poison(&page->data[free_offset], free_len);

            uassert(self->used >= free_len);
            self->used -= free_len;
            self->stats.bytes_free += free_len;
            break; // we are done
        } else {
            uassert(false && "TODO: free full page");
        }
        // page->last_alloc = NULL; // TODO: invalid?

        // if (page->used_start + page->cursor)
        // mem$free(mem$, page);
        page = tpage;
    }
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

bool
AllocatorArena_sanitize(IAllocator allc)
{
    (void)allc;
    _cex_allocator_arena__validate(allc);
    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    if (self->scope_depth == 0) {
        uassert(self->stats.bytes_alloc == self->stats.bytes_free && "memory leaks?");
    }
    allocator_arena_page_s* page = self->last_page;
    while (page) {
        uassert(page->cursor < page->capacity);
        uassert(mem$asan_poison_check(page->__poison_area, sizeof(page->__poison_area)));

        u32 i = 0;
        while (i < page->cursor) {
            allocator_arena_rec_s* rec = (allocator_arena_rec_s*)&page->data[i];
            uassert(rec->size <= page->capacity);
            uassert(rec->size <= page->cursor);
            uassert(rec->ptr_offset <= CEX_ARENA_MAX_ALIGN);
            uassert(rec->ptr_padding <= 8);
            uassert(rec->ptr_alignment <= CEX_ARENA_MAX_ALIGN);
            uassert(rec->is_free <= 1);
            uassert(mem$is_power_of2(rec->ptr_alignment));

            char* alloc_p = ((char*)rec) + rec->ptr_offset;
            u8 poffset = alloc_p[-1];
            uassert(poffset == rec->ptr_offset && "near pointer offset mismatch to rec.ptr_offset");

            if (rec->ptr_padding) {
                uassert(
                    mem$asan_poison_check(alloc_p + rec->size, rec->ptr_padding) &&
                    "poison data overwrite past allocated item"
                );
            }

            if (rec->is_free) {
                uassert(
                    mem$asan_poison_check(alloc_p, rec->size) &&
                    "poison data corruction in freed item area"
                );
            }
            i += rec->ptr_padding + rec->ptr_offset + rec->size;
        }
        if (page->cursor < page->capacity) {
            // unallocated page must be poisoned
            uassert(
                mem$asan_poison_check(&page->data[page->cursor], page->capacity - page->cursor) &&
                "poison data overwrite in unallocated area"
            );
        }

        page = page->prev_page;
    }

    return true;
}

void
AllocatorArena_destroy(IAllocator self)
{
    _cex_allocator_arena__validate(self);
    AllocatorArena_c* allc = (AllocatorArena_c*)self;
#ifdef CEXTEST
    uassert(AllocatorArena_sanitize(self));
#endif

    allocator_arena_page_s* page = allc->last_page;
    while (page) {
        var tpage = page->prev_page;
        mem$free(mem$, page);
        page = tpage;
    }
    mem$free(mem$, allc);
}
