#pragma once
#include "cex_base.h"

const struct _CEX_Error_struct Error = {
    .ok = EOK,                       // Success
    .memory = "MemoryError",         // memory allocation error
    .io = "IOError",                 // IO error
    .overflow = "OverflowError",     // buffer overflow
    .argument = "ArgumentError",     // function argument error
    .integrity = "IntegrityError",   // data integrity error
    .exists = "ExistsError",         // entity or key already exists
    .not_found = "NotFoundError",    // entity or key already exists
    .skip = "ShouldBeSkipped",       // NOT an error, function result must be skipped
    .empty = "EmptyError",           // resource is empty
    .eof = "EOF",                    // end of file reached
    .argsparse = "ProgramArgsError", // program arguments empty or incorrect
    .runtime = "RuntimeError",       // generic runtime error
    .assert = "AssertError",         // generic runtime check
    .os = "OSError",                 // generic OS check
    .timeout = "TimeoutError",       // await interval timeout
};


/*
*                   AllocatorArena.c
*/

#define CEX_ARENA_MAX_ALLOC UINT32_MAX - 1000
#define CEX_ARENA_MAX_ALIGN 64


static void
_cex_allocator_arena__validate(IAllocator self)
{
    (void)self;
#ifndef NDEBUG
    uassert(self != NULL);
    uassert(
        (self->meta.magic_id == CEX_ALLOCATOR_ARENA_MAGIC ||
         self->meta.magic_id == CEX_ALLOCATOR_TEMP_MAGIC) &&
        "bad allocator pointer or mem corruption"
    );
#endif
}


