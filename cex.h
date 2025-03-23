#ifndef CEX_HEADER_H
#define CEX_HEADER_H
/**
 * @file cex/cex.h
 * @brief CEX core file
 */
#pragma once
#include <errno.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>


/*
 *                 CORE TYPES
 */
typedef int8_t i8;
typedef uint8_t u8;
typedef int16_t i16;
typedef uint16_t u16;
typedef int32_t i32;
typedef uint32_t u32;
typedef int64_t i64;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef size_t usize;
typedef ssize_t isize;

/// automatic variable type, supported by GCC/Clang
#define var __auto_type

/*
 *                 BRANCH MANAGEMENT
 * `if(unlikely(condition)) {...}` is helpful for error management, to let compiler
 *  know that the scope inside in if() is less likely to occur (or exceptionally unlikely)
 *  this allows compiler to organize code with less failed branch predictions and faster
 *  performance overall.
 *
 *  Example:
 *  char* s = malloc(128);
 *  if(unlikely(s == NULL)) {
 *      printf("Memory error\n");
 *      abort();
 *  }
 */
#define unlikely(expr) __builtin_expect(!!(expr), 0)
#define likely(expr) __builtin_expect(!!(expr), 1)
#define fallthrough() __attribute__ ((fallthrough));

/*
 *                 ERRORS
 */

/// Generic CEX error is a char*, where NULL means success(no error)
typedef const char* Exc;

/// Equivalent of Error.ok, execution success
#define EOK (Exc) NULL

/// Use `Exception` in function signatures, to force developer to check return value
/// of the function.
#define Exception Exc __attribute__((warn_unused_result))


/**
 * @brief Generic errors list, used as constant pointers, errors must be checked as
 * pointer comparison, not as strcmp() !!!
 */
extern const struct _CEX_Error_struct
{
    Exc ok; // NOTE: must be the 1st, same as EOK
    Exc memory;
    Exc io;
    Exc overflow;
    Exc argument;
    Exc integrity;
    Exc exists;
    Exc not_found;
    Exc skip;
    Exc empty;
    Exc eof;
    Exc argsparse;
    Exc runtime;
    Exc assert;
    Exc os;
    Exc timeout;
} Error;

#ifndef __cex__fprintf

