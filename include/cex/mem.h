#pragma once
#include "cex.h"
#include <stddef.h>
#include <threads.h>

#define CEX_ALLOCATOR_HEAP_MAGIC 0xF00dBa01
#define CEX_ALLOCATOR_TEMP_MAGIC 0xF00dBeef
#define CEX_ALLOCATOR_ARENA_MAGIC 0xFeedF001
#define CEX_ALLOCATOR_TEMP_PAGE_SIZE 1024 * 256

// clang-format off
#define IAllocator const struct Allocator2_i* 
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


void _cex_allocator_memscope_cleanup(IAllocator* allc);

#define tmem$ ((IAllocator)(&_cex__default_global__allocator_temp.alloc))
#define mem$ _cex__default_global__allocator_heap__allc
#define mem$malloc(alloc, size, alignment...)                                                      \
    ({                                                                                             \
        usize _alignment[] = { alignment };                                                        \
        (alloc)->malloc((alloc), size, (sizeof(_alignment) > 0) ? _alignment[0] : 0);              \
    })
#define mem$calloc(alloc, nmemb, size, alignment...)                                               \
    ({                                                                                             \
        usize _alignment[] = { alignment };                                                        \
        (alloc)->calloc((alloc), nmemb, size, (sizeof(_alignment) > 0) ? _alignment[0] : 0);       \
    })

#define mem$realloc(alloc, old_ptr, size, alignment...)                                            \
    ({                                                                                             \
        usize _alignment[] = { alignment };                                                        \
        (alloc)->realloc((alloc), old_ptr, size, (sizeof(_alignment) > 0) ? _alignment[0] : 0);    \
    })

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

#define mem$is_power_of2(s) (((s) != 0) && (((s) & ((s) - 1)) == 0))

#define mem$aligned_round(size, alignment)                                                         \
    (usize)((((usize)(size)) + ((usize)alignment) - 1) & ~(((usize)alignment) - 1))

#define mem$aligned_pointer(p, alignment) (void*)mem$aligned_round(p, alignment)

#define mem$platform() __SIZEOF_SIZE_T__ * 8

#define mem$addressof(typevar, value) ((typeof(typevar)[1]){ (value) })

#define mem$offsetof(var, field) ((char*)&(var)->field - (char*)(var))

#ifndef CEX_DISABLE_POISON
#define CEX_DISABLE_POISON 0
#endif

#if defined(__SANITIZE_ADDRESS__)
#define mem$asan_enabled() 1
#else
#define mem$asan_enabled() 0
#endif

#if !CEX_DISABLE_POISON
void __asan_poison_memory_region(void const volatile* addr, size_t size);
void __asan_unpoison_memory_region(void const volatile* addr, size_t size);
void* __asan_region_is_poisoned(void* beg, size_t size);

#if defined(__SANITIZE_ADDRESS__)
#define mem$asan_poison(addr, size)                                                                \
    ({                                                                                             \
        void* _addr = (addr);                                                                      \
        size_t _size = (size);                                                                     \
        if (__asan_region_is_poisoned(_addr, (size)) == NULL) {                                    \
            memset(_addr, 0xf7, _size);                                                            \
        }                                                                                          \
        __asan_poison_memory_region(_addr, _size);                                                 \
    })
#define mem$asan_unpoison(addr, size)                                                              \
    ({                                                                                             \
        void* _addr = (addr);                                                                      \
        size_t _size = (size);                                                                     \
        __asan_unpoison_memory_region(_addr, _size);                                               \
        memset(_addr, 0x00, _size);                                                                \
    })
#define mem$asan_poison_check(addr, size)                                                          \
    ({                                                                                             \
        void* _addr = addr;                                                                        \
        __asan_region_is_poisoned(_addr, (size)) == _addr;                                         \
    })

#else // #if defined(__SANITIZE_ADDRESS__)

#define mem$asan_enabled() 0
#define mem$asan_poison(addr, len) ({ memset((addr), 0xf7, (len)); })
#define mem$asan_unpoison(addr, len) ({ memset((addr), 0x00, (len)); })
#define mem$asan_poison_check(addr, len)                                                           \
    ({                                                                                             \
        usize _len = (len);                                                                        \
        u8* _addr = (void*)(addr);                                                                 \
        bool result = _addr != NULL && _len > 0;                                                   \
        for (usize i = 0; i < _len; i++) {                                                         \
            if (_addr[i] != 0xf7) {                                                                \
                result = false;                                                                    \
                break;                                                                             \
            }                                                                                      \
        }                                                                                          \
        result;                                                                                    \
    })
#endif // #if defined(__SANITIZE_ADDRESS__)

#else  // #if !CEX_DISABLE_POISON
#define mem$asan_poison(addr, len) 
#define mem$asan_unpoison(addr, len)  
#define mem$asan_poison_check(addr, len) (1) 
#endif // #if !CEX_DISABLE_POISON