static inline usize
_cex_alloc_estimate_page_size(usize page_size, usize alloc_size)
{
    uassert(alloc_size < CEX_ARENA_MAX_ALLOC && "allocation is to big");
    usize base_page_size = mem$aligned_round(
        page_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN,
        alignof(allocator_arena_page_s)
    );
    uassert(base_page_size % alignof(allocator_arena_page_s) == 0 && "expected to be 64 aligned");

    if (alloc_size > 0.7 * base_page_size) {
        if (alloc_size > 1024 * 1024) {
            alloc_size *= 1.1;
            alloc_size += sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN;
        } else {
            alloc_size *= 2;
        }

        usize result = mem$aligned_round(
            alloc_size + sizeof(allocator_arena_page_s) + CEX_ARENA_MAX_ALIGN,
            alignof(allocator_arena_page_s)
        );
        uassert(result % alignof(allocator_arena_page_s) == 0 && "expected to be 64 aligned");
        return result;
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

    if (size - alloc_size == sizeof(allocator_arena_rec_s)) {
        size += sizeof(allocator_arena_rec_s); // adding extra bytes for ASAN poison
    }
    uassert(size - alloc_size >= sizeof(allocator_arena_rec_s));
    uassert(size - alloc_size <= 255 - sizeof(allocator_arena_rec_s) && "ptr_offset oveflow");
    uassert(size < alloc_size + 128 && "weird overflow");

    return (allocator_arena_rec_s){
        .size = alloc_size, // original size of allocation
        .ptr_offset = 0,    // offset from allocator_arena_rec_s to pointer (will be set later!)
        .ptr_alignment = alignment, // expected pointer alignment
        .ptr_padding = size - alloc_size - sizeof(allocator_arena_rec_s), // from last data to next
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

static bool
_cex_allocator_arena__check_pointer_valid(AllocatorArena_c* self, void* old_ptr)
{
    uassert(self->scope_depth > 0);
    allocator_arena_page_s* page = self->last_page;
    allocator_arena_rec_s* rec = _cex_alloc_arena__get_rec(old_ptr);
    while (page) {
        var tpage = page->prev_page;
        if ((char*)rec > (char*)page &&
            (char*)rec < (((char*)page) + sizeof(allocator_arena_page_s) + page->capacity)) {
            uassert((char*)rec >= (char*)page + sizeof(allocator_arena_page_s));

            u32 ptr_scope_mark =
                (((char*)rec) - ((char*)page) - sizeof(allocator_arena_page_s) + page->used_start);

            if (self->scope_depth < arr$len(self->scope_stack)) {
                if (ptr_scope_mark < self->scope_stack[self->scope_depth - 1]) {
                    uassert(
                        ptr_scope_mark >= self->scope_stack[self->scope_depth - 1] &&
                        "trying to operate on pointer from different mem$scope() it will lead to use-after-free / ASAN poison issues"
                    );
                    return false; // using pointer out of scope of previous page
                }
            }
            return true;
        }
        page = tpage;
    }
    return false;
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

    if (self->last_page == NULL ||
        // self->last_page->capacity < req_size + mem$aligned_round(self->last_page->cursor, 8)) {
        self->last_page->capacity < req_size + self->last_page->cursor) {
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
        mem$asan_poison(page->__poison_area, sizeof(page->__poison_area));
        mem$asan_poison(&page->data, page->capacity);

        self->last_page = page;
        self->stats.pages_created++;

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

    allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(self, rec, NULL);
    if (page == NULL) {
        return NULL;
    }
    uassert(page->capacity - page->cursor >= rec.size + rec.ptr_padding + rec.ptr_alignment);
    uassert(page->cursor % 8 == 0);
    uassert(rec.ptr_padding <= 8);

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


#ifdef CEXTEST
    // intentionally set malloc to 0xf7 pattern to mark uninitialized data
    memset(result, 0xf7, rec.size);
#endif

    return result;
}
static void*
_cex_allocator_arena__calloc(IAllocator allc, usize nmemb, usize size, usize alignment)
{
    _cex_allocator_arena__validate(allc);
    if (nmemb > CEX_ARENA_MAX_ALLOC) {
        uassert(nmemb < CEX_ARENA_MAX_ALLOC);
        return NULL;
    }
    if (size > CEX_ARENA_MAX_ALLOC) {
        uassert(size < CEX_ARENA_MAX_ALLOC);
        return NULL;
    }
    usize alloc_size = nmemb * size;
    void* result = _cex_allocator_arena__malloc(allc, alloc_size, alignment);
    if (result != NULL) {
        memset(result, 0, alloc_size);
    }

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

    AllocatorArena_c* self = (AllocatorArena_c*)allc;
    (void)self;
    uassert(
        _cex_allocator_arena__check_pointer_valid(self, ptr) && "pointer doesn't belong to arena"
    );
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
    uassert(size < CEX_ARENA_MAX_ALLOC);

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

    uassert(
        _cex_allocator_arena__check_pointer_valid(self, old_ptr) &&
        "pointer doesn't belong to arena"
    );

    if (unlikely(size <= rec->size)) {
        if (size == rec->size) {
            return old_ptr;
        }
        // we can't change size/padding of this allocation, because this will break iterating
        // ptr_padding is only u8 size, we cant store size, change, so we currently poison new size
        mem$asan_poison((char*)old_ptr + size, rec->size - size);
        return old_ptr;
    }

    if (unlikely(self->last_page && self->last_page->last_alloc == old_ptr)) {
        allocator_arena_rec_s nrec = _cex_alloc_estimate_alloc_size(size, alignment);
        if (nrec.size == 0) {
            goto fail;
        }
        bool is_created = false;
        allocator_arena_page_s* page = _cex_allocator_arena__request_page_size(
            self,
            nrec,
            &is_created
        );
        if (page == NULL) {
            goto fail;
        }
        if (!is_created) {
            // If new page was created, fall back to malloc/copy/free method
            //   but currently we have spare capacity for growth
            u32 extra_bytes = size - rec->size;
            mem$asan_unpoison((char*)old_ptr + rec->size, extra_bytes);
#ifdef CEXTEST
            memset((char*)old_ptr + rec->size, 0xf7, extra_bytes);
#endif
            extra_bytes += (nrec.ptr_padding - rec->ptr_padding);
            page->cursor += extra_bytes;
            self->used += extra_bytes;
            self->stats.bytes_alloc += extra_bytes;
            rec->size = size;
            rec->ptr_padding = nrec.ptr_padding;

            uassert((char*)rec + rec->size + rec->ptr_padding + rec->ptr_offset == &page->data[page->cursor]);
            uassert(page->cursor % 8 == 0);
            uassert(self->used % 8 == 0);
            mem$asan_poison((char*)old_ptr + size, rec->ptr_padding);
            return old_ptr;
        }
        // NOTE: fall through to default way
    }

    void* new_ptr = _cex_allocator_arena__malloc(allc, size, alignment);
    if (new_ptr == NULL) {
        goto fail;
    }
    memcpy(new_ptr, old_ptr, rec->size);
    _cex_allocator_arena__free(allc, old_ptr);
    return new_ptr;
fail:
    _cex_allocator_arena__free(allc, old_ptr);
    return NULL;
}


static const struct Allocator_i*
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

#ifdef CEXTEST
    bool AllocatorArena_sanitize(IAllocator allc);
    uassert(AllocatorArena_sanitize(allc));
#endif
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
            usize free_len = page->cursor;
            uassert(self->used >= free_len);

            self->used -= free_len;
            self->stats.bytes_free += free_len;
            self->last_page = page->prev_page;
            self->stats.pages_free++;
            mem$free(mem$, page);
        }
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

const Allocator_i*
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
        uassert(page->cursor <= page->capacity);
        uassert(mem$asan_poison_check(page->__poison_area, sizeof(page->__poison_area)));

        u32 i = 0;
        while (i < page->cursor) {
            allocator_arena_rec_s* rec = (allocator_arena_rec_s*)&page->data[i];
            uassert(rec->size <= page->capacity);
            uassert(rec->size <= page->cursor);
            uassert(rec->ptr_offset <= CEX_ARENA_MAX_ALIGN);
            uassert(rec->ptr_padding <= 16);
            uassert(rec->ptr_alignment <= CEX_ARENA_MAX_ALIGN);
            uassert(rec->is_free <= 1);
            uassert(mem$is_power_of2(rec->ptr_alignment));

            char* alloc_p = ((char*)rec) + rec->ptr_offset;
            u8 poffset = alloc_p[-1];
            (void)poffset;
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

thread_local AllocatorArena_c _cex__default_global__allocator_temp = {
    .alloc = {
        .malloc = _cex_allocator_arena__malloc,
        .realloc = _cex_allocator_arena__realloc,
        .calloc = _cex_allocator_arena__calloc,
        .free = _cex_allocator_arena__free,
        .scope_enter = _cex_allocator_arena__scope_enter,
        .scope_exit = _cex_allocator_arena__scope_exit,
        .scope_depth = _cex_allocator_arena__scope_depth,
        .meta = {
            .magic_id = CEX_ALLOCATOR_TEMP_MAGIC,
            .is_arena = true,  // coming... soon
            .is_temp = true,
        },
    },
    .page_size = CEX_ALLOCATOR_TEMP_PAGE_SIZE,
};

const struct __class__AllocatorArena AllocatorArena = {
    // Autogenerated by CEX
    // clang-format off
    .create = AllocatorArena_create,
    .sanitize = AllocatorArena_sanitize,
    .destroy = AllocatorArena_destroy,
    // clang-format on
};

/*
*                   AllocatorHeap.c
*/
#include <stdint.h>

// clang-format off
static void* _cex_allocator_heap__malloc(IAllocator self,usize size, usize alignment);
static void* _cex_allocator_heap__calloc(IAllocator self,usize nmemb, usize size, usize alignment);
static void* _cex_allocator_heap__realloc(IAllocator self,void* ptr, usize size, usize alignment);
static void* _cex_allocator_heap__free(IAllocator self,void* ptr);
static const struct Allocator_i*  _cex_allocator_heap__scope_enter(IAllocator self);
static void _cex_allocator_heap__scope_exit(IAllocator self);
static u32 _cex_allocator_heap__scope_depth(IAllocator self);

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
IAllocator const _cex__default_global__allocator_heap__allc = &_cex__default_global__allocator_heap.alloc;
// clang-format on


static void
_cex_allocator_heap__validate(IAllocator self)
{
    (void)self;
#ifndef NDEBUG
    uassert(self != NULL);
    uassert(
        self->meta.magic_id == CEX_ALLOCATOR_HEAP_MAGIC && "bad allocator pointer or mem corruption"
    );
#endif
}

static inline u64
_cex_allocator_heap__hdr_set(u64 size, u8 ptr_offset, u8 alignment)
{
    size &= 0xFFFFFFFFFFFF; // Mask to 48 bits
    return size | ((u64)ptr_offset << 48) | ((u64)alignment << 56);
}

static inline usize
_cex_allocator_heap__hdr_get_size(u64 alloc_hdr)
{
    return alloc_hdr & 0xFFFFFFFFFFFF;
}

static inline u8
_cex_allocator_heap__hdr_get_offset(u64 alloc_hdr)
{
    return (u8)((alloc_hdr >> 48) & 0xFF);
}

static inline u8
_cex_allocator_heap__hdr_get_alignment(u64 alloc_hdr)
{
    return (u8)(alloc_hdr >> 56);
}

static u64
_cex_allocator_heap__hdr_make(usize alloc_size, usize alignment)
{

    usize size = alloc_size;

    if (unlikely(
            alloc_size == 0 || alloc_size > PTRDIFF_MAX || (u64)alloc_size > (u64)0xFFFFFFFFFFFF ||
            alignment > 64
        )) {
        uassert(alloc_size > 0 && "zero size");
        uassert(alloc_size > PTRDIFF_MAX && "size is too high");
        uassert((u64)alloc_size < (u64)0xFFFFFFFFFFFF && "size is too high, or negative overflow");
        uassert(alignment <= 64);
        return 0;
    }

    if (alignment < 8) {
        _Static_assert(alignof(void*) <= 8, "unexpected ptr alignment");
        alignment = 8;
    } else {
        uassert(mem$is_power_of2(alignment) && "must be pow2");

        if ((alloc_size & (alignment - 1)) != 0) {
            uassert(alloc_size % alignment == 0 && "requested size is not aligned");
            return 0;
        }
    }
    size += sizeof(u64);

    // extra area for poisoning
    size += sizeof(u64);

    size = mem$aligned_round(size, alignment);

    return _cex_allocator_heap__hdr_set(size, 0, alignment);
}

static void*
_cex_allocator_heap__alloc(IAllocator self, u8 fill_val, usize size, usize alignment)
{
    _cex_allocator_heap__validate(self);
    AllocatorHeap_c* a = (AllocatorHeap_c*)self;

    u64 hdr = _cex_allocator_heap__hdr_make(size, alignment);
    if (hdr == 0) {
        return NULL;
    }

    usize full_size = _cex_allocator_heap__hdr_get_size(hdr);
    alignment = _cex_allocator_heap__hdr_get_alignment(hdr);

    // Memory alignment
    // |                 <hdr>|<poisn>|---<data>---
    // ^---malloc()
    u8* raw_result = NULL;
    if (fill_val != 0) {
        raw_result = malloc(full_size);
    } else {
        raw_result = calloc(1, full_size);
    }
    u8* result = raw_result;

    if (raw_result) {
        uassert((usize)result % sizeof(u64) == 0 && "malloc returned non 8 byte aligned ptr");

        result = mem$aligned_pointer(raw_result + sizeof(u64) * 2, alignment);
        uassert(mem$aligned_pointer(result, 8) == result);
        uassert(mem$aligned_pointer(result, alignment) == result);

#ifdef CEXTEST
        a->stats.n_allocs++;
        // intentionally set malloc to 0xf7 pattern to mark uninitialized data
        if (fill_val != 0) {
            memset(result, 0xf7, size);
        }
#endif
        usize ptr_offset = result - raw_result;
        uassert(ptr_offset >= sizeof(u64) * 2);
        uassert(ptr_offset <= 64 + 16);
        uassert(ptr_offset <= alignment + sizeof(u64) * 2);
        // poison area after header and before allocated pointer
        mem$asan_poison(result - sizeof(u64), sizeof(u64));
        ((u64*)result)[-2] = _cex_allocator_heap__hdr_set(size, ptr_offset, alignment);

        if (ptr_offset + size < full_size) {
            // Adding padding poison for non 8-byte aligned data
            mem$asan_poison(result + size, full_size - size - ptr_offset);
        }
    }

    return result;
}

static void*
_cex_allocator_heap__malloc(IAllocator self, usize size, usize alignment)
{
    return _cex_allocator_heap__alloc(self, 1, size, alignment);
}
static void*
_cex_allocator_heap__calloc(IAllocator self, usize nmemb, usize size, usize alignment)
{
    if (unlikely(nmemb == 0 || nmemb >= PTRDIFF_MAX)) {
        uassert(nmemb > 0 && "nmemb is zero");
        uassert(nmemb < PTRDIFF_MAX && "nmemb is too high or negative overflow");
        return NULL;
    }
    if (unlikely(size == 0 || size >= PTRDIFF_MAX)) {
        uassert(size > 0 && "size is zero");
        uassert(size < PTRDIFF_MAX && "size is too high or negative overflow");
        return NULL;
    }

    return _cex_allocator_heap__alloc(self, 0, size * nmemb, alignment);
}

static void*
_cex_allocator_heap__realloc(IAllocator self, void* ptr, usize size, usize alignment)
{
    _cex_allocator_heap__validate(self);
    if (unlikely(ptr == NULL)) {
        uassert(ptr != NULL);
        return NULL;
    }
    AllocatorHeap_c* a = (AllocatorHeap_c*)self;

    // Memory alignment
    // |                 <hdr>|<poisn>|---<data>---
    // ^---realloc()
    char* p = ptr;
    uassert(
        mem$asan_poison_check(p - sizeof(u64), sizeof(u64)) &&
        "corrupted pointer or unallocated by mem$"
    );
    u64 old_hdr = *(u64*)(p - sizeof(u64) * 2);
    uassert(old_hdr > 0 && "bad poitner or corrupted malloced header?");
    u64 old_size = _cex_allocator_heap__hdr_get_size(old_hdr);
    (void)old_size;
    u8 old_offset = _cex_allocator_heap__hdr_get_offset(old_hdr);
    u8 old_alignment = _cex_allocator_heap__hdr_get_alignment(old_hdr);
    uassert((old_alignment >= 8 && old_alignment <= 64) && "corrupted header?");
    uassert(old_offset <= 64 + sizeof(u64) * 2 && "corrupted header?");
    if (unlikely(
            (alignment <= 8 && old_alignment != 8) || (alignment > 8 && alignment != old_alignment)
        )) {
        uassert(alignment == old_alignment && "given alignment doesn't match to old one");
        goto fail;
    }

    u64 new_hdr = _cex_allocator_heap__hdr_make(size, alignment);
    if (unlikely(new_hdr == 0)) {
        goto fail;
    }
    usize new_full_size = _cex_allocator_heap__hdr_get_size(new_hdr);
    uassert(new_full_size > size);

    u8* raw_result = realloc(p - old_offset, new_full_size);
    if (unlikely(raw_result == NULL)) {
        goto fail;
    }
    u8* result = mem$aligned_pointer(raw_result + sizeof(u64) * 2, old_alignment);
    uassert(mem$aligned_pointer(result, 8) == result);
    uassert(mem$aligned_pointer(result, old_alignment) == result);

    usize ptr_offset = result - raw_result;
    uassert(ptr_offset <= 64 + 16);
    uassert(ptr_offset <= old_alignment + sizeof(u64) * 2);
    uassert(ptr_offset + size <= new_full_size);

#ifdef CEXTEST
    a->stats.n_reallocs++;
    if (old_size < size) {
        // intentionally set unallocated to 0xf7 pattern to mark uninitialized data
        memset(result + old_size, 0xf7, size - old_size);
    }
#endif
    mem$asan_poison(result - sizeof(u64), sizeof(u64));
    ((u64*)result)[-2] = _cex_allocator_heap__hdr_set(size, ptr_offset, old_alignment);

    if (ptr_offset + size < new_full_size) {
        // Adding padding poison for non 8-byte aligned data
        mem$asan_poison(result + size, new_full_size - size - ptr_offset);
    }

    return result;

fail:
    free(ptr);
    return NULL;
}

static void*
_cex_allocator_heap__free(IAllocator self, void* ptr)
{
    _cex_allocator_heap__validate(self);
    if (ptr != NULL) {
        AllocatorHeap_c* a = (AllocatorHeap_c*)self;

        char* p = ptr;
        uassert(
            mem$asan_poison_check(p - sizeof(u64), sizeof(u64)) &&
            "corrupted pointer or unallocated by mem$"
        );
        u64 hdr = *(u64*)(p - sizeof(u64) * 2);
        uassert(hdr > 0 && "bad poitner or corrupted malloced header?");
        u8 offset = _cex_allocator_heap__hdr_get_offset(hdr);
        u8 alignment = _cex_allocator_heap__hdr_get_alignment(hdr);
        uassert(alignment >= 8 && "corrupted header?");
        uassert(alignment <= 64 && "corrupted header?");
        uassert(offset >= 16 && "corrupted header?");
        uassert(offset <= 64 && "corrupted header?");

#ifdef CEXTEST
        a->stats.n_free++;
        u64 size = _cex_allocator_heap__hdr_get_size(hdr);
        u32 padding = mem$aligned_round(size + offset, alignment) - size - offset;
        if (padding > 0) {
            uassert(
                mem$asan_poison_check(p + size, padding) &&
                "corrupted area after unallocated size by mem$"
            );
        }
#endif

        free(p - offset);
    }
    return NULL;
}

static const struct Allocator_i*
_cex_allocator_heap__scope_enter(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    uassert(false && "this only supported by arenas");
    abort();
}

static void
_cex_allocator_heap__scope_exit(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    uassert(false && "this only supported by arenas");
    abort();
}

static u32
_cex_allocator_heap__scope_depth(IAllocator self)
{
    _cex_allocator_heap__validate(self);
    return 1; // always 1
}

/*
*                   _sprintf.c
*/
// stb_sprintf - v1.10 - public domain snprintf() implementation
// originally by Jeff Roberts / RAD Game Tools, 2015/10/20
// http://github.com/nothings/stb
//
// allowed types:  sc uidBboXx p AaGgEef n
// lengths      :  hh h ll j z t I64 I32 I
//
// Contributors:
//    Fabian "ryg" Giesen (reformatting)
//    github:aganm (attribute format)
//
// Contributors (bugfixes):
//    github:d26435
//    github:trex78
//    github:account-login
//    Jari Komppa (SI suffixes)
//    Rohit Nirmal
//    Marcin Wojdyr
//    Leonard Ritter
//    Stefano Zanotti
//    Adam Allison
//    Arvid Gerstmann
//    Markus Kolb
//
// LICENSE:
//
//   See end of file for license information.


/*
Single file sprintf replacement.

Originally written by Jeff Roberts at RAD Game Tools - 2015/10/20.
Hereby placed in public domain.

This is a full sprintf replacement that supports everything that
the C runtime sprintfs support, including float/double, 64-bit integers,
hex floats, field parameters (%*.*d stuff), length reads backs, etc.

It compiles to roughly 8K with float support, and 4K without.
As a comparison, when using MSVC static libs, calling sprintf drags
in 16K.


FLOATS/DOUBLES:
===============
This code uses a internal float->ascii conversion method that uses
doubles with error correction (double-doubles, for ~105 bits of
precision).  This conversion is round-trip perfect - that is, an atof
of the values output here will give you the bit-exact double back.

If you don't need float or doubles at all, define STB_SPRINTF_NOFLOAT
and you'll save 4K of code space.

64-BIT INTS:
============
This library also supports 64-bit integers and you can use MSVC style or
GCC style indicators (%I64d or %lld).  It supports the C99 specifiers
for size_t and ptr_diff_t (%jd %zd) as well.

EXTRAS:
=======
Like some GCCs, for integers and floats, you can use a ' (single quote)
specifier and commas will be inserted on the thousands: "%'d" on 12345
would print 12,345.

For integers and floats, you can use a "$" specifier and the number
will be converted to float and then divided to get kilo, mega, giga or
tera and then printed, so "%$d" 1000 is "1.0 k", "%$.2d" 2536000 is
"2.53 M", etc. For byte values, use two $:s, like "%$$d" to turn
2536000 to "2.42 Mi". If you prefer JEDEC suffixes to SI ones, use three
$:s: "%$$$d" -> "2.42 M". To remove the space between the number and the
suffix, add "_" specifier: "%_$d" -> "2.53M".

In addition to octal and hexadecimal conversions, you can print
integers in binary: "%b" for 256 would print 100.
*/

// SETTINGS

// #define CEX_SPRINTF_NOFLOAT // disables floating point code (2x less in size)
#ifndef CEX_SPRINTF_MIN
#define CEX_SPRINTF_MIN 512 // size of stack based buffer for small strings
#endif

// #define CEXSP_STATIC   // makes all functions static

#ifdef CEXSP_STATIC
#define CEXSP__PUBLICDEC static
#define CEXSP__PUBLICDEF static
#else
#define CEXSP__PUBLICDEC extern
#define CEXSP__PUBLICDEF
#endif

#define CEXSP__ATTRIBUTE_FORMAT(fmt, va) __attribute__((format(printf, fmt, va)))

typedef char* cexsp_callback_f(const char* buf, void* user, u32 len);

typedef struct cexsp__context
{
    char* buf;
    FILE* file;
    IAllocator allc;
    u32 capacity;
    u32 length;
    u32 has_error;
    char tmp[CEX_SPRINTF_MIN];
} cexsp__context;

// clang-format off
CEXSP__PUBLICDEF int cexsp__vfprintf(FILE* stream, const char* format, va_list va);
CEXSP__PUBLICDEF int cexsp__fprintf(FILE* stream, const char* format, ...);
CEXSP__PUBLICDEC int cexsp__vsnprintf(char* buf, int count, char const* fmt, va_list va);
CEXSP__PUBLICDEC int cexsp__snprintf(char* buf, int count, char const* fmt, ...) CEXSP__ATTRIBUTE_FORMAT(3, 4);
CEXSP__PUBLICDEC int cexsp__vsprintfcb(cexsp_callback_f* callback, void* user, char* buf, char const* fmt, va_list va);
CEXSP__PUBLICDEC void cexsp__set_separators(char comma, char period);
// clang-format on

/*
This code is based on refactored stb_sprintf.h

Original code
https://github.com/nothings/stb/tree/master
ALTERNATIVE A - MIT License
stb_sprintf - v1.10 - public domain snprintf() implementation
Copyright (c) 2017 Sean Barrett
*/

#ifndef CEX_SPRINTF_NOFLOAT
// internal float utility functions
static i32 cexsp__real_to_str(
    char const** start,
    u32* len,
    char* out,
    i32* decimal_pos,
    double value,
    u32 frac_digits
);
static i32 cexsp__real_to_parts(i64* bits, i32* expo, double value);
#define CEXSP__SPECIAL 0x7000
#endif

static char cexsp__period = '.';
static char cexsp__comma = ',';
static struct
{
    short temp; // force next field to be 2-byte aligned
    char pair[201];
} cexsp__digitpair = { 0,
                       "00010203040506070809101112131415161718192021222324"
                       "25262728293031323334353637383940414243444546474849"
                       "50515253545556575859606162636465666768697071727374"
                       "75767778798081828384858687888990919293949596979899" };

CEXSP__PUBLICDEF void
cexsp__set_separators(char pcomma, char pperiod)
{
    cexsp__period = pperiod;
    cexsp__comma = pcomma;
}

#define CEXSP__LEFTJUST 1
#define CEXSP__LEADINGPLUS 2
#define CEXSP__LEADINGSPACE 4
#define CEXSP__LEADING_0X 8
#define CEXSP__LEADINGZERO 16
#define CEXSP__INTMAX 32
#define CEXSP__TRIPLET_COMMA 64
#define CEXSP__NEGATIVE 128
#define CEXSP__METRIC_SUFFIX 256
#define CEXSP__HALFWIDTH 512
#define CEXSP__METRIC_NOSPACE 1024
#define CEXSP__METRIC_1024 2048
#define CEXSP__METRIC_JEDEC 4096

static void
cexsp__lead_sign(u32 fl, char* sign)
{
    sign[0] = 0;
    if (fl & CEXSP__NEGATIVE) {
        sign[0] = 1;
        sign[1] = '-';
    } else if (fl & CEXSP__LEADINGSPACE) {
        sign[0] = 1;
        sign[1] = ' ';
    } else if (fl & CEXSP__LEADINGPLUS) {
        sign[0] = 1;
        sign[1] = '+';
    }
}

static u32
cexsp__strlen_limited(char const* s, u32 limit)
{
    char const* sn = s;
    // handle the last few characters to find actual size
    while (limit && *sn) {
        ++sn;
        --limit;
    }

    return (u32)(sn - s);
}

CEXSP__PUBLICDEF int
cexsp__vsprintfcb(cexsp_callback_f* callback, void* user, char* buf, char const* fmt, va_list va)
{
    static char hex[] = "0123456789abcdefxp";
    static char hexu[] = "0123456789ABCDEFXP";
    char* bf;
    char const* f;
    int tlen = 0;

    bf = buf;
    f = fmt;
    for (;;) {
        i32 fw, pr, tz;
        u32 fl;

// macros for the callback buffer stuff
#define cexsp__chk_cb_bufL(bytes)                                                                  \
    {                                                                                              \
        int len = (int)(bf - buf);                                                                 \
        if ((len + (bytes)) >= CEX_SPRINTF_MIN) {                                                  \
            tlen += len;                                                                           \
            if (0 == (bf = buf = callback(buf, user, len)))                                        \
                goto done;                                                                         \
        }                                                                                          \
    }
#define cexsp__chk_cb_buf(bytes)                                                                   \
    {                                                                                              \
        if (callback) {                                                                            \
            cexsp__chk_cb_bufL(bytes);                                                             \
        }                                                                                          \
    }
#define cexsp__flush_cb()                                                                          \
    {                                                                                              \
        cexsp__chk_cb_bufL(CEX_SPRINTF_MIN - 1);                                                   \
    } // flush if there is even one byte in the buffer
#define cexsp__cb_buf_clamp(cl, v)                                                                 \
    cl = v;                                                                                        \
    if (callback) {                                                                                \
        int lg = CEX_SPRINTF_MIN - (int)(bf - buf);                                                \
        if (cl > lg)                                                                               \
            cl = lg;                                                                               \
    }

        // fast copy everything up to the next % (or end of string)
        for (;;) {
            if (f[0] == '%') {
                goto scandd;
            }
            if (f[0] == 0) {
                goto endfmt;
            }
            cexsp__chk_cb_buf(1);
            *bf++ = f[0];
            ++f;
        }
    scandd:

        ++f;

        // ok, we have a percent, read the modifiers first
        fw = 0;
        pr = -1;
        fl = 0;
        tz = 0;

        // flags
        for (;;) {
            switch (f[0]) {
                // if we have left justify
                case '-':
                    fl |= CEXSP__LEFTJUST;
                    ++f;
                    continue;
                // if we have leading plus
                case '+':
                    fl |= CEXSP__LEADINGPLUS;
                    ++f;
                    continue;
                // if we have leading space
                case ' ':
                    fl |= CEXSP__LEADINGSPACE;
                    ++f;
                    continue;
                // if we have leading 0x
                case '#':
                    fl |= CEXSP__LEADING_0X;
                    ++f;
                    continue;
                // if we have thousand commas
                case '\'':
                    fl |= CEXSP__TRIPLET_COMMA;
                    ++f;
                    continue;
                // if we have kilo marker (none->kilo->kibi->jedec)
                case '$':
                    if (fl & CEXSP__METRIC_SUFFIX) {
                        if (fl & CEXSP__METRIC_1024) {
                            fl |= CEXSP__METRIC_JEDEC;
                        } else {
                            fl |= CEXSP__METRIC_1024;
                        }
                    } else {
                        fl |= CEXSP__METRIC_SUFFIX;
                    }
                    ++f;
                    continue;
                // if we don't want space between metric suffix and number
                case '_':
                    fl |= CEXSP__METRIC_NOSPACE;
                    ++f;
                    continue;
                // if we have leading zero
                case '0':
                    fl |= CEXSP__LEADINGZERO;
                    ++f;
                    goto flags_done;
                default:
                    goto flags_done;
            }
        }
    flags_done:

        // get the field width
        if (f[0] == '*') {
            fw = va_arg(va, u32);
            ++f;
        } else {
            while ((f[0] >= '0') && (f[0] <= '9')) {
                fw = fw * 10 + f[0] - '0';
                f++;
            }
        }
        // get the precision
        if (f[0] == '.') {
            ++f;
            if (f[0] == '*') {
                pr = va_arg(va, u32);
                ++f;
            } else {
                pr = 0;
                while ((f[0] >= '0') && (f[0] <= '9')) {
                    pr = pr * 10 + f[0] - '0';
                    f++;
                }
            }
        }

        // handle integer size overrides
        switch (f[0]) {
            // are we halfwidth?
            case 'h':
                fl |= CEXSP__HALFWIDTH;
                ++f;
                if (f[0] == 'h') {
                    ++f; // QUARTERWIDTH
                }
                break;
            // are we 64-bit (unix style)
            case 'l':
                fl |= ((sizeof(long) == 8) ? CEXSP__INTMAX : 0);
                ++f;
                if (f[0] == 'l') {
                    fl |= CEXSP__INTMAX;
                    ++f;
                }
                break;
            // are we 64-bit on intmax? (c99)
            case 'j':
                fl |= (sizeof(size_t) == 8) ? CEXSP__INTMAX : 0;
                ++f;
                break;
            // are we 64-bit on size_t or ptrdiff_t? (c99)
            case 'z':
                fl |= (sizeof(ptrdiff_t) == 8) ? CEXSP__INTMAX : 0;
                ++f;
                break;
            case 't':
                fl |= (sizeof(ptrdiff_t) == 8) ? CEXSP__INTMAX : 0;
                ++f;
                break;
            // are we 64-bit (msft style)
            case 'I':
                if ((f[1] == '6') && (f[2] == '4')) {
                    fl |= CEXSP__INTMAX;
                    f += 3;
                } else if ((f[1] == '3') && (f[2] == '2')) {
                    f += 3;
                } else {
                    fl |= ((sizeof(void*) == 8) ? CEXSP__INTMAX : 0);
                    ++f;
                }
                break;
            default:
                break;
        }

        // handle each replacement
        switch (f[0]) {
#define CEXSP__NUMSZ 512 // big enough for e308 (with commas) or e-307
            char num[CEXSP__NUMSZ];
            char lead[8];
            char tail[8];
            char* s;
            char const* h;
            u32 l, n, cs;
            u64 n64;
#ifndef CEX_SPRINTF_NOFLOAT
            double fv;
#endif
            i32 dp;
            char const* sn;

            case 's':
                // get the string
                s = va_arg(va, char*);
                if ((void*)s <= (void*)(1024 * 1024)) {
                    if (s == 0) {
                        s = (char*)"(null)";
                    } else {
                        // NOTE: cex is str_s passed as %s, s will be length
                        // try to double check sensible value of pointer
                        s = (char*)"(str_c->%S)";
                    }
                }
                // get the length, limited to desired precision
                // always limit to ~0u chars since our counts are 32b
                l = cexsp__strlen_limited(s, (pr >= 0) ? (unsigned)pr : ~0u);
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                // copy the string in
                goto scopy;
            case 'S': {
                // NOTE: CEX extra (support of str_s)
                str_s sv = va_arg(va, str_s);
                s = sv.buf;
                if (s == 0) {
                    s = (char*)"(null)";
                    l = cexsp__strlen_limited(s, (pr >= 0) ? (unsigned)pr : ~0u);
                } else {
                    l = sv.len > 0xffffffff ? 0xffffffff : sv.len;
                }
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                // copy the string in
                goto scopy;
            }
            case 'c': // char
                // get the character
                s = num + CEXSP__NUMSZ - 1;
                *s = (char)va_arg(va, int);
                l = 1;
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                goto scopy;

            case 'n': // weird write-bytes specifier
            {
                int* d = va_arg(va, int*);
                *d = tlen + (int)(bf - buf);
            } break;

#ifdef CEX_SPRINTF_NOFLOAT
            case 'A':               // float
            case 'a':               // hex float
            case 'G':               // float
            case 'g':               // float
            case 'E':               // float
            case 'e':               // float
            case 'f':               // float
                va_arg(va, double); // eat it
                s = (char*)"No float";
                l = 8;
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                cs = 0;
                CEXSP__NOTUSED(dp);
                goto scopy;
#else
            case 'A': // hex float
            case 'a': // hex float
                h = (f[0] == 'A') ? hexu : hex;
                fv = va_arg(va, double);
                if (pr == -1) {
                    pr = 6; // default is 6
                }
                // read the double into a string
                if (cexsp__real_to_parts((i64*)&n64, &dp, fv)) {
                    fl |= CEXSP__NEGATIVE;
                }

                s = num + 64;

                cexsp__lead_sign(fl, lead);

                if (dp == -1023) {
                    dp = (n64) ? -1022 : 0;
                } else {
                    n64 |= (((u64)1) << 52);
                }
                n64 <<= (64 - 56);
                if (pr < 15) {
                    n64 += ((((u64)8) << 56) >> (pr * 4));
                }
                // add leading chars

                lead[1 + lead[0]] = '0';
                lead[2 + lead[0]] = 'x';
                lead[0] += 2;
                *s++ = h[(n64 >> 60) & 15];
                n64 <<= 4;
                if (pr) {
                    *s++ = cexsp__period;
                }
                sn = s;

                // print the bits
                n = pr;
                if (n > 13) {
                    n = 13;
                }
                if (pr > (i32)n) {
                    tz = pr - n;
                }
                pr = 0;
                while (n--) {
                    *s++ = h[(n64 >> 60) & 15];
                    n64 <<= 4;
                }

                // print the expo
                tail[1] = h[17];
                if (dp < 0) {
                    tail[2] = '-';
                    dp = -dp;
                } else {
                    tail[2] = '+';
                }
                n = (dp >= 1000) ? 6 : ((dp >= 100) ? 5 : ((dp >= 10) ? 4 : 3));
                tail[0] = (char)n;
                for (;;) {
                    tail[n] = '0' + dp % 10;
                    if (n <= 3) {
                        break;
                    }
                    --n;
                    dp /= 10;
                }

                dp = (int)(s - sn);
                l = (int)(s - (num + 64));
                s = num + 64;
                cs = 1 + (3 << 24);
                goto scopy;

            case 'G': // float
            case 'g': // float
                h = (f[0] == 'G') ? hexu : hex;
                fv = va_arg(va, double);
                if (pr == -1) {
                    pr = 6;
                } else if (pr == 0) {
                    pr = 1; // default is 6
                }
                // read the double into a string
                if (cexsp__real_to_str(&sn, &l, num, &dp, fv, (pr - 1) | 0x80000000)) {
                    fl |= CEXSP__NEGATIVE;
                }

                // clamp the precision and delete extra zeros after clamp
                n = pr;
                if (l > (u32)pr) {
                    l = pr;
                }
                while ((l > 1) && (pr) && (sn[l - 1] == '0')) {
                    --pr;
                    --l;
                }

                // should we use %e
                if ((dp <= -4) || (dp > (i32)n)) {
                    if (pr > (i32)l) {
                        pr = l - 1;
                    } else if (pr) {
                        --pr; // when using %e, there is one digit before the decimal
                    }
                    goto doexpfromg;
                }
                // this is the insane action to get the pr to match %g semantics for %f
                if (dp > 0) {
                    pr = (dp < (i32)l) ? l - dp : 0;
                } else {
                    pr = -dp + ((pr > (i32)l) ? (i32)l : pr);
                }
                goto dofloatfromg;

            case 'E': // float
            case 'e': // float
                h = (f[0] == 'E') ? hexu : hex;
                fv = va_arg(va, double);
                if (pr == -1) {
                    pr = 6; // default is 6
                }
                // read the double into a string
                if (cexsp__real_to_str(&sn, &l, num, &dp, fv, pr | 0x80000000)) {
                    fl |= CEXSP__NEGATIVE;
                }
            doexpfromg:
                tail[0] = 0;
                cexsp__lead_sign(fl, lead);
                if (dp == CEXSP__SPECIAL) {
                    s = (char*)sn;
                    cs = 0;
                    pr = 0;
                    goto scopy;
                }
                s = num + 64;
                // handle leading chars
                *s++ = sn[0];

                if (pr) {
                    *s++ = cexsp__period;
                }

                // handle after decimal
                if ((l - 1) > (u32)pr) {
                    l = pr + 1;
                }
                for (n = 1; n < l; n++) {
                    *s++ = sn[n];
                }
                // trailing zeros
                tz = pr - (l - 1);
                pr = 0;
                // dump expo
                tail[1] = h[0xe];
                dp -= 1;
                if (dp < 0) {
                    tail[2] = '-';
                    dp = -dp;
                } else {
                    tail[2] = '+';
                }
                n = (dp >= 100) ? 5 : 4;
                tail[0] = (char)n;
                for (;;) {
                    tail[n] = '0' + dp % 10;
                    if (n <= 3) {
                        break;
                    }
                    --n;
                    dp /= 10;
                }
                cs = 1 + (3 << 24); // how many tens
                goto flt_lead;

            case 'f': // float
                fv = va_arg(va, double);
            doafloat:
                // do kilos
                if (fl & CEXSP__METRIC_SUFFIX) {
                    double divisor;
                    divisor = 1000.0f;
                    if (fl & CEXSP__METRIC_1024) {
                        divisor = 1024.0;
                    }
                    while (fl < 0x4000000) {
                        if ((fv < divisor) && (fv > -divisor)) {
                            break;
                        }
                        fv /= divisor;
                        fl += 0x1000000;
                    }
                }
                if (pr == -1) {
                    pr = 6; // default is 6
                }
                // read the double into a string
                if (cexsp__real_to_str(&sn, &l, num, &dp, fv, pr)) {
                    fl |= CEXSP__NEGATIVE;
                }
            dofloatfromg:
                tail[0] = 0;
                cexsp__lead_sign(fl, lead);
                if (dp == CEXSP__SPECIAL) {
                    s = (char*)sn;
                    cs = 0;
                    pr = 0;
                    goto scopy;
                }
                s = num + 64;

                // handle the three decimal varieties
                if (dp <= 0) {
                    i32 i;
                    // handle 0.000*000xxxx
                    *s++ = '0';
                    if (pr) {
                        *s++ = cexsp__period;
                    }
                    n = -dp;
                    if ((i32)n > pr) {
                        n = pr;
                    }
                    i = n;
                    while (i) {
                        if ((((usize)s) & 3) == 0) {
                            break;
                        }
                        *s++ = '0';
                        --i;
                    }
                    while (i >= 4) {
                        *(u32*)s = 0x30303030;
                        s += 4;
                        i -= 4;
                    }
                    while (i) {
                        *s++ = '0';
                        --i;
                    }
                    if ((i32)(l + n) > pr) {
                        l = pr - n;
                    }
                    i = l;
                    while (i) {
                        *s++ = *sn++;
                        --i;
                    }
                    tz = pr - (n + l);
                    cs = 1 + (3 << 24); // how many tens did we write (for commas below)
                } else {
                    cs = (fl & CEXSP__TRIPLET_COMMA) ? ((600 - (u32)dp) % 3) : 0;
                    if ((u32)dp >= l) {
                        // handle xxxx000*000.0
                        n = 0;
                        for (;;) {
                            if ((fl & CEXSP__TRIPLET_COMMA) && (++cs == 4)) {
                                cs = 0;
                                *s++ = cexsp__comma;
                            } else {
                                *s++ = sn[n];
                                ++n;
                                if (n >= l) {
                                    break;
                                }
                            }
                        }
                        if (n < (u32)dp) {
                            n = dp - n;
                            if ((fl & CEXSP__TRIPLET_COMMA) == 0) {
                                while (n) {
                                    if ((((usize)s) & 3) == 0) {
                                        break;
                                    }
                                    *s++ = '0';
                                    --n;
                                }
                                while (n >= 4) {
                                    *(u32*)s = 0x30303030;
                                    s += 4;
                                    n -= 4;
                                }
                            }
                            while (n) {
                                if ((fl & CEXSP__TRIPLET_COMMA) && (++cs == 4)) {
                                    cs = 0;
                                    *s++ = cexsp__comma;
                                } else {
                                    *s++ = '0';
                                    --n;
                                }
                            }
                        }
                        cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
                        if (pr) {
                            *s++ = cexsp__period;
                            tz = pr;
                        }
                    } else {
                        // handle xxxxx.xxxx000*000
                        n = 0;
                        for (;;) {
                            if ((fl & CEXSP__TRIPLET_COMMA) && (++cs == 4)) {
                                cs = 0;
                                *s++ = cexsp__comma;
                            } else {
                                *s++ = sn[n];
                                ++n;
                                if (n >= (u32)dp) {
                                    break;
                                }
                            }
                        }
                        cs = (int)(s - (num + 64)) + (3 << 24); // cs is how many tens
                        if (pr) {
                            *s++ = cexsp__period;
                        }
                        if ((l - dp) > (u32)pr) {
                            l = pr + dp;
                        }
                        while (n < l) {
                            *s++ = sn[n];
                            ++n;
                        }
                        tz = pr - (l - dp);
                    }
                }
                pr = 0;

                // handle k,m,g,t
                if (fl & CEXSP__METRIC_SUFFIX) {
                    char idx;
                    idx = 1;
                    if (fl & CEXSP__METRIC_NOSPACE) {
                        idx = 0;
                    }
                    tail[0] = idx;
                    tail[1] = ' ';
                    {
                        if (fl >> 24) { // SI kilo is 'k', JEDEC and SI kibits are 'K'.
                            if (fl & CEXSP__METRIC_1024) {
                                tail[idx + 1] = "_KMGT"[fl >> 24];
                            } else {
                                tail[idx + 1] = "_kMGT"[fl >> 24];
                            }
                            idx++;
                            // If printing kibits and not in jedec, add the 'i'.
                            if (fl & CEXSP__METRIC_1024 && !(fl & CEXSP__METRIC_JEDEC)) {
                                tail[idx + 1] = 'i';
                                idx++;
                            }
                            tail[0] = idx;
                        }
                    }
                };

            flt_lead:
                // get the length that we copied
                l = (u32)(s - (num + 64));
                s = num + 64;
                goto scopy;
#endif

            case 'B': // upper binary
            case 'b': // lower binary
                h = (f[0] == 'B') ? hexu : hex;
                lead[0] = 0;
                if (fl & CEXSP__LEADING_0X) {
                    lead[0] = 2;
                    lead[1] = '0';
                    lead[2] = h[0xb];
                }
                l = (8 << 4) | (1 << 8);
                goto radixnum;

            case 'o': // octal
                h = hexu;
                lead[0] = 0;
                if (fl & CEXSP__LEADING_0X) {
                    lead[0] = 1;
                    lead[1] = '0';
                }
                l = (3 << 4) | (3 << 8);
                goto radixnum;

            case 'p': // pointer
                fl |= (sizeof(void*) == 8) ? CEXSP__INTMAX : 0;
                pr = sizeof(void*) * 2;
                fl &= ~CEXSP__LEADINGZERO; // 'p' only prints the pointer with zeros
                                           // fall through - to X

            case 'X': // upper hex
            case 'x': // lower hex
                h = (f[0] == 'X') ? hexu : hex;
                l = (4 << 4) | (4 << 8);
                lead[0] = 0;
                if (fl & CEXSP__LEADING_0X) {
                    lead[0] = 2;
                    lead[1] = '0';
                    lead[2] = h[16];
                }
            radixnum:
                // get the number
                if (fl & CEXSP__INTMAX) {
                    n64 = va_arg(va, u64);
                } else {
                    n64 = va_arg(va, u32);
                }

                s = num + CEXSP__NUMSZ;
                dp = 0;
                // clear tail, and clear leading if value is zero
                tail[0] = 0;
                if (n64 == 0) {
                    lead[0] = 0;
                    if (pr == 0) {
                        l = 0;
                        cs = 0;
                        goto scopy;
                    }
                }
                // convert to string
                for (;;) {
                    *--s = h[n64 & ((1 << (l >> 8)) - 1)];
                    n64 >>= (l >> 8);
                    if (!((n64) || ((i32)((num + CEXSP__NUMSZ) - s) < pr))) {
                        break;
                    }
                    if (fl & CEXSP__TRIPLET_COMMA) {
                        ++l;
                        if ((l & 15) == ((l >> 4) & 15)) {
                            l &= ~15;
                            *--s = cexsp__comma;
                        }
                    }
                };
                // get the tens and the comma pos
                cs = (u32)((num + CEXSP__NUMSZ) - s) + ((((l >> 4) & 15)) << 24);
                // get the length that we copied
                l = (u32)((num + CEXSP__NUMSZ) - s);
                // copy it
                goto scopy;

            case 'u': // unsigned
            case 'i':
            case 'd': // integer
                // get the integer and abs it
                if (fl & CEXSP__INTMAX) {
                    i64 _i64 = va_arg(va, i64);
                    n64 = (u64)_i64;
                    if ((f[0] != 'u') && (_i64 < 0)) {
                        n64 = (u64)-_i64;
                        fl |= CEXSP__NEGATIVE;
                    }
                } else {
                    i32 i = va_arg(va, i32);
                    n64 = (u32)i;
                    if ((f[0] != 'u') && (i < 0)) {
                        n64 = (u32)-i;
                        fl |= CEXSP__NEGATIVE;
                    }
                }

#ifndef CEX_SPRINTF_NOFLOAT
                if (fl & CEXSP__METRIC_SUFFIX) {
                    if (n64 < 1024) {
                        pr = 0;
                    } else if (pr == -1) {
                        pr = 1;
                    }
                    fv = (double)(i64)n64;
                    goto doafloat;
                }
#endif

                // convert to string
                s = num + CEXSP__NUMSZ;
                l = 0;

                for (;;) {
                    // do in 32-bit chunks (avoid lots of 64-bit divides even with constant
                    // denominators)
                    char* o = s - 8;
                    if (n64 >= 100000000) {
                        n = (u32)(n64 % 100000000);
                        n64 /= 100000000;
                    } else {
                        n = (u32)n64;
                        n64 = 0;
                    }
                    if ((fl & CEXSP__TRIPLET_COMMA) == 0) {
                        do {
                            s -= 2;
                            *(u16*)s = *(u16*)&cexsp__digitpair.pair[(n % 100) * 2];
                            n /= 100;
                        } while (n);
                    }
                    while (n) {
                        if ((fl & CEXSP__TRIPLET_COMMA) && (l++ == 3)) {
                            l = 0;
                            *--s = cexsp__comma;
                            --o;
                        } else {
                            *--s = (char)(n % 10) + '0';
                            n /= 10;
                        }
                    }
                    if (n64 == 0) {
                        if ((s[0] == '0') && (s != (num + CEXSP__NUMSZ))) {
                            ++s;
                        }
                        break;
                    }
                    while (s != o) {
                        if ((fl & CEXSP__TRIPLET_COMMA) && (l++ == 3)) {
                            l = 0;
                            *--s = cexsp__comma;
                            --o;
                        } else {
                            *--s = '0';
                        }
                    }
                }

                tail[0] = 0;
                cexsp__lead_sign(fl, lead);

                // get the length that we copied
                l = (u32)((num + CEXSP__NUMSZ) - s);
                if (l == 0) {
                    *--s = '0';
                    l = 1;
                }
                cs = l + (3 << 24);
                if (pr < 0) {
                    pr = 0;
                }

            scopy:
                // get fw=leading/trailing space, pr=leading zeros
                if (pr < (i32)l) {
                    pr = l;
                }
                n = pr + lead[0] + tail[0] + tz;
                if (fw < (i32)n) {
                    fw = n;
                }
                fw -= n;
                pr -= l;

                // handle right justify and leading zeros
                if ((fl & CEXSP__LEFTJUST) == 0) {
                    if (fl & CEXSP__LEADINGZERO) // if leading zeros, everything is in pr
                    {
                        pr = (fw > pr) ? fw : pr;
                        fw = 0;
                    } else {
                        fl &= ~CEXSP__TRIPLET_COMMA; // if no leading zeros, then no commas
                    }
                }

                // copy the spaces and/or zeros
                if (fw + pr) {
                    i32 i;
                    u32 c;

                    // copy leading spaces (or when doing %8.4d stuff)
                    if ((fl & CEXSP__LEFTJUST) == 0) {
                        while (fw > 0) {
                            cexsp__cb_buf_clamp(i, fw);
                            fw -= i;
                            while (i) {
                                if ((((usize)bf) & 3) == 0) {
                                    break;
                                }
                                *bf++ = ' ';
                                --i;
                            }
                            while (i >= 4) {
                                *(u32*)bf = 0x20202020;
                                bf += 4;
                                i -= 4;
                            }
                            while (i) {
                                *bf++ = ' ';
                                --i;
                            }
                            cexsp__chk_cb_buf(1);
                        }
                    }

                    // copy leader
                    sn = lead + 1;
                    while (lead[0]) {
                        cexsp__cb_buf_clamp(i, lead[0]);
                        lead[0] -= (char)i;
                        while (i) {
                            *bf++ = *sn++;
                            --i;
                        }
                        cexsp__chk_cb_buf(1);
                    }

                    // copy leading zeros
                    c = cs >> 24;
                    cs &= 0xffffff;
                    cs = (fl & CEXSP__TRIPLET_COMMA) ? ((u32)(c - ((pr + cs) % (c + 1)))) : 0;
                    while (pr > 0) {
                        cexsp__cb_buf_clamp(i, pr);
                        pr -= i;
                        if ((fl & CEXSP__TRIPLET_COMMA) == 0) {
                            while (i) {
                                if ((((usize)bf) & 3) == 0) {
                                    break;
                                }
                                *bf++ = '0';
                                --i;
                            }
                            while (i >= 4) {
                                *(u32*)bf = 0x30303030;
                                bf += 4;
                                i -= 4;
                            }
                        }
                        while (i) {
                            if ((fl & CEXSP__TRIPLET_COMMA) && (cs++ == c)) {
                                cs = 0;
                                *bf++ = cexsp__comma;
                            } else {
                                *bf++ = '0';
                            }
                            --i;
                        }
                        cexsp__chk_cb_buf(1);
                    }
                }

                // copy leader if there is still one
                sn = lead + 1;
                while (lead[0]) {
                    i32 i;
                    cexsp__cb_buf_clamp(i, lead[0]);
                    lead[0] -= (char)i;
                    while (i) {
                        *bf++ = *sn++;
                        --i;
                    }
                    cexsp__chk_cb_buf(1);
                }

                // copy the string
                n = l;
                while (n) {
                    i32 i;
                    cexsp__cb_buf_clamp(i, n);
                    n -= i;
                    // disabled CEXSP__UNALIGNED
                    // CEXSP__UNALIGNED(while (i >= 4) {
                    //     *(u32 volatile*)bf = *(u32 volatile*)s;
                    //     bf += 4;
                    //     s += 4;
                    //     i -= 4;
                    // })
                    while (i) {
                        *bf++ = *s++;
                        --i;
                    }
                    cexsp__chk_cb_buf(1);
                }

                // copy trailing zeros
                while (tz) {
                    i32 i;
                    cexsp__cb_buf_clamp(i, tz);
                    tz -= i;
                    while (i) {
                        if ((((usize)bf) & 3) == 0) {
                            break;
                        }
                        *bf++ = '0';
                        --i;
                    }
                    while (i >= 4) {
                        *(u32*)bf = 0x30303030;
                        bf += 4;
                        i -= 4;
                    }
                    while (i) {
                        *bf++ = '0';
                        --i;
                    }
                    cexsp__chk_cb_buf(1);
                }

                // copy tail if there is one
                sn = tail + 1;
                while (tail[0]) {
                    i32 i;
                    cexsp__cb_buf_clamp(i, tail[0]);
                    tail[0] -= (char)i;
                    while (i) {
                        *bf++ = *sn++;
                        --i;
                    }
                    cexsp__chk_cb_buf(1);
                }

                // handle the left justify
                if (fl & CEXSP__LEFTJUST) {
                    if (fw > 0) {
                        while (fw) {
                            i32 i;
                            cexsp__cb_buf_clamp(i, fw);
                            fw -= i;
                            while (i) {
                                if ((((usize)bf) & 3) == 0) {
                                    break;
                                }
                                *bf++ = ' ';
                                --i;
                            }
                            while (i >= 4) {
                                *(u32*)bf = 0x20202020;
                                bf += 4;
                                i -= 4;
                            }
                            while (i--) {
                                *bf++ = ' ';
                            }
                            cexsp__chk_cb_buf(1);
                        }
                    }
                }
                break;

            default: // unknown, just copy code
                s = num + CEXSP__NUMSZ - 1;
                *s = f[0];
                l = 1;
                fw = fl = 0;
                lead[0] = 0;
                tail[0] = 0;
                pr = 0;
                dp = 0;
                cs = 0;
                goto scopy;
        }
        ++f;
    }
endfmt:

    if (!callback) {
        *bf = 0;
    } else {
        cexsp__flush_cb();
    }

done:
    return tlen + (int)(bf - buf);
}

// cleanup
#undef CEXSP__LEFTJUST
#undef CEXSP__LEADINGPLUS
#undef CEXSP__LEADINGSPACE
#undef CEXSP__LEADING_0X
#undef CEXSP__LEADINGZERO
#undef CEXSP__INTMAX
#undef CEXSP__TRIPLET_COMMA
#undef CEXSP__NEGATIVE
#undef CEXSP__METRIC_SUFFIX
#undef CEXSP__NUMSZ
#undef cexsp__chk_cb_bufL
#undef cexsp__chk_cb_buf
#undef cexsp__flush_cb
#undef cexsp__cb_buf_clamp

// ============================================================================
//   wrapper functions

static char*
cexsp__clamp_callback(const char* buf, void* user, u32 len)
{
    cexsp__context* c = (cexsp__context*)user;
    c->length += len;

    if (len > c->capacity) {
        len = c->capacity;
    }

    if (len) {
        if (buf != c->buf) {
            const char *s, *se;
            char* d;
            d = c->buf;
            s = buf;
            se = buf + len;
            do {
                *d++ = *s++;
            } while (s < se);
        }
        c->buf += len;
        c->capacity -= len;
    }

    if (c->capacity <= 0) {
        return c->tmp;
    }
    return (c->capacity >= CEX_SPRINTF_MIN) ? c->buf : c->tmp; // go direct into buffer if you can
}


CEXSP__PUBLICDEF int
cexsp__vsnprintf(char* buf, int count, char const* fmt, va_list va)
{
    cexsp__context c;

    if (!buf || count <= 0) {
        return -1;
    } else {
        int l;

        c.buf = buf;
        c.capacity = count;
        c.length = 0;

        cexsp__vsprintfcb(cexsp__clamp_callback, &c, cexsp__clamp_callback(0, &c, 0), fmt, va);

        // zero-terminate
        l = (int)(c.buf - buf);
        if (l >= count) { // should never be greater, only equal (or less) than count
            c.length = l;
            l = count - 1;
        }
        buf[l] = 0;
        uassert(c.length <= INT32_MAX && "length overflow");
    }

    return c.length;
}

CEXSP__PUBLICDEF int
cexsp__snprintf(char* buf, int count, char const* fmt, ...)
{
    int result;
    va_list va;
    va_start(va, fmt);

    result = cexsp__vsnprintf(buf, count, fmt, va);
    va_end(va);

    return result;
}

static char*
cexsp__fprintf_callback(const char* buf, void* user, u32 len)
{
    cexsp__context* c = (cexsp__context*)user;
    c->length += len;
    if (len) {
        if (fwrite(buf, sizeof(char), len, c->file) != (size_t)len) {
            c->has_error = 1;
        }
    }
    return c->tmp;
}

CEXSP__PUBLICDEF int
cexsp__vfprintf(FILE* stream, const char* format, va_list va)
{
    cexsp__context c = { .file = stream, .length = 0 };

    cexsp__vsprintfcb(cexsp__fprintf_callback, &c, cexsp__fprintf_callback(0, &c, 0), format, va);
    uassert(c.length <= INT32_MAX && "length overflow");

    return c.has_error == 0 ? (i32)c.length : -1;
}

CEXSP__PUBLICDEF int
cexsp__fprintf(FILE* stream, const char* format, ...)
{
    int result;
    va_list va;
    va_start(va, format);
    result = cexsp__vfprintf(stream, format, va);
    va_end(va);
    return result;
}

// =======================================================================
//   low level float utility functions

#ifndef CEX_SPRINTF_NOFLOAT

// copies d to bits w/ strict aliasing (this compiles to nothing on /Ox)
#define CEXSP__COPYFP(dest, src)                                                                   \
    {                                                                                              \
        int cn;                                                                                    \
        for (cn = 0; cn < 8; cn++)                                                                 \
            ((char*)&dest)[cn] = ((char*)&src)[cn];                                                \
    }

// get float info
static i32
cexsp__real_to_parts(i64* bits, i32* expo, double value)
{
    double d;
    i64 b = 0;

    // load value and round at the frac_digits
    d = value;

    CEXSP__COPYFP(b, d);

    *bits = b & ((((u64)1) << 52) - 1);
    *expo = (i32)(((b >> 52) & 2047) - 1023);

    return (i32)((u64)b >> 63);
}

static double const cexsp__bot[23] = { 1e+000, 1e+001, 1e+002, 1e+003, 1e+004, 1e+005,
                                       1e+006, 1e+007, 1e+008, 1e+009, 1e+010, 1e+011,
                                       1e+012, 1e+013, 1e+014, 1e+015, 1e+016, 1e+017,
                                       1e+018, 1e+019, 1e+020, 1e+021, 1e+022 };
static double const cexsp__negbot[22] = { 1e-001, 1e-002, 1e-003, 1e-004, 1e-005, 1e-006,
                                          1e-007, 1e-008, 1e-009, 1e-010, 1e-011, 1e-012,
                                          1e-013, 1e-014, 1e-015, 1e-016, 1e-017, 1e-018,
                                          1e-019, 1e-020, 1e-021, 1e-022 };
static double const cexsp__negboterr[22] = {
    -5.551115123125783e-018,  -2.0816681711721684e-019, -2.0816681711721686e-020,
    -4.7921736023859299e-021, -8.1803053914031305e-022, 4.5251888174113741e-023,
    4.5251888174113739e-024,  -2.0922560830128471e-025, -6.2281591457779853e-026,
    -3.6432197315497743e-027, 6.0503030718060191e-028,  2.0113352370744385e-029,
    -3.0373745563400371e-030, 1.1806906454401013e-032,  -7.7705399876661076e-032,
    2.0902213275965398e-033,  -7.1542424054621921e-034, -7.1542424054621926e-035,
    2.4754073164739869e-036,  5.4846728545790429e-037,  9.2462547772103625e-038,
    -4.8596774326570872e-039
};
static double const cexsp__top[13] = { 1e+023, 1e+046, 1e+069, 1e+092, 1e+115, 1e+138, 1e+161,
                                       1e+184, 1e+207, 1e+230, 1e+253, 1e+276, 1e+299 };
static double const cexsp__negtop[13] = { 1e-023, 1e-046, 1e-069, 1e-092, 1e-115, 1e-138, 1e-161,
                                          1e-184, 1e-207, 1e-230, 1e-253, 1e-276, 1e-299 };
static double const cexsp__toperr[13] = { 8388608,
                                          6.8601809640529717e+028,
                                          -7.253143638152921e+052,
                                          -4.3377296974619174e+075,
                                          -1.5559416129466825e+098,
                                          -3.2841562489204913e+121,
                                          -3.7745893248228135e+144,
                                          -1.7356668416969134e+167,
                                          -3.8893577551088374e+190,
                                          -9.9566444326005119e+213,
                                          6.3641293062232429e+236,
                                          -5.2069140800249813e+259,
                                          -5.2504760255204387e+282 };
static double const cexsp__negtoperr[13] = { 3.9565301985100693e-040,  -2.299904345391321e-063,
                                             3.6506201437945798e-086,  1.1875228833981544e-109,
                                             -5.0644902316928607e-132, -6.7156837247865426e-155,
                                             -2.812077463003139e-178,  -5.7778912386589953e-201,
                                             7.4997100559334532e-224,  -4.6439668915134491e-247,
                                             -6.3691100762962136e-270, -9.436808465446358e-293,
                                             8.0970921678014997e-317 };

#if defined(_MSC_VER) && (_MSC_VER <= 1200)
static u64 const cexsp__powten[20] = { 1,
                                       10,
                                       100,
                                       1000,
                                       10000,
                                       100000,
                                       1000000,
                                       10000000,
                                       100000000,
                                       1000000000,
                                       10000000000,
                                       100000000000,
                                       1000000000000,
                                       10000000000000,
                                       100000000000000,
                                       1000000000000000,
                                       10000000000000000,
                                       100000000000000000,
                                       1000000000000000000,
                                       10000000000000000000U };
#define cexsp__tento19th ((u64)1000000000000000000)
#else
static u64 const cexsp__powten[20] = { 1,
                                       10,
                                       100,
                                       1000,
                                       10000,
                                       100000,
                                       1000000,
                                       10000000,
                                       100000000,
                                       1000000000,
                                       10000000000ULL,
                                       100000000000ULL,
                                       1000000000000ULL,
                                       10000000000000ULL,
                                       100000000000000ULL,
                                       1000000000000000ULL,
                                       10000000000000000ULL,
                                       100000000000000000ULL,
                                       1000000000000000000ULL,
                                       10000000000000000000ULL };
#define cexsp__tento19th (1000000000000000000ULL)
#endif

#define cexsp__ddmulthi(oh, ol, xh, yh)                                                            \
    {                                                                                              \
        double ahi = 0, alo, bhi = 0, blo;                                                         \
        i64 bt;                                                                                    \
        oh = xh * yh;                                                                              \
        CEXSP__COPYFP(bt, xh);                                                                     \
        bt &= ((~(u64)0) << 27);                                                                   \
        CEXSP__COPYFP(ahi, bt);                                                                    \
        alo = xh - ahi;                                                                            \
        CEXSP__COPYFP(bt, yh);                                                                     \
        bt &= ((~(u64)0) << 27);                                                                   \
        CEXSP__COPYFP(bhi, bt);                                                                    \
        blo = yh - bhi;                                                                            \
        ol = ((ahi * bhi - oh) + ahi * blo + alo * bhi) + alo * blo;                               \
    }

#define cexsp__ddtoS64(ob, xh, xl)                                                                 \
    {                                                                                              \
        double ahi = 0, alo, vh, t;                                                                \
        ob = (i64)xh;                                                                              \
        vh = (double)ob;                                                                           \
        ahi = (xh - vh);                                                                           \
        t = (ahi - xh);                                                                            \
        alo = (xh - (ahi - t)) - (vh + t);                                                         \
        ob += (i64)(ahi + alo + xl);                                                               \
    }

#define cexsp__ddrenorm(oh, ol)                                                                    \
    {                                                                                              \
        double s;                                                                                  \
        s = oh + ol;                                                                               \
        ol = ol - (s - oh);                                                                        \
        oh = s;                                                                                    \
    }

#define cexsp__ddmultlo(oh, ol, xh, xl, yh, yl) ol = ol + (xh * yl + xl * yh);

#define cexsp__ddmultlos(oh, ol, xh, yl) ol = ol + (xh * yl);

static void
cexsp__raise_to_power10(double* ohi, double* olo, double d, i32 power) // power can be -323
                                                                       // to +350
{
    double ph, pl;
    if ((power >= 0) && (power <= 22)) {
        cexsp__ddmulthi(ph, pl, d, cexsp__bot[power]);
    } else {
        i32 e, et, eb;
        double p2h, p2l;

        e = power;
        if (power < 0) {
            e = -e;
        }
        et = (e * 0x2c9) >> 14; /* %23 */
        if (et > 13) {
            et = 13;
        }
        eb = e - (et * 23);

        ph = d;
        pl = 0.0;
        if (power < 0) {
            if (eb) {
                --eb;
                cexsp__ddmulthi(ph, pl, d, cexsp__negbot[eb]);
                cexsp__ddmultlos(ph, pl, d, cexsp__negboterr[eb]);
            }
            if (et) {
                cexsp__ddrenorm(ph, pl);
                --et;
                cexsp__ddmulthi(p2h, p2l, ph, cexsp__negtop[et]);
                cexsp__ddmultlo(p2h, p2l, ph, pl, cexsp__negtop[et], cexsp__negtoperr[et]);
                ph = p2h;
                pl = p2l;
            }
        } else {
            if (eb) {
                e = eb;
                if (eb > 22) {
                    eb = 22;
                }
                e -= eb;
                cexsp__ddmulthi(ph, pl, d, cexsp__bot[eb]);
                if (e) {
                    cexsp__ddrenorm(ph, pl);
                    cexsp__ddmulthi(p2h, p2l, ph, cexsp__bot[e]);
                    cexsp__ddmultlos(p2h, p2l, cexsp__bot[e], pl);
                    ph = p2h;
                    pl = p2l;
                }
            }
            if (et) {
                cexsp__ddrenorm(ph, pl);
                --et;
                cexsp__ddmulthi(p2h, p2l, ph, cexsp__top[et]);
                cexsp__ddmultlo(p2h, p2l, ph, pl, cexsp__top[et], cexsp__toperr[et]);
                ph = p2h;
                pl = p2l;
            }
        }
    }
    cexsp__ddrenorm(ph, pl);
    *ohi = ph;
    *olo = pl;
}

// given a float value, returns the significant bits in bits, and the position of the
//   decimal point in decimal_pos.  +/-INF and NAN are specified by special values
//   returned in the decimal_pos parameter.
// frac_digits is absolute normally, but if you want from first significant digits (got %g and %e),
// or in 0x80000000
static i32
cexsp__real_to_str(
    char const** start,
    u32* len,
    char* out,
    i32* decimal_pos,
    double value,
    u32 frac_digits
)
{
    double d;
    i64 bits = 0;
    i32 expo, e, ng, tens;

    d = value;
    CEXSP__COPYFP(bits, d);
    expo = (i32)((bits >> 52) & 2047);
    ng = (i32)((u64)bits >> 63);
    if (ng) {
        d = -d;
    }

    if (expo == 2047) // is nan or inf?
    {
        // CEX: lower case nan/inf
        *start = (bits & ((((u64)1) << 52) - 1)) ? "nan" : "inf";
        *decimal_pos = CEXSP__SPECIAL;
        *len = 3;
        return ng;
    }

    if (expo == 0) // is zero or denormal
    {
        if (((u64)bits << 1) == 0) // do zero
        {
            *decimal_pos = 1;
            *start = out;
            out[0] = '0';
            *len = 1;
            return ng;
        }
        // find the right expo for denormals
        {
            i64 v = ((u64)1) << 51;
            while ((bits & v) == 0) {
                --expo;
                v >>= 1;
            }
        }
    }

    // find the decimal exponent as well as the decimal bits of the value
    {
        double ph, pl;

        // log10 estimate - very specifically tweaked to hit or undershoot by no more than 1 of
        // log10 of all expos 1..2046
        tens = expo - 1023;
        tens = (tens < 0) ? ((tens * 617) / 2048) : (((tens * 1233) / 4096) + 1);

        // move the significant bits into position and stick them into an int
        cexsp__raise_to_power10(&ph, &pl, d, 18 - tens);

        // get full as much precision from double-double as possible
        cexsp__ddtoS64(bits, ph, pl);

        // check if we undershot
        if (((u64)bits) >= cexsp__tento19th) {
            ++tens;
        }
    }

    // now do the rounding in integer land
    frac_digits = (frac_digits & 0x80000000) ? ((frac_digits & 0x7ffffff) + 1)
                                             : (tens + frac_digits);
    if ((frac_digits < 24)) {
        u32 dg = 1;
        if ((u64)bits >= cexsp__powten[9]) {
            dg = 10;
        }
        while ((u64)bits >= cexsp__powten[dg]) {
            ++dg;
            if (dg == 20) {
                goto noround;
            }
        }
        if (frac_digits < dg) {
            u64 r;
            // add 0.5 at the right position and round
            e = dg - frac_digits;
            if ((u32)e >= 24) {
                goto noround;
            }
            r = cexsp__powten[e];
            bits = bits + (r / 2);
            if ((u64)bits >= cexsp__powten[dg]) {
                ++tens;
            }
            bits /= r;
        }
    noround:;
    }

    // kill long trailing runs of zeros
    if (bits) {
        u32 n;
        for (;;) {
            if (bits <= 0xffffffff) {
                break;
            }
            if (bits % 1000) {
                goto donez;
            }
            bits /= 1000;
        }
        n = (u32)bits;
        while ((n % 1000) == 0) {
            n /= 1000;
        }
        bits = n;
    donez:;
    }

    // convert to string
    out += 64;
    e = 0;
    for (;;) {
        u32 n;
        char* o = out - 8;
        // do the conversion in chunks of U32s (avoid most 64-bit divides, worth it, constant
        // denomiators be damned)
        if (bits >= 100000000) {
            n = (u32)(bits % 100000000);
            bits /= 100000000;
        } else {
            n = (u32)bits;
            bits = 0;
        }
        while (n) {
            out -= 2;
            *(u16*)out = *(u16*)&cexsp__digitpair.pair[(n % 100) * 2];
            n /= 100;
            e += 2;
        }
        if (bits == 0) {
            if ((e) && (out[0] == '0')) {
                ++out;
                --e;
            }
            break;
        }
        while (out != o) {
            *--out = '0';
            ++e;
        }
    }

    *decimal_pos = tens;
    *start = out;
    *len = e;
    return ng;
}

#undef cexsp__ddmulthi
#undef cexsp__ddrenorm
#undef cexsp__ddmultlo
#undef cexsp__ddmultlos
#undef CEXSP__SPECIAL
#undef CEXSP__COPYFP

#endif // CEX_SPRINTF_NOFLOAT


/*
*                   all.c
*/
#include "os/os.c"



/*
*                   argparse.c
*/
/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#include <cex/all.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const u32 ARGPARSE_OPT_UNSET = 1;
const u32 ARGPARSE_OPT_LONG = (1 << 1);

// static const u8 1 /*ARGPARSE_OPT_GROUP*/  = 1;
// static const u8 2 /*ARGPARSE_OPT_BOOLEAN*/ = 2;
// static const u8 3 /*ARGPARSE_OPT_BIT*/ = 3;
// static const u8 4 /*ARGPARSE_OPT_INTEGER*/ = 4;
// static const u8 5 /*ARGPARSE_OPT_FLOAT*/ = 5;
// static const u8 6 /*ARGPARSE_OPT_STRING*/ = 6;

static const char*
argparse__prefix_skip(const char* str, const char* prefix)
{
    usize len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static int
argparse__prefix_cmp(const char* str, const char* prefix)
{
    for (;; str++, prefix++) {
        if (!*prefix) {
            return 0;
        } else if (*str != *prefix) {
            return (unsigned char)*prefix - (unsigned char)*str;
        }
    }
}

static Exception
argparse__error(argparse_c* self, const argparse_opt_s* opt, const char* reason, int flags)
{
    (void)self;
    if (flags & ARGPARSE_OPT_LONG) {
        fprintf(stdout, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        fprintf(stdout, "error: option `-%c` %s\n", opt->short_name, reason);
    }

    return Error.argument;
}

static void
argparse_usage(argparse_c* self)
{
    uassert(self->_ctx.argv != NULL && "usage before parse!");

    fprintf(stdout, "Usage:\n");
    if (self->usage) {

        for$iter(str_s, it, str.slice.iter_split(str.sstr(self->usage), "\n", &it.iterator))
        {
            if (it.val->len == 0) {
                break;
            }

            char* fn = strrchr(self->program_name, '/');
            if (fn != NULL) {
                fprintf(stdout, "%s ", fn + 1);
            } else {
                fprintf(stdout, "%s ", self->program_name);
            }

            if (fwrite(it.val->buf, sizeof(char), it.val->len, stdout)) {
                ;
            }

            fputc('\n', stdout);
        }
    } else {
        fprintf(stdout, "%s [options] [--] [arg1 argN]\n", self->program_name);
    }

    // print description
    if (self->description) {
        fprintf(stdout, "%s\n", self->description);
    }

    fputc('\n', stdout);

    const argparse_opt_s* options;

    // figure out best width
    usize usage_opts_width = 0;
    usize len;
    options = self->options;
    for (u32 i = 0; i < self->options_len; i++, options++) {
        len = 0;
        if ((options)->short_name) {
            len += 2;
        }
        if ((options)->short_name && (options)->long_name) {
            len += 2; // separator ", "
        }
        if ((options)->long_name) {
            len += strlen((options)->long_name) + 2;
        }
        if (options->type == 4 /*ARGPARSE_OPT_INTEGER*/) {
            len += strlen("=<int>");
        }
        if (options->type == 5 /*ARGPARSE_OPT_FLOAT*/) {
            len += strlen("=<flt>");
        } else if (options->type == 6 /*ARGPARSE_OPT_STRING*/) {
            len += strlen("=<str>");
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) {
            usage_opts_width = len;
        }
    }
    usage_opts_width += 4; // 4 spaces prefix

    options = self->options;
    for (u32 i = 0; i < self->options_len; i++, options++) {
        usize pos = 0;
        usize pad = 0;
        if (options->type == 1 /*ARGPARSE_OPT_GROUP*/) {
            fputc('\n', stdout);
            fprintf(stdout, "%s", options->help);
            fputc('\n', stdout);
            continue;
        }
        pos = fprintf(stdout, "    ");
        if (options->short_name) {
            pos += fprintf(stdout, "-%c", options->short_name);
        }
        if (options->long_name && options->short_name) {
            pos += fprintf(stdout, ", ");
        }
        if (options->long_name) {
            pos += fprintf(stdout, "--%s", options->long_name);
        }
        if (options->type == 4 /*ARGPARSE_OPT_INTEGER*/) {
            pos += fprintf(stdout, "=<int>");
        } else if (options->type == 5 /*ARGPARSE_OPT_FLOAT*/) {
            pos += fprintf(stdout, "=<flt>");
        } else if (options->type == 6 /*ARGPARSE_OPT_STRING*/) {
            pos += fprintf(stdout, "=<str>");
        }
        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            fputc('\n', stdout);
            pad = usage_opts_width;
        }
        fprintf(stdout, "%*s%s\n", (int)pad + 2, "", options->help);
    }

    // print epilog
    if (self->epilog) {
        fprintf(stdout, "%s\n", self->epilog);
    }
}
static Exception
argparse__getvalue(argparse_c* self, argparse_opt_s* opt, int flags)
{
    const char* s = NULL;
    if (!opt->value) {
        goto skipped;
    }

    switch (opt->type) {
        case 2 /*ARGPARSE_OPT_BOOLEAN*/:
            if (flags & ARGPARSE_OPT_UNSET) {
                *(int*)opt->value = 0;
            } else {
                *(int*)opt->value = 1;
            }
            opt->is_present = true;
            break;
        case 3 /*ARGPARSE_OPT_BIT*/:
            if (flags & ARGPARSE_OPT_UNSET) {
                *(int*)opt->value &= ~opt->data;
            } else {
                *(int*)opt->value |= opt->data;
            }
            opt->is_present = true;
            break;
        case 6 /*ARGPARSE_OPT_STRING*/:
            if (self->_ctx.optvalue) {
                *(const char**)opt->value = self->_ctx.optvalue;
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                self->_ctx.cpidx++;
                *(const char**)opt->value = *++self->_ctx.argv;
            } else {
                return argparse__error(self, opt, "requires a value", flags);
            }
            opt->is_present = true;
            break;
        case 4 /*ARGPARSE_OPT_INTEGER*/:
            errno = 0;
            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return argparse__error(self, opt, "requires a value", flags);
                }

                *(int*)opt->value = strtol(self->_ctx.optvalue, (char**)&s, 0);
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                self->_ctx.cpidx++;
                *(int*)opt->value = strtol(*++self->_ctx.argv, (char**)&s, 0);
            } else {
                return argparse__error(self, opt, "requires a value", flags);
            }
            if (errno == ERANGE) {
                return argparse__error(self, opt, "numerical result out of range", flags);
            }
            if (s[0] != '\0') { // no digits or contains invalid characters
                return argparse__error(self, opt, "expects an integer value", flags);
            }
            opt->is_present = true;
            break;
        case 5 /*ARGPARSE_OPT_FLOAT*/:
            errno = 0;

            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return argparse__error(self, opt, "requires a value", flags);
                }
                *(float*)opt->value = strtof(self->_ctx.optvalue, (char**)&s);
                self->_ctx.optvalue = NULL;
            } else if (self->_ctx.argc > 1) {
                self->_ctx.argc--;
                self->_ctx.cpidx++;
                *(float*)opt->value = strtof(*++self->_ctx.argv, (char**)&s);
            } else {
                return argparse__error(self, opt, "requires a value", flags);
            }
            if (errno == ERANGE) {
                return argparse__error(self, opt, "numerical result out of range", flags);
            }
            if (s[0] != '\0') { // no digits or contains invalid characters
                return argparse__error(self, opt, "expects a numerical value", flags);
            }
            opt->is_present = true;
            break;
        default:
            uassert(false && "unhandled");
            return Error.runtime;
    }

skipped:
    if (opt->callback) {
        opt->is_present = true;
        return opt->callback(self, opt);
    } else {
        if (opt->short_name == 'h') {
            argparse_usage(self);
            return Error.argsparse;
        }
    }

    return Error.ok;
}

static Exception
argparse__options_check(argparse_c* self, bool reset)
{
    var options = self->options;

    for (u32 i = 0; i < self->options_len; i++, options++) {
        switch (options->type) {
            case 1 /*ARGPARSE_OPT_GROUP*/:
                continue;
            case 2 /*ARGPARSE_OPT_BOOLEAN*/:
            case 3 /*ARGPARSE_OPT_BIT*/:
            case 4 /*ARGPARSE_OPT_INTEGER*/:
            case 5 /*ARGPARSE_OPT_FLOAT*/:
            case 6 /*ARGPARSE_OPT_STRING*/:
                if (reset) {
                    options->is_present = 0;
                    if (!(options->short_name || options->long_name)) {
                        uassert(
                            (options->short_name || options->long_name) &&
                            "options both long/short_name NULL"
                        );
                        return Error.argument;
                    }
                    if (options->value == NULL && options->short_name != 'h') {
                        uassertf(
                            options->value != NULL,
                            "option value [%c/%s] is null\n",
                            options->short_name,
                            options->long_name
                        );
                        return Error.argument;
                    }
                } else {
                    if (options->required && !options->is_present) {
                        fprintf(
                            stdout,
                            "Error: missing required option: -%c/--%s\n",
                            options->short_name,
                            options->long_name
                        );
                        return Error.argsparse;
                    }
                }
                continue;
            default:
                uassertf(false, "wrong option type: %d", options->type);
        }
    }

    return Error.ok;
}

static Exception
argparse__short_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        if (options->short_name == *self->_ctx.optvalue) {
            self->_ctx.optvalue = self->_ctx.optvalue[1] ? self->_ctx.optvalue + 1 : NULL;
            return argparse__getvalue(self, options, 0);
        }
    }
    return Error.not_found;
}