// NOTE: you may try to define our own fprintf
#define __cex__fprintf(stream, prefix, filename, line, func, format, ...)                          \
    (({                                                                                            \
        fprintf(stream, "%s ( %s:%d %s() ) ", prefix, filename, line, func);                       \
        fprintf(stream, format, ##__VA_ARGS__);                                                    \
    }))

static inline bool
__cex__fprintf_dummy(void)
{
    return true; // WARN: must always return true!
}

#endif

#if __INTELLISENSE__
// VS code linter doesn't support __FILE_NAME__ builtin macro
#define __FILE_NAME__ __FILE__
#endif

#ifndef CEX_LOG_LVL
// NOTE: you may override this level to manage log$* verbosity
#define CEX_LOG_LVL 5
#endif

#if CEX_LOG_LVL > 0
#define log$error(format, ...)                                                                     \
    (__cex__fprintf(                                                                               \
        stdout,                                                                                    \
        "[ERROR]  ",                                                                               \
        __FILE_NAME__,                                                                             \
        __LINE__,                                                                                  \
        __func__,                                                                                  \
        format "\n",                                                                               \
        ##__VA_ARGS__                                                                              \
    ))
#else
#define log$error(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 1
#define log$warn(format, ...)                                                                      \
    (__cex__fprintf(                                                                               \
        stdout,                                                                                    \
        "[WARN]   ",                                                                               \
        __FILE_NAME__,                                                                             \
        __LINE__,                                                                                  \
        __func__,                                                                                  \
        format "\n",                                                                               \
        ##__VA_ARGS__                                                                              \
    ))
#else
#define log$warn(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 2
#define log$info(format, ...)                                                                      \
    (__cex__fprintf(                                                                               \
        stdout,                                                                                    \
        "[INFO]   ",                                                                               \
        __FILE_NAME__,                                                                             \
        __LINE__,                                                                                  \
        __func__,                                                                                  \
        format "\n",                                                                               \
        ##__VA_ARGS__                                                                              \
    ))
#else
#define log$info(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 3
#define log$debug(format, ...)                                                                     \
    (__cex__fprintf(                                                                               \
        stdout,                                                                                    \
        "[DEBUG]  ",                                                                               \
        __FILE_NAME__,                                                                             \
        __LINE__,                                                                                  \
        __func__,                                                                                  \
        format "\n",                                                                               \
        ##__VA_ARGS__                                                                              \
    ))
#else
#define log$debug(format, ...) __cex__fprintf_dummy()
#endif


#if CEX_LOG_LVL > 0
#define __cex__traceback(uerr, fail_func)                                                          \
    (__cex__fprintf(                                                                               \
         stdout,                                                                                   \
         "[^STCK]  ",                                                                              \
         __FILE_NAME__,                                                                            \
         __LINE__,                                                                                 \
         __func__,                                                                                 \
         "^^^^^ [%s] in function call `%s`\n",                                                     \
         uerr,                                                                                     \
         fail_func                                                                                 \
     ),                                                                                            \
     1)

/**
 * @brief Non disposable assert, returns Error.assert CEX exception when failed
 */
#define e$assert(A)                                                                                \
    ({                                                                                             \
        if (unlikely(!((A)))) {                                                                    \
            __cex__fprintf(stdout, "[ASSERT] ", __FILE_NAME__, __LINE__, __func__, "%s\n", #A);    \
            return Error.assert;                                                                   \
        }                                                                                          \
    })


#define e$assertf(A, format, ...)                                                                  \
    ({                                                                                             \
        if (unlikely(!((A)))) {                                                                    \
            __cex__fprintf(                                                                        \
                stdout,                                                                            \
                "[ASSERT] ",                                                                       \
                __FILE_NAME__,                                                                     \
                __LINE__,                                                                          \
                __func__,                                                                          \
                format "\n",                                                                       \
                ##__VA_ARGS__                                                                      \
            );                                                                                     \
            return Error.assert;                                                                   \
        }                                                                                          \
    })
#else // #if CEX_LOG_LVL > 0
#define __cex__traceback(uerr, fail_func) __cex__fprintf_dummy()
#define e$assert(A)                                                                                \
    ({                                                                                             \
        if (unlikely(!((A)))) {                                                                    \
            return Error.assert;                                                                   \
        }                                                                                          \
    })


#define e$assertf(A, format, ...)                                                                  \
    ({                                                                                             \
        if (unlikely(!((A)))) {                                                                    \
            return Error.assert;                                                                   \
        }                                                                                          \
    })
#endif // #if CEX_LOG_LVL > 0


/**
 *                 ASSERTIONS MACROS
 */


#ifdef NDEBUG
#define uassertf(cond, format, ...) ((void)(0))
#define uassert(cond) ((void)(0))
#define __cex_test_postmortem_exists() 0
#else


#ifdef __SANITIZE_ADDRESS__
// This should be linked when gcc sanitizer enabled
void __sanitizer_print_stack_trace();
#define sanitizer_stack_trace() __sanitizer_print_stack_trace()
#else
#define sanitizer_stack_trace() ((void)(0))
#endif

#ifdef CEXTEST
// this prevents spamming on stderr (i.e. cextest.h output stream in silent mode)
#define __CEX_OUT_STREAM stdout
// NOTE: __cex_test_postmortem_f - function pointer, defined at cex/test/test.h
// used for program state logging
extern void* __cex_test_postmortem_ctx;
extern void (*__cex_test_postmortem_f)(void* ctx);
#define __cex_test_postmortem_exists() (__cex_test_postmortem_f != NULL)

int __cex_test_uassert_enabled = 1;
#define uassert_disable() __cex_test_uassert_enabled = 0
#define uassert_enable() __cex_test_uassert_enabled = 1
#define uassert_is_enabled() (__cex_test_uassert_enabled)
#else
#define uassert_disable()                                                                          \
    _Static_assert(false, "uassert_disable() allowed only when compiled with -DCEXTEST")
#define uassert_enable() (void)0
#define uassert_is_enabled() true
#define __CEX_OUT_STREAM stderr
#define __cex_test_postmortem_ctx NULL
#define __cex_test_postmortem_exists() 0
#endif

#ifndef __cex__abort
#ifdef CEXTEST
#define __cex__abort() raise(SIGTRAP)
#else
#define __cex__abort() abort()
#endif
#endif

#ifndef __cex__assert
#define __cex__assert()                                                                            \
    ({                                                                                             \
        fflush(stdout);                                                                            \
        fflush(stderr);                                                                            \
        sanitizer_stack_trace();                                                                   \
        if (__cex_test_postmortem_exists()) {                                                      \
            fprintf(stderr, "=========== POSTMORTEM ===========\n");                               \
            __cex_test_postmortem_f(__cex_test_postmortem_ctx);                                    \
            fflush(stdout);                                                                        \
            fflush(stderr);                                                                        \
            fprintf(stderr, "=========== POSTMORTEM END ===========\n");                           \
        }                                                                                          \
        __cex__abort();                                                                            \
    })
#endif


/**
 * @def uassert(A)
 * @brief Custom assertion, with support of sanitizer call stack printout at failure.
 */
#define uassert(A)                                                                                 \
    ({                                                                                             \
        if (unlikely(!((A)))) {                                                                    \
            __cex__fprintf(                                                                        \
                __CEX_OUT_STREAM,                                                                  \
                "[ASSERT] ",                                                                       \
                __FILE_NAME__,                                                                     \
                __LINE__,                                                                          \
                __func__,                                                                          \
                "%s\n",                                                                            \
                #A                                                                                 \
            );                                                                                     \
            if (uassert_is_enabled()) {                                                            \
                __cex__assert();                                                                   \
            }                                                                                      \
        }                                                                                          \
    })

#define uassertf(A, format, ...)                                                                   \
    ({                                                                                             \
        if (unlikely(!((A)))) {                                                                    \
            __cex__fprintf(                                                                        \
                __CEX_OUT_STREAM,                                                                  \
                "[ASSERT] ",                                                                       \
                __FILE_NAME__,                                                                     \
                __LINE__,                                                                          \
                __func__,                                                                          \
                format "\n",                                                                       \
                ##__VA_ARGS__                                                                      \
            );                                                                                     \
            if (uassert_is_enabled()) {                                                            \
                __cex__assert();                                                                   \
            }                                                                                      \
        }                                                                                          \
    })
#endif


// cex$tmpname - internal macro for generating temporary variable names (unique__line_num)
#define cex$concat3(c, a, b) c##a##b
#define cex$concat(a, b) a##b
#define cex$varname(a, b) cex$concat3(__cex__, a, b)
#define cex$tmpname(base) cex$varname(base, __LINE__)

#define e$raise(return_uerr, error_msg, ...)                                                       \
    (log$error("[%s] " error_msg, return_uerr, ##__VA_ARGS__), (return_uerr))

// WARNING: DO NOT USE break/continue inside e$except/e$except_silent {scope!}
#define e$except(_var_name, _func)                                                                 \
    for (Exc _var_name = _func;                                                                    \
         unlikely((_var_name != EOK) && (__cex__traceback(_var_name, #_func)));                    \
         _var_name = EOK)

#define e$except_silent(_var_name, _func)                                                          \
    for (Exc _var_name = _func; unlikely(_var_name != EOK); _var_name = EOK)

#define e$except_errno(_expression)                                                                \
    if (unlikely(                                                                                  \
            ((_expression) == -1) &&                                                               \
            log$error("`%s` failed errno: %d, msg: %s", #_expression, errno, strerror(errno))      \
        ))

#define e$except_null(_expression)                                                                 \
    if (unlikely(((_expression) == NULL) && log$error("`%s` returned NULL", #_expression)))

#define e$ret(_func)                                                                               \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func))                         \
         );                                                                                        \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    return cex$tmpname(__cex_err_traceback_)

#define e$goto(_func, _label)                                                                      \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func))                         \
         );                                                                                        \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    goto _label


/*
 *                  ARRAYS / SLICES / ITERATORS INTERFACE
 */
#define slice$define(eltype)                                                                       \
    struct                                                                                         \
    {                                                                                              \
        typeof(eltype)* arr;                                                                       \
        usize len;                                                                                 \
    }

struct _cex_arr_slice
{
    isize start;
    isize end;
    isize __placeholder;
};

#define _arr$slice_get(slice, array, array_len, ...)                                               \
    {                                                                                              \
        struct _cex_arr_slice _slice = { __VA_ARGS__, .__placeholder = 0 };                        \
        isize _len = array_len;                                                                    \
        if (unlikely(_slice.start < 0))                                                            \
            _slice.start += _len;                                                                  \
        if (_slice.end == 0) /* _end=0 equivalent of python's arr[_star:] */                       \
            _slice.end = _len;                                                                     \
        else if (unlikely(_slice.end < 0))                                                         \
            _slice.end += _len;                                                                    \
        _slice.end = _slice.end < _len ? _slice.end : _len;                                        \
        _slice.start = _slice.start > 0 ? _slice.start : 0;                                        \
        /*log$debug("instart: %d, inend: %d, start: %ld, end: %ld", start, end, _start, _end); */  \
        if (_slice.start < _slice.end && array != NULL) {                                          \
            slice.arr = &((array)[_slice.start]);                                                  \
            slice.len = (usize)(_slice.end - _slice.start);                                        \
        }                                                                                          \
    }


/**
 * @brief Gets array generic slice (typed as array)
 *
 * Example:
 * var s = arr$slice(arr, .start = 1, .end = -2);
 * var s = arr$slice(arr, 1, -2);
 * var s = arr$slice(arr, .start = -2);
 * var s = arr$slice(arr, .end = 3);
 *
 * Note: arr$slice creates a temporary type, and it's preferable to use var keyword
 *
 * @param array - generic array
 * @param .start - start index, may be negative to get item from the end of array
 * @param .end - end index, 0 - means all, or may be negative to get item from the end of array
 * @return struct {eltype* arr, usize len}, or {NULL, 0} if bad slice index/not found/NULL array
 *
 * @warning returns {.arr = NULL, .len = 0} if bad indexes or array
 */
#define arr$slice(array, ...)                                                                      \
    ({                                                                                             \
        slice$define(*array) s = { .arr = NULL, .len = 0 };                                        \
        _arr$slice_get(s, array, arr$len(array), __VA_ARGS__);                                     \
        s;                                                                                         \
    })


/**
 * @brief cex_iterator_s - CEX iterator interface
 */
typedef struct
{
    // NOTE: fields order must match with for$iter union!
    void* val;
    struct
    {
        union
        {
            usize i;
            u64 ikey;
            char* skey;
            void* pkey;
        };
    } idx;
    char _ctx[48];
} cex_iterator_s;
_Static_assert(sizeof(usize) == sizeof(void*), "usize expected as sizeof ptr");
_Static_assert(alignof(usize) == alignof(void*), "alignof pointer != alignof usize");
// _Static_assert(alignof(cex_iterator_s) == alignof(void*), "alignof");
_Static_assert(sizeof(cex_iterator_s) <= 64, "cex size");

/**
 * @brief Iterates via iterator function (see usage below)
 *
 * Iterator function signature:
 * u32* array_iterator(u32 array[], u32 len, cex_iterator_s* iter)
 *
 * for$iter(u32, it, array_iterator(arr2, arr$len(arr2), &it.iterator))
 */
#define for$iter(eltype, it, iter_func)                                                            \
    union cex$tmpname(__cex_iter_)                                                                 \
    {                                                                                              \
        cex_iterator_s iterator;                                                                   \
        struct /* NOTE:  iterator above and this struct shadow each other */                       \
        {                                                                                          \
            typeof(eltype)* val;                                                                   \
            struct                                                                                 \
            {                                                                                      \
                union                                                                              \
                {                                                                                  \
                    usize i;                                                                       \
                    u64 ikey;                                                                      \
                    char* skey;                                                                    \
                    void* pkey;                                                                    \
                };                                                                                 \
            } idx;                                                                                 \
        };                                                                                         \
    };                                                                                             \
                                                                                                   \
    for (union cex$tmpname(__cex_iter_) it = { .val = (iter_func) }; it.val != NULL;               \
         it.val = (iter_func))


// Check windows
#if _WIN32 || _WIN64
#if _WIN64
#define CEX_ENV64BIT
#else
#define CEX_ENV32BIT
#endif
#endif

// Check GCC
#if __GNUC__
#if __x86_64__ || __ppc64__
#define CEX_ENV64BIT
#else
#define CEX_ENV32BIT
#endif
#endif


/*
*                   mem.h
*/
#include <stddef.h>
#include <threads.h>

#define CEX_ALLOCATOR_HEAP_MAGIC 0xF00dBa01
#define CEX_ALLOCATOR_TEMP_MAGIC 0xF00dBeef
#define CEX_ALLOCATOR_ARENA_MAGIC 0xFeedF001
#define CEX_ALLOCATOR_TEMP_PAGE_SIZE 1024 * 256

// clang-format off
#define IAllocator const struct Allocator_i*
typedef struct Allocator_i
{
    // >>> cacheline
    alignas(64) void* (*const malloc)(IAllocator self, usize size, usize alignment);
    void* (*const calloc)(IAllocator self, usize nmemb, usize size, usize alignment);
    void* (*const realloc)(IAllocator self, void* ptr, usize new_size, usize alignment);
    void* (*const free)(IAllocator self, void* ptr);
    const struct Allocator_i* (*const scope_enter)(IAllocator self);   /* Only for arenas/temp alloc! */
    void (*const scope_exit)(IAllocator self);    /* Only for arenas/temp alloc! */
    u32 (*const scope_depth)(IAllocator self);  /* Current mem$scope depth */
    struct {
        u32 magic_id;
        bool is_arena;
        bool is_temp;
    } meta;
    //<<< 64 byte cacheline
} Allocator_i;
// clang-format on
_Static_assert(alignof(Allocator_i) == 64, "size");
_Static_assert(sizeof(Allocator_i) == 64, "size");
_Static_assert(sizeof((Allocator_i){ 0 }.meta) == 8, "size");


void _cex_allocator_memscope_cleanup(IAllocator* allc);

#define tmem$ ((IAllocator)(&_cex__default_global__allocator_temp.alloc))
#define mem$ _cex__default_global__allocator_heap__allc
#define mem$malloc(alloc, size, alignment...)                                                      \
    ({                                                                                             \
        /* NOLINTBEGIN*/\
        usize _alignment[] = { alignment };                                                        \
        (alloc)->malloc((alloc), size, (sizeof(_alignment) > 0) ? _alignment[0] : 0);              \
        /* NOLINTEND*/\
    })
#define mem$calloc(alloc, nmemb, size, alignment...)                                               \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        usize _alignment[] = { alignment };                                                        \
        (alloc)->calloc((alloc), nmemb, size, (sizeof(_alignment) > 0) ? _alignment[0] : 0);       \
        /* NOLINTEND*/\
    })

#define mem$realloc(alloc, old_ptr, size, alignment...)                                            \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        usize _alignment[] = { alignment };                                                        \
        (alloc)->realloc((alloc), old_ptr, size, (sizeof(_alignment) > 0) ? _alignment[0] : 0);    \
        /* NOLINTEND*/\
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


#ifndef NDEBUG
#ifndef CEX_DISABLE_POISON
#define CEX_DISABLE_POISON 0
#endif
#else // #ifndef NDEBUG
#ifndef CEX_DISABLE_POISON
#define CEX_DISABLE_POISON 1
#endif
#endif

#if defined(__SANITIZE_ADDRESS__)
#define mem$asan_enabled() 1
#else
#define mem$asan_enabled() 0
#endif

#ifdef CEXTEST
#define _mem$asan_poison_mark(addr, c, size) memset(addr, c, size)
#define _mem$asan_poison_check_mark(addr, len)                                                     \
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

#else // #ifdef CEXTEST
#define _mem$asan_poison_mark(addr, c, size) (void)0
#define _mem$asan_poison_check_mark(addr, len) (1)
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
            _mem$asan_poison_mark(_addr, 0xf7, _size); /* Marks are only enabled in CEXTEST */     \
        }                                                                                          \
        __asan_poison_memory_region(_addr, _size);                                                 \
    })
#define mem$asan_unpoison(addr, size)                                                              \
    ({                                                                                             \
        void* _addr = (addr);                                                                      \
        size_t _size = (size);                                                                     \
        __asan_unpoison_memory_region(_addr, _size);                                               \
        _mem$asan_poison_mark(_addr, 0x00, _size); /* Marks are only enabled in CEXTEST */         \
    })
#define mem$asan_poison_check(addr, size)                                                          \
    ({                                                                                             \
        void* _addr = addr;                                                                        \
        __asan_region_is_poisoned(_addr, (size)) == _addr;                                         \
    })

#else // #if defined(__SANITIZE_ADDRESS__)

#define mem$asan_enabled() 0
#define mem$asan_poison(addr, len) _mem$asan_poison_mark((addr), 0xf7, (len))
#define mem$asan_unpoison(addr, len) _mem$asan_poison_mark((addr), 0x00, (len))

#ifdef CEXTEST
#define mem$asan_poison_check(addr, len) _mem$asan_poison_check_mark((addr), (len))
#else // #ifdef CEXTEST
#define mem$asan_poison_check(addr, len) (1)
#endif

#endif // #if defined(__SANITIZE_ADDRESS__)

#else // #if !CEX_DISABLE_POISON
#define mem$asan_poison(addr, len)
#define mem$asan_unpoison(addr, len)
#define mem$asan_poison_check(addr, len) (1)
#endif // #if !CEX_DISABLE_POISON

/*
*                   AllocatorHeap.h
*/

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

/*
*                   AllocatorArena.h
*/
#include <stddef.h>

#define CEX_ALLOCATOR_MAX_SCOPE_STACK 16

typedef struct allocator_arena_page_s allocator_arena_page_s;


typedef struct
{
    alignas(64) const Allocator_i alloc;

    allocator_arena_page_s* last_page;
    usize used;

    u32 page_size;
    u32 scope_depth; // current scope mark, used by mem$scope
    struct
    {
        usize bytes_alloc;
        usize bytes_realloc;
        usize bytes_free;
        u32 pages_created;
        u32 pages_free;
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

extern thread_local AllocatorArena_c _cex__default_global__allocator_temp;

struct __class__AllocatorArena
{
    // Autogenerated by CEX
    // clang-format off

const Allocator_i* (*create)(usize page_size);
void       (*destroy)(IAllocator self);
bool       (*sanitize)(IAllocator allc);
    // clang-format on
};
extern const struct __class__AllocatorArena AllocatorArena; // CEX Autogen

/*
*                   ds.h
*/
#include <stddef.h>
#include <string.h>

// this is a simple string arena allocator, initialize with e.g. 'cexds_string_arena my_arena={0}'.
typedef struct cexds_string_arena cexds_string_arena;
extern char* cexds_stralloc(cexds_string_arena* a, char* str);
extern void cexds_strreset(cexds_string_arena* a);

///////////////
//
// Everything below here is implementation details
//


// clang-format off
struct cexds_hm_new_kwargs_s;
struct cexds_arr_new_kwargs_s;
struct cexds_hash_index;
enum _CexDsKeyType_e
{
    _CexDsKeyType__generic,
    _CexDsKeyType__charptr,
    _CexDsKeyType__charbuf,
    _CexDsKeyType__cexstr,
};
extern void* cexds_arrgrowf(void* a, size_t elemsize, size_t addlen, size_t min_cap, IAllocator allc);
extern void cexds_arrfreef(void* a);
extern bool cexds_arr_integrity(const void* arr, size_t magic_num);
extern void cexds_hmfree_func(void* p, size_t elemsize);
extern void cexds_hmclear_func(struct cexds_hash_index* t, struct cexds_hash_index* old_table, size_t cexds_hash_seed);
extern void* cexds_hminit(size_t elemsize, IAllocator allc, enum _CexDsKeyType_e key_type, struct cexds_hm_new_kwargs_s* kwargs);
extern void* cexds_hmget_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset);
extern void* cexds_hmput_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset, void* full_elem, void* result);
extern bool cexds_hmdel_key(void* a, size_t elemsize, void* key, size_t keysize, size_t keyoffset);
// clang-format on

#define CEXDS_ARR_MAGIC 0xC001DAAD
#define CEXDS_HM_MAGIC 0xF001C001
#define CEXDS_HDR_PAD 64
_Static_assert(mem$is_power_of2(CEXDS_HDR_PAD), "expected pow of 2");


// cexds array alignment
// v malloc'd pointer              v-element 1
// |..... <cexds_array_header>|====!====!====
//    ^ padding      ^cap^len ^         ^-element 2
//                            ^-- arr$ user space pointer (element 0)
//
typedef struct
{
    alignas(64) void* hash_table;
    IAllocator allocator;
    u32 allocator_scope_depth;
    u32 magic_num;
    enum _CexDsKeyType_e hm_key_type;
    size_t hm_seed;
    size_t capacity;
    size_t length; // This MUST BE LAST before __poison_area
    u8 __poison_area[sizeof(size_t)];
} cexds_array_header;
_Static_assert(alignof(cexds_array_header) == 64, "align");
_Static_assert(alignof(cexds_array_header) == CEXDS_HDR_PAD, "size too high");
_Static_assert(sizeof(cexds_array_header) % alignof(size_t) == 0, "align size");
_Static_assert(sizeof(cexds_array_header) == 64, "size");

#define cexds_header(t) ((cexds_array_header*)(((char*)(t)) - sizeof(cexds_array_header)))

#define arr$(T) T*

struct cexds_arr_new_kwargs_s
{
    usize capacity;
};
#define arr$new(a, allocator, kwargs...)                                                           \
    ({                                                                                             \
        _Static_assert(_Alignof(typeof(*a)) <= 64, "array item alignment too high");               \
        uassert(allocator != NULL);                                                                \
        struct cexds_arr_new_kwargs_s _kwargs = { kwargs };                                        \
        (a) = (typeof(*a)*)cexds_arrgrowf(NULL, sizeof(*a), _kwargs.capacity, 0, allocator);       \
    })

#define arr$free(a) (cexds_arr_integrity(a, CEXDS_ARR_MAGIC), cexds_arrfreef((a)), (a) = NULL)

#define arr$setcap(a, n) (cexds_arr_integrity(a, CEXDS_ARR_MAGIC), arr$grow(a, 0, n))
#define arr$clear(a) (cexds_arr_integrity(a, CEXDS_ARR_MAGIC), cexds_header(a)->length = 0)
#define arr$cap(a) ((a) ? (cexds_header(a)->capacity) : 0)

#define arr$del(a, i)                                                                              \
    ({                                                                                             \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
        uassert((usize)i < cexds_header(a)->length && "out of bounds");                            \
        memmove(&(a)[i], &(a)[(i) + 1], sizeof *(a) * (cexds_header(a)->length - 1 - (i)));        \
        cexds_header(a)->length--;                                                                 \
    })
#define arr$delswap(a, i)                                                                          \
    ({                                                                                             \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
        uassert((usize)i < cexds_header(a)->length && "out of bounds");                            \
        (a)[i] = arr$last(a);                                                                      \
        cexds_header(a)->length -= 1;                                                              \
    })

#define arr$last(a)                                                                                \
    ({                                                                                             \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
        uassert(cexds_header(a)->length > 0 && "empty array");                                     \
        (a)[cexds_header(a)->length - 1];                                                          \
    })
#define arr$at(a, i)                                                                               \
    ({                                                                                             \
        cexds_arr_integrity(a, 0); /* may work also on hm$ */                                      \
        uassert((usize)i < cexds_header(a)->length && "out of bounds");                            \
        (a)[i];                                                                                    \
    })

#define arr$pop(a)                                                                                 \
    ({                                                                                             \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
        cexds_header(a)->length--;                                                                 \
        (a)[cexds_header(a)->length];                                                              \
    })

#define arr$push(a, value...)                                                                      \
    ({                                                                                             \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$push memory error");                                             \
            abort();                                                                               \
        }                                                                                          \
        (a)[cexds_header(a)->length++] = (value);                                                  \
    })

#define arr$pushm(a, items...)                                                                     \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                            \
        typeof(*a) _args[] = { items };                                                            \
        _Static_assert(sizeof(_args) > 0, "You must pass at least one item");                      \
        arr$pusha(a, _args, arr$len(_args));                                                       \
        /* NOLINTEND */                                                                            \
    })

#define arr$pusha(a, array, array_len...)                                                          \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        cexds_arr_integrity(a, CEXDS_ARR_MAGIC);                                                   \
        uassert(array != NULL);                                                                    \
        usize _arr_len_va[] = { array_len };                                                       \
        (void)_arr_len_va;                                                                         \
        usize arr_len = (sizeof(_arr_len_va) > 0) ? _arr_len_va[0] : arr$len(array);               \
        uassert(arr_len < PTRDIFF_MAX && "negative length or overflow");                           \
        if (unlikely(!arr$grow_check(a, arr_len))) {                                               \
            uassert(false && "arr$pusha memory error");                                            \
            abort();                                                                               \
        }                                                                                          \
        /* NOLINTEND */                                                                            \
        for (usize i = 0; i < arr_len; i++) {                                                      \
            (a)[cexds_header(a)->length++] = ((array)[i]);                                         \
        }                                                                                          \
    })


#define arr$ins(a, i, value...)                                                                    \
    do {                                                                                           \
        if (unlikely(!arr$grow_check(a, 1))) {                                                     \
            uassert(false && "arr$ins memory error");                                              \
            abort();                                                                               \
        }                                                                                          \
        cexds_header(a)->length++;                                                                 \
        uassert((usize)i < cexds_header(a)->length && "i out of bounds");                          \
        memmove(&(a)[(i) + 1], &(a)[i], sizeof(*(a)) * (cexds_header(a)->length - 1 - (i)));       \
        (a)[i] = (value);                                                                          \
    } while (0)

#define arr$grow_check(a, add_extra)                                                               \
    ((cexds_arr_integrity(a, CEXDS_ARR_MAGIC) &&                                                   \
      cexds_header(a)->length + (add_extra) > cexds_header(a)->capacity)                           \
         ? (arr$grow(a, add_extra, 0), a != NULL)                                                  \
         : true)

#define arr$grow(a, add_len, min_cap)                                                              \
    ((a) = cexds_arrgrowf((a), sizeof *(a), (add_len), (min_cap), NULL))


// NOLINT
#define arr$len(arr)                                                                               \
    ({                                                                                             \
        _Pragma("GCC diagnostic push");                                                            \
        /* NOTE: temporary disable syntax error to support both static array length and arr$(T) */ \
        _Pragma("GCC diagnostic ignored \"-Wsizeof-pointer-div\"");                                \
        /* NOLINTBEGIN */                                                                          \
        __builtin_types_compatible_p(typeof(arr), typeof(&(arr)[0]))   /* check if array or ptr */ \
            ? (cexds_arr_integrity(arr, 0), cexds_header(arr)->length) /* some pointer or arr$ */  \
            : (sizeof(arr) / sizeof((arr)[0])                          /* static array[] */        \
              );                                                                                   \
        /* NOLINTEND */                                                                            \
        _Pragma("GCC diagnostic pop");                                                             \
    })
// NOLINT

#define for$each(v, array, array_len...)                                                            \
        /* NOLINTBEGIN*/\
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    typeof((array)[0])* cex$tmpname(arr_arrp) = &(array)[0];                                       \
    usize cex$tmpname(arr_index) = 0;                                                              \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
        /* NOLINTEND */                                                                            \
    for (typeof((array)[0]) v = { 0 };                                                             \
         (cex$tmpname(arr_index) < cex$tmpname(arr_length) &&                                      \
          ((v) = cex$tmpname(arr_arrp)[cex$tmpname(arr_index)], 1));                               \
         cex$tmpname(arr_index)++)

#define for$eachp(v, array, array_len...)                                                           \
        /* NOLINTBEGIN*/\
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    typeof((array)[0])* cex$tmpname(arr_arrp) = &(array)[0];                                       \
    usize cex$tmpname(arr_index) = 0;                                                              \
        /* NOLINTEND */                                                                            \
    for (typeof((array)[0])* v = cex$tmpname(arr_arrp);                                            \
         cex$tmpname(arr_index) < cex$tmpname(arr_length);                                         \
         cex$tmpname(arr_index)++, v++)

/*
 *                 HASH MAP
 */

#define hm$(_KeyType, _ValType)                                                                    \
    struct                                                                                         \
    {                                                                                              \
        _KeyType key;                                                                              \
        _ValType value;                                                                            \
    }*

#define hm$s(_StructType) _StructType*

struct cexds_hm_new_kwargs_s
{
    usize capacity;
    size_t seed;
    bool copy_keys;
};


#define hm$new(t, allocator, kwargs...)                                                            \
    ({                                                                                             \
        _Static_assert(_Alignof(typeof(*t)) <= 64, "hashmap record alignment too high");           \
        uassert(allocator != NULL);                                                                \
        enum _CexDsKeyType_e _key_type = _Generic(                                                 \
            &((t)->key),                                                                           \
            str_s *: _CexDsKeyType__cexstr,                                                        \
            char(**): _CexDsKeyType__charptr,                                                      \
            const char(**): _CexDsKeyType__charptr,                                                \
            char(*)[]: _CexDsKeyType__charbuf,                                                     \
            const char(*)[]: _CexDsKeyType__charbuf,                                               \
            default: _CexDsKeyType__generic                                                        \
        );                                                                                         \
        struct cexds_hm_new_kwargs_s _kwargs = { kwargs };                                         \
        (t) = (typeof(*t)*)cexds_hminit(sizeof(*t), (allocator), _key_type, &_kwargs);             \
    })


#define hm$set(t, k, v...)                                                                         \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = cexds_hmput_key(                                                                     \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key),      /* offset of key in hm struct */                       \
            NULL,                           /* no full element set */                              \
            &result                         /* NULL on memory error */                             \
        );                                                                                         \
        if (result)                                                                                \
            result->value = (v);                                                                   \
        result;                                                                                    \
    })

