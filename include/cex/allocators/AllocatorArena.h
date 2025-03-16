#pragma once
#include <cex/all.h>
#include <cex/allocator2.h>
#include <stddef.h>

#define CEX_ALLOCATOR_MAX_SCOPE_STACK 16

typedef struct allocator_arena_page_s allocator_arena_page_s;


typedef struct
{
    alignas(64) const Allocator2_i alloc;

    allocator_arena_page_s* last_page;
    usize used;

    u32 page_size;
    u32 scope_depth; // current scope mark, used by mem$scope
    struct
    {
        usize bytes_alloc;
        usize bytes_realloc;
        usize bytes_free;
    } stats;

    // each mark is a `used` value at alloc.scope_enter()
    usize scope_stack[CEX_ALLOCATOR_MAX_SCOPE_STACK];

} AllocatorArena_c;

_Static_assert(sizeof(AllocatorArena_c) == 256, "size!");
_Static_assert(offsetof(AllocatorArena_c, alloc) == 0, "base must be the 1st struct member");

typedef struct allocator_arena_page_s
{
    alignas(64) allocator_arena_page_s* prev_page;
    usize used_start;     // as of AllocatorArena.used field
    u32 cursor;           // current allocated size of this page
    u32 capacity;         // max capacity of this page (excluding header)
    void* last_alloc;     // last allocated pointer (viable for realloc)
    u8 __poison_area[32]; // barrier of sanitizer poison + space reserve
    char data[];          // trailing chunk of data
} allocator_arena_page_s;
_Static_assert(sizeof(allocator_arena_page_s) == 64, "size!");

typedef struct allocator_arena_rec_s
{
    u32 size;            // allocation size
    u8 ptr_padding;      // padding in bytes to next rec (also poisoned!)
    u8 ptr_alignment;    // requested pointer alignment
    u8 is_free;          // indication that address has been free()'d
    u8 ptr_offset;       // byte offset for allocated pointer for this item
} allocator_arena_rec_s;
_Static_assert(sizeof(allocator_arena_rec_s) == 8, "size!");
_Static_assert(offsetof(allocator_arena_rec_s, ptr_offset) == 7, "ptr_offset must be last");