static Exception
argparse__long_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        const char* rest;
        int opt_flags = 0;
        if (!options->long_name) {
            continue;
        }

        rest = argparse__prefix_skip(self->_ctx.argv[0] + 2, options->long_name);
        if (!rest) {
            // negation disabled?
            if (options->flags & ARGPARSE_OPT_NONEG) {
                continue;
            }
            // only OPT_BOOLEAN/OPT_BIT supports negation
            if (options->type != 2 /*ARGPARSE_OPT_BOOLEAN*/ &&
                options->type != 3 /*ARGPARSE_OPT_BIT*/) {
                continue;
            }

            if (argparse__prefix_cmp(self->_ctx.argv[0] + 2, "no-")) {
                continue;
            }
            rest = argparse__prefix_skip(self->_ctx.argv[0] + 2 + 3, options->long_name);
            if (!rest) {
                continue;
            }
            opt_flags |= ARGPARSE_OPT_UNSET;
        }
        if (*rest) {
            if (*rest != '=') {
                continue;
            }
            self->_ctx.optvalue = rest + 1;
        }
        return argparse__getvalue(self, options, opt_flags | ARGPARSE_OPT_LONG);
    }
    return Error.not_found;
}


static Exception
argparse__report_error(argparse_c* self, Exc err)
{
    // invalidate argc
    self->_ctx.argc = 0;

    if (err == Error.not_found) {
        fprintf(stdout, "error: unknown option `%s`\n", self->_ctx.argv[0]);
    } else if (err == Error.integrity) {
        fprintf(stdout, "error: option `%s` follows argument\n", self->_ctx.argv[0]);
    }
    return Error.argsparse;
}