#define hm$setp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        (t) = cexds_hmput_key(                                                                     \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key),      /* offset of key in hm struct */                       \
            NULL,                           /* no full element set */                              \
            &result                         /* NULL on memory error */                             \
        );                                                                                         \
        (result ? &result->value : NULL);                                                          \
    })

#define hm$sets(t, v...)                                                                           \
    ({                                                                                             \
        typeof(t) result = NULL;                                                                   \
        typeof(*t) _val = (v);                                                                     \
        (t) = cexds_hmput_key(                                                                     \
            (t),                                                                                   \
            sizeof(*t),                /* size of hashmap item */                                  \
            &_val.key,                 /* temp on stack pointer to (k) value */                    \
            sizeof((t)->key),          /* size of key */                                           \
            offsetof(typeof(*t), key), /* offset of key in hm struct */                            \
            &(_val),                   /* full element write */                                    \
            &result                    /* NULL on memory error */                                  \
        );                                                                                         \
        result;                                                                                    \
    })

#define hm$get(t, k, def...)                                                                       \
    ({                                                                                             \
        typeof(t) result = cexds_hmget_key(                                                        \
            (t),                                                                                   \
            sizeof(*t),                     /* size of hashmap item */                             \
            ((typeof((t)->key)[1]){ (k) }), /* temp on stack pointer to (k) value */               \
            sizeof((t)->key),               /* size of key */                                      \
            offsetof(typeof(*t), key)       /* offset of key in hm struct */                       \
        );                                                                                         \
        typeof((t)->value) _def[1] = { def }; /* default value, always 0 if def... is empty! */    \
        result ? result->value : _def[0];                                                          \
    })

#define hm$getp(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = cexds_hmget_key(                                                        \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
        result ? &result->value : NULL;                                                            \
    })

#define hm$gets(t, k)                                                                              \
    ({                                                                                             \
        typeof(t) result = cexds_hmget_key(                                                        \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
        result;                                                                                    \
    })

#define hm$clear(t)                                                                                \
    ({                                                                                             \
        cexds_arr_integrity(t, CEXDS_HM_MAGIC);                                                    \
        cexds_hmclear_func(cexds_header((t))->hash_table, NULL, cexds_header(t)->hm_seed);         \
        cexds_header(t)->length = 0;                                                               \
        true;                                                                                      \
    })

#define hm$del(t, k)                                                                               \
    ({                                                                                             \
        cexds_hmdel_key(                                                                           \
            (t),                                                                                   \
            sizeof *(t),                                                                           \
            ((typeof((t)->key)[1]){ (k) }),                                                        \
            sizeof(t)->key,                                                                        \
            offsetof(typeof(*t), key)                                                              \
        );                                                                                         \
    })


#define hm$free(t) (cexds_hmfree_func((t), sizeof *(t)), (t) = NULL)

#define hm$len(t)                                                                                  \
    ({                                                                                             \
        cexds_arr_integrity(t, CEXDS_HM_MAGIC);                                                    \
        cexds_header((t))->length;                                                                 \
    })

typedef struct cexds_string_block
{
    struct cexds_string_block* next;
    char storage[8];
} cexds_string_block;

struct cexds_string_arena
{
    cexds_string_block* storage;
    size_t remaining;
    unsigned char block;
    unsigned char mode; // this isn't used by the string arena itself
};

enum
{
    CEXDS_SH_NONE,
    CEXDS_SH_DEFAULT,
    CEXDS_SH_STRDUP,
    CEXDS_SH_ARENA
};

#define cexds_arrgrowf cexds_arrgrowf
#define cexds_shmode_func_wrapper(t, e, m) cexds_shmode_func(e, m)

/*
*                   str.h
*/
/**
 * @file
 * @brief
 */

#include <stdalign.h>
#include <stdint.h>

/// Represents char* slice (string view) + may not be null-term at len!
typedef struct
{
    usize len;
    char* buf;
} str_s;

_Static_assert(alignof(str_s) == alignof(usize), "align");
_Static_assert(sizeof(str_s) == sizeof(usize) * 2, "size");


/**
 * @brief creates str_s, instance from string literals/constants: str$s("my string")
 *
 * Uses compile time string length calculation, only literals
 *
 */
#define str$s(string)                                                                               \
    (str_s){ .buf = /* WARNING: only literals!!!*/ "" string, .len = sizeof((string)) - 1 }


/**
 * @brief creates slice of str_s instance
 */
#define str$sslice(str_self, ...)                                                                   \
    ({                                                                                             \
        slice$define(*(str_self.buf)) __slice = { .arr = NULL, .len = 0 };                         \
        _arr$slice_get(__slice, str_self.buf, str_self.len, __VA_ARGS__);                          \
        __slice;                                                                                   \
    })


struct __module__str
{
    // Autogenerated by CEX
    // clang-format off

char*           (*clone)(const char* s, IAllocator allc);
Exception       (*copy)(char* dest, const char* src, usize destlen);
bool            (*ends_with)(const char* str, const char* suffix);
bool            (*eq)(const char* a, const char* b);
bool            (*eqi)(const char *a, const char *b);
char*           (*find)(const char* haystack, const char* needle);
char*           (*findr)(const char* haystack, const char* needle);
char*           (*fmt)(IAllocator allc, const char* format, ...);
char*           (*join)(arr$(char*) str_arr, const char* join_by, IAllocator allc);
usize           (*len)(const char* s);
char*           (*lower)(const char* s, IAllocator allc);
char*           (*replace)(const char* str, const char* old_sub, const char* new_sub, IAllocator allc);
str_s           (*sbuf)(char* s, usize length);
arr$(char*)     (*split)(const char* s, const char* split_by, IAllocator allc);
arr$(char*)     (*split_lines)(const char* s, IAllocator allc);
Exception       (*sprintf)(char* dest, usize dest_len, const char* format, ...);
str_s           (*sstr)(const char* ccharptr);
bool            (*starts_with)(const char* str, const char* prefix);
str_s           (*sub)(const char* s, isize start, isize end);
char*           (*upper)(const char* s, IAllocator allc);
Exception       (*vsprintf)(char* dest, usize dest_len, const char* format, va_list va);

struct {  // sub-module .slice >>>
    char*           (*clone)(str_s s, IAllocator allc);
    int             (*cmp)(str_s self, str_s other);
    int             (*cmpi)(str_s self, str_s other);
    Exception       (*copy)(char* dest, str_s src, usize destlen);
    bool            (*ends_with)(str_s s, str_s suffix);
    bool            (*eq)(str_s a, str_s b);
    str_s*          (*iter_split)(str_s s, const char* split_by, cex_iterator_s* iterator);
    str_s           (*lstrip)(str_s s);
    str_s           (*remove_prefix)(str_s s, str_s prefix);
    str_s           (*remove_suffix)(str_s s, str_s suffix);
    str_s           (*rstrip)(str_s s);
    bool            (*starts_with)(str_s str, str_s prefix);
    str_s           (*strip)(str_s s);
    str_s           (*sub)(str_s s, isize start, isize end);
} slice;  // sub-module .slice <<<

struct {  // sub-module .convert >>>
    Exception       (*to_f32)(const char* s, f32* num);
    Exception       (*to_f64)(const char* s, f64* num);
    Exception       (*to_i16)(const char* s, i16* num);
    Exception       (*to_i32)(const char* s, i32* num);
    Exception       (*to_i64)(const char* s, i64* num);
    Exception       (*to_i8)(const char* s, i8* num);
    Exception       (*to_u16)(const char* s, u16* num);
    Exception       (*to_u32)(const char* s, u32* num);
    Exception       (*to_u64)(const char* s, u64* num);
    Exception       (*to_u8)(const char* s, u8* num);
} convert;  // sub-module .convert <<<
    // clang-format on
};
extern const struct __module__str str; // CEX Autogen

/*
*                   sbuf.h
*/

typedef char* sbuf_c;

typedef struct
{
    struct
    {
        u32 magic : 16;   // used for sanity checks
        u32 elsize : 8;   // maybe multibyte strings in the future?
        u32 nullterm : 8; // always zero to prevent usage of direct buffer
    } header;
    u32 length;
    u32 capacity;
    const Allocator_i* allocator;
} __attribute__((packed)) sbuf_head_s;

_Static_assert(alignof(sbuf_head_s) == 1, "align");
_Static_assert(alignof(sbuf_head_s) == alignof(char), "align");


struct __module__sbuf
{
    // Autogenerated by CEX
    // clang-format off

Exception       (*append)(sbuf_c* self, const char* s);
Exception       (*appendf)(sbuf_c* self, const char* format, ...);
u32             (*capacity)(const sbuf_c* self);
void            (*clear)(sbuf_c* self);
sbuf_c          (*create)(u32 capacity, IAllocator allocator);
sbuf_c          (*create_static)(char* buf, usize buf_size);
sbuf_c          (*create_temp)(void);
sbuf_c          (*destroy)(sbuf_c* self);
Exception       (*grow)(sbuf_c* self, u32 new_capacity);
bool            (*isvalid)(sbuf_c* self);
u32             (*len)(const sbuf_c* self);
void            (*update_len)(sbuf_c* self);
    // clang-format on
};
extern const struct __module__sbuf sbuf; // CEX Autogen

/*
*                   io.h
*/
#include <stdio.h>

struct __module__io
{
    // Autogenerated by CEX
    // clang-format off

void            (*fclose)(FILE** file);
Exception       (*fflush)(FILE* file);
int             (*fileno)(FILE* file);
Exception       (*fopen)(FILE** file, const char* filename, const char* mode);
Exception       (*fprintf)(FILE* stream, const char* format, ...);
Exception       (*fread)(FILE* file, void* obj_buffer, usize obj_el_size, usize* obj_count);
Exception       (*fread_all)(FILE* file, str_s* s, IAllocator allc);
Exception       (*fread_line)(FILE* file, str_s* s, IAllocator allc);
Exception       (*fseek)(FILE* file, long offset, int whence);
Exception       (*ftell)(FILE* file, usize* size);
Exception       (*fwrite)(FILE* file, const void* obj_buffer, usize obj_el_size, usize obj_count);
bool            (*isatty)(FILE* file);
void            (*printf)(const char* format, ...);
void            (*rewind)(FILE* file);

struct {  // sub-module .file >>>
    char*           (*load)(const char* path, IAllocator allc);
    char*           (*readln)(FILE* file, IAllocator allc);
    Exception       (*save)(const char* path, const char* contents);
    usize           (*size)(FILE* file);
    Exception       (*writeln)(FILE* file, char* line);
} file;  // sub-module .file <<<
    // clang-format on
};
extern const struct __module__io io; // CEX Autogen

/*
*                   argparse.h
*/
/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
#ifndef ARGPARSE_H
#define ARGPARSE_H

#include <stdint.h>

struct argparse_c;
struct argparse_opt_s;

typedef Exception argparse_callback_f(struct argparse_c* self, const struct argparse_opt_s* option);


enum argparse_option_flags
{
    ARGPARSE_OPT_NONEG = 1, /* disable negation */
};
/**
 *  argparse option
 *
 *  `type`:
 *    holds the type of the option
 *
 *  `short_name`:
 *    the character to use as a short option name, '\0' if none.
 *
 *  `long_name`:
 *    the long option name, without the leading dash, NULL if none.
 *
 *  `value`:
 *    stores pointer to the value to be filled.
 *
 *  `help`:
 *    the short help message associated to what the option does.
 *    Must never be NULL (except for ARGPARSE_OPT_END).
 *
 *  `required`:
 *    if 'true' option presence is mandatory (default: false)
 *
 *
 *  `callback`:
 *    function is called when corresponding argument is parsed.
 *
 *  `data`:
 *    associated data. Callbacks can use it like they want.
 *
 *  `flags`:
 *    option flags.
 *
 *  `is_present`:
 *    true if option present in args
 */
typedef struct argparse_opt_s
{
    u8 type;
    const char short_name;
    const char* long_name;
    void* value;
    const char* help;
    bool required;
    argparse_callback_f* callback;
    intptr_t data;
    int flags;
    bool is_present; // also setting in in argparse$opt* macro, allows optional parameter sugar
} argparse_opt_s;

/**
 * argpparse
 */
typedef struct argparse_c
{
    // user supplied options
    argparse_opt_s* options;
    u32 options_len;

    const char* usage;        // usage text (can be multiline), each line prepended by program_name
    const char* description;  // a description after usage
    const char* epilog;       // a description at the end
    const char* program_name; // program name in usage (by default: argv[0])

    struct
    {
        u32 stop_at_non_option : 1;
        u32 ignore_unknown_args : 1;
    } flags;
    //
    //
    // internal context
    struct
    {
        int argc;
        char** argv;
        char** out;
        int cpidx;
        const char* optvalue; // current option value
        bool has_argument;
    } _ctx;
} argparse_c;


// built-in option macros
// clang-format off
#define argparse$opt_bool(...)    { 2 /*ARGPARSE_OPT_BOOLEAN*/, __VA_ARGS__, .is_present=0}
#define argparse$opt_bit(...)     { 3 /*ARGPARSE_OPT_BIT*/, __VA_ARGS__, .is_present=0 }
#define argparse$opt_i64(...)     { 4 /*ARGPARSE_OPT_INTEGER*/, __VA_ARGS__, .is_present=0 }
#define argparse$opt_f32(...)     { 5 /*ARGPARSE_OPT_FLOAT*/, __VA_ARGS__, .is_present=0 }
#define argparse$opt_str(...)     { 6 /*ARGPARSE_OPT_STRING*/, __VA_ARGS__, .is_present=0 }
#define argparse$opt_group(h)     { 1 /*ARGPARSE_OPT_GROUP*/, 0, NULL, NULL, h, false, NULL, 0, 0, .is_present=0 }
#define argparse$opt_help()       argparse$opt_bool('h', "help", NULL,                           \
                                                    "show this help message and exit", false,    \
                                                    NULL, 0, ARGPARSE_OPT_NONEG)