static Exception
argparse_parse(argparse_c* self, int argc, char** argv)
{
    if (self->options_len == 0) {
        return "zero options provided or self.options_len field not set";
    }
    uassert(argc > 0);
    uassert(argv != NULL);
    uassert(argv[0] != NULL);

    if (self->program_name == NULL) {
        self->program_name = argv[0];
    }

    // reset if we have several runs
    memset(&self->_ctx, 0, sizeof(self->_ctx));

    self->_ctx.argc = argc - 1;
    self->_ctx.argv = argv + 1;
    self->_ctx.out = argv;

    e$except_silent(err, argparse__options_check(self, true))
    {
        return err;
    }

    for (; self->_ctx.argc; self->_ctx.argc--, self->_ctx.argv++) {
        const char* arg = self->_ctx.argv[0];
        if (arg[0] != '-' || !arg[1]) {
            self->_ctx.has_argument = true;

            if (self->flags.stop_at_non_option) {
                self->_ctx.argc--;
                self->_ctx.argv++;
                break;
            }
            continue;
        }
        // short option
        if (arg[1] != '-') {
            if (self->_ctx.has_argument) {
                // options are not allowed after arguments
                return argparse__report_error(self, Error.integrity);
            }

            self->_ctx.optvalue = arg + 1;
            self->_ctx.cpidx++;
            e$except_silent(err, argparse__short_opt(self, self->options))
            {
                return argparse__report_error(self, err);
            }
            while (self->_ctx.optvalue) {
                e$except_silent(err, argparse__short_opt(self, self->options))
                {
                    return argparse__report_error(self, err);
                }
            }
            continue;
        }
        // if '--' presents
        if (!arg[2]) {
            self->_ctx.argc--;
            self->_ctx.argv++;
            self->_ctx.cpidx++;
            break;
        }
        // long option
        if (self->_ctx.has_argument) {
            // options are not allowed after arguments
            return argparse__report_error(self, Error.integrity);
        }
        e$except_silent(err, argparse__long_opt(self, self->options))
        {
            return argparse__report_error(self, err);
        }
        self->_ctx.cpidx++;
        continue;
    }

    e$except_silent(err, argparse__options_check(self, false))
    {
        return err;
    }

    self->_ctx.out += self->_ctx.cpidx + 1; // excludes 1st argv[0], program_name
    self->_ctx.argc = argc - self->_ctx.cpidx - 1;

    return Error.ok;
}

static u32
argparse_argc(argparse_c* self)
{
    uassert(self->_ctx.out != NULL && "argparse.parse() not called?");
    return self->_ctx.argc;
}

static char**
argparse_argv(argparse_c* self)
{
    uassert(self->_ctx.out != NULL && "argparse.parse() not called?");
    return self->_ctx.out;
}


const struct __module__argparse argparse = {
    // Autogenerated by CEX
    // clang-format off
    .usage = argparse_usage,
    .parse = argparse_parse,
    .argc = argparse_argc,
    .argv = argparse_argv,
    // clang-format on
};

/*
*                   cex_base.c
*/

const struct _CEX_Error_struct Error = {
    .ok = EOK,                       // Success
    .memory = "MemoryError",         // memory allocation error
    .io = "IOError",                 // IO error
    .overflow = "OverflowError",     // buffer overflow
    .argument = "ArgumentError",     // function argument error
    .integrity = "IntegrityError",   // data integrity error
    .exists = "ExistsError",         // entity or key already exists
    .not_found = "NotFoundError",    // entity or key already exists
    .skip = "ShouldBeSkipped",       // NOT an error, function result must be skipped
    .empty = "EmptyError",           // resource is empty
    .eof = "EOF",                    // end of file reached
    .argsparse = "ProgramArgsError", // program arguments empty or incorrect
    .runtime = "RuntimeError",       // generic runtime error
    .assert = "AssertError",         // generic runtime check
    .os = "OSError",                 // generic OS check
    .timeout = "TimeoutError",       // await interval timeout
};

/*
*                   ds.c
*/

#include <assert.h>
#include <stddef.h>
#include <string.h>


#ifdef CEXDS_STATISTICS
#define CEXDS_STATS(x) x
size_t cexds_array_grow;
size_t cexds_hash_grow;
size_t cexds_hash_shrink;
size_t cexds_hash_rebuild;
size_t cexds_hash_probes;
size_t cexds_hash_alloc;
size_t cexds_rehash_probes;
size_t cexds_rehash_items;
#else
#define CEXDS_STATS(x)
#endif

//
// cexds_arr implementation
//

#define cexds_base(t) ((char*)(t) - CEXDS_HDR_PAD)
#define cexds_item_ptr(t, i, elemsize) ((char*)a + elemsize * i)

bool
cexds_arr_integrity(const void* arr, size_t magic_num)
{
    (void)magic_num;
    cexds_array_header* hdr = cexds_header(arr);
    (void)hdr;

#ifndef NDEBUG

    uassert(arr != NULL && "array uninitialized or out-of-mem");
    // WARNING: next can trigger sanitizer with "stack/heap-buffer-underflow on address"
    //          when arr pointer is invalid arr$ / hm$ type pointer
    if (magic_num == 0) {
        uassert(((hdr->magic_num == CEXDS_ARR_MAGIC) || hdr->magic_num == CEXDS_HM_MAGIC));
    } else {
        uassert(hdr->magic_num == magic_num);
    }

    uassert(mem$asan_poison_check(hdr->__poison_area, sizeof(hdr->__poison_area)));

#endif


    return true;
}

void*
cexds_arrgrowf(void* a, size_t elemsize, size_t addlen, size_t min_cap, IAllocator allc)
{
    uassert(addlen < PTRDIFF_MAX && "negative or overflow");
    uassert(min_cap < PTRDIFF_MAX && "negative or overflow");

    if (a == NULL) {
        if (allc == NULL) {
            uassert(allc != NULL && "using uninitialized arr/hm or out-of-mem error");
            // unconditionally abort even in production
            abort();
        }
    } else {
        cexds_arr_integrity(a, 0);
    }
    cexds_array_header temp = { 0 }; // force debugging
    size_t min_len = (a ? cexds_header(a)->length : 0) + addlen;
    (void)sizeof(temp);

    // compute the minimum capacity needed
    if (min_len > min_cap) {
        min_cap = min_len;
    }

    // increase needed capacity to guarantee O(1) amortized
    if (min_cap < 2 * arr$cap(a)) {
        min_cap = 2 * arr$cap(a);
    } else if (min_cap < 16) {
        min_cap = 16;
    }
    uassert(min_cap < PTRDIFF_MAX && "negative or overflow after processing");
    uassert(addlen > 0 || min_cap > 0);

    if (min_cap <= arr$cap(a)) {
        return a;
    }

    void* b;
    if (a == NULL) {
        b = mem$malloc(
            allc,
            mem$aligned_round(elemsize * min_cap + CEXDS_HDR_PAD, CEXDS_HDR_PAD),
            CEXDS_HDR_PAD
        );
    } else {
        cexds_array_header* hdr = cexds_header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        // NOTE: we must unpoison to prevent false ASAN use-after-poison check if data is copied
        mem$asan_unpoison(hdr->__poison_area, sizeof(hdr->__poison_area));
        b = mem$realloc(
            cexds_header(a)->allocator,
            cexds_base(a),
            mem$aligned_round(elemsize * min_cap + CEXDS_HDR_PAD, CEXDS_HDR_PAD),
            CEXDS_HDR_PAD
        );
    }

    if (b == NULL) {
        // oops memory error
        return NULL;
    }

    b = (char*)b + CEXDS_HDR_PAD;
    cexds_array_header* hdr = cexds_header(b);
    if (a == NULL) {
        hdr->length = 0;
        hdr->hash_table = NULL;
        hdr->allocator = allc;
        hdr->magic_num = CEXDS_ARR_MAGIC;
        hdr->allocator_scope_depth = allc->scope_depth(allc);
        mem$asan_poison(hdr->__poison_area, sizeof(hdr->__poison_area));
    } else {
        mem$asan_poison(hdr->__poison_area, sizeof(hdr->__poison_area));
        CEXDS_STATS(++cexds_array_grow);
    }
    hdr->capacity = min_cap;

    return b;
}

void
cexds_arrfreef(void* a)
{
    if (a != NULL) {
        uassert(cexds_header(a)->allocator != NULL);
        cexds_array_header* h = cexds_header(a);
        h->allocator->free(h->allocator, cexds_base(a));
    }
}

//
// cexds_hm hash table implementation
//

#define CEXDS_BUCKET_LENGTH 8
#define CEXDS_BUCKET_SHIFT (CEXDS_BUCKET_LENGTH == 8 ? 3 : 2)
#define CEXDS_BUCKET_MASK (CEXDS_BUCKET_LENGTH - 1)
#define CEXDS_CACHE_LINE_SIZE 64

#define cexds_hash_table(a) ((cexds_hash_index*)cexds_header(a)->hash_table)

typedef struct
{
    size_t hash[CEXDS_BUCKET_LENGTH];
    ptrdiff_t index[CEXDS_BUCKET_LENGTH];
} cexds_hash_bucket; // in 32-bit, this is one 64-byte cache line; in 64-bit, each array is one
                     // 64-byte cache line
_Static_assert(sizeof(cexds_hash_bucket) == 128, "cacheline aligned");

typedef struct cexds_hash_index
{
    char* temp_key; // this MUST be the first field of the hash table
    size_t slot_count;
    size_t used_count;
    size_t used_count_threshold;
    size_t used_count_shrink_threshold;
    size_t tombstone_count;
    size_t tombstone_count_threshold;
    size_t seed;
    size_t slot_count_log2;
    cexds_string_arena string;

    // not a separate allocation, just 64-byte aligned storage after this struct
    cexds_hash_bucket* storage;
} cexds_hash_index;

#define CEXDS_INDEX_EMPTY -1
#define CEXDS_INDEX_DELETED -2
#define CEXDS_INDEX_IN_USE(x) ((x) >= 0)

#define CEXDS_HASH_EMPTY 0
#define CEXDS_HASH_DELETED 1

#define CEXDS_SIZE_T_BITS ((sizeof(size_t)) * 8)

static inline size_t
cexds_probe_position(size_t hash, size_t slot_count, size_t slot_log2)
{
    size_t pos;
    (void)(slot_log2);
    pos = hash & (slot_count - 1);
#ifdef CEXDS_INTERNAL_BUCKET_START
    pos &= ~CEXDS_BUCKET_MASK;
#endif
    return pos;
}

static inline size_t
cexds_log2(size_t slot_count)
{
    size_t n = 0;
    while (slot_count > 1) {
        slot_count >>= 1;
        ++n;
    }
    return n;
}

void
cexds_hmclear_func(struct cexds_hash_index* t, cexds_hash_index* old_table, size_t cexds_hash_seed)
{
    if (t == NULL) {
        // typically external call of uninitialized table
        return;
    }

    uassert(t->slot_count > 0);
    t->tombstone_count = 0;
    t->used_count = 0;

    // TODO: cleanup strings from old table???

    if (old_table) {
        t->string = old_table->string;
        // reuse old seed so we can reuse old hashes so below "copy out old data" doesn't do any
        // hashing
        t->seed = old_table->seed;
    } else {
        memset(&t->string, 0, sizeof(t->string));
        t->seed = cexds_hash_seed;
    }

    size_t i, j;
    for (i = 0; i < t->slot_count >> CEXDS_BUCKET_SHIFT; ++i) {
        cexds_hash_bucket* b = &t->storage[i];
        for (j = 0; j < CEXDS_BUCKET_LENGTH; ++j) {
            b->hash[j] = CEXDS_HASH_EMPTY;
        }
        for (j = 0; j < CEXDS_BUCKET_LENGTH; ++j) {
            b->index[j] = CEXDS_INDEX_EMPTY;
        }
    }
}

static cexds_hash_index*
cexds_make_hash_index(
    size_t slot_count,
    cexds_hash_index* old_table,
    IAllocator allc,
    size_t cexds_hash_seed
)
{
    cexds_hash_index* t = mem$malloc(
        allc,
        (slot_count >> CEXDS_BUCKET_SHIFT) * sizeof(cexds_hash_bucket) + sizeof(cexds_hash_index) +
            CEXDS_CACHE_LINE_SIZE - 1
    );
    t->storage = (cexds_hash_bucket*)mem$aligned_pointer((size_t)(t + 1), CEXDS_CACHE_LINE_SIZE);
    t->slot_count = slot_count;
    t->slot_count_log2 = cexds_log2(slot_count);
    t->tombstone_count = 0;
    t->used_count = 0;

    // compute without overflowing
    t->used_count_threshold = slot_count - (slot_count >> 2);
    t->tombstone_count_threshold = (slot_count >> 3) + (slot_count >> 4);
    t->used_count_shrink_threshold = slot_count >> 2;

    if (slot_count <= CEXDS_BUCKET_LENGTH) {
        t->used_count_shrink_threshold = 0;
    }
    // to avoid infinite loop, we need to guarantee that at least one slot is empty and will
    // terminate probes
    uassert(t->used_count_threshold + t->tombstone_count_threshold < t->slot_count);
    CEXDS_STATS(++cexds_hash_alloc);

    cexds_hmclear_func(t, old_table, cexds_hash_seed);

    // copy out the old data, if any
    if (old_table) {
        size_t i, j;
        t->used_count = old_table->used_count;
        for (i = 0; i < old_table->slot_count >> CEXDS_BUCKET_SHIFT; ++i) {
            cexds_hash_bucket* ob = &old_table->storage[i];
            for (j = 0; j < CEXDS_BUCKET_LENGTH; ++j) {
                if (CEXDS_INDEX_IN_USE(ob->index[j])) {
                    size_t hash = ob->hash[j];
                    size_t pos = cexds_probe_position(hash, t->slot_count, t->slot_count_log2);
                    size_t step = CEXDS_BUCKET_LENGTH;
                    CEXDS_STATS(++cexds_rehash_items);
                    for (;;) {
                        size_t limit, z;
                        cexds_hash_bucket* bucket;
                        bucket = &t->storage[pos >> CEXDS_BUCKET_SHIFT];
                        CEXDS_STATS(++cexds_rehash_probes);

                        for (z = pos & CEXDS_BUCKET_MASK; z < CEXDS_BUCKET_LENGTH; ++z) {
                            if (bucket->hash[z] == 0) {
                                bucket->hash[z] = hash;
                                bucket->index[z] = ob->index[j];
                                goto done;
                            }
                        }

                        limit = pos & CEXDS_BUCKET_MASK;
                        for (z = 0; z < limit; ++z) {
                            if (bucket->hash[z] == 0) {
                                bucket->hash[z] = hash;
                                bucket->index[z] = ob->index[j];
                                goto done;
                            }
                        }

                        pos += step; // quadratic probing
                        step += CEXDS_BUCKET_LENGTH;
                        pos &= (t->slot_count - 1);
                    }
                }
            done:;
            }
        }
    }

    return t;
}

#define CEXDS_ROTATE_LEFT(val, n) (((val) << (n)) | ((val) >> (CEXDS_SIZE_T_BITS - (n))))
#define CEXDS_ROTATE_RIGHT(val, n) (((val) >> (n)) | ((val) << (CEXDS_SIZE_T_BITS - (n))))


#ifdef CEXDS_SIPHASH_2_4
#define CEXDS_SIPHASH_C_ROUNDS 2
#define CEXDS_SIPHASH_D_ROUNDS 4
typedef int CEXDS_SIPHASH_2_4_can_only_be_used_in_64_bit_builds[sizeof(size_t) == 8 ? 1 : -1];
#endif

#ifndef CEXDS_SIPHASH_C_ROUNDS
#define CEXDS_SIPHASH_C_ROUNDS 1
#endif
#ifndef CEXDS_SIPHASH_D_ROUNDS
#define CEXDS_SIPHASH_D_ROUNDS 1
#endif

static inline size_t
cexds_hash_string(const char* str, size_t str_cap, size_t seed)
{
    size_t hash = seed;
    // NOTE: using max buffer capacity capping, this allows using hash
    //       on char buf[N] - without stack overflowing
    for (size_t i = 0; i < str_cap && *str; i++) {
        hash = CEXDS_ROTATE_LEFT(hash, 9) + (unsigned char)*str++;
    }

    // Thomas Wang 64-to-32 bit mix function, hopefully also works in 32 bits
    hash ^= seed;
    hash = (~hash) + (hash << 18);
    hash ^= hash ^ CEXDS_ROTATE_RIGHT(hash, 31);
    hash = hash * 21;
    hash ^= hash ^ CEXDS_ROTATE_RIGHT(hash, 11);
    hash += (hash << 6);
    hash ^= CEXDS_ROTATE_RIGHT(hash, 22);
    return hash + seed;
}

static inline size_t
cexds_siphash_bytes(const void* p, size_t len, size_t seed)
{
    unsigned char* d = (unsigned char*)p;
    size_t i, j;
    size_t v0, v1, v2, v3, data;

    // hash that works on 32- or 64-bit registers without knowing which we have
    // (computes different results on 32-bit and 64-bit platform)
    // derived from siphash, but on 32-bit platforms very different as it uses 4 32-bit state not 4
    // 64-bit
    v0 = ((((size_t)0x736f6d65 << 16) << 16) + 0x70736575) ^ seed;
    v1 = ((((size_t)0x646f7261 << 16) << 16) + 0x6e646f6d) ^ ~seed;
    v2 = ((((size_t)0x6c796765 << 16) << 16) + 0x6e657261) ^ seed;
    v3 = ((((size_t)0x74656462 << 16) << 16) + 0x79746573) ^ ~seed;

#ifdef CEXDS_TEST_SIPHASH_2_4
    // hardcoded with key material in the siphash test vectors
    v0 ^= 0x0706050403020100ull ^ seed;
    v1 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
    v2 ^= 0x0706050403020100ull ^ seed;
    v3 ^= 0x0f0e0d0c0b0a0908ull ^ ~seed;
#endif

#define CEXDS_SIPROUND()                                                                           \
    do {                                                                                           \
        v0 += v1;                                                                                  \
        v1 = CEXDS_ROTATE_LEFT(v1, 13);                                                            \
        v1 ^= v0;                                                                                  \
        v0 = CEXDS_ROTATE_LEFT(v0, CEXDS_SIZE_T_BITS / 2);                                         \
        v2 += v3;                                                                                  \
        v3 = CEXDS_ROTATE_LEFT(v3, 16);                                                            \
        v3 ^= v2;                                                                                  \
        v2 += v1;                                                                                  \
        v1 = CEXDS_ROTATE_LEFT(v1, 17);                                                            \
        v1 ^= v2;                                                                                  \
        v2 = CEXDS_ROTATE_LEFT(v2, CEXDS_SIZE_T_BITS / 2);                                         \
        v0 += v3;                                                                                  \
        v3 = CEXDS_ROTATE_LEFT(v3, 21);                                                            \
        v3 ^= v0;                                                                                  \
    } while (0)

    for (i = 0; i + sizeof(size_t) <= len; i += sizeof(size_t), d += sizeof(size_t)) {
        data = d[0] | (d[1] << 8) | (d[2] << 16) | (d[3] << 24);
        data |= (size_t)(d[4] | (d[5] << 8) | (d[6] << 16) | (d[7] << 24))
             << 16 << 16; // discarded if size_t == 4

        v3 ^= data;
        for (j = 0; j < CEXDS_SIPHASH_C_ROUNDS; ++j) {
            CEXDS_SIPROUND();
        }
        v0 ^= data;
    }
    data = len << (CEXDS_SIZE_T_BITS - 8);
    switch (len - i) {
        case 7:
            data |= ((size_t)d[6] << 24) << 24; // fall through
        case 6:
            data |= ((size_t)d[5] << 20) << 20; // fall through
        case 5:
            data |= ((size_t)d[4] << 16) << 16; // fall through
        case 4:
            data |= (d[3] << 24); // fall through
        case 3:
            data |= (d[2] << 16); // fall through
        case 2:
            data |= (d[1] << 8); // fall through
        case 1:
            data |= d[0]; // fall through
        case 0:
            break;
    }
    v3 ^= data;
    for (j = 0; j < CEXDS_SIPHASH_C_ROUNDS; ++j) {
        CEXDS_SIPROUND();
    }
    v0 ^= data;
    v2 ^= 0xff;
    for (j = 0; j < CEXDS_SIPHASH_D_ROUNDS; ++j) {
        CEXDS_SIPROUND();
    }

#ifdef CEXDS_SIPHASH_2_4
    return v0 ^ v1 ^ v2 ^ v3;
#else
    return v1 ^ v2 ^
           v3; // slightly stronger since v0^v3 in above cancels out final round operation? I
               // tweeted at the authors of SipHash about this but they didn't reply
#endif
}

static inline size_t
cexds_hash_bytes(const void* p, size_t len, size_t seed)
{
#ifdef CEXDS_SIPHASH_2_4
    return cexds_siphash_bytes(p, len, seed);
#else
    unsigned char* d = (unsigned char*)p;

    if (len == 4) {
        u32 hash = (u32)d[0] | ((u32)d[1] << 8) | ((u32)d[2] << 16) | ((u32)d[3] << 24);
        // HASH32-BB  Bob Jenkin's presumably-accidental version of Thomas Wang hash with rotates
        // turned into shifts. Note that converting these back to rotates makes it run a lot slower,
        // presumably due to collisions, so I'm not really sure what's going on.
        hash ^= seed;
        hash = (hash ^ 61) ^ (hash >> 16);
        hash = hash + (hash << 3);
        hash = hash ^ (hash >> 4);
        hash = hash * 0x27d4eb2d;
        hash ^= seed;
        hash = hash ^ (hash >> 15);
        return (((size_t)hash << 16 << 16) | hash) ^ seed;
    } else if (len == 8 && sizeof(size_t) == 8) {
        size_t hash = (size_t)d[0] | ((size_t)d[1] << 8) | ((size_t)d[2] << 16) |
                      ((size_t)d[3] << 24);

        hash |= (size_t)((size_t)d[4] | ((size_t)d[5] << 8) | ((size_t)d[6] << 16) |
                         ((size_t)d[7] << 24))
             << 16 << 16;
        hash ^= seed;
        hash = (~hash) + (hash << 21);
        hash ^= CEXDS_ROTATE_RIGHT(hash, 24);
        hash *= 265;
        hash ^= CEXDS_ROTATE_RIGHT(hash, 14);
        hash ^= seed;
        hash *= 21;
        hash ^= CEXDS_ROTATE_RIGHT(hash, 28);
        hash += (hash << 31);
        hash = (~hash) + (hash << 18);
        return hash;
    } else {
        return cexds_siphash_bytes(p, len, seed);
    }
#endif
}

static inline size_t
cexds_hash(enum _CexDsKeyType_e key_type, const void* key, size_t key_size, size_t seed)
{
    switch (key_type) {
        case _CexDsKeyType__generic:
            return cexds_hash_bytes(key, key_size, seed);

        case _CexDsKeyType__charptr:
            // NOTE: we can't know char* length without touching it,
            // 65536 is a max key length in case of char was not null term
            return cexds_hash_string(*(char**)key, 65536, seed);

        case _CexDsKeyType__charbuf:
            return cexds_hash_string(key, key_size, seed);

        case _CexDsKeyType__cexstr: {
            str_s* s = (str_s*)key;
            return cexds_hash_string(s->buf, s->len, seed);
        }
    }
    uassert(false && "unexpected key type");
    abort();
}

static bool
cexds_compare_strings_bounded(
    const char* a_str,
    const char* b_str,
    size_t a_capacity,
    size_t b_capacity
)
{
    size_t i = 0;
    size_t min_cap = a_capacity;
    const char* long_str = NULL;

    if (a_capacity != b_capacity) {
        if (a_capacity > b_capacity) {
            min_cap = b_capacity;
            long_str = a_str;
        } else {
            min_cap = a_capacity;
            long_str = b_str;
        }
    }
    while (i < min_cap) {
        if (a_str[i] != b_str[i]) {
            return false;
        } else if (a_str[i] == '\0') {
            // both are equal and zero term
            return true;
        }
        i++;
    }
    // Edge case when buf capacity are equal (long_str is NULL)
    // or  buf[3] ="foo" / buf[100] = "foo", ensure that buf[100] ends with '\0'
    return !long_str || long_str[i] == '\0';
}

static bool
cexds_is_key_equal(
    void* a,
    size_t elemsize,
    void* key,
    size_t keysize,
    size_t keyoffset,
    enum _CexDsKeyType_e key_type,
    size_t i
)
{
    void* hm_key = cexds_item_ptr(a, i, elemsize) + keyoffset;

    switch (key_type) {
        case _CexDsKeyType__generic:
            return 0 == memcmp(key, hm_key, keysize);

        case _CexDsKeyType__charptr:
            return 0 == strcmp(*(char**)key, *(char**)hm_key);
        case _CexDsKeyType__charbuf:
            return 0 == strcmp((char*)key, (char*)hm_key);

        case _CexDsKeyType__cexstr: {
            str_s* _k = (str_s*)key;
            str_s* _hm = (str_s*)hm_key;
            if (_k->len != _hm->len) {
                return false;
            }
            return 0 == memcmp(_k->buf, _hm->buf, _k->len);
        }
    }
    uassert(false && "unexpected key type");
    abort();
}

void
cexds_hmfree_func(void* a, size_t elemsize)
{
    if (a == NULL) {
        return;
    }
    cexds_arr_integrity(a, CEXDS_HM_MAGIC);

    cexds_array_header* h = cexds_header(a);
    uassert(h->allocator != NULL);

    if (cexds_hash_table(a) != NULL) {
        if (cexds_hash_table(a)->string.mode == CEXDS_SH_STRDUP) {
            size_t i;
            // skip 0th element, which is default
            for (i = 1; i < h->length; ++i) {
                h->allocator->free(h->allocator, *(char**)((char*)a + elemsize * i));
            }
        }
        cexds_strreset(&cexds_hash_table(a)->string);
    }
    h->allocator->free(h->allocator, h->hash_table);
    h->allocator->free(h->allocator, cexds_base(a));
}