// clang-format on

struct __module__argparse
{
    // Autogenerated by CEX
    // clang-format off

u32             (*argc)(argparse_c* self);
char**          (*argv)(argparse_c* self);
Exception       (*parse)(argparse_c* self, int argc, char** argv);
void            (*usage)(argparse_c* self);
    // clang-format on
};
extern const struct __module__argparse argparse; // CEX Autogen
#endif

/*
*                   os/os.h
*/


typedef struct os_cmd_flags_s
{
    u32 combine_stdouterr : 1; // if 1 - combines output from stderr to stdout
    u32 no_inherit_env : 1;    // if 1 - disables using existing env variable of parent process
    u32 no_search_path : 1;    // if 1 - disables propagating PATH= env to command line
    u32 no_window : 1;         // if 1 - disables creation of new window if OS supported
} os_cmd_flags_s;
_Static_assert(sizeof(os_cmd_flags_s) == sizeof(u32), "size?");

typedef struct os_cmd_c
{
    struct subprocess_s _subpr;
    os_cmd_flags_s _flags;
    bool _is_subprocess;
} os_cmd_c;

typedef struct os_fs_filetype_s {
    Exc result;
    usize is_valid: 1;
    usize is_directory: 1;
    usize is_symlink: 1;
    usize is_file: 1;
    usize is_other: 1;
} os_fs_filetype_s;
_Static_assert(sizeof(os_fs_filetype_s) == sizeof(usize)*2, "size?");

typedef Exception os_fs_dir_walk_f(const char* path, os_fs_filetype_s ftype, void* user_ctx);

#define os$cmd(args...)                                                                            \
    ({                                                                                             \
        const char* _args[] = { args, NULL };                                                      \
        usize _args_len = arr$len(_args);                                                          \
        os_cmd_c _cmd = { 0 };                                                                     \
        Exc result = EOK;                                                                          \
        e$except(err, os.cmd.run(_args, _args_len, &_cmd))                                         \
        {                                                                                          \
            result = err;                                                                          \
        }                                                                                          \
        if (result == EOK) {                                                                       \
            e$except(err, os.cmd.join(&_cmd, 0, NULL))                                             \
            {                                                                                      \
                result = err;                                                                      \
            }                                                                                      \
        }                                                                                          \
        result;                                                                                    \
    })

struct __module__os
{
    // Autogenerated by CEX
    // clang-format off

void            (*sleep)(u32 period_millisec);

struct {  // sub-module .fs >>>
    arr$(char*)     (*dir_list)(const char* path, bool is_recursive, IAllocator allc);
    Exception       (*dir_walk)(const char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx);
    os_fs_filetype_s (*file_type)(const char* path);
    Exception       (*getcwd)(sbuf_c* out);
    Exception       (*mkdir)(const char* path);
    Exception       (*remove)(const char* path);
    Exception       (*rename)(const char* old_path, const char* new_path);
} fs;  // sub-module .fs <<<

struct {  // sub-module .env >>>
    const char*     (*get)(const char* name, const char* deflt);
    void            (*set)(const char* name, const char* value, bool overwrite);
    void            (*unset)(const char* name);
} env;  // sub-module .env <<<

struct {  // sub-module .path >>>
    char*           (*basename)(const char* path, IAllocator allc);
    char*           (*dirname)(const char* path, IAllocator allc);
    Exception       (*exists)(const char* file_path);
    char*           (*join)(arr$(char*) parts, IAllocator allc);
    str_s           (*split)(const char* path, bool return_dir);
} path;  // sub-module .path <<<

struct {  // sub-module .cmd >>>
    Exception       (*create)(os_cmd_c* self, arr$(char*) args, arr$(char*) env, os_cmd_flags_s* flags);
    bool            (*is_alive)(os_cmd_c* self);
    Exception       (*join)(os_cmd_c* self, u32 timeout_sec, i32* out_ret_code);
    Exception       (*kill)(os_cmd_c* self);
    char*           (*read_all)(os_cmd_c* self, IAllocator allc);
    char*           (*read_line)(os_cmd_c* self, IAllocator allc);
    Exception       (*run)(const char** args, usize args_len, os_cmd_c* out_cmd);
    FILE*           (*stderr)(os_cmd_c* self);
    FILE*           (*stdin)(os_cmd_c* self);
    FILE*           (*stdout)(os_cmd_c* self);
    Exception       (*write_line)(os_cmd_c* self, char* line);
} cmd;  // sub-module .cmd <<<
    // clang-format on
};
extern const struct __module__os os; // CEX Autogen

/*
*                   CEX IMPLEMENTATION 
*/


#ifdef CEX_IMPLEMENTATION

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
*                   mem.c
*/

void
_cex_allocator_memscope_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    (*allc)->scope_exit(*allc);
}

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
*                   argparse.c
*/
/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */
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
*                   os/os.c
*/
#include "cex/cex_base.h"

#ifdef _WIN32
#define os$PATH_SEP '\\'
#else
#define os$PATH_SEP '/'
#endif

#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <linux/limits.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

static void
os_sleep(u32 period_millisec)
{
#ifdef _WIN32
    Sleep(period_millisec);
#else
    usleep(period_millisec * 1000);
#endif
}

static Exception
os__fs__rename(const char* old_path, const char* new_path)
{
    if (old_path == NULL || old_path[0] == '\0') {
        return Error.argument;
    }
    if (new_path == NULL || new_path[0] == '\0') {
        return Error.argument;
    }
#ifdef _WIN32
    if (!MoveFileEx(old_path, new_path, MOVEFILE_REPLACE_EXISTING)) {
        return Error.os;
    }
    return EOK;
#else
    if (rename(old_path, new_path) < 0) {

        if (errno == ENOENT) {
            return Error.not_found;
        } else {
            return strerror(errno);
        }
    }
    return EOK;
#endif // _WIN32
}

static Exception
os__fs__mkdir(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
#ifdef _WIN32
    int result = mkdir(path);
#else
    int result = mkdir(path, 0755);
#endif
    if (result < 0) {
        uassert(errno != 0);
        if (errno == EEXIST) {
            return EOK;
        }
        return strerror(errno);
    }
    return EOK;
}