static ptrdiff_t
cexds_hm_find_slot(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset)
{
    cexds_arr_integrity(a, CEXDS_HM_MAGIC);
    cexds_hash_index* table = cexds_hash_table(a);
    size_t hash = cexds_hash(cexds_header(a)->hm_key_type, key, keysize, table->seed);
    size_t step = CEXDS_BUCKET_LENGTH;
    enum _CexDsKeyType_e key_type = cexds_header(a)->hm_key_type;

    if (hash < 2) {
        hash += 2; // stored hash values are forbidden from being 0, so we can detect empty slots
    }

    size_t pos = cexds_probe_position(hash, table->slot_count, table->slot_count_log2);

    // TODO: check when this could be infinite loop (due to overflow or something)?
    for (;;) {
        CEXDS_STATS(++cexds_hash_probes);
        cexds_hash_bucket* bucket = &table->storage[pos >> CEXDS_BUCKET_SHIFT];

        // start searching from pos to end of bucket, this should help performance on small hash
        // tables that fit in cache
        for (size_t i = pos & CEXDS_BUCKET_MASK; i < CEXDS_BUCKET_LENGTH; ++i) {
            if (bucket->hash[i] == hash) {
                if (cexds_is_key_equal(
                        a,
                        elemsize,
                        key,
                        keysize,
                        keyoffset,
                        key_type,
                        bucket->index[i]
                    )) {
                    return (pos & ~CEXDS_BUCKET_MASK) + i;
                }
            } else if (bucket->hash[i] == CEXDS_HASH_EMPTY) {
                return -1;
            }
        }

        // search from beginning of bucket to pos
        size_t limit = pos & CEXDS_BUCKET_MASK;
        for (size_t i = 0; i < limit; ++i) {
            if (bucket->hash[i] == hash) {
                if (cexds_is_key_equal(
                        a,
                        elemsize,
                        key,
                        keysize,
                        keyoffset,
                        key_type,
                        bucket->index[i]
                    )) {
                    return (pos & ~CEXDS_BUCKET_MASK) + i;
                }
            } else if (bucket->hash[i] == CEXDS_HASH_EMPTY) {
                return -1;
            }
        }

        // quadratic probing
        pos += step;
        step += CEXDS_BUCKET_LENGTH;
        pos &= (table->slot_count - 1);
    }
}

void*
cexds_hmget_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset)
{
    cexds_arr_integrity(a, CEXDS_HM_MAGIC);

    cexds_hash_index* table = (cexds_hash_index*)cexds_header(a)->hash_table;
    if (table != NULL) {
        ptrdiff_t slot = cexds_hm_find_slot(a, elemsize, key, keysize, keyoffset);
        if (slot >= 0) {
            cexds_hash_bucket* b = &table->storage[slot >> CEXDS_BUCKET_SHIFT];
            size_t idx = b->index[slot & CEXDS_BUCKET_MASK];
            return ((char*)a + elemsize * idx);
        }
    }
    return NULL;
}

void*
cexds_hminit(
    size_t elemsize,
    IAllocator allc,
    enum _CexDsKeyType_e key_type,
    struct cexds_hm_new_kwargs_s* kwargs
)
{
    size_t capacity = 0;
    size_t hm_seed = 0xBadB0dee;

    if (kwargs) {
        capacity = kwargs->capacity;
        hm_seed = kwargs->seed ? kwargs->seed : hm_seed;
    }
    void* a = cexds_arrgrowf(NULL, elemsize, 0, capacity, allc);
    if (a == NULL) {
        return NULL; // memory error
    }

    uassert(cexds_header(a)->magic_num == CEXDS_ARR_MAGIC);
    uassert(cexds_header(a)->hash_table == NULL);

    cexds_header(a)->magic_num = CEXDS_HM_MAGIC;
    cexds_header(a)->hm_seed = hm_seed;
    cexds_header(a)->hm_key_type = key_type;

    cexds_hash_index* table = cexds_header(a)->hash_table;

    // ensure slot counts must be pow of 2
    uassert(mem$is_power_of2(arr$cap(a)));
    table = cexds_header(a)->hash_table = cexds_make_hash_index(
        arr$cap(a),
        NULL,
        cexds_header(a)->allocator,
        cexds_header(a)->hm_seed
    );

    if (table) {
        // NEW Table initialization here
        // nt->string.mode = mode >= CEXDS_HM_STRING ? CEXDS_SH_DEFAULT : 0;
        table->string.mode = 0;
    }

    return a;
}

static char* cexds_strdup(char* str);

void*
cexds_hmput_key(
    void* a,
    size_t elemsize,
    void* key,
    size_t keysize,
    size_t keyoffset,
    void* full_elem,
    void* result
)
{
    uassert(result != NULL);
    cexds_arr_integrity(a, CEXDS_HM_MAGIC);

    void** out_result = (void**)result;
    enum _CexDsKeyType_e key_type = cexds_header(a)->hm_key_type;
    *out_result = NULL;

    cexds_hash_index* table = (cexds_hash_index*)cexds_header(a)->hash_table;
    if (table == NULL || table->used_count >= table->used_count_threshold) {

        size_t slot_count = (table == NULL) ? CEXDS_BUCKET_LENGTH : table->slot_count * 2;
        cexds_array_header* hdr = cexds_header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        cexds_hash_index* nt = cexds_make_hash_index(
            slot_count,
            table,
            cexds_header(a)->allocator,
            cexds_header(a)->hm_seed
        );

        if (nt == NULL) {
            uassert(nt != NULL && "new hash table memory error");
            *out_result = NULL;
            goto end;
        }

        if (table) {
            cexds_header(a)->allocator->free(cexds_header(a)->allocator, table);
        } else {
            // NEW Table initialization here
            // nt->string.mode = mode >= CEXDS_HM_STRING ? CEXDS_SH_DEFAULT : 0;
            nt->string.mode = 0;
        }
        cexds_header(a)->hash_table = table = nt;
        CEXDS_STATS(++cexds_hash_grow);
    }

    // we iterate hash table explicitly because we want to track if we saw a tombstone
    {
        size_t hash = cexds_hash(cexds_header(a)->hm_key_type, key, keysize, table->seed);
        size_t step = CEXDS_BUCKET_LENGTH;
        size_t pos;
        ptrdiff_t tombstone = -1;
        cexds_hash_bucket* bucket;

        // stored hash values are forbidden from being 0, so we can detect empty slots to early out
        // quickly
        if (hash < 2) {
            hash += 2;
        }

        pos = cexds_probe_position(hash, table->slot_count, table->slot_count_log2);

        for (;;) {
            size_t limit, i;
            CEXDS_STATS(++cexds_hash_probes);
            bucket = &table->storage[pos >> CEXDS_BUCKET_SHIFT];

            // start searching from pos to end of bucket
            for (i = pos & CEXDS_BUCKET_MASK; i < CEXDS_BUCKET_LENGTH; ++i) {
                if (bucket->hash[i] == hash) {
                    if (cexds_is_key_equal(
                            a,
                            elemsize,
                            key,
                            keysize,
                            keyoffset,
                            key_type,
                            bucket->index[i]
                        )) {

                        *out_result = cexds_item_ptr(a, bucket->index[i], elemsize);
                        goto process_key;
                    }
                } else if (bucket->hash[i] == 0) {
                    pos = (pos & ~CEXDS_BUCKET_MASK) + i;
                    goto found_empty_slot;
                } else if (tombstone < 0) {
                    if (bucket->index[i] == CEXDS_INDEX_DELETED) {
                        tombstone = (ptrdiff_t)((pos & ~CEXDS_BUCKET_MASK) + i);
                    }
                }
            }

            // search from beginning of bucket to pos
            limit = pos & CEXDS_BUCKET_MASK;
            for (i = 0; i < limit; ++i) {
                if (bucket->hash[i] == hash) {
                    if (cexds_is_key_equal(
                            a,
                            elemsize,
                            key,
                            keysize,
                            keyoffset,
                            key_type,
                            bucket->index[i]
                        )) {
                        *out_result = cexds_item_ptr(a, bucket->index[i], elemsize);
                        goto process_key;
                    }
                } else if (bucket->hash[i] == 0) {
                    pos = (pos & ~CEXDS_BUCKET_MASK) + i;
                    goto found_empty_slot;
                } else if (tombstone < 0) {
                    if (bucket->index[i] == CEXDS_INDEX_DELETED) {
                        tombstone = (ptrdiff_t)((pos & ~CEXDS_BUCKET_MASK) + i);
                    }
                }
            }

            // quadratic probing
            pos += step;
            step += CEXDS_BUCKET_LENGTH;
            pos &= (table->slot_count - 1);
        }
    found_empty_slot:
        if (tombstone >= 0) {
            pos = tombstone;
            --table->tombstone_count;
        }
        ++table->used_count;

        {
            ptrdiff_t i = (ptrdiff_t)cexds_header(a)->length;
            // we want to do cexds_arraddn(1), but we can't use the macros since we don't have
            // something of the right type
            if ((size_t)i + 1 > arr$cap(a)) {
                *(void**)&a = cexds_arrgrowf(a, elemsize, 1, 0, NULL);
                if (a == NULL) {
                    uassert(a != NULL && "new array for table memory error");
                    *out_result = NULL;
                    goto end;
                }
            }

            uassert((size_t)i + 1 <= arr$cap(a));
            cexds_header(a)->length = i + 1;
            bucket = &table->storage[pos >> CEXDS_BUCKET_SHIFT];
            bucket->hash[pos & CEXDS_BUCKET_MASK] = hash;
            bucket->index[pos & CEXDS_BUCKET_MASK] = i;
            *out_result = cexds_item_ptr(a, i, elemsize);
        }
        goto process_key;
    }

process_key:
    uassert(*out_result != NULL);
    switch (table->string.mode) {
        case CEXDS_SH_STRDUP:
            uassert(false);
            // cexds_temp_key(a) = *(char**)((char*)a + elemsize * i) = cexds_strdup((char*)key
            // );
            break;
        case CEXDS_SH_ARENA:
            uassert(false);
            // cexds_temp_key(a) = *(char**)((char*)a + elemsize * i
            // ) = cexds_stralloc(&table->string, (char*)key);
            break;
        case CEXDS_SH_DEFAULT:
            uassert(false);
            // cexds_temp_key(a) = *(char**)((char*)a + elemsize * i) = (char*)key;
            break;
        default:
            if (full_elem) {
                memcpy(((char*)*out_result), full_elem, elemsize);
            } else {
                memcpy(((char*)*out_result) + keyoffset, key, keysize);
            }
            break;
    }

end:
    return a;
}


bool
cexds_hmdel_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset)
{
    cexds_arr_integrity(a, CEXDS_HM_MAGIC);

    cexds_hash_index* table = (cexds_hash_index*)cexds_header(a)->hash_table;

    uassert(cexds_header(a)->allocator != NULL);
    if (table == 0) {
        return false;
    }

    ptrdiff_t slot = cexds_hm_find_slot(a, elemsize, key, keysize, keyoffset);
    if (slot < 0) {
        return false;
    }

    cexds_hash_bucket* b = &table->storage[slot >> CEXDS_BUCKET_SHIFT];
    int i = slot & CEXDS_BUCKET_MASK;
    ptrdiff_t old_index = b->index[i];
    ptrdiff_t final_index = (ptrdiff_t)cexds_header(a)->length - 1;
    uassert(slot < (ptrdiff_t)table->slot_count);
    uassert(table->used_count > 0);
    --table->used_count;
    ++table->tombstone_count;
    // uassert(table->tombstone_count < table->slot_count/4);
    b->hash[i] = CEXDS_HASH_DELETED;
    b->index[i] = CEXDS_INDEX_DELETED;

    // if (mode == CEXDS_HM_STRING && table->string.mode == CEXDS_SH_STRDUP) {
    //     // FIX: this may conflict with static alloc
    //     cexds_header(a)->allocator->free(*(char**)((char*)a + elemsize * old_index));
    // }

    // if indices are the same, memcpy is a no-op, but back-pointer-fixup will fail, so
    // skip
    if (old_index != final_index) {
        // Replacing deleted element by last one, and update hashmap buckets for last element
        memmove((char*)a + elemsize * old_index, (char*)a + elemsize * final_index, elemsize);

        void* key_data_p = NULL;
        switch (cexds_header(a)->hm_key_type) {
            case _CexDsKeyType__generic:
                key_data_p = (char*)a + elemsize * old_index + keyoffset;
                break;

            case _CexDsKeyType__charbuf:
                key_data_p = (char*)((char*)a + elemsize * old_index + keyoffset);
                break;

            case _CexDsKeyType__charptr:
                key_data_p = *(char**)((char*)a + elemsize * old_index + keyoffset);
                break;

            case _CexDsKeyType__cexstr: {
                str_s* s = (str_s*)((char*)a + elemsize * old_index + keyoffset);
                key_data_p = s;
                break;
            }
        }
        uassert(key_data_p != NULL);
        slot = cexds_hm_find_slot(a, elemsize, key_data_p, keysize, keyoffset);
        uassert(slot >= 0);
        b = &table->storage[slot >> CEXDS_BUCKET_SHIFT];
        i = slot & CEXDS_BUCKET_MASK;
        uassert(b->index[i] == final_index);
        b->index[i] = old_index;
    }
    cexds_header(a)->length -= 1;

    if (table->used_count < table->used_count_shrink_threshold &&
        table->slot_count > CEXDS_BUCKET_LENGTH) {
        cexds_array_header* hdr = cexds_header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        cexds_header(a)->hash_table = cexds_make_hash_index(
            table->slot_count >> 1,
            table,
            cexds_header(a)->allocator,
            cexds_header(a)->hm_seed
        );
        cexds_header(a)->allocator->free(cexds_header(a)->allocator, table);
        CEXDS_STATS(++cexds_hash_shrink);
    } else if (table->tombstone_count > table->tombstone_count_threshold) {
        cexds_array_header* hdr = cexds_header(a);
        (void)hdr;
        uassert(
            hdr->allocator->scope_depth(hdr->allocator) == hdr->allocator_scope_depth &&
            "passing object between different mem$scope() will lead to use-after-free / ASAN poison issues"
        );
        cexds_header(a)->hash_table = cexds_make_hash_index(
            table->slot_count,
            table,
            cexds_header(a)->allocator,
            cexds_header(a)->hm_seed
        );
        cexds_header(a)->allocator->free(cexds_header(a)->allocator, table);
        CEXDS_STATS(++cexds_hash_rebuild);
    }

    return a;
}

static char*
cexds_strdup(char* str)
{
    // to keep replaceable allocator simple, we don't want to use strdup.
    // rolling our own also avoids problem of strdup vs _strdup
    size_t len = strlen(str) + 1;
    // char* p = (char*)CEXDS_REALLOC(NULL, 0, len);
    // TODO
    char* p = (char*)realloc(NULL, len);
    memmove(p, str, len);
    return p;
}

#ifndef CEXDS_STRING_ARENA_BLOCKSIZE_MIN
#define CEXDS_STRING_ARENA_BLOCKSIZE_MIN 512u
#endif
#ifndef CEXDS_STRING_ARENA_BLOCKSIZE_MAX
#define CEXDS_STRING_ARENA_BLOCKSIZE_MAX (1u << 20)
#endif

char*
cexds_stralloc(cexds_string_arena* a, char* str)
{
    char* p;
    size_t len = strlen(str) + 1;
    if (len > a->remaining) {
        // compute the next blocksize
        size_t blocksize = a->block;

        // size is 512, 512, 1024, 1024, 2048, 2048, 4096, 4096, etc., so that
        // there are log(SIZE) allocations to free when we destroy the table
        blocksize = (size_t)(CEXDS_STRING_ARENA_BLOCKSIZE_MIN) << (blocksize >> 1);

        // if size is under 1M, advance to next blocktype
        if (blocksize < (size_t)(CEXDS_STRING_ARENA_BLOCKSIZE_MAX)) {
            ++a->block;
        }

        if (len > blocksize) {
            // if string is larger than blocksize, then just allocate the full size.
            // note that we still advance string_block so block size will continue
            // increasing, so e.g. if somebody only calls this with 1000-long strings,
            // eventually the arena will start doubling and handling those as well
            cexds_string_block* sb = realloc(NULL, sizeof(*sb) - 8 + len);
            memmove(sb->storage, str, len);
            if (a->storage) {
                // insert it after the first element, so that we don't waste the space there
                sb->next = a->storage->next;
                a->storage->next = sb;
            } else {
                sb->next = 0;
                a->storage = sb;
                a->remaining = 0; // this is redundant, but good for clarity
            }
            return sb->storage;
        } else {
            cexds_string_block* sb = realloc(NULL, sizeof(*sb) - 8 + blocksize);
            sb->next = a->storage;
            a->storage = sb;
            a->remaining = blocksize;
        }
    }

    uassert(len <= a->remaining);
    p = a->storage->storage + a->remaining - len;
    a->remaining -= len;
    memmove(p, str, len);
    return p;
}

void
cexds_strreset(cexds_string_arena* a)
{
    cexds_string_block *x, *y;
    x = a->storage;
    while (x) {
        y = x->next;
        free(x);
        x = y;
    }
    memset(a, 0, sizeof(*a));
}

/*
*                   io.c
*/
#include <errno.h>
#include <unistd.h>


Exception
io_fopen(FILE** file, const char* filename, const char* mode)
{
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }

    *file = NULL;

    if (filename == NULL) {
        return Error.argument;
    }
    if (mode == NULL) {
        uassert(mode != NULL);
        return Error.argument;
    }

    *file = fopen(filename, mode);
    if (*file == NULL) {
        switch (errno) {
            case ENOENT:
                return Error.not_found;
            default:
                return strerror(errno);
        }
    }
    return Error.ok;
}

int
io_fileno(FILE* file)
{
    uassert(file != NULL);
    return fileno(file);
}

bool
io_isatty(FILE* file)
{
    uassert(file != NULL);
    return isatty(fileno(file)) == 1;
}

Exception
io_fflush(FILE* file)
{
    uassert(file != NULL);

    int ret = fflush(file);
    if (unlikely(ret == -1)) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

Exception
io_fseek(FILE* file, long offset, int whence)
{
    uassert(file != NULL);

    int ret = fseek(file, offset, whence);
    if (unlikely(ret == -1)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
    } else {
        return Error.ok;
    }
}

void
io_rewind(FILE* file)
{
    uassert(file != NULL);
    rewind(file);
}

Exception
io_ftell(FILE* file, usize* size)
{
    uassert(file != NULL);

    long ret = ftell(file);
    if (unlikely(ret < 0)) {
        if (errno == EINVAL) {
            return Error.argument;
        } else {
            return Error.io;
        }
        *size = 0;
    } else {
        *size = ret;
        return Error.ok;
    }
}

usize
io__file__size(FILE* file)
{
    uassert(file != NULL);

    usize fsize = 0;
    usize old_pos = 0;

    e$except_silent(err, io_ftell(file, &old_pos))
    {
        return 0;
    }
    e$except_silent(err, io_fseek(file, 0, SEEK_END))
    {
        return 0;
    }
    e$except_silent(err, io_ftell(file, &fsize))
    {
        return 0;
    }
    e$except_silent(err, io_fseek(file, old_pos, SEEK_SET))
    {
        return 0;
    }

    return fsize;
}

Exception
io_fread(FILE* file, void* restrict obj_buffer, usize obj_el_size, usize* obj_count)
{
    if (file == NULL) {
        return Error.argument;
    }
    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == NULL || *obj_count == 0) {
        return Error.argument;
    }

    const usize ret_count = fread(obj_buffer, obj_el_size, *obj_count, file);

    if (ret_count != *obj_count) {
        if (ferror(file)) {
            *obj_count = 0;
            return Error.io;
        } else {
            *obj_count = ret_count;
            return (ret_count == 0) ? Error.eof : Error.ok;
        }
    }

    return Error.ok;
}

Exception
io_fread_all(FILE* file, str_s* s, IAllocator allc)
{
    Exc result = Error.runtime;
    usize buf_size = 0;
    char* buf = NULL;


    if (file == NULL) {
        result = Error.argument;
        goto fail;
    }
    uassert(s != NULL);
    uassert(allc != NULL);

    // Forbid console and stdin
    if (unlikely(io_isatty(file))) {
        result = "io.fread_all() not allowed for pipe/socket/std[in/out/err]";
        goto fail;
    }

    usize _fsize = io__file__size(file);
    if (unlikely(_fsize > INT32_MAX)) {
        result = "io.fread_all() file is too big.";
        goto fail;
    }
    bool is_stream = false;
    if (unlikely(_fsize == 0)) {
        if (feof(file)) {
            result = EOK; // return just empty data
            goto fail;
        } else {
            is_stream = true;
        }
    }
    buf_size = (is_stream ? 4096 : _fsize) + 1 + 15;
    buf = mem$malloc(allc, buf_size);

    if (unlikely(buf == NULL)) {
        buf_size = 0;
        result = Error.memory;
        goto fail;
    }

    size_t total_read = 0;
    while (1) {
        if (total_read == buf_size) {
            if (unlikely(total_read > INT32_MAX)) {
                result = "io.fread_all() stream output is too big.";
                goto fail;
            }
            if (buf_size > 100 * 1024 * 1024) {
                buf_size *= 1.2;
            } else {
                buf_size *= 2;
            }
            char* new_buf = mem$realloc(allc, buf, buf_size);
            if (!new_buf) {
                result = Error.memory;
                goto fail;
            }
            buf = new_buf;
        }

        // Read data into the buf
        size_t bytes_read = fread(buf + total_read, 1, buf_size - total_read, file);
        if (bytes_read == 0) {
            if (feof(file)) {
                break;
            } else if (ferror(file)) {
                result = Error.io;
                goto fail;
            }
        }
        total_read += bytes_read;
    }
    char* final_buffer = mem$realloc(allc, buf, total_read + 1);
    if (!final_buffer) {
        result = Error.memory;
        goto fail;
    }
    buf = final_buffer;

    *s = (str_s){
        .buf = buf,
        .len = total_read,
    };
    buf[total_read] = '\0';
    return EOK;

fail:
    *s = (str_s){
        .buf = NULL,
        .len = 0,
    };

    if (io_fseek(file, 0, SEEK_END)) {
        // unused result
    }

    if (buf) {
        mem$free(allc, buf);
    }
    return result;
}