static os_fs_filetype_s
os__fs__file_type(const char* path)
{
    os_fs_filetype_s result = { .result = Error.argument };
    if (path == NULL || path[0] == '\0') {
        return result;
    }

#ifdef _WIN32
    DWORD attr = GetFileAttributesA(path);
    if (attr == INVALID_FILE_ATTRIBUTES) {
        nob_log(
            NOB_ERROR,
            "Could not get file attributes of %s: %s",
            path,
            nob_win32_error_message(GetLastError())
        );
        return -1;
    }
    result.is_valid = true;
    result.result = EOK;

    if (attr & FILE_ATTRIBUTE_DIRECTORY) {
        return NOB_FILE_DIRECTORY;
    }
    // TODO: detect symlinks on Windows (whatever that means on Windows anyway)
    return NOB_FILE_REGULAR;
#else // _WIN32
    struct stat statbuf;
    if (lstat(path, &statbuf) < 0) {
        if (errno == ENOENT) {
            result.result = Error.not_found;
        } else {
            result.result = strerror(errno);
        }
        return result;
    }
    result.is_valid = true;
    result.result = EOK;
    result.is_other = true;

    if (!S_ISLNK(statbuf.st_mode)) {
        if (S_ISREG(statbuf.st_mode)) {
            result.is_file = true;
            result.is_other = false;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            result.is_directory = true;
            result.is_other = false;
        }
    } else {
        result.is_symlink = true;
        if (stat(path, &statbuf) < 0) {
            if (errno == ENOENT) {
                result.result = Error.not_found;
            } else {
                result.result = strerror(errno);
            }
            return result;
        }
        if (S_ISREG(statbuf.st_mode)) {
            result.is_file = true;
            result.is_other = false;
        }
        if (S_ISDIR(statbuf.st_mode)) {
            result.is_directory = true;
            result.is_other = false;
        }
    }
    return result;
#endif
}

static Exception
os__fs__remove(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
#ifdef _WIN32
    if (!DeleteFileA(path)) {
        return nob_win32_error_message(GetLastError());
    }
    return EOK;
#else
    if (remove(path) < 0) {
        return strerror(errno);
    }
    return EOK;
#endif
}


Exception
os__fs__dir_walk(const char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx)
{
    (void)user_ctx;
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
    Exc result = Error.os;
    uassert(callback_fn != NULL && "you must provide callback_fn");

    DIR* dp = opendir(path);

    if (unlikely(dp == NULL)) {
        if (errno == ENOENT) {
            result = Error.not_found;
        }
        goto end;
    }

    u32 path_len = strlen(path);
    if (path_len > PATH_MAX - 256) {
        result = Error.overflow;
        goto end;
    }

    char path_buf[PATH_MAX];

    struct dirent* ep;
    while ((ep = readdir(dp)) != NULL) {
        if (str.eq(ep->d_name, ".")) {
            continue;
        }
        if (str.eq(ep->d_name, "..")) {
            continue;
        }
        memcpy(path_buf, path, path_len);
        u32 path_offset = 0;
        if (path_buf[path_len - 1] != '/' && path_buf[path_len - 1] != '\\') {
            path_buf[path_len] = os$PATH_SEP;
            path_offset = 1;
        }

        e$except_silent(
            err,
            str.copy(
                path_buf + path_len + path_offset,
                ep->d_name,
                sizeof(path_buf) - path_len - 1 - path_offset
            )
        )
        {
            result = err;
            goto end;
        }

        var ftype = os.fs.file_type(path_buf);
        if (ftype.result != EOK) {
            result = ftype.result;
            goto end;
        }

        if (ftype.is_symlink) {
            continue; // does not follow symlinks!
        }

        if (is_recursive && ftype.is_directory) {
            e$except_silent(err, os__fs__dir_walk(path_buf, is_recursive, callback_fn, user_ctx))
            {
                result = err;
                goto end;
            }

        } else {
            e$except_silent(err, callback_fn(path_buf, ftype, user_ctx))
            {
                result = err;
                goto end;
            }
        }
    }

    if (errno != 0) {
        result = strerror(errno);
        goto end;
    }

    result = EOK;
end:
    if (dp != NULL) {
        (void)closedir(dp);
    }
    return result;
}

static Exception
_os__fs__dir_list_walker(const char* path, os_fs_filetype_s ftype, void* user_ctx)
{
    (void)ftype;
    arr$(const char*) arr = *(arr$(const char*)*)user_ctx;
    uassert(arr != NULL);

    // Doing graceful memory check, otherwise arr$push will assert
    if (!arr$grow_check(arr, 1)) {
        return Error.memory;
    }
    arr$push(arr, path);
    *(arr$(const char*)*)user_ctx = arr; // in case of reallocation update this
    return EOK;
}

static arr$(char*) os__fs__dir_list(const char* path, bool is_recursive, IAllocator allc)
{

    if (unlikely(path == NULL)) {
        return NULL;
    }
    if (allc == NULL) {
        return NULL;
    }
    arr$(char*) result = arr$new(result, allc);
    if (result == NULL) {
        return NULL;
    }

    // NOTE: we specially pass &result, in order to maintain realloc-pointer change
    e$except_silent(err, os__fs__dir_walk(path, is_recursive, _os__fs__dir_list_walker, &result))
    {
        if (result != NULL) {
            arr$free(result);
        }
        result = NULL;
    }
    return result;
}

static Exception
os__fs__getcwd(sbuf_c* out)
{

    uassert(sbuf.isvalid(out) && "out is not valid sbuf_c (or missing initialization)");

    e$except_silent(err, sbuf.grow(out, PATH_MAX + 1))
    {
        return err;
    }
    sbuf.clear(out);

    errno = 0;
    if (unlikely(getcwd(*out, sbuf.capacity(out)) == NULL)) {
        return strerror(errno);
    }

    sbuf.update_len(out);

    return EOK;
}

static const char*
os__env__get(const char* name, const char* deflt)
{
    const char* result = getenv(name);

    if (result == NULL) {
        result = deflt;
    }

    return result;
}

static void
os__env__set(const char* name, const char* value, bool overwrite)
{
    setenv(name, value, overwrite);
}

static void
os__env__unset(const char* name)
{
    unsetenv(name);
}

static Exception
os__path__exists(const char* file_path)
{

    if (file_path == NULL) {
        return Error.argument;
    }

    usize path_len = strlen(file_path);
    if (path_len == 0) {
        return Error.argument;
    }

    if (path_len >= PATH_MAX) {
        return Error.argument;
    }

#if _WIN32
    errno = 0;
    DWORD dwAttrib = GetFileAttributesA(file_path);
    if (dwAttrib != INVALID_FILE_ATTRIBUTES) {

        return EOK;
    } else {
        /*
        Here are some common file-related functions and the types of errors they might set:
CreateFile: Used to open or create a file.
ERROR_FILE_NOT_FOUND (2): The file does not exist.
ERROR_PATH_NOT_FOUND (3): The specified path does not exist.
ERROR_ACCESS_DENIED (5): The file cannot be accessed due to permissions.
ERROR_SHARING_VIOLATION (32): The file is in use by another process.
ReadFile: Used to read data from a file.
ERROR_HANDLE_EOF (38): Attempted to read past the end of the file.
ERROR_IO_DEVICE (1117): An I/O error occurred.
WriteFile: Used to write data to a file.
ERROR_DISK_FULL (112): The disk is full.
ERROR_ACCESS_DENIED (5): The file is read-only or locked.
CloseHandle: Used to close a file handle.
ERROR_INVALID_HANDLE (6): The handle is invalid.
DeleteFile: Used to delete a file.
ERROR_FILE_NOT_FOUND (2): The file does not exist.
ERROR_ACCESS_DENIED (5): The file is read-only or in use.
MoveFile: Used to move or rename a file.
ERROR_FILE_EXISTS (80): The destination file already exists.
ERROR_ACCESS_DENIED (5): The file is in use or locked.
Example: Using GetLastError() with File Operations
    */
        // TODO: handle other win32 errors!
        uassert(false && "TODO handle other win32 errors!");
        return Error.not_found;
    }
#else
    struct stat statbuf;
    if (stat(file_path, &statbuf) < 0) {
        if (errno == ENOENT) {
            return Error.not_found;
        }
        return strerror(errno);
    }
    return EOK;
#endif
}

static char*
os__path__join(arr$(char*) parts, IAllocator allc)
{
    char sep[2] = { os$PATH_SEP, '\0' };
    return str.join(parts, sep, allc);
}

static str_s
os__path__split(const char* path, bool return_dir)
{
    if (path == NULL) {
        return (str_s){ 0 };
    }
    usize pathlen = strlen(path);
    if (pathlen == 0) {
        return str$s("");
    }

    isize last_slash_idx = -1;

    for (usize i = pathlen; i-- > 0;) {
        if (path[i] == '/' || path[i] == '\\') {
            last_slash_idx = i;
            break;
        }
    }
    if (last_slash_idx != -1) {
        if (return_dir) {
            return str.sub(path, 0, last_slash_idx == 0 ? 1 : last_slash_idx);
        } else {
            if ((usize)last_slash_idx == pathlen - 1) {
                return str$s("");
            } else {
                return str.sub(path, last_slash_idx + 1, 0);
            }
        }

    } else {
        if (return_dir) {
            return str$s("");
        } else {
            return str.sstr(path);
        }
    }
}

static char*
os__path__basename(const char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    str_s fname = os__path__split(path, false);
    return str.slice.clone(fname, allc);
}

static char*
os__path__dirname(const char* path, IAllocator allc)
{
    if (path == NULL || path[0] == '\0') {
        return NULL;
    }
    str_s fname = os__path__split(path, true);
    return str.slice.clone(fname, allc);
}


static Exception
os__cmd__create(os_cmd_c* self, arr$(char*) args, arr$(char*) env, os_cmd_flags_s* flags)
{
    (void)env;
    uassert(self != NULL);
    if (args == NULL || arr$len(args) == 0) {
        return "`args` is empty or null";
    }
    usize args_len = arr$len(args);
    if (args_len == 1 || args[args_len - 1] != NULL) {
        return "`args` last item must be a NULL";
    }
    for (u32 i = 0; i < args_len - 1; i++) {
        if (args[i] == NULL) {
            return "one of `args` items is NULL, which may indicate string operation failure";
        }
    }

    *self = (os_cmd_c){
        ._is_subprocess = true,
        ._flags = (flags) ? *flags : (os_cmd_flags_s){ 0 },
    };

    int sub_flags = 0;
    if (!self->_flags.no_inherit_env) {
        sub_flags |= subprocess_option_inherit_environment;
    }
    if (self->_flags.no_window) {
        sub_flags |= subprocess_option_no_window;
    }
    if (!self->_flags.no_search_path) {
        sub_flags |= subprocess_option_search_user_path;
    }
    if (self->_flags.combine_stdouterr) {
        sub_flags |= subprocess_option_combined_stdout_stderr;
    }

    int result = subprocess_create((const char* const*)args, sub_flags, &self->_subpr);
    if (result != 0) {
        uassert(errno != 0);
        return strerror(errno);
    }

    return EOK;
}

static bool
os__cmd__is_alive(os_cmd_c* self)
{
    return subprocess_alive(&self->_subpr);
}

static Exception
os__cmd__kill(os_cmd_c* self)
{
    if (subprocess_alive(&self->_subpr)) {
        if (subprocess_terminate(&self->_subpr) != 0) {
            return Error.os;
        }
    }
    return EOK;
}

static Exception
os__cmd__join(os_cmd_c* self, u32 timeout_sec, i32* out_ret_code)
{
    uassert(self != NULL);
    Exc result = Error.os;
    int ret_code = 1;

    if (timeout_sec == 0) {
        // timeout_sec == 0 -> infinite wait
        int join_result = subprocess_join(&self->_subpr, &ret_code);
        if (join_result != 0) {
            ret_code = -1;
            result = Error.os;
            goto end;
        }
    } else {
        uassert(timeout_sec < INT32_MAX && "timeout is negative or too high");
        u64 timeout_elapsed_ms = 0;
        u64 timeout_ms = timeout_sec * 1000;
        do {
            if (os__cmd__is_alive(self)) {
                os_sleep(100); // 100 ms sleep
            } else {
                subprocess_join(&self->_subpr, &ret_code);
                break;
            }
            timeout_elapsed_ms += 100;
        } while (timeout_elapsed_ms < timeout_ms);

        if (timeout_elapsed_ms >= timeout_ms) {
            result = Error.timeout;
            if (os__cmd__kill(self)) { // discard
            }
            subprocess_join(&self->_subpr, NULL);
            ret_code = -1;
            goto end;
        }
    }

    if (ret_code != 0) {
        result = Error.runtime;
        goto end;
    }

    result = Error.ok;

end:
    if (out_ret_code) {
        *out_ret_code = ret_code;
    }
    subprocess_destroy(&self->_subpr);
    memset(self, 0, sizeof(os_cmd_c));
    return result;
}

static FILE*
os__cmd__stdout(os_cmd_c* self)
{
    return self->_subpr.stdout_file;
}


static FILE*
os__cmd__stderr(os_cmd_c* self)
{
    return self->_subpr.stderr_file;
}

static FILE*
os__cmd__stdin(os_cmd_c* self)
{
    return self->_subpr.stdin_file;
}

static char*
os__cmd__read_all(os_cmd_c* self, IAllocator allc)
{
    uassert(self != NULL);
    uassert(allc != NULL);
    if (self->_subpr.stdout_file) {
        str_s out = { 0 };
        if (io.fread_all(self->_subpr.stdout_file, &out, allc)) {
            return NULL;
        }
        return out.buf;
    }
    return NULL;
}

static char*
os__cmd__read_line(os_cmd_c* self, IAllocator allc)
{
    uassert(self != NULL);
    uassert(allc != NULL);
    if (self->_subpr.stdout_file) {
        str_s out = { 0 };
        if (io.fread_line(self->_subpr.stdout_file, &out, allc)) {
            return NULL;
        }
        return out.buf;
    }
    return NULL;
}

static Exception
os__cmd__write_line(os_cmd_c* self, char* line)
{
    uassert(self != NULL);
    if (line == NULL) {
        return Error.argument;
    }

    if (self->_subpr.stdin_file == NULL) {
        return Error.not_found;
    }

    e$except_silent(err, io.file.writeln(self->_subpr.stdin_file, line))
    {
        return err;
    }
    fflush(self->_subpr.stdin_file);

    return EOK;
}


static Exception
os__cmd__run(const char** args, usize args_len, os_cmd_c* out_cmd)
{
    uassert(out_cmd != NULL);
    memset(out_cmd, 0, sizeof(os_cmd_c));

    if (args == NULL || args_len == 0) {
        return e$raise(Error.argument, "`args` argument is empty or null");
    }
    if (args_len == 1 || args[args_len - 1] != NULL) {
        return e$raise(Error.argument, "`args` last item must be a NULL");
    }

    for (u32 i = 0; i < args_len - 1; i++) {
        if (args[i] == NULL) {
            return e$raise(
                Error.argument,
                "`args` item[%d] is NULL, which may indicate string operation failure",
                i
            );
        }
    }


#ifdef _WIN32
    // FIX:  WIN32 uncompilable

    // https://docs.microsoft.com/en-us/windows/win32/procthread/creating-a-child-process-with-redirected-input-and-output

    STARTUPINFO siStartInfo;
    ZeroMemory(&siStartInfo, sizeof(siStartInfo));
    siStartInfo.cb = sizeof(STARTUPINFO);
    // NOTE: theoretically setting NULL to std handles should not be a problem
    // https://docs.microsoft.com/en-us/windows/console/getstdhandle?redirectedfrom=MSDN#attachdetach-behavior
    // TODO: check for errors in GetStdHandle
    siStartInfo.hStdError = redirect.fderr ? *redirect.fderr : GetStdHandle(STD_ERROR_HANDLE);
    siStartInfo.hStdOutput = redirect.fdout ? *redirect.fdout : GetStdHandle(STD_OUTPUT_HANDLE);
    siStartInfo.hStdInput = redirect.fdin ? *redirect.fdin : GetStdHandle(STD_INPUT_HANDLE);
    siStartInfo.dwFlags |= STARTF_USESTDHANDLES;

    PROCESS_INFORMATION piProcInfo;
    ZeroMemory(&piProcInfo, sizeof(PROCESS_INFORMATION));

    // TODO: use a more reliable rendering of the command instead of cmd_render
    // cmd_render is for logging primarily
    nob_cmd_render(cmd, &sb);
    nob_sb_append_null(&sb);
    BOOL bSuccess = CreateProcessA(
        NULL,
        sb.items,
        NULL,
        NULL,
        TRUE,
        0,
        NULL,
        NULL,
        &siStartInfo,
        &piProcInfo
    );
    nob_sb_free(sb);

    if (!bSuccess) {
        nob_log(
            NOB_ERROR,
            "Could not create child process: %s",
            nob_win32_error_message(GetLastError())
        );
        return NOB_INVALID_PROC;
    }

    CloseHandle(piProcInfo.hThread);

    return piProcInfo.hProcess;
#else
    pid_t cpid = fork();
    if (cpid < 0) {
        return e$raise(Error.os, "Could not fork child process: %s", strerror(errno));
    }

    if (cpid == 0) {
        if (execvp(args[0], (char* const*)args) < 0) {
            log$error("Could not exec child process: %s", strerror(errno));
            exit(1);
        }
        uassert(false && "unreachable");
    }

    *out_cmd = (os_cmd_c){ ._is_subprocess = false,
                           ._subpr = {
                               .child = cpid,
                               .alive = 1,
                           } };
    return EOK;

#endif
}


const struct __module__os os = {
    // Autogenerated by CEX
    // clang-format off
    .sleep = os_sleep,

    .fs = {  // sub-module .fs >>>
        .rename = os__fs__rename,
        .mkdir = os__fs__mkdir,
        .file_type = os__fs__file_type,
        .remove = os__fs__remove,
        .dir_walk = os__fs__dir_walk,
        .dir_list = os__fs__dir_list,
        .getcwd = os__fs__getcwd,
    },  // sub-module .fs <<<

    .env = {  // sub-module .env >>>
        .get = os__env__get,
        .set = os__env__set,
        .unset = os__env__unset,
    },  // sub-module .env <<<

    .path = {  // sub-module .path >>>
        .exists = os__path__exists,
        .join = os__path__join,
        .split = os__path__split,
        .basename = os__path__basename,
        .dirname = os__path__dirname,
    },  // sub-module .path <<<

    .cmd = {  // sub-module .cmd >>>
        .create = os__cmd__create,
        .is_alive = os__cmd__is_alive,
        .kill = os__cmd__kill,
        .join = os__cmd__join,
        .stdout = os__cmd__stdout,
        .stderr = os__cmd__stderr,
        .stdin = os__cmd__stdin,
        .read_all = os__cmd__read_all,
        .read_line = os__cmd__read_line,
        .write_line = os__cmd__write_line,
        .run = os__cmd__run,
    },  // sub-module .cmd <<<
    // clang-format on
};

#endif // #ifdef CEX_IMPLEMENTATION


#endif // #ifndef CEX_HEADER_H