Exception
io_fread_line(FILE* file, str_s* s, IAllocator allc)
{
    Exc result = Error.runtime;
    usize cursor = 0;
    FILE* fh = file;
    char* buf = NULL;
    usize buf_size = 0;

    if (unlikely(file == NULL)) {
        result = Error.argument;
        goto fail;
    }
    uassert(s != NULL);

    int c = EOF;
    while ((c = fgetc(fh)) != EOF) {
        if (unlikely(c == '\n')) {
            // Handle windows \r\n new lines also
            if (cursor > 0 && buf[cursor - 1] == '\r') {
                cursor--;
            }
            break;
        }
        if (unlikely(c == '\0')) {
            // plain text file should not have any zero bytes in there
            result = Error.integrity;
            goto fail;
        }

        if (unlikely(cursor >= buf_size)) {
            if (buf == NULL) {
                uassert(cursor == 0 && "no buf, cursor expected 0");

                buf = mem$malloc(allc, 256);
                if (buf == NULL) {
                    result = Error.memory;
                    goto fail;
                }
                buf_size = 256 - 1; // keep extra for null
                buf[buf_size] = '\0';
                buf[cursor] = '\0';
            } else {
                uassert(cursor > 0 && "no buf, cursor expected 0");
                uassert(buf_size > 0 && "empty buffer, weird");

                if (buf_size + 1 < 256) {
                    // Cap minimal buf size
                    buf_size = 256 - 1;
                }

                // Grow initial size by factor of 2
                buf = mem$realloc(allc, buf, (buf_size + 1) * 2);
                if (buf == NULL) {
                    buf_size = 0;
                    result = Error.memory;
                    goto fail;
                }
                buf_size = (buf_size + 1) * 2 - 1;
                buf[buf_size] = '\0';
            }
        }
        buf[cursor] = c;
        cursor++;
    }


    if (unlikely(ferror(file))) {
        result = Error.io;
        goto fail;
    }

    if (unlikely(cursor == 0 && feof(file))) {
        result = Error.eof;
        goto fail;
    }

    if (buf != NULL) {
        buf[cursor] = '\0';
    } else {
        buf = mem$malloc(allc, 1);
        buf[0] = '\0';
        cursor = 0;
    }
    *s = (str_s){
        .buf = buf,
        .len = cursor,
    };
    return Error.ok;

fail:
    if (buf) {
        mem$free(allc, buf);
    }
    *s = (str_s){
        .buf = NULL,
        .len = 0,
    };
    return result;
}

Exception
io_fprintf(FILE* stream, const char* format, ...)
{
    uassert(stream != NULL);

    va_list va;
    va_start(va, format);
    int result = cexsp__vfprintf(stream, format, va);
    va_end(va);

    if (result == -1) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

void
io_printf(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    cexsp__vfprintf(stdout, format, va);
    va_end(va);
}

Exception
io_fwrite(FILE* file, const void* restrict obj_buffer, usize obj_el_size, usize obj_count)
{
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }

    if (obj_buffer == NULL) {
        return Error.argument;
    }
    if (obj_el_size == 0) {
        return Error.argument;
    }
    if (obj_count == 0) {
        return Error.argument;
    }

    const usize ret_count = fwrite(obj_buffer, obj_el_size, obj_count, file);

    if (ret_count != obj_count) {
        return Error.io;
    } else {
        return Error.ok;
    }
}

Exception
io__file__writeln(FILE* file, char* line)
{
    errno = 0;
    if (file == NULL) {
        uassert(file != NULL);
        return Error.argument;
    }
    if (line == NULL) {
        return Error.argument;
    }
    usize line_len = strlen(line);
    usize ret_count = fwrite(line, 1, line_len, file);
    if (ret_count != line_len) {
        return Error.io;
    }

    char new_line[] = { '\n' };
    ret_count = fwrite(new_line, 1, sizeof(new_line), file);

    if (ret_count != sizeof(new_line)) {
        return Error.io;
    }
    return Error.ok;
}

void
io_fclose(FILE** file)
{
    uassert(file != NULL);

    if (*file != NULL) {
        fclose(*file);
    }
    *file = NULL;
}


Exception
io__file__save(const char* path, const char* contents)
{
    if (path == NULL) {
        return Error.argument;
    }

    if (contents == NULL) {
        return Error.argument;
    }

    FILE* file;
    e$except_silent(err, io_fopen(&file, path, "w"))
    {
        return err;
    }

    usize contents_len = strlen(contents);
    e$except_silent(err, io_fwrite(file, contents, 1, contents_len))
    {
        io_fclose(&file);
        return err;
    }

    io_fclose(&file);
    return EOK;
}

char*
io__file__load(const char* path, IAllocator allc)
{
    errno = 0;
    uassert(allc != NULL);
    if (path == NULL) {
        errno = EINVAL;
        return NULL;
    }
    FILE* file;
    e$except_silent(err, io_fopen(&file, path, "r"))
    {
        return NULL;
    }

    str_s out_content = (str_s){ 0 };
    e$except_silent(err, io_fread_all(file, &out_content, allc))
    {
        if (err == Error.eof) {
            uassert(out_content.buf == NULL);
            out_content.buf = mem$malloc(allc, 1);
            if (out_content.buf) {
                out_content.buf[0] = '\0';
            }
            return out_content.buf;
        }
        if (out_content.buf) {
            mem$free(allc, out_content.buf);
        }
        return NULL;
    }
    return out_content.buf;
}

char*
io__file__readln(FILE* file, IAllocator allc)
{
    errno = 0;
    uassert(allc != NULL);
    if (file == NULL) {
        errno = EINVAL;
        return NULL;
    }

    str_s out_content = (str_s){ 0 };
    e$except_silent(err, io_fread_line(file, &out_content, allc))
    {
        if (err == Error.eof) {
            return NULL;
        }

        if (out_content.buf) {
            mem$free(allc, out_content.buf);
        }
        return NULL;
    }
    return out_content.buf;
}

const struct __module__io io = {
    // Autogenerated by CEX
    // clang-format off
    .fopen = io_fopen,
    .fileno = io_fileno,
    .isatty = io_isatty,
    .fflush = io_fflush,
    .fseek = io_fseek,
    .rewind = io_rewind,
    .ftell = io_ftell,
    .fread = io_fread,
    .fread_all = io_fread_all,
    .fread_line = io_fread_line,
    .fprintf = io_fprintf,
    .printf = io_printf,
    .fwrite = io_fwrite,
    .fclose = io_fclose,

    .file = {  // sub-module .file >>>
        .size = io__file__size,
        .writeln = io__file__writeln,
        .save = io__file__save,
        .load = io__file__load,
        .readln = io__file__readln,
    },  // sub-module .file <<<
    // clang-format on
};

/*
*                   mem.c
*/

void
_cex_allocator_memscope_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    (*allc)->scope_exit(*allc);
}

/*
*                   sbuf.c
*/
#include "cex/all.h"
#include <stdarg.h>

struct _sbuf__sprintf_ctx
{
    sbuf_head_s* head;
    char* buf;
    Exc err;
    u32 count;
    u32 length;
    char tmp[CEX_SPRINTF_MIN];
};

static inline sbuf_head_s*
sbuf__head(sbuf_c self)
{
    uassert(self != NULL);
    sbuf_head_s* head = (sbuf_head_s*)(self - sizeof(sbuf_head_s));

    uassert(head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");
    uassert(head->capacity > 0 && "zero capacity or memory corruption");
    uassert(head->length <= head->capacity && "count > capacity");
    uassert(head->header.nullterm == 0 && "nullterm != 0");

    return head;
}
static inline usize
sbuf__alloc_capacity(usize capacity)
{
    uassert(capacity < INT32_MAX && "requested capacity exceeds 2gb, maybe overflow?");

    capacity += sizeof(sbuf_head_s) + 1; // also +1 for nullterm

    if (capacity >= 512) {
        return capacity * 1.2;
    } else {
        // Round up to closest pow*2 int
        u64 p = 4;
        while (p < capacity) {
            p *= 2;
        }
        return p;
    }
}
static inline Exception
sbuf__grow_buffer(sbuf_c* self, u32 length)
{
    sbuf_head_s* head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));

    if (unlikely(head->allocator == NULL)) {
        // sbuf is static, bad luck, overflow
        return Error.overflow;
    }

    u32 new_capacity = sbuf__alloc_capacity(length);
    head = mem$realloc(head->allocator, head, new_capacity);
    if (unlikely(head == NULL)) {
        *self = NULL;
        return Error.memory;
    }

    head->capacity = new_capacity - sizeof(sbuf_head_s) - 1,
    *self = (char*)head + sizeof(sbuf_head_s);
    (*self)[head->capacity] = '\0';
    return Error.ok;
}

static sbuf_c
sbuf_create(u32 capacity, IAllocator allocator)
{
    if (unlikely(allocator == NULL)) {
        uassert(allocator != NULL);
        return NULL;
    }

    if (capacity < 512) {
        capacity = sbuf__alloc_capacity(capacity);
    }

    char* buf = mem$malloc(allocator, capacity);
    if (unlikely(buf == NULL)) {
        return NULL;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = capacity - sizeof(sbuf_head_s) - 1,
        .allocator = allocator,
    };

    sbuf_c self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (self)[0] = '\0';
    (self)[head->capacity] = '\0';

    return self;
}

static sbuf_c
sbuf_create_temp(void)
{
    uassert(
        tmem$->scope_depth(tmem$) > 0 && "trying create tmem$ allocator outside mem$scope(tmem$)"
    );
    return sbuf_create(100, tmem$);
}

static sbuf_c
sbuf_create_static(char* buf, usize buf_size)
{
    if (unlikely(buf == NULL)) {
        uassert(buf != NULL);
        return NULL;
    }
    if (unlikely(buf_size == 0)) {
        uassert(buf_size > 0);
        return NULL;
    }
    if (unlikely(buf_size <= sizeof(sbuf_head_s) + 1)) {
        uassert(buf_size > sizeof(sbuf_head_s) + 1);
        return NULL;
    }

    sbuf_head_s* head = (sbuf_head_s*)buf;
    *head = (sbuf_head_s){
        .header = { 0xf00e, .elsize = 1, .nullterm = '\0' },
        .length = 0,
        .capacity = buf_size - sizeof(sbuf_head_s) - 1,
        .allocator = NULL,
    };
    sbuf_c self = buf + sizeof(sbuf_head_s);

    // null terminate start of the string and capacity
    (self)[0] = '\0';
    (self)[head->capacity] = '\0';

    return self;
}

static Exception
sbuf_grow(sbuf_c* self, u32 new_capacity)
{
    sbuf_head_s* head = sbuf__head(*self);
    if (new_capacity <= head->capacity) {
        // capacity is enough, no need to grow
        return Error.ok;
    }
    return sbuf__grow_buffer(self, new_capacity);
}

static void
sbuf_update_len(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    uassert((*self)[head->capacity] == '\0' && "capacity null term smashed!");

    head->length = strlen(*self);
    (*self)[head->length] = '\0';
}

static void
sbuf_clear(sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    head->length = 0;
    (*self)[head->length] = '\0';
}

static u32
sbuf_len(const sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    return head->length;
}

static u32
sbuf_capacity(const sbuf_c* self)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    return head->capacity;
}

static sbuf_c
sbuf_destroy(sbuf_c* self)
{
    uassert(self != NULL);

    if (*self != NULL) {
        sbuf_head_s* head = sbuf__head(*self);

        // NOTE: null-terminate string to avoid future usage,
        // it will appear as empty string if references anywhere else
        ((char*)head)[0] = '\0';
        (*self)[0] = '\0';
        *self = NULL;

        if (head->allocator != NULL) {
            // allocator is NULL for static sbuf
            mem$free(head->allocator, head);
        }
        memset(self, 0, sizeof(*self));
    }
    return NULL;
}

static char*
sbuf__sprintf_callback(const char* buf, void* user, u32 len)
{
    struct _sbuf__sprintf_ctx* ctx = (struct _sbuf__sprintf_ctx*)user;
    sbuf_c sbuf = ((char*)ctx->head + sizeof(sbuf_head_s));

    uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");
    if (unlikely(ctx->err != EOK)) {
        return NULL;
    }
    uassert((buf != ctx->buf) || (sbuf + ctx->length + len <= sbuf + ctx->count && "out of bounds"));

    if (unlikely(ctx->length + len > ctx->count)) {
        bool buf_is_tmp = buf != ctx->buf;

        if (len > INT32_MAX || ctx->length + len > (u32)INT32_MAX) {
            ctx->err = Error.integrity;
            return NULL;
        }

        // sbuf likely changed after realloc
        e$except_silent(err, sbuf__grow_buffer(&sbuf, ctx->length + len + 1))
        {
            ctx->err = err;
            return NULL;
        }
        // re-fetch head in case of realloc
        ctx->head = (sbuf_head_s*)(sbuf - sizeof(sbuf_head_s));
        uassert(ctx->head->header.magic == 0xf00e && "not a sbuf_head_s / bad pointer");

        ctx->buf = sbuf + ctx->head->length;
        ctx->count = ctx->head->capacity;
        uassert(ctx->count >= ctx->length);
        if (!buf_is_tmp) {
            buf = ctx->buf;
        }
    }

    ctx->length += len;
    ctx->head->length += len;
    if (len > 0) {
        if (buf != ctx->buf) {
            memcpy(ctx->buf, buf, len); // copy data only if previously tmp buffer used
        }
        ctx->buf += len;
    }
    return ((ctx->count - ctx->length) >= CEX_SPRINTF_MIN) ? ctx->buf : ctx->tmp;
}

static Exception
sbuf__appendfva(sbuf_c* self, const char* format, va_list va)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    struct _sbuf__sprintf_ctx ctx = {
        .head = head,
        .err = EOK,
        .buf = *self + head->length,
        .length = head->length,
        .count = head->capacity,
    };

    cexsp__vsprintfcb(
        sbuf__sprintf_callback,
        &ctx,
        sbuf__sprintf_callback(NULL, &ctx, 0),
        format,
        va
    );

    // re-fetch self in case of realloc in sbuf__sprintf_callback
    *self = ((char*)ctx.head + sizeof(sbuf_head_s));

    // always null terminate
    (*self)[ctx.head->length] = '\0';
    (*self)[ctx.head->capacity] = '\0';

    return ctx.err;
}

static Exception
sbuf_appendf(sbuf_c* self, const char* format, ...)
{

    va_list va;
    va_start(va, format);
    Exc result = sbuf__appendfva(self, format, va);
    va_end(va);
    return result;
}

static Exception
sbuf_append(sbuf_c* self, const char* s)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    if (s == NULL) {
        return Error.argument;
    }

    u32 length = head->length;
    u32 capacity = head->capacity;
    u32 slen = strlen(s);

    uassert(*self != s && "buffer overlap");

    // Try resize
    if (length + slen > capacity - 1) {
        e$except_silent(err, sbuf__grow_buffer(self, length + slen))
        {
            return err;
        }
    }
    memcpy((*self + length), s, slen);
    length += slen;

    // always null terminate
    (*self)[length] = '\0';

    // re-fetch head in case of realloc
    head = (sbuf_head_s*)(*self - sizeof(sbuf_head_s));
    head->length = length;

    return Error.ok;
}

static bool
sbuf_isvalid(sbuf_c* self)
{
    if (self == NULL) {
        return false;
    }
    if (*self == NULL) {
        return false;
    }

    sbuf_head_s* head = (sbuf_head_s*)((char*)(*self) - sizeof(sbuf_head_s));

    if (head->header.magic != 0xf00e) {
        return false;
    }
    if (head->capacity == 0) {
        return false;
    }
    if (head->length > head->capacity) {
        return false;
    }
    if (head->header.nullterm != 0) {
        return false;
    }

    return true;
}

// static inline isize
// sbuf__index(const char* self, usize self_len, const char* c, u8 clen)
// {
//     isize result = -1;
//
//     u8 split_by_idx[UINT8_MAX] = { 0 };
//     for (u8 i = 0; i < clen; i++) {
//         split_by_idx[(u8)c[i]] = 1;
//     }
//
//     for (usize i = 0; i < self_len; i++) {
//         if (split_by_idx[(u8)self[i]]) {
//             result = i;
//             break;
//         }
//     }
//
//     return result;
// }

const struct __module__sbuf sbuf = {
    // Autogenerated by CEX
    // clang-format off
    .create = sbuf_create,
    .create_temp = sbuf_create_temp,
    .create_static = sbuf_create_static,
    .grow = sbuf_grow,
    .update_len = sbuf_update_len,
    .clear = sbuf_clear,
    .len = sbuf_len,
    .capacity = sbuf_capacity,
    .destroy = sbuf_destroy,
    .appendf = sbuf_appendf,
    .append = sbuf_append,
    .isvalid = sbuf_isvalid,
    // clang-format on
};

/*
*                   str.c
*/
#include <ctype.h>
#include <ctype.h>
#include <float.h>
#include <math.h>

static inline bool
str__isvalid(const str_s* s)
{
    return s->buf != NULL;
}

static inline isize
str__index(str_s* s, const char* c, u8 clen)
{
    isize result = -1;

    if (!str__isvalid(s)) {
        return -1;
    }

    u8 split_by_idx[UINT8_MAX] = { 0 };
    for (u8 i = 0; i < clen; i++) {
        split_by_idx[(u8)c[i]] = 1;
    }

    for (usize i = 0; i < s->len; i++) {
        if (split_by_idx[(u8)s->buf[i]]) {
            result = i;
            break;
        }
    }

    return result;
}

static str_s
str_sstr(const char* ccharptr)
{
    if (unlikely(ccharptr == NULL)) {
        return (str_s){ 0 };
    }

    return (str_s){
        .buf = (char*)ccharptr,
        .len = strlen(ccharptr),
    };
}


static str_s
str_sbuf(char* s, usize length)
{
    if (unlikely(s == NULL)) {
        return (str_s){ 0 };
    }

    return (str_s){
        .buf = s,
        .len = strnlen(s, length),
    };
}

static bool
str_eq(const char* a, const char* b)
{
    if (unlikely(a == NULL || b == NULL)) {
        // NOTE: if both are NULL - this function intentionally return false
        return false;
    }
    return strcmp(a, b) == 0;
}

bool str_eqi(const char *a, const char *b) {
    if (unlikely(a == NULL || b == NULL)) {
        // NOTE: if both are NULL - this function intentionally return false
        return false;
    }
    while (*a && *b) {
        if (tolower((u8)*a) != tolower((u8)*b)) {
            return false;
        }
        a++;
        b++;
    }
    return (*a == '\0' && *b == '\0');
}

static bool
str__slice__eq(str_s a, str_s b)
{
    if (unlikely(a.buf == NULL || b.buf == NULL)) {
        // NOTE: if both are NULL - this function intentionally return false
        return false;
    }
    if (a.len != b.len) {
        return false;
    }
    return memcmp(a.buf, b.buf, a.len) == 0;
}

static str_s
str__slice__sub(str_s s, isize start, isize end)
{
    slice$define(*s.buf) slice = { 0 };
    if (s.buf != NULL) {
        _arr$slice_get(slice, s.buf, s.len, start, end);
    }

    return (str_s){
        .buf = slice.arr,
        .len = slice.len,
    };
}

static str_s
str_sub(const char* s, isize start, isize end)
{
    str_s slice = str_sstr(s);
    return str__slice__sub(slice, start, end);
}

static Exception
str_copy(char* dest, const char* src, usize destlen)
{
    uassert(dest != src && "buffers overlap");
    if (unlikely(dest == NULL || destlen == 0)) {
        return Error.argument;
    }
    dest[0] = '\0'; // If we fail next, it still writes empty string
    if (unlikely(src == NULL)) {
        return Error.argument;
    }

    char* pend = stpncpy(dest, src, destlen);
    dest[destlen - 1] = '\0'; // always secure last byte of destlen

    if (unlikely((pend - dest) >= (isize)destlen)) {
        return Error.overflow;
    }

    return Error.ok;
}

char*
str_replace(const char* str, const char* old_sub, const char* new_sub, IAllocator allc)
{
    if (str == NULL || old_sub == NULL || new_sub == NULL || old_sub[0] == '\0') {
        return NULL;
    }
    size_t str_len = strlen(str);
    size_t old_sub_len = strlen(old_sub);
    size_t new_sub_len = strlen(new_sub);

    size_t count = 0;
    const char* pos = str;
    while ((pos = strstr(pos, old_sub)) != NULL) {
        count++;
        pos += old_sub_len;
    }
    size_t new_str_len = str_len + count * (new_sub_len - old_sub_len);
    char* new_str = (char*)mem$malloc(allc, new_str_len + 1); // +1 for the null terminator
    if (new_str == NULL) {
        return NULL; // Memory allocation failed
    }
    new_str[0] = '\0';

    char* current_pos = new_str;
    const char* start = str;
    while (count--) {
        const char* found = strstr(start, old_sub);
        size_t segment_len = found - start;
        memcpy(current_pos, start, segment_len);
        current_pos += segment_len;
        memcpy(current_pos, new_sub, new_sub_len);
        current_pos += new_sub_len;
        start = found + old_sub_len;
    }
    strcpy(current_pos, start);
    new_str[new_str_len] = '\0';
    return new_str;
}

static Exception
str__slice__copy(char* dest, str_s src, usize destlen)
{
    uassert(dest != src.buf && "buffers overlap");
    if (unlikely(dest == NULL || destlen == 0)) {
        return Error.argument;
    }
    dest[0] = '\0';
    if (unlikely(src.buf == NULL)) {
        return Error.argument;
    }
    if (src.len >= destlen) {
        return Error.overflow;
    }
    memcpy(dest, src.buf, src.len);
    dest[src.len] = '\0';
    dest[destlen - 1] = '\0';
    return Error.ok;
}

static Exception
str_vsprintf(char* dest, usize dest_len, const char* format, va_list va)
{
    if (unlikely(dest == NULL)) {
        return Error.argument;
    }
    if (unlikely(dest_len == 0)) {
        return Error.argument;
    }
    uassert(format != NULL);

    dest[dest_len - 1] = '\0'; // always null term at capacity

    int result = cexsp__vsnprintf(dest, dest_len, format, va);

    if (result < 0 || (usize)result >= dest_len) {
        dest[0] = '\0';
        return Error.overflow;
    }

    return EOK;
}

static Exception
str_sprintf(char* dest, usize dest_len, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    Exc result = str_vsprintf(dest, dest_len, format, va);
    va_end(va);
    return result;
}


static usize
str_len(const char* s)
{
    if (s == NULL) {
        return 0;
    }
    return strlen(s);
}

static char*
str_find(const char* haystack, const char* needle)
{
    if (unlikely(haystack == NULL || needle == NULL || needle[0] == '\0')) {
        return NULL;
    }
    return strstr(haystack, needle);
}

char*
str_findr(const char* haystack, const char* needle)
{
    if (unlikely(haystack == NULL || needle == NULL || needle[0] == '\0')) {
        return NULL;
    }
    usize haystack_len = strlen(haystack);
    usize needle_len = strlen(needle);
    if (unlikely(needle_len > haystack_len)) {
        return NULL;
    }
    for (const char* ptr = haystack + haystack_len - needle_len; ptr >= haystack; ptr--) {
        if (unlikely(strncmp(ptr, needle, needle_len) == 0)) {
            uassert(ptr >= haystack);
            uassert(ptr <= haystack + haystack_len);
            return (char*)ptr;
        }
    }
    return NULL;
}


static bool
str__slice__starts_with(str_s str, str_s prefix)
{
    if (unlikely(!str.buf || !prefix.buf || prefix.len == 0 || prefix.len > str.len)) {
        return false;
    }
    return memcmp(str.buf, prefix.buf, prefix.len) == 0;
}
static bool
str__slice__ends_with(str_s s, str_s suffix)
{
    if (unlikely(!s.buf || !suffix.buf || suffix.len == 0 || suffix.len > s.len)) {
        return false;
    }
    return s.len >= suffix.len && !memcmp(s.buf + s.len - suffix.len, suffix.buf, suffix.len);
}

static bool
str_starts_with(const char* str, const char* prefix)
{
    if (str == NULL || prefix == NULL || prefix[0] == '\0') {
        return false;
    }

    while (*prefix && *str == *prefix) {
        ++str, ++prefix;
    }
    return *prefix == 0;
}

static bool
str_ends_with(const char* str, const char* suffix)
{
    if (str == NULL || suffix == NULL || suffix[0] == '\0') {
        return false;
    }
    size_t slen = strlen(str);
    size_t sufflen = strlen(suffix);

    return slen >= sufflen && !memcmp(str + slen - sufflen, suffix, sufflen);
}

static str_s
str__slice__remove_prefix(str_s s, str_s prefix)
{
    if (!str__slice__starts_with(s, prefix)) {
        return s;
    }

    return (str_s){
        .buf = s.buf + prefix.len,
        .len = s.len - prefix.len,
    };
}

static str_s
str__slice__remove_suffix(str_s s, str_s suffix)
{
    if (!str__slice__ends_with(s, suffix)) {
        return s;
    }
    return (str_s){
        .buf = s.buf,
        .len = s.len - suffix.len,
    };
}

static inline void
str__strip_left(str_s* s)
{
    char* cend = s->buf + s->len;

    while (s->buf < cend) {
        switch (*s->buf) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->buf++;
                s->len--;
                break;
            default:
                return;
        }
    }
}

static inline void
str__strip_right(str_s* s)
{
    while (s->len > 0) {
        switch (s->buf[s->len - 1]) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->len--;
                break;
            default:
                return;
        }
    }
}


static str_s
str__slice__lstrip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_left(&result);
    return result;
}

static str_s
str__slice__rstrip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_right(&result);
    return result;
}

static str_s
str__slice__strip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_left(&result);
    str__strip_right(&result);
    return result;
}

static int
str__slice__cmp(str_s self, str_s other)
{
    if (unlikely(self.buf == NULL)) {
        if (other.buf == NULL) {
            return 0;
        } else {
            return -1;
        }
    }

    if (unlikely(self.len == 0)) {
        if (other.buf == NULL) {
            return 1;
        } else if (other.len == 0) {
            return 0;
        } else {
            return -other.buf[0];
        }
    }

    usize min_len = self.len < other.len ? self.len : other.len;
    int cmp = memcmp(self.buf, other.buf, min_len);

    if (unlikely(cmp == 0 && self.len != other.len)) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

static int
str__slice__cmpi(str_s self, str_s other)
{
    if (unlikely(self.buf == NULL)) {
        if (other.buf == NULL) {
            return 0;
        } else {
            return -1;
        }
    }

    if (unlikely(self.len == 0)) {
        if (other.buf == NULL) {
            return 1;
        } else if (other.len == 0) {
            return 0;
        } else {
            return -other.buf[0];
        }
    }

    usize min_len = self.len < other.len ? self.len : other.len;

    int cmp = 0;
    char* s = self.buf;
    char* o = other.buf;
    for (usize i = 0; i < min_len; i++) {
        cmp = tolower(*s) - tolower(*o);
        if (cmp != 0) {
            return cmp;
        }
        s++;
        o++;
    }

    if (cmp == 0 && self.len != other.len) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

static str_s*
str__slice__iter_split(str_s s, const char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        usize cursor;
        usize split_by_len;
        str_s str;
        usize str_len;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) <= alignof(usize), "cex_iterator_s _ctx misalign");

    if (unlikely(iterator->val == NULL)) {
        // First run handling
        if (unlikely(!str__isvalid(&s))) {
            return NULL;
        }
        ctx->split_by_len = strlen(split_by);

        if (ctx->split_by_len == 0) {
            return NULL;
        }
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        isize idx = str__index(&s, split_by, ctx->split_by_len);
        if (idx < 0) {
            idx = s.len;
        }
        ctx->cursor = idx;
        ctx->str = str.slice.sub(s, 0, idx);

        iterator->val = &ctx->str;
        iterator->idx.i = 0;
        return iterator->val;
    } else {
        uassert(iterator->val == &ctx->str);

        if (ctx->cursor >= s.len) {
            return NULL; // reached the end stops
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == s.len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            iterator->idx.i++;
            ctx->str = (str_s){ .buf = "", .len = 0 };
            return iterator->val;
        }

        // Get remaining string after prev split_by char
        str_s tok = str.slice.sub(s, ctx->cursor, 0);
        isize idx = str__index(&tok, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->str = tok;
            ctx->cursor = s.len;
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->str = str.slice.sub(tok, 0, idx);
            ctx->cursor += idx;
        }

        return iterator->val;
    }
}


static Exception
str__to_signed_num(const char* self, i64* num, i64 num_min, i64 num_max)
{
    _Static_assert(sizeof(i64) == 8, "unexpected u64 size");
    uassert(num_min < num_max);
    uassert(num_min <= 0);
    uassert(num_max > 0);
    uassert(num_max > 64);
    uassert(num_min == 0 || num_min < -64);
    uassert(num_min >= INT64_MIN + 1 && "try num_min+1, negation overflow");

    if (unlikely(self == NULL)) {
        return Error.argument;
    }

    const char* s = self;
    usize len = strlen(self);
    usize i = 0;

    for (; s[i] == ' ' && i < len; i++) {
    }

    u64 neg = 1;
    if (s[i] == '-') {
        neg = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) {
        return Error.argument;
    }

    if ((len - i) >= 2 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        i += 2;
        base = 16;
        if (unlikely(i >= len)) {
            // edge case when '0x'
            return Error.argument;
        }
    }

    u64 cutoff = (u64)(neg == 1 ? (u64)num_max : (u64)-num_min);
    u64 cutlim = cutoff % (u64)base;
    cutoff /= (u64)base;

    u64 acc = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'F') {
            c = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 10;
        } else {
            break;
        }

        if (unlikely(c >= base)) {
            return Error.argument;
        }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') {
            return Error.argument;
        }
    }

    *num = (i64)acc * neg;

    return Error.ok;
}

static Exception
str__to_unsigned_num(const char* s, u64* num, u64 num_max)
{
    _Static_assert(sizeof(u64) == 8, "unexpected u64 size");
    uassert(num_max > 0);
    uassert(num_max > 64);

    if (unlikely(s == NULL)) {
        return Error.argument;
    }

    usize len = strlen(s);
    usize i = 0;

    for (; s[i] == ' ' && i < len; i++) {
    }

    if (s[i] == '-') {
        return Error.argument;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) {
        return Error.argument;
    }

    if ((len - i) >= 2 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        i += 2;
        base = 16;

        if (unlikely(i >= len)) {
            // edge case when '0x'
            return Error.argument;
        }
    }

    u64 cutoff = num_max;
    u64 cutlim = cutoff % (u64)base;
    cutoff /= (u64)base;

    u64 acc = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'F') {
            c = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 10;
        } else {
            break;
        }

        if (unlikely(c >= base)) {
            return Error.argument;
        }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') {
            return Error.argument;
        }
    }

    *num = (i64)acc;

    return Error.ok;
}

static Exception
str__to_double(const char* self, double* num, i32 exp_min, i32 exp_max)
{
    _Static_assert(sizeof(double) == 8, "unexpected double precision");
    if (unlikely(self == NULL)) {
        return Error.argument;
    }

    const char* s = self;
    usize len = strlen(s);
    usize i = 0;
    double number = 0.0;

    for (; s[i] == ' ' && i < len; i++) {
    }

    double sign = 1;
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }

    if (unlikely(i >= len)) {
        return Error.argument;
    }

    if (unlikely(s[i] == 'n' || s[i] == 'i' || s[i] == 'N' || s[i] == 'I')) {
        if (unlikely(len - i < 3)) {
            return Error.argument;
        }
        if (s[i] == 'n' || s[i] == 'N') {
            if ((s[i + 1] == 'a' || s[i + 1] == 'A') && (s[i + 2] == 'n' || s[i + 2] == 'N')) {
                number = NAN;
                i += 3;
            }
        } else {
            // s[i] = 'i'
            if ((s[i + 1] == 'n' || s[i + 1] == 'N') && (s[i + 2] == 'f' || s[i + 2] == 'F')) {
                number = (double)INFINITY * sign;
                i += 3;
            }
            // INF 'INITY' part (optional but still valid)
            // clang-format off
            if (unlikely(len - i >= 5)) {
                if ((s[i + 0] == 'i' || s[i + 0] == 'I') &&
                    (s[i + 1] == 'n' || s[i + 1] == 'N') &&
                    (s[i + 2] == 'i' || s[i + 2] == 'I') &&
                    (s[i + 3] == 't' || s[i + 3] == 'T') &&
                    (s[i + 4] == 'y' || s[i + 4] == 'Y')) {
                    i += 5;
                }
            }
            // clang-format on
        }

        // Allow trailing spaces, but no other character allowed
        for (; i < len; i++) {
            if (s[i] != ' ') {
                return Error.argument;
            }
        }

        *num = number;
        return Error.ok;
    }

    i32 exponent = 0;
    u32 num_decimals = 0;
    u32 num_digits = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c == 'e' || c == 'E' || c == '.') {
            break;
        } else {
            return Error.argument;
        }

        number = number * 10. + c;
        num_digits++;
    }
    // Process decimal part
    if (i < len && s[i] == '.') {
        i++;

        for (; i < len; i++) {
            u8 c = (u8)s[i];

            if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
                break;
            }
            number = number * 10. + c;
            num_decimals++;
            num_digits++;
        }
        exponent -= num_decimals;
    }


    number *= sign;

    if (i < len - 1 && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        sign = 1;
        if (s[i] == '-') {
            sign = -1;
            i++;
        } else if (s[i] == '+') {
            i++;
        }

        u32 n = 0;
        for (; i < len; i++) {
            u8 c = (u8)s[i];

            if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
                break;
            }

            n = n * 10 + c;
        }

        exponent += n * sign;
    }

    if (num_digits == 0) {
        return Error.argument;
    }

    if (exponent < exp_min || exponent > exp_max) {
        return Error.overflow;
    }

    // Scale the result
    double p10 = 10.;
    i32 n = exponent;
    if (n < 0) {
        n = -n;
    }
    while (n) {
        if (n & 1) {
            if (exponent < 0) {
                number /= p10;
            } else {
                number *= p10;
            }
        }
        n >>= 1;
        p10 *= p10;
    }

    if (number == HUGE_VAL) {
        return Error.overflow;
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') {
            return Error.argument;
        }
    }

    *num = number;

    return Error.ok;
}

static Exception
str__convert__to_f32(const char* s, f32* num)
{
    uassert(num != NULL);
    f64 res = 0;
    Exc r = str__to_double(s, &res, -37, 38);
    *num = (f32)res;
    return r;
}

static Exception
str__convert__to_f64(const char* s, f64* num)
{
    uassert(num != NULL);
    return str__to_double(s, num, -307, 308);
}

static Exception
str__convert__to_i8(const char* s, i8* num)
{
    uassert(num != NULL);
    i64 res = 0;
    Exc r = str__to_signed_num(s, &res, INT8_MIN, INT8_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_i16(const char* s, i16* num)
{
    uassert(num != NULL);
    i64 res = 0;
    var r = str__to_signed_num(s, &res, INT16_MIN, INT16_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_i32(const char* s, i32* num)
{
    uassert(num != NULL);
    i64 res = 0;
    var r = str__to_signed_num(s, &res, INT32_MIN, INT32_MAX);
    *num = res;
    return r;
}


static Exception
str__convert__to_i64(const char* s, i64* num)
{
    uassert(num != NULL);
    i64 res = 0;
    // NOTE:INT64_MIN+1 because negating of INT64_MIN leads to UB!
    var r = str__to_signed_num(s, &res, INT64_MIN + 1, INT64_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_u8(const char* s, u8* num)
{
    uassert(num != NULL);
    u64 res = 0;
    Exc r = str__to_unsigned_num(s, &res, UINT8_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_u16(const char* s, u16* num)
{
    uassert(num != NULL);
    u64 res = 0;
    Exc r = str__to_unsigned_num(s, &res, UINT16_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_u32(const char* s, u32* num)
{
    uassert(num != NULL);
    u64 res = 0;
    Exc r = str__to_unsigned_num(s, &res, UINT32_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_u64(const char* s, u64* num)
{
    uassert(num != NULL);
    u64 res = 0;
    Exc r = str__to_unsigned_num(s, &res, UINT64_MAX);
    *num = res;

    return r;
}

static char*
str__fmt_callback(const char* buf, void* user, u32 len)
{
    (void)buf;
    cexsp__context* ctx = user;
    if (unlikely(ctx->has_error)) {
        return NULL;
    }

    if (unlikely(
            len >= CEX_SPRINTF_MIN && (ctx->buf == NULL || ctx->length + len >= ctx->capacity)
        )) {

        if (len > INT32_MAX || ctx->length + len > INT32_MAX) {
            ctx->has_error = true;
            return NULL;
        }

        uassert(ctx->allc != NULL);

        if (ctx->buf == NULL) {
            ctx->buf = mem$calloc(ctx->allc, 1, CEX_SPRINTF_MIN * 2);
            if (ctx->buf == NULL) {
                ctx->has_error = true;
                return NULL;
            }
            ctx->capacity = CEX_SPRINTF_MIN * 2;
        } else {
            ctx->buf = mem$realloc(ctx->allc, ctx->buf, ctx->capacity + CEX_SPRINTF_MIN * 2);
            if (ctx->buf == NULL) {
                ctx->has_error = true;
                return NULL;
            }
            ctx->capacity += CEX_SPRINTF_MIN * 2;
        }
    }
    ctx->length += len;

    // fprintf(stderr, "len: %d, total_len: %d capacity: %d\n", len, ctx->length, ctx->capacity);
    if (len > 0) {
        if (ctx->buf) {
            if (buf == ctx->tmp) {
                uassert(ctx->length <= CEX_SPRINTF_MIN);
                memcpy(ctx->buf, buf, len);
            }
        }
    }


    return (ctx->buf != NULL) ? &ctx->buf[ctx->length] : ctx->tmp;
}

static char*
str_fmt(IAllocator allc, const char* format, ...)
{
    va_list va;
    va_start(va, format);

    cexsp__context ctx = {
        .allc = allc,
    };
    // TODO: add optional flag, to check if any va is null?
    cexsp__vsprintfcb(str__fmt_callback, &ctx, ctx.tmp, format, va);
    va_end(va);

    if (unlikely(ctx.has_error)) {
        mem$free(allc, ctx.buf);
        return NULL;
    }

    if (ctx.buf) {
        uassert(ctx.length < ctx.capacity);
        ctx.buf[ctx.length] = '\0';
        ctx.buf[ctx.capacity - 1] = '\0';
    } else {
        uassert(ctx.length <= arr$len(ctx.tmp) - 1);
        ctx.buf = mem$malloc(allc, ctx.length + 1);
        if (ctx.buf == NULL) {
            return NULL;
        }
        memcpy(ctx.buf, ctx.tmp, ctx.length);
        ctx.buf[ctx.length] = '\0';
    }
    va_end(va);
    return ctx.buf;
}

static char*
str__slice__clone(str_s s, IAllocator allc)
{
    if (s.buf == NULL) {
        return NULL;
    }
    char* result = mem$malloc(allc, s.len + 1);
    if (result) {
        memcpy(result, s.buf, s.len);
        result[s.len] = '\0';
    }
    return result;
}

static char*
str_clone(const char* s, IAllocator allc)
{
    if (s == NULL) {
        return NULL;
    }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(allc, slen + 1);
    if (result) {
        memcpy(result, s, slen);
        result[slen] = '\0';
    }
    return result;
}

static char*
str_lower(const char* s, IAllocator allc)
{
    if (s == NULL) {
        return NULL;
    }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(allc, slen + 1);
    if (result) {
        for (usize i = 0; i < slen; i++) {
            result[i] = tolower(s[i]);
        }
        result[slen] = '\0';
    }
    return result;
}

static char*
str_upper(const char* s, IAllocator allc)
{
    if (s == NULL) {
        return NULL;
    }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(allc, slen + 1);
    if (result) {
        for (usize i = 0; i < slen; i++) {
            result[i] = toupper(s[i]);
        }
        result[slen] = '\0';
    }
    return result;
}

static arr$(char*) str_split(const char* s, const char* split_by, IAllocator allc)
{
    str_s src = str_sstr(s);
    if (src.buf == NULL || split_by == NULL) {
        return NULL;
    }
    arr$(char*) result = arr$new(result, allc);
    if (result == NULL) {
        return NULL;
    }

    for$iter(str_s, it, str__slice__iter_split(src, split_by, &it.iterator))
    {
        char* tok = str__slice__clone(*it.val, allc);
        arr$push(result, tok);
    }

    return result;
}

static arr$(char*) str_split_lines(const char* s, IAllocator allc)
{
    uassert(allc != NULL);
    if (s == NULL) {
        return NULL;
    }
    arr$(char*) result = arr$new(result, allc);
    if (result == NULL) {
        return NULL;
    }
    char c;
    const char* line_start = s;
    const char* cur = s;
    while ((c = *cur)) {
        switch (c) {
            case '\r':
                if (cur[1] == '\n') {
                    goto default_next;
                }
                fallthrough();
            case '\n':
            case '\v':
            case '\f': {
                str_s line = { .buf = (char*)line_start, .len = cur - line_start };
                if (line.len > 0 && line.buf[line.len - 1] == '\r') {
                    line.len--;
                }
                char* tok = str__slice__clone(line, allc);
                arr$push(result, tok);
                line_start = cur + 1;
                fallthrough();
            }
            default:
            default_next:
                cur++;
        }
    }
    return result;
}

static char*
str_join(arr$(char*) str_arr, const char* join_by, IAllocator allc)
{
    if (str_arr == NULL || join_by == NULL) {
        return NULL;
    }

    usize jlen = strlen(join_by);
    if (jlen == 0) {
        return NULL;
    }

    char* result = NULL;
    usize cursor = 0;
    for$each(s, str_arr)
    {
        if (s == NULL) {
            if (result != NULL) {
                mem$free(allc, result);
            }
            return NULL;
        }
        usize slen = strlen(s);
        if (result == NULL) {
            result = mem$malloc(allc, slen + jlen + 1);
        } else {
            result = mem$realloc(allc, result, cursor + slen + jlen + 1);
        }
        if (result == NULL) {
            return NULL; // memory error
        }

        if (cursor > 0) {
            memcpy(&result[cursor], join_by, jlen);
            cursor += jlen;
        }

        memcpy(&result[cursor], s, slen);
        cursor += slen;
    }
    if (result) {
        result[cursor] = '\0';
    }

    return result;
}

const struct __module__str str = {
    // Autogenerated by CEX
    // clang-format off
    .sstr = str_sstr,
    .sbuf = str_sbuf,
    .eq = str_eq,
    .eqi = str_eqi,
    .sub = str_sub,
    .copy = str_copy,
    .replace = str_replace,
    .vsprintf = str_vsprintf,
    .sprintf = str_sprintf,
    .len = str_len,
    .find = str_find,
    .findr = str_findr,
    .starts_with = str_starts_with,
    .ends_with = str_ends_with,
    .fmt = str_fmt,
    .clone = str_clone,
    .lower = str_lower,
    .upper = str_upper,
    .split = str_split,
    .split_lines = str_split_lines,
    .join = str_join,

    .slice = {  // sub-module .slice >>>
        .eq = str__slice__eq,
        .sub = str__slice__sub,
        .copy = str__slice__copy,
        .starts_with = str__slice__starts_with,
        .ends_with = str__slice__ends_with,
        .remove_prefix = str__slice__remove_prefix,
        .remove_suffix = str__slice__remove_suffix,
        .lstrip = str__slice__lstrip,
        .rstrip = str__slice__rstrip,
        .strip = str__slice__strip,
        .cmp = str__slice__cmp,
        .cmpi = str__slice__cmpi,
        .iter_split = str__slice__iter_split,
        .clone = str__slice__clone,
    },  // sub-module .slice <<<

    .convert = {  // sub-module .convert >>>
        .to_f32 = str__convert__to_f32,
        .to_f64 = str__convert__to_f64,
        .to_i8 = str__convert__to_i8,
        .to_i16 = str__convert__to_i16,
        .to_i32 = str__convert__to_i32,
        .to_i64 = str__convert__to_i64,
        .to_u8 = str__convert__to_u8,
        .to_u16 = str__convert__to_u16,
        .to_u32 = str__convert__to_u32,
        .to_u64 = str__convert__to_u64,
    },  // sub-module .convert <<<
    // clang-format on
};