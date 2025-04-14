#pragma once
#ifndef CEX_HEADER_H
#define CEX_HEADER_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdalign.h>
#include <threads.h>
#include <stdarg.h>

#ifdef _WIN32
typedef struct _IO_FILE FILE;
#else
typedef struct _IO_FILE FILE;
#endif

#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <signal.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/*
 *                  GLOBAL CEX VARS / DEFINES
 */

/// disables all asserts and safety checks (fast release mode)
// #define NDEBUG

/// custom fprintf() function for asserts/logs/etc
// #define __cex__fprintf(stream, prefix, filename, line, func, format, ...)

/// customize abort() behavior
// #define __cex__abort()

// customize uassert() behavior
// #define __cex__assert()

// you may override this level to manage log$* verbosity
// #define CEX_LOG_LVL 5

/// disable ASAN memory poisoning and mem$asan_poison*
// #define CEX_DISABLE_POISON 1

/// size of stack based buffer for small strings
// #define CEX_SPRINTF_MIN 512

/// disables float printing for io.printf/et al functions (code size reduction)
// #define CEX_SPRINTF_NOFLOAT




/*
*                          src/cex_base.h
*/
/**
 * @file cex/cex.h
 * @brief CEX core file
 */

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
typedef ptrdiff_t isize;

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
#define fallthrough() __attribute__((fallthrough));

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
    io.fprintf(stream, "%s ( %s:%d %s() ) " format, prefix, filename, line, func, ##__VA_ARGS__)

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
// LVL Value
// 0 - mute all including assert messages, tracebacks, errors
// 1 - allow log$error + assert messages, tracebacks
// 2 - allow log$warn
// 3 - allow log$info
// 4 - allow log$debug
// 5 - allow log$trace
// NOTE: you may override this level to manage log$* verbosity
#define CEX_LOG_LVL 4
#endif

#if CEX_LOG_LVL > 0
#define log$error(format, ...)                                                                     \
    (__cex__fprintf(stdout, "[ERROR]  ", __FILE_NAME__, __LINE__, __func__, format, ##__VA_ARGS__))
#else
#define log$error(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 1
#define log$warn(format, ...)                                                                      \
    (__cex__fprintf(stdout, "[WARN]   ", __FILE_NAME__, __LINE__, __func__, format, ##__VA_ARGS__))
#else
#define log$warn(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 2
#define log$info(format, ...)                                                                      \
    (__cex__fprintf(stdout, "[INFO]   ", __FILE_NAME__, __LINE__, __func__, format, ##__VA_ARGS__))
#else
#define log$info(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 3
#define log$debug(format, ...)                                                                     \
    (__cex__fprintf(stdout, "[DEBUG]  ", __FILE_NAME__, __LINE__, __func__, format, ##__VA_ARGS__))
#else
#define log$debug(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 4
#define log$trace(format, ...)                                                                     \
    (__cex__fprintf(stdout, "[TRACE]  ", __FILE_NAME__, __LINE__, __func__, format, ##__VA_ARGS__))
#else
#define log$trace(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 0
#define __cex__traceback(uerr, fail_func)                                                          \
    (__cex__fprintf(                                                                               \
        stdout,                                                                                    \
        "[^STCK]  ",                                                                               \
        __FILE_NAME__,                                                                             \
        __LINE__,                                                                                  \
        __func__,                                                                                  \
        "^^^^^ [%s] in function call `%s`\n",                                                      \
        uerr,                                                                                      \
        fail_func                                                                                  \
    ))

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

#ifdef __SANITIZE_ADDRESS__
// This should be linked when gcc sanitizer enabled
void __sanitizer_print_stack_trace();
#define sanitizer_stack_trace() __sanitizer_print_stack_trace()
#else
#define sanitizer_stack_trace() ((void)(0))
#endif

#ifdef NDEBUG
#define uassertf(cond, format, ...) ((void)(0))
#define uassert(cond) ((void)(0))
#define __cex_test_postmortem_exists() 0
#else

#ifdef CEXTEST
// this prevents spamming on stderr (i.e. cextest.h output stream in silent mode)
int __cex_test_uassert_enabled = 1;
#define uassert_disable() __cex_test_uassert_enabled = 0
#define uassert_enable() __cex_test_uassert_enabled = 1
#define uassert_is_enabled() (__cex_test_uassert_enabled)
#else
#define uassert_disable()                                                                          \
    _Static_assert(false, "uassert_disable() allowed only when compiled with -DCEXTEST")
#define uassert_enable() (void)0
#define uassert_is_enabled() true
#define __cex_test_postmortem_ctx NULL
#define __cex_test_postmortem_exists() 0
#define __cex_test_postmortem_f(ctx)
#endif // #ifdef CEXTEST


/**
 * @def uassert(A)
 * @brief Custom assertion, with support of sanitizer call stack printout at failure.
 */
#define uassert(A)                                                                                 \
    ({                                                                                             \
        if (unlikely(!((A)))) {                                                                    \
            __cex__fprintf(                                                                        \
                (uassert_is_enabled() ? stderr : stdout),                                          \
                "[ASSERT] ",                                                                       \
                __FILE_NAME__,                                                                     \
                __LINE__,                                                                          \
                __func__,                                                                          \
                "%s\n",                                                                            \
                #A                                                                                 \
            );                                                                                     \
            if (uassert_is_enabled()) {                                                            \
                __cex__panic();                                                                    \
            }                                                                                      \
        }                                                                                          \
    })

#define uassertf(A, format, ...)                                                                   \
    ({                                                                                             \
        if (unlikely(!((A)))) {                                                                    \
            __cex__fprintf(                                                                        \
                (uassert_is_enabled() ? stderr : stdout),                                          \
                "[ASSERT] ",                                                                       \
                __FILE_NAME__,                                                                     \
                __LINE__,                                                                          \
                __func__,                                                                          \
                format "\n",                                                                       \
                ##__VA_ARGS__                                                                      \
            );                                                                                     \
            if (uassert_is_enabled()) {                                                            \
                __cex__panic();                                                                    \
            }                                                                                      \
        }                                                                                          \
    })
#endif

__attribute__((noinline)) void __cex__panic(void);

#define unreachable(format, ...)                                                                   \
    ({                                                                                             \
        __cex__fprintf(                                                                            \
            stderr,                                                                                \
            "[UNREACHABLE] ",                                                                      \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format "\n",                                                                           \
            ##__VA_ARGS__                                                                          \
        );                                                                                         \
        __cex__panic();                                                                            \
    })

// cex$tmpname - internal macro for generating temporary variable names (unique__line_num)
#define cex$concat3(c, a, b) c##a##b
#define cex$concat(a, b) a##b
#define _cex$stringize(...) #__VA_ARGS__
#define cex$stringize(...) _cex$stringize(__VA_ARGS__)
#define cex$varname(a, b) cex$concat3(__cex__, a, b)
#define cex$tmpname(base) cex$varname(base, __LINE__)

#define e$raise(return_uerr, error_msg, ...)                                                       \
    (log$error("[%s] " error_msg "\n", return_uerr, ##__VA_ARGS__), (return_uerr))

// WARNING: DO NOT USE break/continue inside e$except/e$except_silent {scope!}
#define e$except(_var_name, _func)                                                                 \
    for (Exc _var_name = _func;                                                                    \
         unlikely((_var_name != EOK) && (__cex__traceback(_var_name, #_func), 1));                    \
         _var_name = EOK)

#if defined(CEXTEST) || defined(CEXBUILD)
#define e$except_silent(_var_name, _func) e$except(_var_name, _func)
#else
#define e$except_silent(_var_name, _func)                                                          \
    for (Exc _var_name = _func; unlikely(_var_name != EOK); _var_name = EOK)
#endif

#define e$except_errno(_expression)                                                                \
    if (unlikely(                                                                                  \
            ((_expression) == -1) &&                                                               \
            (log$error("`%s` failed errno: %d, msg: %s\n", #_expression, errno, strerror(errno)),  \
             1)                                                                                    \
        ))

#define e$except_null(_expression)                                                                 \
    if (unlikely(((_expression) == NULL) && (log$error("`%s` returned NULL\n", #_expression), 1)))

#define e$ret(_func)                                                                               \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func), 1)                      \
         );                                                                                        \
         cex$tmpname(__cex_err_traceback_) = EOK)                                                  \
    return cex$tmpname(__cex_err_traceback_)

#define e$goto(_func, _label)                                                                      \
    for (Exc cex$tmpname(__cex_err_traceback_) = _func; unlikely(                                  \
             (cex$tmpname(__cex_err_traceback_) != EOK) &&                                         \
             (__cex__traceback(cex$tmpname(__cex_err_traceback_), #_func), 1)                      \
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
        /*log$debug("instart: %d, inend: %d, start: %ld, end: %ld\n", start, end, _start, _end); */                                                                                                \
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
    struct
    {
        union
        {
            usize i;
            char* skey;
            void* pkey;
        };
    } idx;
    char _ctx[47];
    u8 stopped;
    u8 initialized;
} cex_iterator_s;
_Static_assert(sizeof(usize) == sizeof(void*), "usize expected as sizeof ptr");
_Static_assert(alignof(usize) == alignof(void*), "alignof pointer != alignof usize");
_Static_assert(alignof(cex_iterator_s) == alignof(void*), "alignof");
_Static_assert(sizeof(cex_iterator_s) <= 64, "cex size");

/**
 * @brief Iterates via iterator function (see usage below)
 *
 * Iterator function signature:
 * u32* array_iterator(u32 array[], u32 len, cex_iterator_s* iter)
 *
 * for$iter(u32, it, array_iterator(arr2, arr$len(arr2), &it.iterator))
 */
#define for$iter(it_val_type, it, iter_func)                                                            \
    struct cex$tmpname(__cex_iter_)                                                                 \
    {                                                                                              \
        it_val_type val;                                                                   \
        union /* NOTE:  iterator above and this struct shadow each other */                       \
        {                                                                                          \
            cex_iterator_s iterator;                                                                   \
            struct                                                                                 \
            {                                                                                      \
                union                                                                              \
                {                                                                                  \
                    usize i;                                                                       \
                    char* skey;                                                                    \
                    void* pkey;                                                                    \
                };                                                                                 \
            } idx;                                                                                 \
        };                                                                                         \
    };                                                                                             \
                                                                                                   \
    for (struct cex$tmpname(__cex_iter_) it = { .val = (iter_func) }; !it.iterator.stopped;               \
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
*                          src/mem.h
*/

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
*                          src/AllocatorHeap.h
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
*                          src/AllocatorArena.h
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

struct __cex_namespace__AllocatorArena
{
    // Autogenerated by CEX
    // clang-format off

const Allocator_i* (*create)(usize page_size);
void            (*destroy)(IAllocator self);
bool            (*sanitize)(IAllocator allc);
    // clang-format on
};
__attribute__ ((visibility("hidden"))) extern const struct __cex_namespace__AllocatorArena AllocatorArena;



/*
*                          src/ds.h
*/

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
extern usize cexds_arr_len(const void* arr);
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
        /* NOLINTBEGIN */                                                                          \
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
        __builtin_types_compatible_p(typeof(arr), typeof(&(arr)[0])) /* check if array or ptr */   \
            ? cexds_arr_len(arr)                                     /* some pointer or arr$ */    \
            : (sizeof(arr) / sizeof((arr)[0])                        /* static array[] */          \
              );                                                                                   \
        /* NOLINTEND */                                                                            \
        _Pragma("GCC diagnostic pop");                                                             \
    })
// NOLINT

#define for$each(v, array, array_len...)                                                           \
    /* NOLINTBEGIN*/                                                                               \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    typeof((array)[0])* cex$tmpname(arr_arrp) = &(array)[0];                                       \
    usize cex$tmpname(arr_index) = 0;                                                              \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    /* NOLINTEND */                                                                                \
    for (typeof((array)[0]) v = { 0 };                                                             \
         (cex$tmpname(arr_index) < cex$tmpname(arr_length) &&                                      \
          ((v) = cex$tmpname(arr_arrp)[cex$tmpname(arr_index)], 1));                               \
         cex$tmpname(arr_index)++)

#define for$eachp(v, array, array_len...)                                                          \
    /* NOLINTBEGIN*/                                                                               \
    usize cex$tmpname(arr_length_opt)[] = { array_len }; /* decide if user passed array_len */     \
    usize cex$tmpname(arr_length) = (sizeof(cex$tmpname(arr_length_opt)) > 0)                      \
                                      ? cex$tmpname(arr_length_opt)[0]                             \
                                      : arr$len(array); /* prevents multi call of (length)*/       \
    uassert(cex$tmpname(arr_length) < PTRDIFF_MAX && "negative length or overflow");               \
    typeof((array)[0])* cex$tmpname(arr_arrp) = &(array)[0];                                       \
    usize cex$tmpname(arr_index) = 0;                                                              \
    /* NOLINTEND */                                                                                \
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
        if (t != NULL) {                                                                           \
            cexds_arr_integrity(t, CEXDS_HM_MAGIC);                                                \
        }                                                                                          \
        (t) ? cexds_header((t))->length : 0;                                                       \
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
*                          src/_sprintf.h
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
*                          src/str.h
*/
/**
 * @file
 * @brief
 */


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

#define str$join(allocator, str_join_by, str_parts...)                                                     \
    ({                                                                                             \
        const char* _args[] = { str_parts };                                                      \
        usize _args_len = arr$len(_args);                                                          \
        str.join(_args, _args_len, str_join_by, allocator);                                                 \
    })

struct __cex_namespace__str {
    // Autogenerated by CEX
    // clang-format off

    char*           (*clone)(const char* s, IAllocator allc);
    Exception       (*copy)(char* dest, const char* src, usize destlen);
    bool            (*ends_with)(const char* str, const char* suffix);
    bool            (*eq)(const char* a, const char* b);
    bool            (*eqi)(const char* a, const char* b);
    char*           (*find)(const char* haystack, const char* needle);
    char*           (*findr)(const char* haystack, const char* needle);
    char*           (*fmt)(IAllocator allc, const char* format,...);
    char*           (*join)(const char** str_arr, usize str_arr_len, const char* join_by, IAllocator allc);
    usize           (*len)(const char* s);
    char*           (*lower)(const char* s, IAllocator allc);
    bool            (*match)(const char* s, const char* pattern);
    char*           (*replace)(const char* str, const char* old_sub, const char* new_sub, IAllocator allc);
    str_s           (*sbuf)(char* s, usize length);
    arr$(char*)     (*split)(const char* s, const char* split_by, IAllocator allc);
    arr$(char*)     (*split_lines)(const char* s, IAllocator allc);
    Exception       (*sprintf)(char* dest, usize dest_len, const char* format,...);
    /// Creates string slice of input c-str (NULL tolerant, (str_s){0} on error)
    str_s           (*sstr)(const char* ccharptr);
    bool            (*starts_with)(const char* str, const char* prefix);
    str_s           (*sub)(const char* s, isize start, isize end);
    char*           (*upper)(const char* s, IAllocator allc);
    Exception       (*vsprintf)(char* dest, usize dest_len, const char* format, va_list va);

    struct {
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
    } convert;

    struct {
        char*           (*clone)(str_s s, IAllocator allc);
        int             (*cmp)(str_s self, str_s other);
        int             (*cmpi)(str_s self, str_s other);
        Exception       (*copy)(char* dest, str_s src, usize destlen);
        bool            (*ends_with)(str_s s, str_s suffix);
        bool            (*eq)(str_s a, str_s b);
        isize           (*index_of)(str_s str, str_s needle);
        str_s           (*iter_split)(str_s s, const char* split_by, cex_iterator_s* iterator);
        str_s           (*lstrip)(str_s s);
        bool            (*match)(str_s s, const char* pattern);
        str_s           (*remove_prefix)(str_s s, str_s prefix);
        str_s           (*remove_suffix)(str_s s, str_s suffix);
        str_s           (*rstrip)(str_s s);
        bool            (*starts_with)(str_s str, str_s prefix);
        str_s           (*strip)(str_s s);
        str_s           (*sub)(str_s s, isize start, isize end);
    } slice;

    // clang-format on
};
__attribute__((visibility("hidden"))) extern const struct __cex_namespace__str str;

// CEX Autogen



/*
*                          src/sbuf.h
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

struct __cex_namespace__sbuf
{
    // Autogenerated by CEX
    // clang-format off

Exception       (*append)(sbuf_c* self, const char* s);
Exception       (*appendf)(sbuf_c* self, const char* format, ...);
Exception       (*appendfva)(sbuf_c* self, const char* format, va_list va);
u32             (*capacity)(const sbuf_c* self);
void            (*clear)(sbuf_c* self);
sbuf_c          (*create)(u32 capacity, IAllocator allocator);
sbuf_c          (*create_static)(char* buf, usize buf_size);
sbuf_c          (*create_temp)(void);
sbuf_c          (*destroy)(sbuf_c* self);
Exception       (*grow)(sbuf_c* self, u32 new_capacity);
bool            (*isvalid)(sbuf_c* self);
u32             (*len)(const sbuf_c* self);
void            (*shrink)(sbuf_c* self, u32 new_length);
void            (*update_len)(sbuf_c* self);
    // clang-format on
};
__attribute__ ((visibility("hidden"))) extern const struct __cex_namespace__sbuf sbuf;



/*
*                          src/io.h
*/

#define io$ansi(text, ansi_col) "\033[" ansi_col "m" text "\033[0m"

struct __cex_namespace__io
{
    // Autogenerated by CEX
    // clang-format off

void            (*fclose)(FILE** file);
Exception       (*fflush)(FILE* file);
int             (*fileno)(FILE* file);
Exception       (*fopen)(FILE** file, const char* filename, const char* mode);
Exc             (*fprintf)(FILE* stream, const char* format, ...);
Exception       (*fread)(FILE* file, void* obj_buffer, usize obj_el_size, usize* obj_count);
Exception       (*fread_all)(FILE* file, str_s* s, IAllocator allc);
Exception       (*fread_line)(FILE* file, str_s* s, IAllocator allc);
Exception       (*fseek)(FILE* file, long offset, int whence);
Exception       (*ftell)(FILE* file, usize* size);
Exception       (*fwrite)(FILE* file, const void* obj_buffer, usize obj_el_size, usize obj_count);
bool            (*isatty)(FILE* file);
int             (*printf)(const char* format, ...);
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
__attribute__ ((visibility("hidden"))) extern const struct __cex_namespace__io io;



/*
*                          src/argparse.h
*/
/**
 * Copyright (C) 2012-2015 Yecheng Fu <cofyc.jackson at gmail dot com>
 * All rights reserved.
 *
 * Use of this source code is governed by a MIT-style license that can be found
 * in the LICENSE file.
 */

struct argparse_c;
struct argparse_opt_s;

typedef Exception (*argparse_callback_f)(
    struct argparse_c* self,
    const struct argparse_opt_s* option,
    void* ctx
);
typedef Exception (*argparse_convert_f)(const char* s, void* out_val);
typedef Exception (*argparse_command_f)(int argc, char** argv, void* user_ctx);

typedef struct argparse_opt_s
{
    u8 type;
    void* value;
    const char short_name;
    const char* long_name;
    const char* help;
    bool required;
    argparse_callback_f callback;
    void* callback_data;
    argparse_convert_f convert;
    bool is_present; // also setting in in argparse$opt* macro, allows optional parameter sugar
} argparse_opt_s;

typedef struct argparse_cmd_s
{
    const char* name;
    argparse_command_f func;
    const char* help;
    bool is_default;
} argparse_cmd_s;

enum CexArgParseType_e
{
    CexArgParseType__na,
    CexArgParseType__group,
    CexArgParseType__boolean,
    CexArgParseType__string,
    CexArgParseType__i8,
    CexArgParseType__u8,
    CexArgParseType__i16,
    CexArgParseType__u16,
    CexArgParseType__i32,
    CexArgParseType__u32,
    CexArgParseType__i64,
    CexArgParseType__u64,
    CexArgParseType__f32,
    CexArgParseType__f64,
    CexArgParseType__generic,
};

/**
 * argpparse
 */
typedef struct argparse_c
{
    // user supplied options
    argparse_opt_s* options;
    u32 options_len;

    argparse_cmd_s* commands;
    u32 commands_len;

    const char* usage;        // usage text (can be multiline), each line prepended by program_name
    const char* description;  // a description after usage
    const char* epilog;       // a description at the end
    const char* program_name; // program name in usage (by default: argv[0])

    int argc;    // current argument count (excludes program_name)
    char** argv; // current arguments list

    struct
    {
        // internal context
        char** out;
        int cpidx;
        const char* optvalue; // current option value
        bool has_argument;
        argparse_cmd_s* current_command;
    } _ctx;
} argparse_c;


// built-in option macros
#define argparse$opt(value, ...)                                                                   \
    ({                                                                                             \
        u32 val_type = _Generic(                                                                   \
            (value),                                                                               \
            bool*: CexArgParseType__boolean,                                                       \
            i8*: CexArgParseType__i8,                                                              \
            u8*: CexArgParseType__u8,                                                              \
            i16*: CexArgParseType__i16,                                                            \
            u16*: CexArgParseType__u16,                                                            \
            i32*: CexArgParseType__i32,                                                            \
            u32*: CexArgParseType__u32,                                                            \
            i64*: CexArgParseType__i64,                                                            \
            u64*: CexArgParseType__u64,                                                            \
            f32*: CexArgParseType__f32,                                                            \
            f64*: CexArgParseType__f64,                                                            \
            const char**: CexArgParseType__string,                                                 \
            char**: CexArgParseType__string,                                                       \
            default: CexArgParseType__generic                                                      \
        );                                                                                         \
        argparse_convert_f conv_f = _Generic(                                                      \
            (value),                                                                               \
            bool*: NULL,                                                                           \
            const char**: NULL,                                                                    \
            char**: NULL,                                                                          \
            i8*: (argparse_convert_f)str.convert.to_i8,                                            \
            u8*: (argparse_convert_f)str.convert.to_u8,                                            \
            i16*: (argparse_convert_f)str.convert.to_i16,                                          \
            u16*: (argparse_convert_f)str.convert.to_u16,                                          \
            i32*: (argparse_convert_f)str.convert.to_i32,                                          \
            u32*: (argparse_convert_f)str.convert.to_u32,                                          \
            i64*: (argparse_convert_f)str.convert.to_i64,                                          \
            u64*: (argparse_convert_f)str.convert.to_u64,                                          \
            f32*: (argparse_convert_f)str.convert.to_f32,                                          \
            f64*: (argparse_convert_f)str.convert.to_f64,                                          \
            default: NULL                                                                          \
        );                                                                                         \
        (argparse_opt_s){ val_type,                                                                \
                          (value),                                                                 \
                          __VA_ARGS__,                                                             \
                          .convert = (argparse_convert_f)conv_f,                                   \
                          .is_present = 0 };                                                       \
    })
// clang-format off
#define argparse$opt_group(h)     { CexArgParseType__group, NULL, '\0', NULL, h, false, NULL, 0, 0, .is_present=0 }
#define argparse$opt_help()       {CexArgParseType__boolean, NULL, 'h', "help",                           \
                                        "show this help message and exit", false,    \
                                        NULL, 0, .is_present = 0}
#define argparse$pop(argc, argv) ((argc > 0) ? (--argc, (*argv)++) : NULL)

__attribute__((visibility("hidden"))) extern const struct __cex_namespace__argparse argparse;

struct __cex_namespace__argparse {
    // Autogenerated by CEX
    // clang-format off
    
    const char*     (*next)(argparse_c* self);
    Exception       (*parse)(argparse_c* self, int argc, char** argv);
    Exception       (*run_command)(argparse_c* self, void* user_ctx);
    void            (*usage)(argparse_c* self);
    
    // clang-format on
};



/*
*                          src/_subprocess.h
*/
/*
   The latest version of this library is available on GitHub;
   https://github.com/sheredom/subprocess.h
*/

/*
   This is free and unencumbered software released into the public domain.

   Anyone is free to copy, modify, publish, use, compile, sell, or
   distribute this software, either in source code form or as a compiled
   binary, for any purpose, commercial or non-commercial, and by any
   means.

   In jurisdictions that recognize copyright laws, the author or authors
   of this software dedicate any and all copyright interest in the
   software to the public domain. We make this dedication for the benefit
   of the public at large and to the detriment of our heirs and
   successors. We intend this dedication to be an overt act of
   relinquishment in perpetuity of all present and future rights to this
   software under copyright law.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
   IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
   OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
   ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
   OTHER DEALINGS IN THE SOFTWARE.

   For more information, please refer to <http://unlicense.org/>
*/

#ifndef SHEREDOM_SUBPROCESS_H_INCLUDED
#define SHEREDOM_SUBPROCESS_H_INCLUDED

#if defined(_MSC_VER)
#pragma warning(push, 1)

/* disable warning: '__cplusplus' is not defined as a preprocessor macro,
 * replacing with '0' for '#if/#elif' */
#pragma warning(disable : 4668)
#endif

#include <stdio.h>
#include <string.h>

#if defined(_MSC_VER)
#pragma warning(pop)
#endif

#if defined(__TINYC__)
#define SUBPROCESS_ATTRIBUTE(a) __attribute((a))
#else
#define SUBPROCESS_ATTRIBUTE(a) __attribute__((a))
#endif

#if defined(_MSC_VER)
#define subprocess_pure
#define subprocess_weak __inline
#define subprocess_tls __declspec(thread)
#elif defined(__MINGW32__)
#define subprocess_pure SUBPROCESS_ATTRIBUTE(pure)
#define subprocess_weak static SUBPROCESS_ATTRIBUTE(used)
#define subprocess_tls __thread
#elif defined(__clang__) || defined(__GNUC__) || defined(__TINYC__)
#define subprocess_pure SUBPROCESS_ATTRIBUTE(pure)
#define subprocess_weak SUBPROCESS_ATTRIBUTE(weak)
#define subprocess_tls __thread
#else
#error Non clang, non gcc, non MSVC compiler found!
#endif

struct subprocess_s;

enum subprocess_option_e {
  // stdout and stderr are the same FILE.
  subprocess_option_combined_stdout_stderr = 0x1,

  // The child process should inherit the environment variables of the parent.
  subprocess_option_inherit_environment = 0x2,

  // Enable asynchronous reading of stdout/stderr before it has completed.
  subprocess_option_enable_async = 0x4,

  // Enable the child process to be spawned with no window visible if supported
  // by the platform.
  subprocess_option_no_window = 0x8,

  // Search for program names in the PATH variable. Always enabled on Windows.
  // Note: this will **not** search for paths in any provided custom environment
  // and instead uses the PATH of the spawning process.
  subprocess_option_search_user_path = 0x10
};

#if defined(__cplusplus)
extern "C" {
#endif

/// @brief Create a process.
/// @param command_line An array of strings for the command line to execute for
/// this process. The last element must be NULL to signify the end of the array.
/// The memory backing this parameter only needs to persist until this function
/// returns.
/// @param options A bit field of subprocess_option_e's to pass.
/// @param out_process The newly created process.
/// @return On success zero is returned.
subprocess_weak int subprocess_create(const char *const command_line[],
                                      int options,
                                      struct subprocess_s *const out_process);

/// @brief Create a process (extended create).
/// @param command_line An array of strings for the command line to execute for
/// this process. The last element must be NULL to signify the end of the array.
/// The memory backing this parameter only needs to persist until this function
/// returns.
/// @param options A bit field of subprocess_option_e's to pass.
/// @param environment An optional array of strings for the environment to use
/// for a child process (each element of the form FOO=BAR). The last element
/// must be NULL to signify the end of the array.
/// @param out_process The newly created process.
/// @return On success zero is returned.
///
/// If `options` contains `subprocess_option_inherit_environment`, then
/// `environment` must be NULL.
subprocess_weak int
subprocess_create_ex(const char *const command_line[], int options,
                     const char *const environment[],
                     struct subprocess_s *const out_process);

/// @brief Get the standard input file for a process.
/// @param process The process to query.
/// @return The file for standard input of the process.
///
/// The file returned can be written to by the parent process to feed data to
/// the standard input of the process.
subprocess_pure subprocess_weak FILE *
subprocess_stdin(const struct subprocess_s *const process);

/// @brief Get the standard output file for a process.
/// @param process The process to query.
/// @return The file for standard output of the process.
///
/// The file returned can be read from by the parent process to read data from
/// the standard output of the child process.
subprocess_pure subprocess_weak FILE *
subprocess_stdout(const struct subprocess_s *const process);

/// @brief Get the standard error file for a process.
/// @param process The process to query.
/// @return The file for standard error of the process.
///
/// The file returned can be read from by the parent process to read data from
/// the standard error of the child process.
///
/// If the process was created with the subprocess_option_combined_stdout_stderr
/// option bit set, this function will return NULL, and the subprocess_stdout
/// function should be used for both the standard output and error combined.
subprocess_pure subprocess_weak FILE *
subprocess_stderr(const struct subprocess_s *const process);

/// @brief Wait for a process to finish execution.
/// @param process The process to wait for.
/// @param out_return_code The return code of the returned process (can be
/// NULL).
/// @return On success zero is returned.
///
/// Joining a process will close the stdin pipe to the process.
subprocess_weak int subprocess_join(struct subprocess_s *const process,
                                    int *const out_return_code);

/// @brief Destroy a previously created process.
/// @param process The process to destroy.
/// @return On success zero is returned.
///
/// If the process to be destroyed had not finished execution, it may out live
/// the parent process.
subprocess_weak int subprocess_destroy(struct subprocess_s *const process);

/// @brief Terminate a previously created process.
/// @param process The process to terminate.
/// @return On success zero is returned.
///
/// If the process to be destroyed had not finished execution, it will be
/// terminated (i.e killed).
subprocess_weak int subprocess_terminate(struct subprocess_s *const process);

/// @brief Read the standard output from the child process.
/// @param process The process to read from.
/// @param buffer The buffer to read into.
/// @param size The maximum number of bytes to read.
/// @return The number of bytes actually read into buffer. Can only be 0 if the
/// process has complete.
///
/// The only safe way to read from the standard output of a process during it's
/// execution is to use the `subprocess_option_enable_async` option in
/// conjunction with this method.
subprocess_weak unsigned
subprocess_read_stdout(struct subprocess_s *const process, char *const buffer,
                       unsigned size);

/// @brief Read the standard error from the child process.
/// @param process The process to read from.
/// @param buffer The buffer to read into.
/// @param size The maximum number of bytes to read.
/// @return The number of bytes actually read into buffer. Can only be 0 if the
/// process has complete.
///
/// The only safe way to read from the standard error of a process during it's
/// execution is to use the `subprocess_option_enable_async` option in
/// conjunction with this method.
subprocess_weak unsigned
subprocess_read_stderr(struct subprocess_s *const process, char *const buffer,
                       unsigned size);

/// @brief Returns if the subprocess is currently still alive and executing.
/// @param process The process to check.
/// @return If the process is still alive non-zero is returned.
subprocess_weak int subprocess_alive(struct subprocess_s *const process);

#if defined(__cplusplus)
#define SUBPROCESS_CAST(type, x) static_cast<type>(x)
#define SUBPROCESS_PTR_CAST(type, x) reinterpret_cast<type>(x)
#define SUBPROCESS_CONST_CAST(type, x) const_cast<type>(x)
#define SUBPROCESS_NULL NULL
#else
#define SUBPROCESS_CAST(type, x) ((type)(x))
#define SUBPROCESS_PTR_CAST(type, x) ((type)(x))
#define SUBPROCESS_CONST_CAST(type, x) ((type)(x))
#define SUBPROCESS_NULL 0
#endif

#if !defined(_WIN32)
#include <signal.h>
#include <spawn.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#endif

#if defined(_WIN32)

#if (_MSC_VER < 1920)
#ifdef _WIN64
typedef __int64 subprocess_intptr_t;
typedef unsigned __int64 subprocess_size_t;
#else
typedef int subprocess_intptr_t;
typedef unsigned int subprocess_size_t;
#endif
#else
#include <inttypes.h>

typedef intptr_t subprocess_intptr_t;
typedef size_t subprocess_size_t;
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif

typedef struct _PROCESS_INFORMATION *LPPROCESS_INFORMATION;
typedef struct _SECURITY_ATTRIBUTES *LPSECURITY_ATTRIBUTES;
typedef struct _STARTUPINFOA *LPSTARTUPINFOA;
typedef struct _OVERLAPPED *LPOVERLAPPED;

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#ifdef _MSC_VER
#pragma warning(push, 1)
#endif
#ifdef __MINGW32__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wpedantic"
#endif

struct subprocess_subprocess_information_s {
  void *hProcess;
  void *hThread;
  unsigned long dwProcessId;
  unsigned long dwThreadId;
};

struct subprocess_security_attributes_s {
  unsigned long nLength;
  void *lpSecurityDescriptor;
  int bInheritHandle;
};

struct subprocess_startup_info_s {
  unsigned long cb;
  char *lpReserved;
  char *lpDesktop;
  char *lpTitle;
  unsigned long dwX;
  unsigned long dwY;
  unsigned long dwXSize;
  unsigned long dwYSize;
  unsigned long dwXCountChars;
  unsigned long dwYCountChars;
  unsigned long dwFillAttribute;
  unsigned long dwFlags;
  unsigned short wShowWindow;
  unsigned short cbReserved2;
  unsigned char *lpReserved2;
  void *hStdInput;
  void *hStdOutput;
  void *hStdError;
};

struct subprocess_overlapped_s {
  uintptr_t Internal;
  uintptr_t InternalHigh;
  union {
    struct {
      unsigned long Offset;
      unsigned long OffsetHigh;
    } DUMMYSTRUCTNAME;
    void *Pointer;
  } DUMMYUNIONNAME;

  void *hEvent;
};

#ifdef __MINGW32__
#pragma GCC diagnostic pop
#endif
#ifdef _MSC_VER
#pragma warning(pop)
#endif

__declspec(dllimport) unsigned long __stdcall GetLastError(void);
__declspec(dllimport) int __stdcall SetHandleInformation(void *, unsigned long,
                                                         unsigned long);
__declspec(dllimport) int __stdcall CreatePipe(void **, void **,
                                               LPSECURITY_ATTRIBUTES,
                                               unsigned long);
__declspec(dllimport) void *__stdcall CreateNamedPipeA(
    const char *, unsigned long, unsigned long, unsigned long, unsigned long,
    unsigned long, unsigned long, LPSECURITY_ATTRIBUTES);
__declspec(dllimport) int __stdcall ReadFile(void *, void *, unsigned long,
                                             unsigned long *, LPOVERLAPPED);
__declspec(dllimport) unsigned long __stdcall GetCurrentProcessId(void);
__declspec(dllimport) unsigned long __stdcall GetCurrentThreadId(void);
__declspec(dllimport) void *__stdcall CreateFileA(const char *, unsigned long,
                                                  unsigned long,
                                                  LPSECURITY_ATTRIBUTES,
                                                  unsigned long, unsigned long,
                                                  void *);
__declspec(dllimport) void *__stdcall CreateEventA(LPSECURITY_ATTRIBUTES, int,
                                                   int, const char *);
__declspec(dllimport) int __stdcall CreateProcessA(
    const char *, char *, LPSECURITY_ATTRIBUTES, LPSECURITY_ATTRIBUTES, int,
    unsigned long, void *, const char *, LPSTARTUPINFOA, LPPROCESS_INFORMATION);
__declspec(dllimport) int __stdcall CloseHandle(void *);
__declspec(dllimport) unsigned long __stdcall WaitForSingleObject(
    void *, unsigned long);
__declspec(dllimport) int __stdcall GetExitCodeProcess(
    void *, unsigned long *lpExitCode);
__declspec(dllimport) int __stdcall TerminateProcess(void *, unsigned int);
__declspec(dllimport) unsigned long __stdcall WaitForMultipleObjects(
    unsigned long, void *const *, int, unsigned long);
__declspec(dllimport) int __stdcall GetOverlappedResult(void *, LPOVERLAPPED,
                                                        unsigned long *, int);

#if defined(_DLL)
#define SUBPROCESS_DLLIMPORT __declspec(dllimport)
#else
#define SUBPROCESS_DLLIMPORT
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wreserved-identifier"
#endif

SUBPROCESS_DLLIMPORT int __cdecl _fileno(FILE *);
SUBPROCESS_DLLIMPORT int __cdecl _open_osfhandle(subprocess_intptr_t, int);
SUBPROCESS_DLLIMPORT subprocess_intptr_t __cdecl _get_osfhandle(int);

#ifndef __MINGW32__
void *__cdecl _alloca(subprocess_size_t);
#else
#include <malloc.h>
#endif

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#else
typedef size_t subprocess_size_t;
#endif

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wpadded"
#endif
struct subprocess_s {
  FILE *stdin_file;
  FILE *stdout_file;
  FILE *stderr_file;

#if defined(_WIN32)
  void *hProcess;
  void *hStdInput;
  void *hEventOutput;
  void *hEventError;
#else
  pid_t child;
  int return_status;
#endif

  subprocess_size_t alive;
};
#ifdef __clang__
#pragma clang diagnostic pop
#endif

#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunsafe-buffer-usage"
#endif
#endif

#if defined(_WIN32)
subprocess_weak int subprocess_create_named_pipe_helper(void **rd, void **wr);
int subprocess_create_named_pipe_helper(void **rd, void **wr) {
  const unsigned long pipeAccessInbound = 0x00000001;
  const unsigned long fileFlagOverlapped = 0x40000000;
  const unsigned long pipeTypeByte = 0x00000000;
  const unsigned long pipeWait = 0x00000000;
  const unsigned long genericWrite = 0x40000000;
  const unsigned long openExisting = 3;
  const unsigned long fileAttributeNormal = 0x00000080;
  const void *const invalidHandleValue =
      SUBPROCESS_PTR_CAST(void *, ~(SUBPROCESS_CAST(subprocess_intptr_t, 0)));
  struct subprocess_security_attributes_s saAttr = {sizeof(saAttr),
                                                    SUBPROCESS_NULL, 1};
  char name[256] = {0};
  static subprocess_tls long index = 0;
  const long unique = index++;

#if defined(_MSC_VER) && _MSC_VER < 1900
#pragma warning(push, 1)
#pragma warning(disable : 4996)
  _snprintf(name, sizeof(name) - 1,
            "\\\\.\\pipe\\sheredom_subprocess_h.%08lx.%08lx.%ld",
            GetCurrentProcessId(), GetCurrentThreadId(), unique);
#pragma warning(pop)
#else
  snprintf(name, sizeof(name) - 1,
           "\\\\.\\pipe\\sheredom_subprocess_h.%08lx.%08lx.%ld",
           GetCurrentProcessId(), GetCurrentThreadId(), unique);
#endif

  *rd =
      CreateNamedPipeA(name, pipeAccessInbound | fileFlagOverlapped,
                       pipeTypeByte | pipeWait, 1, 4096, 4096, SUBPROCESS_NULL,
                       SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr));

  if (invalidHandleValue == *rd) {
    return -1;
  }

  *wr = CreateFileA(name, genericWrite, SUBPROCESS_NULL,
                    SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr),
                    openExisting, fileAttributeNormal, SUBPROCESS_NULL);

  if (invalidHandleValue == *wr) {
    return -1;
  }

  return 0;
}
#endif

int subprocess_create(const char *const commandLine[], int options,
                      struct subprocess_s *const out_process) {
  return subprocess_create_ex(commandLine, options, SUBPROCESS_NULL,
                              out_process);
}

int subprocess_create_ex(const char *const commandLine[], int options,
                         const char *const environment[],
                         struct subprocess_s *const out_process) {
#if defined(_WIN32)
  int fd;
  void *rd, *wr;
  char *commandLineCombined;
  subprocess_size_t len;
  int i, j;
  int need_quoting;
  unsigned long flags = 0;
  const unsigned long startFUseStdHandles = 0x00000100;
  const unsigned long handleFlagInherit = 0x00000001;
  const unsigned long createNoWindow = 0x08000000;
  struct subprocess_subprocess_information_s processInfo;
  struct subprocess_security_attributes_s saAttr = {sizeof(saAttr),
                                                    SUBPROCESS_NULL, 1};
  char *used_environment = SUBPROCESS_NULL;
  struct subprocess_startup_info_s startInfo = {0,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                0,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL,
                                                SUBPROCESS_NULL};

  startInfo.cb = sizeof(startInfo);
  startInfo.dwFlags = startFUseStdHandles;

  if (subprocess_option_no_window == (options & subprocess_option_no_window)) {
    flags |= createNoWindow;
  }

  if (subprocess_option_inherit_environment !=
      (options & subprocess_option_inherit_environment)) {
    if (SUBPROCESS_NULL == environment) {
      used_environment = SUBPROCESS_CONST_CAST(char *, "\0\0");
    } else {
      // We always end with two null terminators.
      len = 2;

      for (i = 0; environment[i]; i++) {
        for (j = 0; '\0' != environment[i][j]; j++) {
          len++;
        }

        // For the null terminator too.
        len++;
      }

      used_environment = SUBPROCESS_CAST(char *, _alloca(len));

      // Re-use len for the insertion position
      len = 0;

      for (i = 0; environment[i]; i++) {
        for (j = 0; '\0' != environment[i][j]; j++) {
          used_environment[len++] = environment[i][j];
        }

        used_environment[len++] = '\0';
      }

      // End with the two null terminators.
      used_environment[len++] = '\0';
      used_environment[len++] = '\0';
    }
  } else {
    if (SUBPROCESS_NULL != environment) {
      return -1;
    }
  }

  if (!CreatePipe(&rd, &wr, SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr),
                  0)) {
    return -1;
  }

  if (!SetHandleInformation(wr, handleFlagInherit, 0)) {
    return -1;
  }

  fd = _open_osfhandle(SUBPROCESS_PTR_CAST(subprocess_intptr_t, wr), 0);

  if (-1 != fd) {
    out_process->stdin_file = _fdopen(fd, "wb");

    if (SUBPROCESS_NULL == out_process->stdin_file) {
      return -1;
    }
  }

  startInfo.hStdInput = rd;

  if (options & subprocess_option_enable_async) {
    if (subprocess_create_named_pipe_helper(&rd, &wr)) {
      return -1;
    }
  } else {
    if (!CreatePipe(&rd, &wr,
                    SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 0)) {
      return -1;
    }
  }

  if (!SetHandleInformation(rd, handleFlagInherit, 0)) {
    return -1;
  }

  fd = _open_osfhandle(SUBPROCESS_PTR_CAST(subprocess_intptr_t, rd), 0);

  if (-1 != fd) {
    out_process->stdout_file = _fdopen(fd, "rb");

    if (SUBPROCESS_NULL == out_process->stdout_file) {
      return -1;
    }
  }

  startInfo.hStdOutput = wr;

  if (subprocess_option_combined_stdout_stderr ==
      (options & subprocess_option_combined_stdout_stderr)) {
    out_process->stderr_file = out_process->stdout_file;
    startInfo.hStdError = startInfo.hStdOutput;
  } else {
    if (options & subprocess_option_enable_async) {
      if (subprocess_create_named_pipe_helper(&rd, &wr)) {
        return -1;
      }
    } else {
      if (!CreatePipe(&rd, &wr,
                      SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 0)) {
        return -1;
      }
    }

    if (!SetHandleInformation(rd, handleFlagInherit, 0)) {
      return -1;
    }

    fd = _open_osfhandle(SUBPROCESS_PTR_CAST(subprocess_intptr_t, rd), 0);

    if (-1 != fd) {
      out_process->stderr_file = _fdopen(fd, "rb");

      if (SUBPROCESS_NULL == out_process->stderr_file) {
        return -1;
      }
    }

    startInfo.hStdError = wr;
  }

  if (options & subprocess_option_enable_async) {
    out_process->hEventOutput =
        CreateEventA(SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 1, 1,
                     SUBPROCESS_NULL);
    out_process->hEventError =
        CreateEventA(SUBPROCESS_PTR_CAST(LPSECURITY_ATTRIBUTES, &saAttr), 1, 1,
                     SUBPROCESS_NULL);
  } else {
    out_process->hEventOutput = SUBPROCESS_NULL;
    out_process->hEventError = SUBPROCESS_NULL;
  }

  // Combine commandLine together into a single string
  len = 0;
  for (i = 0; commandLine[i]; i++) {
    // for the trailing \0
    len++;

    // Quote the argument if it has a space in it
    if (strpbrk(commandLine[i], "\t\v ") != SUBPROCESS_NULL ||
        commandLine[i][0] == SUBPROCESS_NULL)
      len += 2;

    for (j = 0; '\0' != commandLine[i][j]; j++) {
      switch (commandLine[i][j]) {
      default:
        break;
      case '\\':
        if (commandLine[i][j + 1] == '"') {
          len++;
        }

        break;
      case '"':
        len++;
        break;
      }
      len++;
    }
  }

  commandLineCombined = SUBPROCESS_CAST(char *, _alloca(len));

  if (!commandLineCombined) {
    return -1;
  }

  // Gonna re-use len to store the write index into commandLineCombined
  len = 0;

  for (i = 0; commandLine[i]; i++) {
    if (0 != i) {
      commandLineCombined[len++] = ' ';
    }

    need_quoting = strpbrk(commandLine[i], "\t\v ") != SUBPROCESS_NULL ||
                   commandLine[i][0] == SUBPROCESS_NULL;
    if (need_quoting) {
      commandLineCombined[len++] = '"';
    }

    for (j = 0; '\0' != commandLine[i][j]; j++) {
      switch (commandLine[i][j]) {
      default:
        break;
      case '\\':
        if (commandLine[i][j + 1] == '"') {
          commandLineCombined[len++] = '\\';
        }

        break;
      case '"':
        commandLineCombined[len++] = '\\';
        break;
      }

      commandLineCombined[len++] = commandLine[i][j];
    }
    if (need_quoting) {
      commandLineCombined[len++] = '"';
    }
  }

  commandLineCombined[len] = '\0';

  if (!CreateProcessA(
          SUBPROCESS_NULL,
          commandLineCombined, // command line
          SUBPROCESS_NULL,     // process security attributes
          SUBPROCESS_NULL,     // primary thread security attributes
          1,                   // handles are inherited
          flags,               // creation flags
          used_environment,    // used environment
          SUBPROCESS_NULL,     // use parent's current directory
          SUBPROCESS_PTR_CAST(LPSTARTUPINFOA,
                              &startInfo), // STARTUPINFO pointer
          SUBPROCESS_PTR_CAST(LPPROCESS_INFORMATION, &processInfo))) {
    return -1;
  }

  out_process->hProcess = processInfo.hProcess;

  out_process->hStdInput = startInfo.hStdInput;

  // We don't need the handle of the primary thread in the called process.
  CloseHandle(processInfo.hThread);

  if (SUBPROCESS_NULL != startInfo.hStdOutput) {
    CloseHandle(startInfo.hStdOutput);

    if (startInfo.hStdError != startInfo.hStdOutput) {
      CloseHandle(startInfo.hStdError);
    }
  }

  out_process->alive = 1;

  return 0;
#else
  int stdinfd[2];
  int stdoutfd[2];
  int stderrfd[2];
  pid_t child;
  extern char **environ;
  char *const empty_environment[1] = {SUBPROCESS_NULL};
  posix_spawn_file_actions_t actions;
  char *const *used_environment;

  if (subprocess_option_inherit_environment ==
      (options & subprocess_option_inherit_environment)) {
    if (SUBPROCESS_NULL != environment) {
      return -1;
    }
  }

  if (0 != pipe(stdinfd)) {
    return -1;
  }

  if (0 != pipe(stdoutfd)) {
    return -1;
  }

  if (subprocess_option_combined_stdout_stderr !=
      (options & subprocess_option_combined_stdout_stderr)) {
    if (0 != pipe(stderrfd)) {
      return -1;
    }
  }

  if (environment) {
#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
    used_environment = SUBPROCESS_CONST_CAST(char *const *, environment);
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  } else if (subprocess_option_inherit_environment ==
             (options & subprocess_option_inherit_environment)) {
    used_environment = environ;
  } else {
    used_environment = empty_environment;
  }

  if (0 != posix_spawn_file_actions_init(&actions)) {
    return -1;
  }

  // Close the stdin write end
  if (0 != posix_spawn_file_actions_addclose(&actions, stdinfd[1])) {
    posix_spawn_file_actions_destroy(&actions);
    return -1;
  }

  // Map the read end to stdin
  if (0 !=
      posix_spawn_file_actions_adddup2(&actions, stdinfd[0], STDIN_FILENO)) {
    posix_spawn_file_actions_destroy(&actions);
    return -1;
  }

  // Close the stdout read end
  if (0 != posix_spawn_file_actions_addclose(&actions, stdoutfd[0])) {
    posix_spawn_file_actions_destroy(&actions);
    return -1;
  }

  // Map the write end to stdout
  if (0 !=
      posix_spawn_file_actions_adddup2(&actions, stdoutfd[1], STDOUT_FILENO)) {
    posix_spawn_file_actions_destroy(&actions);
    return -1;
  }

  if (subprocess_option_combined_stdout_stderr ==
      (options & subprocess_option_combined_stdout_stderr)) {
    if (0 != posix_spawn_file_actions_adddup2(&actions, STDOUT_FILENO,
                                              STDERR_FILENO)) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
  } else {
    // Close the stderr read end
    if (0 != posix_spawn_file_actions_addclose(&actions, stderrfd[0])) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
    // Map the write end to stdout
    if (0 != posix_spawn_file_actions_adddup2(&actions, stderrfd[1],
                                              STDERR_FILENO)) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
  }

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wcast-qual"
#pragma clang diagnostic ignored "-Wold-style-cast"
#endif
  if (subprocess_option_search_user_path ==
      (options & subprocess_option_search_user_path)) {
    if (0 != posix_spawnp(&child, commandLine[0], &actions, SUBPROCESS_NULL,
                          SUBPROCESS_CONST_CAST(char *const *, commandLine),
                          used_environment)) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
  } else {
    if (0 != posix_spawn(&child, commandLine[0], &actions, SUBPROCESS_NULL,
                         SUBPROCESS_CONST_CAST(char *const *, commandLine),
                         used_environment)) {
      posix_spawn_file_actions_destroy(&actions);
      return -1;
    }
  }
#ifdef __clang__
#pragma clang diagnostic pop
#endif

  // Close the stdin read end
  close(stdinfd[0]);
  // Store the stdin write end
  out_process->stdin_file = fdopen(stdinfd[1], "wb");

  // Close the stdout write end
  close(stdoutfd[1]);
  // Store the stdout read end
  out_process->stdout_file = fdopen(stdoutfd[0], "rb");

  if (subprocess_option_combined_stdout_stderr ==
      (options & subprocess_option_combined_stdout_stderr)) {
    out_process->stderr_file = out_process->stdout_file;
  } else {
    // Close the stderr write end
    close(stderrfd[1]);
    // Store the stderr read end
    out_process->stderr_file = fdopen(stderrfd[0], "rb");
  }

  // Store the child's pid
  out_process->child = child;

  out_process->alive = 1;

  posix_spawn_file_actions_destroy(&actions);
  return 0;
#endif
}

FILE *subprocess_stdin(const struct subprocess_s *const process) {
  return process->stdin_file;
}

FILE *subprocess_stdout(const struct subprocess_s *const process) {
  return process->stdout_file;
}

FILE *subprocess_stderr(const struct subprocess_s *const process) {
  if (process->stdout_file != process->stderr_file) {
    return process->stderr_file;
  } else {
    return SUBPROCESS_NULL;
  }
}

int subprocess_join(struct subprocess_s *const process,
                    int *const out_return_code) {
#if defined(_WIN32)
  const unsigned long infinite = 0xFFFFFFFF;

  if (process->stdin_file) {
    fclose(process->stdin_file);
    process->stdin_file = SUBPROCESS_NULL;
  }

  if (process->hStdInput) {
    CloseHandle(process->hStdInput);
    process->hStdInput = SUBPROCESS_NULL;
  }

  WaitForSingleObject(process->hProcess, infinite);

  if (out_return_code) {
    if (!GetExitCodeProcess(
            process->hProcess,
            SUBPROCESS_PTR_CAST(unsigned long *, out_return_code))) {
      return -1;
    }
  }

  process->alive = 0;

  return 0;
#else
  int status;

  if (process->stdin_file) {
    fclose(process->stdin_file);
    process->stdin_file = SUBPROCESS_NULL;
  }

  if (process->child) {
    if (process->child != waitpid(process->child, &status, 0)) {
      return -1;
    }

    process->child = 0;

    if (WIFEXITED(status)) {
      process->return_status = WEXITSTATUS(status);
    } else {
      process->return_status = EXIT_FAILURE;
    }

    process->alive = 0;
  }

  if (out_return_code) {
    *out_return_code = process->return_status;
  }

  return 0;
#endif
}

int subprocess_destroy(struct subprocess_s *const process) {
  if (process->stdin_file) {
    fclose(process->stdin_file);
    process->stdin_file = SUBPROCESS_NULL;
  }

  if (process->stdout_file) {
    fclose(process->stdout_file);

    if (process->stdout_file != process->stderr_file) {
      fclose(process->stderr_file);
    }

    process->stdout_file = SUBPROCESS_NULL;
    process->stderr_file = SUBPROCESS_NULL;
  }

#if defined(_WIN32)
  if (process->hProcess) {
    CloseHandle(process->hProcess);
    process->hProcess = SUBPROCESS_NULL;

    if (process->hStdInput) {
      CloseHandle(process->hStdInput);
    }

    if (process->hEventOutput) {
      CloseHandle(process->hEventOutput);
    }

    if (process->hEventError) {
      CloseHandle(process->hEventError);
    }
  }
#endif

  return 0;
}

int subprocess_terminate(struct subprocess_s *const process) {
#if defined(_WIN32)
  unsigned int killed_process_exit_code;
  int success_terminate;
  int windows_call_result;

  killed_process_exit_code = 99;
  windows_call_result =
      TerminateProcess(process->hProcess, killed_process_exit_code);
  success_terminate = (windows_call_result == 0) ? 1 : 0;
  return success_terminate;
#else
  int result;
  result = kill(process->child, 9);
  return result;
#endif
}

unsigned subprocess_read_stdout(struct subprocess_s *const process,
                                char *const buffer, unsigned size) {
#if defined(_WIN32)
  void *handle;
  unsigned long bytes_read = 0;
  struct subprocess_overlapped_s overlapped = {0, 0, {{0, 0}}, SUBPROCESS_NULL};
  overlapped.hEvent = process->hEventOutput;

  handle = SUBPROCESS_PTR_CAST(void *,
                               _get_osfhandle(_fileno(process->stdout_file)));

  if (!ReadFile(handle, buffer, size, &bytes_read,
                SUBPROCESS_PTR_CAST(LPOVERLAPPED, &overlapped))) {
    const unsigned long errorIoPending = 997;
    unsigned long error = GetLastError();

    // Means we've got an async read!
    if (error == errorIoPending) {
      if (!GetOverlappedResult(handle,
                               SUBPROCESS_PTR_CAST(LPOVERLAPPED, &overlapped),
                               &bytes_read, 1)) {
        const unsigned long errorIoIncomplete = 996;
        const unsigned long errorHandleEOF = 38;
        error = GetLastError();

        if ((error != errorIoIncomplete) && (error != errorHandleEOF)) {
          return 0;
        }
      }
    }
  }

  return SUBPROCESS_CAST(unsigned, bytes_read);
#else
  const int fd = fileno(process->stdout_file);
  const ssize_t bytes_read = read(fd, buffer, size);

  if (bytes_read < 0) {
    return 0;
  }

  return SUBPROCESS_CAST(unsigned, bytes_read);
#endif
}

unsigned subprocess_read_stderr(struct subprocess_s *const process,
                                char *const buffer, unsigned size) {
#if defined(_WIN32)
  void *handle;
  unsigned long bytes_read = 0;
  struct subprocess_overlapped_s overlapped = {0, 0, {{0, 0}}, SUBPROCESS_NULL};
  overlapped.hEvent = process->hEventError;

  handle = SUBPROCESS_PTR_CAST(void *,
                               _get_osfhandle(_fileno(process->stderr_file)));

  if (!ReadFile(handle, buffer, size, &bytes_read,
                SUBPROCESS_PTR_CAST(LPOVERLAPPED, &overlapped))) {
    const unsigned long errorIoPending = 997;
    unsigned long error = GetLastError();

    // Means we've got an async read!
    if (error == errorIoPending) {
      if (!GetOverlappedResult(handle,
                               SUBPROCESS_PTR_CAST(LPOVERLAPPED, &overlapped),
                               &bytes_read, 1)) {
        const unsigned long errorIoIncomplete = 996;
        const unsigned long errorHandleEOF = 38;
        error = GetLastError();

        if ((error != errorIoIncomplete) && (error != errorHandleEOF)) {
          return 0;
        }
      }
    }
  }

  return SUBPROCESS_CAST(unsigned, bytes_read);
#else
  const int fd = fileno(process->stderr_file);
  const ssize_t bytes_read = read(fd, buffer, size);

  if (bytes_read < 0) {
    return 0;
  }

  return SUBPROCESS_CAST(unsigned, bytes_read);
#endif
}

int subprocess_alive(struct subprocess_s *const process) {
  int is_alive = SUBPROCESS_CAST(int, process->alive);

  if (!is_alive) {
    return 0;
  }
#if defined(_WIN32)
  {
    const unsigned long zero = 0x0;
    const unsigned long wait_object_0 = 0x00000000L;

    is_alive = wait_object_0 != WaitForSingleObject(process->hProcess, zero);
  }
#else
  {
    int status;
    is_alive = 0 == waitpid(process->child, &status, WNOHANG);

    // If the process was successfully waited on we need to cleanup now.
    if (!is_alive) {
      if (WIFEXITED(status)) {
        process->return_status = WEXITSTATUS(status);
      } else {
        process->return_status = EXIT_FAILURE;
      }

      // Since we've already successfully waited on the process, we need to wipe
      // the child now.
      process->child = 0;

      if (subprocess_join(process, SUBPROCESS_NULL)) {
        return -1;
      }
    }
  }
#endif

  if (!is_alive) {
    process->alive = 0;
  }

  return is_alive;
}

#if defined(__clang__)
#if __has_warning("-Wunsafe-buffer-usage")
#pragma clang diagnostic pop
#endif
#endif

#if defined(__cplusplus)
} // extern "C"
#endif

#endif /* SHEREDOM_SUBPROCESS_H_INCLUDED */



/*
*                          src/os.h
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

typedef struct os_fs_stat_s
{
    union
    {
        Exc error;
        time_t mtime;
    };
    usize is_valid : 1;
    usize is_directory : 1;
    usize is_symlink : 1;
    usize is_file : 1;
    usize is_other : 1;
} os_fs_stat_s;
_Static_assert(sizeof(os_fs_stat_s) == sizeof(usize) * 2, "size?");

typedef Exception os_fs_dir_walk_f(const char* path, os_fs_stat_s ftype, void* user_ctx);


#ifdef _WIN32
#define os$PATH_SEP '\\'
#else
#define os$PATH_SEP '/'
#endif

#if defined(CEXBUILD) && CEX_LOG_LVL > 3
#define _os$args_print(msg, args, args_len)                                                        \
    log$debug(msg "");                                                                             \
    for (u32 i = 0; i < args_len - 1; i++) {                                                       \
        const char* a = args[i];                                                                   \
        io.printf(" ");                                                                            \
        if (str.find(a, " ")) {                                                                    \
            io.printf("\'%s\'", a);                                                                \
        } else if (a == NULL || *a == '\0') {                                                      \
            io.printf("\'(empty arg)\'");                                                          \
        } else {                                                                                   \
            io.printf("%s", a);                                                                    \
        }                                                                                          \
    }                                                                                              \
    io.printf("\n");
#else
#define _os$args_print(msg, args, args_len)
#endif

#define os$cmda(args, args_len...)                                                                 \
    ({                                                                                             \
        /* NOLINTBEGIN */                                                                          \
        _Static_assert(sizeof(args) > 0, "You must pass at least one item");                       \
        usize _args_len_va[] = { args_len };                                                       \
        (void)_args_len_va;                                                                        \
        usize _args_len = (sizeof(_args_len_va) > 0) ? _args_len_va[0] : arr$len(args);            \
        uassert(_args_len < PTRDIFF_MAX && "negative length or overflow");                         \
        _os$args_print("CMD:", args, _args_len);                                                   \
        os_cmd_c _cmd = { 0 };                                                                     \
        Exc result = os.cmd.run((const char**)args, _args_len, &_cmd);                             \
        if (result == EOK) {                                                                       \
            result = os.cmd.join(&_cmd, 0, NULL);                                                   \
        };                                                                                         \
        result;                                                                                    \
        /* NOLINTEND */                                                                            \
    })

#define os$cmd(args...)                                                                            \
    ({                                                                                             \
        const char* _args[] = { args, NULL };                                                      \
        usize _args_len = arr$len(_args);                                                          \
        os$cmda(_args, _args_len);                                                                 \
    })

#define os$path_join(allocator, path_parts...)                                                     \
    ({                                                                                             \
        const char* _args[] = { path_parts };                                                      \
        usize _args_len = arr$len(_args);                                                          \
        os.path.join(_args, _args_len, allocator);                                                 \
    })

struct __cex_namespace__os
{
    // Autogenerated by CEX
    // clang-format off

void            (*sleep)(u32 period_millisec);

struct {  // sub-module .fs >>>
    Exception       (*dir_walk)(const char* path, bool is_recursive, os_fs_dir_walk_f callback_fn, void* user_ctx);
    arr$(char*)     (*find)(const char* path, bool is_recursive, IAllocator allc);
    Exception       (*getcwd)(sbuf_c* out);
    Exception       (*mkdir)(const char* path);
    Exception       (*mkpath)(const char* path);
    Exception       (*remove)(const char* path);
    Exception       (*remove_tree)(const char* path);
    Exception       (*rename)(const char* old_path, const char* new_path);
    os_fs_stat_s    (*stat)(const char* path);
} fs;  // sub-module .fs <<<

struct {  // sub-module .env >>>
    const char*     (*get)(const char* name, const char* deflt);
    void            (*set)(const char* name, const char* value, bool overwrite);
    void            (*unset)(const char* name);
} env;  // sub-module .env <<<

struct {  // sub-module .path >>>
    char*           (*basename)(const char* path, IAllocator allc);
    char*           (*dirname)(const char* path, IAllocator allc);
    bool            (*exists)(const char* file_path);
    char*           (*join)(const char** parts, u32 parts_len, IAllocator allc);
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
__attribute__((visibility("hidden"))) extern const struct __cex_namespace__os os;



/*
*                          src/test.h
*/

typedef Exception (*_cex_test_case_f)(void);

#define CEXTEST_AMSG_MAX_LEN 512
struct _cex_test_case_s
{
    _cex_test_case_f test_fn;
    const char* test_name;
    u32 test_line;
};

struct _cex_test_context_s
{
    arr$(struct _cex_test_case_s) test_cases;
    int orig_stderr_fd;    // initial stdout
    int orig_stdout_fd;    // initial stderr
    FILE* out_stream;      // test case captured output
    int tests_run;         // number of tests run
    int tests_failed;      // number of tests failed
    bool quiet_mode;       // quiet mode (for run all)
    const char* case_name; // current running case name
    _cex_test_case_f setup_case_fn;
    _cex_test_case_f teardown_case_fn;
    _cex_test_case_f setup_suite_fn;
    _cex_test_case_f teardown_suite_fn;
    bool has_ansi;
    bool no_stdout_capture;
    bool breakpoint;
    const char* const suite_file;
    char* case_filter;
    char str_buf[CEXTEST_AMSG_MAX_LEN];
};

#if defined(__clang__)
#define test$NOOPT __attribute__((optnone))
#elif defined(__GNUC__) || defined(__GNUG__)
#define test$NOOPT __attribute__((optimize("O0")))
#elif defined(_MSC_VER)
#error "MSVC deprecated"
#endif

#define _test$log_err(msg) __FILE__ ":" cex$stringize(__LINE__) " -> " msg
#define _test$tassert_breakpoint()                                                                 \
    ({                                                                                             \
        if (_cex_test__mainfn_state.breakpoint) {                                                  \
            fprintf(stderr, "[BREAK] %s\n", _test$log_err("breakpoint hit"));                      \
            raise(SIGTRAP);                                                                        \
        }                                                                                          \
    })


#define test$case(NAME)                                                                             \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                      \
    static Exception cext_test_##NAME();                                                            \
    static void cext_test_register_##NAME(void) __attribute__((constructor));                       \
    static void cext_test_register_##NAME(void)                                                     \
    {                                                                                               \
        if (_cex_test__mainfn_state.test_cases == NULL) {                                           \
            _cex_test__mainfn_state.test_cases = arr$new(_cex_test__mainfn_state.test_cases, mem$); \
            uassert(_cex_test__mainfn_state.test_cases != NULL && "memory error");                  \
        };                                                                                          \
        arr$push(                                                                                   \
            _cex_test__mainfn_state.test_cases,                                                     \
            (struct _cex_test_case_s                                                                \
            ){ .test_fn = &cext_test_##NAME, .test_name = #NAME, .test_line = __LINE__ }            \
        );                                                                                          \
    }                                                                                               \
    Exception test$NOOPT cext_test_##NAME(void)


#ifndef CEXTEST
#define test$env_check()                                                                           \
    fprintf(stderr, "CEXTEST was not defined, pass -DCEXTEST or #define CEXTEST");                 \
    exit(1);
#else
#define test$env_check() (void)0
#endif

#define test$main()                                                                                \
    struct _cex_test_context_s _cex_test__mainfn_state = { .suite_file = __FILE__ };               \
    int main(int argc, char** argv)                                                                \
    {                                                                                              \
        test$env_check();                                                                          \
        argv[0] = __FILE__;                                                                        \
        return cex_test_main_fn(argc, argv);                                                       \
    }

#define test$setup_suite()                                                                         \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                     \
    static Exception cext_test__setup_suite_fn();                                                  \
    static void cext_test__register_setup_suite_fn(void) __attribute__((constructor));             \
    static void cext_test__register_setup_suite_fn(void)                                           \
    {                                                                                              \
        uassert(_cex_test__mainfn_state.setup_suite_fn == NULL);                                   \
        _cex_test__mainfn_state.setup_suite_fn = &cext_test__setup_suite_fn;                       \
    }                                                                                              \
    Exception test$NOOPT cext_test__setup_suite_fn(void)

#define test$teardown_suite()                                                                      \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                     \
    static Exception cext_test__teardown_suite_fn();                                               \
    static void cext_test__register_teardown_suite_fn(void) __attribute__((constructor));          \
    static void cext_test__register_teardown_suite_fn(void)                                        \
    {                                                                                              \
        uassert(_cex_test__mainfn_state.teardown_suite_fn == NULL);                                \
        _cex_test__mainfn_state.teardown_suite_fn = &cext_test__teardown_suite_fn;                 \
    }                                                                                              \
    Exception test$NOOPT cext_test__teardown_suite_fn(void)

#define test$setup_case()                                                                          \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                     \
    static Exception cext_test__setup_case_fn();                                                   \
    static void cext_test__register_setup_case_fn(void) __attribute__((constructor));              \
    static void cext_test__register_setup_case_fn(void)                                            \
    {                                                                                              \
        uassert(_cex_test__mainfn_state.setup_case_fn == NULL);                                    \
        _cex_test__mainfn_state.setup_case_fn = &cext_test__setup_case_fn;                         \
    }                                                                                              \
    Exception test$NOOPT cext_test__setup_case_fn(void)

#define test$teardown_case()                                                                       \
    extern struct _cex_test_context_s _cex_test__mainfn_state;                                     \
    static Exception cext_test__teardown_case_fn();                                                \
    static void cext_test__register_teardown_case_fn(void) __attribute__((constructor));           \
    static void cext_test__register_teardown_case_fn(void)                                         \
    {                                                                                              \
        uassert(_cex_test__mainfn_state.teardown_case_fn == NULL);                                 \
        _cex_test__mainfn_state.teardown_case_fn = &cext_test__teardown_case_fn;                   \
    }                                                                                              \
    Exception test$NOOPT cext_test__teardown_case_fn(void)

#define _test$tassert_fn(a, b)                                                                     \
    ({                                                                                             \
        _Generic(                                                                                  \
            (a),                                                                                   \
            i32: _check_eq_int,                                                                    \
            u32: _check_eq_int,                                                                    \
            i64: _check_eq_int,                                                                    \
            u64: _check_eq_int,                                                                    \
            i16: _check_eq_int,                                                                    \
            u16: _check_eq_int,                                                                    \
            i8: _check_eq_int,                                                                     \
            u8: _check_eq_int,                                                                     \
            char: _check_eq_int,                                                                   \
            bool: _check_eq_int,                                                                   \
            char*: _check_eq_str,                                                                  \
            const char*: _check_eq_str,                                                            \
            str_s: _check_eqs_slice,                                                               \
            f32: _check_eq_f32,                                                                    \
            f64: _check_eq_f32,                                                                    \
            default: _check_eq_int                                                                 \
        );                                                                                         \
    })

#define tassert(A)                                                                                 \
    ({ /* ONLY for test$case USE */                                                                \
       if (!(A)) {                                                                                 \
           _test$tassert_breakpoint();                                                             \
           return _test$log_err(#A);                                                               \
       }                                                                                           \
    })

#define tassertf(A, M, ...)                                                                        \
    ({ /* ONLY for test$case USE */                                                                \
       if (!(A)) {                                                                                 \
           _test$tassert_breakpoint();                                                             \
           if (str.sprintf(                                                                        \
                   _cex_test__mainfn_state.str_buf,                                                \
                   CEXTEST_AMSG_MAX_LEN - 1,                                                       \
                   _test$log_err(M),                                                               \
                   ##__VA_ARGS__                                                                   \
               )) {                                                                                \
           }                                                                                       \
           return _cex_test__mainfn_state.str_buf;                                                 \
       }                                                                                           \
    })
#define tassert_eq(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__eq))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

#define tassert_er(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        if ((cex$tmpname(err) = _check_eq_err((a), (b), __LINE__))) {                              \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

#define tassert_eq_almost(a, b, delta)                                                             \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        if ((cex$tmpname(err) = _check_eq_almost((a), (b), (delta), __LINE__))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })
#define tassert_eq_ptr(a, b)                                                                       \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        if ((cex$tmpname(err) = _check_eq_ptr((a), (b), __LINE__))) {                              \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })
#define tassert_eq_mem(a, b...)                                                                    \
    ({                                                                                             \
        var _a = (a);                                                                              \
        var _b = (b);                                                                              \
        _Static_assert(                                                                            \
            __builtin_types_compatible_p(__typeof__(_a), __typeof__(_b)),                          \
            "incompatible"                                                                         \
        );                                                                                         \
        _Static_assert(sizeof(_a) == sizeof(_b), "different size");                                \
        if (memcmp(&_a, &_b, sizeof(_a)) != 0) {                                                   \
            _test$tassert_breakpoint();                                                            \
            if (str.sprintf(                                                                       \
                    _cex_test__mainfn_state.str_buf,                                               \
                    CEXTEST_AMSG_MAX_LEN - 1,                                                      \
                    _test$log_err("a and b are not binary equal")                                  \
                )) {                                                                               \
            }                                                                                      \
            return _cex_test__mainfn_state.str_buf;                                                \
        }                                                                                          \
    })

#define tassert_eq_arr(a, b...)                                                                    \
    ({                                                                                             \
        var _a = (a);                                                                              \
        var _b = (b);                                                                              \
        _Static_assert(                                                                            \
            __builtin_types_compatible_p(__typeof__(*a), __typeof__(*b)),                          \
            "incompatible"                                                                         \
        );                                                                                         \
        _Static_assert(sizeof(*_a) == sizeof(*_b), "different size");                              \
        usize _alen = arr$len(a);                                                                  \
        usize _blen = arr$len(b);                                                                  \
        usize _itsize = sizeof(*_a);                                                               \
        if (_alen != _blen) {                                                                      \
            _test$tassert_breakpoint();                                                            \
            if (str.sprintf(                                                                       \
                    _cex_test__mainfn_state.str_buf,                                               \
                    CEXTEST_AMSG_MAX_LEN - 1,                                                      \
                    _test$log_err("array length is different %ld != %ld"),                         \
                    _alen,                                                                         \
                    _blen                                                                          \
                )) {                                                                               \
            }                                                                                      \
            return _cex_test__mainfn_state.str_buf;                                                \
        } else {                                                                                   \
            for (usize i = 0; i < _alen; i++) {                                                    \
                if (memcmp(&(_a[i]), &(_b[i]), _itsize) != 0) {                                    \
                    _test$tassert_breakpoint();                                                    \
                    if (str.sprintf(                                                               \
                            _cex_test__mainfn_state.str_buf,                                       \
                            CEXTEST_AMSG_MAX_LEN - 1,                                              \
                            _test$log_err("array element at index [%d] is different"),             \
                            i                                                                      \
                        )) {                                                                       \
                    }                                                                              \
                    return _cex_test__mainfn_state.str_buf;                                        \
                }                                                                                  \
            }                                                                                      \
        }                                                                                          \
    })

#define tassert_ne(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__ne))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

#define tassert_le(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__le))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

#define tassert_lt(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__lt))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

#define tassert_ge(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__ge))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })

#define tassert_gt(a, b)                                                                           \
    ({                                                                                             \
        Exc cex$tmpname(err) = NULL;                                                               \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((cex$tmpname(err) = genf((a), (b), __LINE__, _cex_test_eq_op__gt))) {                  \
            _test$tassert_breakpoint();                                                            \
            return cex$tmpname(err);                                                               \
        }                                                                                          \
    })



/*
*                          src/cex_code_gen.h
*/
#ifdef CEXBUILD

typedef struct cex_codegen_s
{
    sbuf_c* buf;
    u32 indent;
    Exc error;
} cex_codegen_s;

/*
 *                  CODE GEN MACROS
 */
#define cg$var __cex_code_gen
#define cg$init(out_sbuf)                                                                          \
    cex_codegen_s cex$tmpname(code_gen) = { .buf = (out_sbuf) };                                   \
    cex_codegen_s* cg$var = &cex$tmpname(code_gen)
#define cg$is_valid() (cg$var != NULL && cg$var->buf != NULL && cg$var->error == EOK)
#define cg$indent() ({ cg$var->indent += 4; })
#define cg$dedent()                                                                                \
    ({                                                                                             \
        if (cg$var->indent >= 4)                                                                   \
            cg$var->indent -= 4;                                                                   \
    })

/*
 *                  CODE MACROS
 */
#define $pn(text)                                                                                  \
    ((text && text[0] == '\0') ? cex_codegen_print_line(cg$var, "\n")                              \
                               : cex_codegen_print_line(cg$var, "%s\n", text))
#define $pf(format, ...) cex_codegen_print_line(cg$var, format "\n", __VA_ARGS__)
#define $pa(format, ...) cex_codegen_print(cg$var, true, format, __VA_ARGS__)

// clang-format off
#define $scope(format, ...) \
    for (cex_codegen_s* cex$tmpname(codegen_scope)  \
                __attribute__ ((__cleanup__(cex_codegen_print_scope_exit))) =  \
                cex_codegen_print_scope_enter(cg$var, format, __VA_ARGS__),\
        *cex$tmpname(codegen_sentinel) = cg$var;\
        cex$tmpname(codegen_sentinel); \
        cex$tmpname(codegen_sentinel) = NULL)
#define $func(format, ...) $scope(format, __VA_ARGS__)
// clang-format on


#define $if(format, ...) $scope("if (" format ") ", __VA_ARGS__)
#define $elseif(format, ...)                                                                       \
    $pa(" else ", "");                                                                             \
    $if(format, __VA_ARGS__)
#define $else()                                                                                    \
    $pa(" else", "");                                                                              \
    $scope(" ", "")


#define $while(format, ...) $scope("while (" format ") ", __VA_ARGS__)
#define $for(format, ...) $scope("for (" format ") ", __VA_ARGS__)
#define $foreach(format, ...) $scope("for$each (" format ") ", __VA_ARGS__)


#define $switch(format, ...) $scope("switch (" format ") ", __VA_ARGS__)
#define $case(format, ...)                                                                         \
    for (cex_codegen_s * cex$tmpname(codegen_scope)                                                \
                             __attribute__((__cleanup__(cex_codegen_print_case_exit))) =           \
             cex_codegen_print_case_enter(cg$var, "case " format, __VA_ARGS__),                    \
                             *cex$tmpname(codegen_sentinel) = cg$var;                              \
         cex$tmpname(codegen_sentinel);                                                            \
         cex$tmpname(codegen_sentinel) = NULL)
#define $default()                                                                                 \
    for (cex_codegen_s * cex$tmpname(codegen_scope)                                                \
                             __attribute__((__cleanup__(cex_codegen_print_case_exit))) =           \
             cex_codegen_print_case_enter(cg$var, "default", NULL),                                \
                             *cex$tmpname(codegen_sentinel) = cg$var;                              \
         cex$tmpname(codegen_sentinel);                                                            \
         cex$tmpname(codegen_sentinel) = NULL)


void cex_codegen_print_line(cex_codegen_s* cg, const char* format, ...);
void cex_codegen_print(cex_codegen_s* cg, bool rep_new_line, const char* format, ...);
cex_codegen_s* cex_codegen_print_scope_enter(cex_codegen_s* cg, const char* format, ...);
void cex_codegen_print_scope_exit(cex_codegen_s** cgptr);
cex_codegen_s* cex_codegen_print_case_enter(cex_codegen_s* cg, const char* format, ...);
void cex_codegen_print_case_exit(cex_codegen_s** cgptr);
void cex_codegen_indent(cex_codegen_s* cg);

#endif // ifdef CEXBUILD



/*
*                          src/cexy.h
*/

#if defined(CEXBUILD)

#ifndef cexy$cc
#if defined(__clang__)
#define cexy$cc "clang"
#elif defined(__GNUC__)
#define cexy$cc "gcc"
#else
# #error "Compiler type is not supported"
#endif
#endif // #ifndef cexy$cc

#ifndef cexy$cc_include
#define cexy$cc_include "-I."
#endif

#ifndef cexy$build_dir
#define cexy$build_dir "build/"
#endif

#ifndef cexy$cc_args
#define cexy$cc_args "-Wall", "-Wextra"
#endif

#ifndef cexy$ld_args
#define cexy$ld_args ""
#endif

#ifndef cexy$ld_libs
#define cexy$ld_libs ""
#endif

#ifndef cexy$debug_cmd
#define cexy$debug_cmd "gdb", "--args"
#endif


#ifndef cexy$process_ignore_kw
/**
@brief For ignoring extra macro keywords in function signatures of libraries, as str.match() pattern
string
*/
#define cexy$process_ignore_kw ""
#endif

#ifndef cexy$cc_args_test
#define cexy$cc_args_test                                                                          \
    "-DCEXTEST", "-Wall", "-Wextra", "-Werror", "-Wno-unused-function", "-g3", "-Itests/",         \
        "-fsanitize-address-use-after-scope", "-fsanitize=address", "-fsanitize=undefined",        \
        "-fsanitize=leak", "-fstack-protector-strong"

#endif

#define cexy$cmd_process                                                                           \
    { .name = "process",                                                                           \
      .func = cexy.cmd.process,                                                                    \
      .help = "Creates CEX namespaces from project source code" }


#define cexy$cmd_all cexy$cmd_process

#define cexy$initialize() cexy.build_self(argc, argv, __FILE__)

#define cexy$description "Cex build system"
/* clang-format off */
#define cexy$epilog                                                                                \
    "\nList of #define cexy$* variables used in build system (you may redefine them)\n"            \
    "* cexy$build_dir            " cexy$build_dir "\n"                                             \
    "* cexy$cc                   " cexy$cc "\n"                                                    \
    "* cexy$cc_include           " cex$stringize(cexy$cc_include) "\n"                             \
    "* cexy$cc_args              " cex$stringize(cexy$cc_args) "\n"                                \
    "* cexy$cc_args_test         " cex$stringize(cexy$cc_args_test) "\n"                           \
    "* cexy$ld_args              " cex$stringize(cexy$ld_args) "\n"                                \
    "* cexy$ld_libs              " cex$stringize(cexy$ld_libs) "\n"                                \
    "* cexy$debug_cmd            " cex$stringize(cexy$debug_cmd) "\n"                              \
    "* cexy$process_ignore_kw    " cex$stringize(cexy$process_ignore_kw) "\n"
/* clang-format off */
struct __cex_namespace__cexy {
    // Autogenerated by CEX
    // clang-format off

    void            (*build_self)(int argc, char** argv, const char* cex_source);
    bool            (*src_changed)(const char* target_path, arr$(char*) src_array);
    bool            (*src_include_changed)(const char* target_path, const char* src_path, arr$(char*) alt_include_path);
    char*           (*target_make)(const char* src_path, const char* build_dir, const char* name_or_extension, IAllocator allocator);

    struct {
        Exception       (*process)(int argc, char** argv, void* user_ctx);
    } cmd;

    struct {
        Exception       (*clean)(const char* target);
        Exception       (*create)(const char* target);
        Exception       (*make_target_pattern)(const char** target);
        Exception       (*run)(const char* target, bool is_debug, int argc, char** argv);
    } test;

    // clang-format on
};
__attribute__((visibility("hidden"))) extern const struct __cex_namespace__cexy cexy;

#endif // #if defined(CEXBUILD)



/*
*                          src/CexParser.h
*/

#define CexTknList                                                                                 \
    X(eof)                                                                                         \
    X(unk)                                                                                         \
    X(error)                                                                                       \
    X(ident)                                                                                       \
    X(number)                                                                                      \
    X(string)                                                                                      \
    X(char)                                                                                        \
    X(comment_single)                                                                              \
    X(comment_multi)                                                                               \
    X(preproc)                                                                                     \
    X(lparen)                                                                                      \
    X(rparen)                                                                                      \
    X(lbrace)                                                                                      \
    X(rbrace)                                                                                      \
    X(lbracket)                                                                                    \
    X(rbracket)                                                                                    \
    X(bracket_block)                                                                               \
    X(brace_block)                                                                                 \
    X(paren_block)                                                                                 \
    X(star)                                                                                        \
    X(dot)                                                                                         \
    X(comma)                                                                                       \
    X(eq)                                                                                          \
    X(colon)                                                                                       \
    X(question)                                                                                    \
    X(eos)                                                                                         \
    X(typedef)                                                                                     \
    X(func_decl)                                                                                   \
    X(func_def)                                                                                    \
    X(macro_const)                                                                                 \
    X(macro_func)                                                                                  \
    X(var_decl)                                                                                    \
    X(var_def)                                                                                     \
    X(cex_module_struct)                                                                           \
    X(cex_module_decl)                                                                             \
    X(cex_module_def)                                                                              \
    X(global_misc)                                                                                 \
    X(count)

typedef enum CexTkn_e
{
#define X(name) CexTkn__##name,
    CexTknList
#undef X
} CexTkn_e;

static const char* CexTkn_str[] = {
#define X(name) cex$stringize(name),
    CexTknList
#undef X
};

typedef struct cex_token_s
{
    CexTkn_e type;
    str_s value;
} cex_token_s;

typedef struct CexParser_c
{
    char* content;
    char* cur;
    char* content_end;
    u32 line;
    bool fold_scopes;
} CexParser_c;

typedef struct cex_decl_s
{
    str_s name;      // function/macro/const/var name
    str_s docs;      // reference to closest /// or /** block
    str_s body;      // reference to code {} if applicable
    sbuf_c ret_type; // refined return type
    sbuf_c args;     // refined argument list
    CexTkn_e type;   // decl type (typedef, func, macro, etc)
    bool is_static;
    bool is_inline;
} cex_decl_s;


__attribute__((visibility("hidden"))) extern const struct __cex_namespace__CexParser CexParser;

struct __cex_namespace__CexParser {
    // Autogenerated by CEX
    // clang-format off
    
    CexParser_c     (*create)(char* content, u32 content_len, bool fold_scopes);
    void            (*decl_free)(cex_decl_s* decl, IAllocator alloc);
    cex_decl_s*     (*decl_parse)(cex_token_s decl_token, arr$(cex_token_s) children, const char* ignore_keywords_pattern, IAllocator alloc);
    cex_token_s     (*next_entity)(CexParser_c* lx, arr$(cex_token_s)* children);
    cex_token_s     (*next_token)(CexParser_c* lx);
    void            (*reset)(CexParser_c* lx);
    
    // clang-format on
};



/*
*                   CEX IMPLEMENTATION 
*/



#ifdef CEX_IMPLEMENTATION



/*
*                          src/cex_base.c
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

void
__cex__panic(void)
{
    fflush(stdout);
    fflush(stderr);
    sanitizer_stack_trace();

#ifdef CEXTEST
    raise(SIGTRAP);
#else
    abort();
#endif
    return;
}



/*
*                          src/mem.c
*/

void
_cex_allocator_memscope_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    (*allc)->scope_exit(*allc);
}



/*
*                          src/AllocatorHeap.c
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
    (void)a;

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
    (void)a;

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
        (void)a;

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
*                          src/AllocatorArena.c
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

const struct __cex_namespace__AllocatorArena AllocatorArena = {
    // Autogenerated by CEX
    // clang-format off
    .create = AllocatorArena_create,
    .sanitize = AllocatorArena_sanitize,
    .destroy = AllocatorArena_destroy,
    // clang-format on
};



/*
*                          src/ds.c
*/


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

inline usize
cexds_arr_len(const void* arr)
{
    return (arr) ? cexds_header(arr)->length : 0;
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
*                          src/_sprintf.c
*/
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
                // %ld/%lld - is always 64 bits 
                fl |= CEXSP__INTMAX;
                ++f;
                if (f[0] == 'l') {
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
                        n64 = (_i64 != INT64_MIN) ? (u64)-_i64 : INT64_MIN;
                        fl |= CEXSP__NEGATIVE;
                    }
                } else {
                    i32 i = va_arg(va, i32);
                    n64 = (u32)i;
                    if ((f[0] != 'u') && (i < 0)) {
                        n64 =  (i != INT32_MIN) ? (u32)-i : INT32_MIN;
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
*                          src/str.c
*/

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

/**
 * @brief Creates string slice of input c-str (NULL tolerant, (str_s){0} on error)
 *
 * @param ccharptr pointer to a string
 * @return 
 */
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
        return a == b;
    }
    return strcmp(a, b) == 0;
}

bool
str_eqi(const char* a, const char* b)
{
    if (unlikely(a == NULL || b == NULL)) {
        return a == b;
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
        return (a.buf == NULL && b.buf == NULL);
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
        // NOTE: even with overflow, data is truncated and written to the dest + null term
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

static isize
str__slice__index_of(str_s str, str_s needle)
{
    if (unlikely(!str.buf || !needle.buf || needle.len == 0 || needle.len > str.len)) {
        return -1;
    }
    if (unlikely(needle.len == 1)) {
        char n = needle.buf[0];
        for (usize i = 0; i < str.len; i++) { 
            if (str.buf[i] == n) {
                return i;
            }
        }
    } else {
        for (usize i = 0; i <= str.len - needle.len; i++) {
            if (memcmp(&str.buf[i], needle.buf, needle.len) == 0) {
                return i;
            }
        }
    }
    return -1;
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

static str_s
str__slice__iter_split(str_s s, const char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        usize cursor;
        usize split_by_len;
        usize str_len;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) <= alignof(usize), "cex_iterator_s _ctx misalign");

    if (unlikely(!iterator->initialized)) {
        iterator->initialized = 1;
        // First run handling
        if (unlikely(!str__isvalid(&s) || s.len == 0)) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }
        ctx->split_by_len = strlen(split_by);

        if (ctx->split_by_len == 0) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        isize idx = str__index(&s, split_by, ctx->split_by_len);
        if (idx < 0) {
            idx = s.len;
        }
        ctx->cursor = idx;
        iterator->idx.i = 0;
        if (idx == 0) {
            // first line is \n
            return (str_s){.buf = "", .len = 0};
        } else {
            return str.slice.sub(s, 0, idx);
        }
    } else {
        if (ctx->cursor >= s.len) {
            iterator->stopped = 1;
            return (str_s){ 0 };
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == s.len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            iterator->idx.i++;
            return (str_s){ .buf = "", .len = 0 };
        }

        // Get remaining string after prev split_by char
        str_s tok = str.slice.sub(s, ctx->cursor, 0);
        isize idx = str__index(&tok, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->cursor = s.len;
            // iterator->stopped = 1;
            return tok;
        } else if (idx == 0) {
            return (str_s){ .buf = "", .len = 0 };
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->cursor += idx;
            return str.slice.sub(tok, 0, idx);
        }
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
        char* tok = str__slice__clone(it.val, allc);
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
str_join(const char** str_arr, usize str_arr_len, const char* join_by, IAllocator allc)
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
    for$each(s, str_arr, str_arr_len)
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


static bool
_str_match(const char* str, isize str_len, const char* pattern)
{
    if (unlikely(str == NULL || str_len <= 0)) {
        return false;
    }
    uassert(pattern && "null pattern");

    while (*pattern != '\0') {
        switch (*pattern) {
            case '*':
                while (*pattern == '*') {
                    pattern++;
                }

                if (!*pattern) {
                    return true;
                }

                if (*pattern != '?' && *pattern != '[' && *pattern != '\\') {
                    while (str_len > 0 && *pattern != *str) {
                        str++;
                        str_len--;
                    }
                }

                while (str_len > 0) {
                    if (_str_match(str, str_len, pattern)) {
                        return true;
                    }
                    str++;
                    str_len--;
                }
                return false;

            case '?':
                if (str_len == 0) {
                    return false; // '?' requires a character
                }
                str++;
                str_len--;
                pattern++;
                break;

            case '(': {
                const char* strstart = str;
                isize str_len_start = str_len;
                if (unlikely(*(pattern + 1) == ')')) {
                    uassert(false && "Empty '()' group");
                    return false;
                }

                while (str_len_start > 0) {
                    pattern++;
                    str = strstart;
                    str_len = str_len_start;
                    bool matched = false;
                    while (*pattern != '\0') {
                        if (unlikely(*pattern == '\\')) {
                            pattern++;
                            if (unlikely(*pattern == '\0')) {
                                uassert(false && "Unterminated \\ sequence inside '()' group");
                                return false;
                            }
                        }
                        if (*pattern == *str) {
                            matched = true;
                        } else {
                            while (*pattern != '|' && *pattern != ')' && *pattern != '\0') {
                                matched = false;
                                pattern++;
                            }
                            break;
                        }
                        pattern++;
                        str++;
                        str_len--;
                    }
                    if (*pattern == '|') {
                        if (!matched) {
                            continue;
                        }
                        // we have found the match, just skip to the end of group
                        while (*pattern != ')' && *pattern != '\0') {
                            pattern++;
                        }
                    }

                    if (unlikely(*pattern != ')')) {
                        uassert(false && "Invalid pattern - no closing ')'");
                        return false;
                    }

                    pattern++;
                    if (!matched) {
                        return false;
                    } else {
                        // All good find next pattern
                        break; // while (*str != '\0') {
                    }
                }
                break;
            }
            case '[': {
                const char* pstart = pattern;
                while (str_len > 0) {
                    bool negate = false;
                    bool repeating = false;
                    pattern = pstart + 1;

                    if (unlikely(*pattern == '!')) {
                        uassert(*(pattern + 1) != ']' && "expected some chars after [!..]");
                        negate = true;
                        pattern++;
                    }

                    bool matched = false;
                    while (*pattern != ']' && *pattern != '\0') {
                        if (*(pattern + 1) == '-' && *(pattern + 2) != ']' &&
                            *(pattern + 2) != '\0') {
                            // Handle character ranges like a-zA-Z0-9
                            uassertf(
                                *pattern < *(pattern + 2),
                                "pattern [n-m] sequence, n must be less than m: [%c-%c]",
                                *pattern,
                                *(pattern + 2)
                            );
                            if (*str >= *pattern && *str <= *(pattern + 2)) {
                                matched = true;
                            }
                            pattern += 3;
                        } else if (*pattern == '\\') {
                            // Escape sequence
                            pattern++;
                            if (*pattern != '\0') {
                                if (*pattern == *str) {
                                    matched = true;
                                }
                                pattern++;
                            }
                        } else {
                            if (unlikely(*pattern == '+' && *(pattern + 1) == ']')) {
                                // repeating group [a-z+]@, match all cases until @
                                repeating = true;
                            } else {
                                if (*pattern == *str) {
                                    matched = true;
                                }
                            }
                            pattern++;
                        }
                    }

                    if (unlikely(*pattern != ']')) {
                        uassert(false && "Invalid pattern - no closing ']'");
                        return false;
                    } else {
                        pattern++;
                        if (matched == negate) {
                            if (repeating) {
                                // We have not matched char, but it may match to next pattern
                                break;
                            }
                            return false;
                        }

                        str++;
                        str_len--;
                        if (!repeating) {
                            break; // while (*str != '\0') {
                        }
                    }
                }

                if (str_len == 0) {
                    return *str == *pattern;
                }
                break;
            }

            case '\\':
                // Escape next character
                pattern++;
                if (*pattern == '\0') {
                    return false;
                }
                fallthrough();

            default:
                if (*pattern != *str) {
                    return false;
                }
                str++;
                str_len--;
                pattern++;
        }
    }

    return str_len == 0;
}

static bool
str__slice__match(str_s s, const char* pattern)
{
    return _str_match(s.buf, s.len, pattern);
}
static bool
str_match(const char* s, const char* pattern)
{
    return _str_match(s, str.len(s), pattern);
}

const struct __cex_namespace__str str = {
    // Autogenerated by CEX
    // clang-format off

    .clone = str_clone,
    .copy = str_copy,
    .ends_with = str_ends_with,
    .eq = str_eq,
    .eqi = str_eqi,
    .find = str_find,
    .findr = str_findr,
    .fmt = str_fmt,
    .join = str_join,
    .len = str_len,
    .lower = str_lower,
    .match = str_match,
    .replace = str_replace,
    .sbuf = str_sbuf,
    .split = str_split,
    .split_lines = str_split_lines,
    .sprintf = str_sprintf,
    .sstr = str_sstr,
    .starts_with = str_starts_with,
    .sub = str_sub,
    .upper = str_upper,
    .vsprintf = str_vsprintf,

    .convert = {
        .to_f32 = str__convert__to_f32,
        .to_f64 = str__convert__to_f64,
        .to_i16 = str__convert__to_i16,
        .to_i32 = str__convert__to_i32,
        .to_i64 = str__convert__to_i64,
        .to_i8 = str__convert__to_i8,
        .to_u16 = str__convert__to_u16,
        .to_u32 = str__convert__to_u32,
        .to_u64 = str__convert__to_u64,
        .to_u8 = str__convert__to_u8,
    },

    .slice = {
        .clone = str__slice__clone,
        .cmp = str__slice__cmp,
        .cmpi = str__slice__cmpi,
        .copy = str__slice__copy,
        .ends_with = str__slice__ends_with,
        .eq = str__slice__eq,
        .index_of = str__slice__index_of,
        .iter_split = str__slice__iter_split,
        .lstrip = str__slice__lstrip,
        .match = str__slice__match,
        .remove_prefix = str__slice__remove_prefix,
        .remove_suffix = str__slice__remove_suffix,
        .rstrip = str__slice__rstrip,
        .starts_with = str__slice__starts_with,
        .strip = str__slice__strip,
        .sub = str__slice__sub,
    },

    // clang-format on
};



/*
*                          src/sbuf.c
*/
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

static void
sbuf_shrink(sbuf_c* self, u32 new_length)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);
    uassert(new_length <= head->length);
    uassert(new_length <= head->capacity);
    head->length = new_length;
    (*self)[head->length] = '\0';
}

static u32
sbuf_len(const sbuf_c* self)
{
    uassert(self != NULL);
    if (*self == NULL) {
        return 0;
    }
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
sbuf_appendfva(sbuf_c* self, const char* format, va_list va)
{
    if (unlikely(self == NULL)) {
        return Error.argument;
    }
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
    Exc result = sbuf_appendfva(self, format, va);
    va_end(va);
    return result;
}

static Exception
sbuf_append(sbuf_c* self, const char* s)
{
    uassert(self != NULL);
    sbuf_head_s* head = sbuf__head(*self);

    if (unlikely(s == NULL)) {
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

const struct __cex_namespace__sbuf sbuf = {
    // Autogenerated by CEX
    // clang-format off
    .create = sbuf_create,
    .create_temp = sbuf_create_temp,
    .create_static = sbuf_create_static,
    .grow = sbuf_grow,
    .update_len = sbuf_update_len,
    .clear = sbuf_clear,
    .shrink = sbuf_shrink,
    .len = sbuf_len,
    .capacity = sbuf_capacity,
    .destroy = sbuf_destroy,
    .appendfva = sbuf_appendfva,
    .appendf = sbuf_appendf,
    .append = sbuf_append,
    .isvalid = sbuf_isvalid,
    // clang-format on
};



/*
*                          src/io.c
*/

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
    // TODO: add windows version
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

Exc
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

int
io_printf(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    int result = cexsp__vfprintf(stdout, format, va);
    va_end(va);
    return result;
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
    if (contents_len > 0) {
        e$except_silent(err, io_fwrite(file, contents, 1, contents_len))
        {
            io_fclose(&file);
            return err;
        }
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

const struct __cex_namespace__io io = {
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
*                          src/argparse.c
*/

static const char*
argparse__prefix_skip(const char* str, const char* prefix)
{
    usize len = strlen(prefix);
    return strncmp(str, prefix, len) ? NULL : str + len;
}

static Exception
argparse__error(argparse_c* self, const argparse_opt_s* opt, const char* reason, bool is_long)
{
    (void)self;
    if (is_long) {
        fprintf(stdout, "error: option `--%s` %s\n", opt->long_name, reason);
    } else {
        fprintf(stdout, "error: option `-%c` %s\n", opt->short_name, reason);
    }

    return Error.argument;
}

static void
argparse_usage(argparse_c* self)
{
    uassert(self->argv != NULL && "usage before parse!");

    io.printf("Usage:\n");
    if (self->usage) {

        for$iter(str_s, it, str.slice.iter_split(str.sstr(self->usage), "\n", &it.iterator))
        {
            if (it.val.len == 0) {
                break;
            }

            char* fn = strrchr(self->program_name, '/');
            if (fn != NULL) {
                io.printf("%s ", fn + 1);
            } else {
                io.printf("%s ", self->program_name);
            }

            if (fwrite(it.val.buf, sizeof(char), it.val.len, stdout)) {
                ;
            }

            fputc('\n', stdout);
        }
    } else {
        if (self->commands) {
            io.printf("%s {", self->program_name);
            for$eachp(c, self->commands, self->commands_len)
            {
                isize i = c - self->commands;
                if (i == 0) {
                    io.printf("%s", c->name);
                } else {
                    io.printf(",%s", c->name);
                }
            }
            io.printf("} [cmd_options] [cmd_args]\n");

        } else {
            io.printf("%s [options] [--] [arg1 argN]\n", self->program_name);
        }
    }

    // print description
    if (self->description) {
        io.printf("%s\n", self->description);
    }

    fputc('\n', stdout);


    // figure out best width
    usize usage_opts_width = 0;
    usize len = 0;
    for$eachp(opt, self->options, self->options_len)
    {
        len = 0;
        if (opt->short_name) {
            len += 2;
        }
        if (opt->short_name && opt->long_name) {
            len += 2; // separator ", "
        }
        if (opt->long_name) {
            len += strlen(opt->long_name) + 2;
        }
        switch (opt->type) {
            case CexArgParseType__boolean:
                break;
            case CexArgParseType__string:
                break;
            case CexArgParseType__i8:
            case CexArgParseType__u8:
            case CexArgParseType__i16:
            case CexArgParseType__u16:
            case CexArgParseType__i32:
            case CexArgParseType__u32:
            case CexArgParseType__i64:
            case CexArgParseType__u64:
            case CexArgParseType__f32:
            case CexArgParseType__f64:
                len += strlen("=<xxx>");
                break;

            default:
                break;
        }
        len = (len + 3) - ((len + 3) & 3);
        if (usage_opts_width < len) {
            usage_opts_width = len;
        }
    }
    usage_opts_width += 4; // 4 spaces prefix

    for$eachp(opt, self->options, self->options_len)
    {
        usize pos = 0;
        usize pad = 0;
        if (opt->type == CexArgParseType__group) {
            fputc('\n', stdout);
            io.printf("%s", opt->help);
            fputc('\n', stdout);
            continue;
        }
        pos = io.printf("    ");
        if (opt->short_name) {
            pos += io.printf("-%c", opt->short_name);
        }
        if (opt->long_name && opt->short_name) {
            pos += io.printf(", ");
        }
        if (opt->long_name) {
            pos += io.printf("--%s", opt->long_name);
        }

        if (pos <= usage_opts_width) {
            pad = usage_opts_width - pos;
        } else {
            fputc('\n', stdout);
            pad = usage_opts_width;
        }
        if (!str.find(opt->help, "\n")) {
            io.printf("%*s%s", (int)pad + 2, "", opt->help);
        } else {
            u32 i = 0;
            for$iter(str_s, it, str.slice.iter_split(str.sstr(opt->help), "\n", &it.iterator)) {
                str_s clean = str.slice.strip(it.val);
                if (clean.len == 0) {
                    continue;
                }
                if (i > 0) {
                    io.printf("\n");
                }
                io.printf("%*s%S", (i==0) ? pad+2 : usage_opts_width + 2, "", clean);
                i++;
            }
        }

        if (!opt->required && opt->value) {
            io.printf(" (default: ");
            switch (opt->type) {
                case CexArgParseType__boolean:
                    io.printf("%c", *(bool*)opt->value ? 'Y' : 'N');
                    break;
                case CexArgParseType__string:
                    if (*(const char**)opt->value != NULL){
                        io.printf("'%s'", *(const char**)opt->value);
                    } else {
                        io.printf("''");
                    }
                    break;
                case CexArgParseType__i8:
                    io.printf("%d", *(i8*)opt->value);
                    break;
                case CexArgParseType__i16:
                    io.printf("%d", *(i16*)opt->value);
                    break;
                case CexArgParseType__i32:
                    io.printf("%d", *(i32*)opt->value);
                    break;
                case CexArgParseType__u8:
                    io.printf("%u", *(u8*)opt->value);
                    break;
                case CexArgParseType__u16:
                    io.printf("%u", *(u16*)opt->value);
                    break;
                case CexArgParseType__u32:
                    io.printf("%u", *(u32*)opt->value);
                    break;
                case CexArgParseType__i64:
                    io.printf("%ld", *(i64*)opt->value);
                    break;
                case CexArgParseType__u64:
                    io.printf("%lu", *(u64*)opt->value);
                    break;
                case CexArgParseType__f32:
                    io.printf("%g", *(f32*)opt->value);
                    break;
                case CexArgParseType__f64:
                    io.printf("%g", *(f64*)opt->value);
                    break;
                default:
                    break;
            }
            io.printf(")");
        }
        io.printf("\n");
    }

    for$eachp(cmd, self->commands, self->commands_len)
    {
        io.printf("%-20s%s%s\n", cmd->name, cmd->help, cmd->is_default ? " (default)" : "");
    }

    // print epilog
    if (self->epilog) {
        io.printf("%s\n", self->epilog);
    }
}
static Exception
argparse__getvalue(argparse_c* self, argparse_opt_s* opt, bool is_long)
{
    if (!opt->value) {
        goto skipped;
    }

    switch (opt->type) {
        case CexArgParseType__boolean:
            *(bool*)opt->value = !(*(bool*)opt->value);
            opt->is_present = true;
            break;
        case CexArgParseType__string:
            if (self->_ctx.optvalue) {
                *(const char**)opt->value = self->_ctx.optvalue;
                self->_ctx.optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                self->_ctx.cpidx++;
                *(const char**)opt->value = *++self->argv;
            } else {
                return argparse__error(self, opt, "requires a value", is_long);
            }
            opt->is_present = true;
            break;
        case CexArgParseType__i8:
        case CexArgParseType__u8:
        case CexArgParseType__i16:
        case CexArgParseType__u16:
        case CexArgParseType__i32:
        case CexArgParseType__u32:
        case CexArgParseType__i64:
        case CexArgParseType__u64:
        case CexArgParseType__f32:
        case CexArgParseType__f64:
            if (self->_ctx.optvalue) {
                if (self->_ctx.optvalue[0] == '\0') {
                    return argparse__error(self, opt, "requires a value", is_long);
                }
                uassert(opt->convert != NULL);
                e$except_silent(err, opt->convert(self->_ctx.optvalue, opt->value))
                {
                    return argparse__error(self, opt, "argument parsing error", is_long);
                }
                self->_ctx.optvalue = NULL;
            } else if (self->argc > 1) {
                self->argc--;
                self->_ctx.cpidx++;
                self->argv++;
                e$except_silent(err, opt->convert(*self->argv, opt->value))
                {
                    return argparse__error(self, opt, "argument parsing error", is_long);
                }
            } else {
                return argparse__error(self, opt, "requires a value", is_long);
            }
            if (opt->type == CexArgParseType__f32) {
                f32 res = *(f32*)opt->value;
                if (isnanf(res) || res == INFINITY || res == -INFINITY) {
                    return argparse__error(
                        self,
                        opt,
                        "argument parsing error (float out of range)",
                        is_long
                    );
                }
            } else if (opt->type == CexArgParseType__f64) {
                f64 res = *(f64*)opt->value;
                if (isnanf(res) || res == INFINITY || res == -INFINITY) {
                    return argparse__error(
                        self,
                        opt,
                        "argument parsing error (float out of range)",
                        is_long
                    );
                }
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
        return opt->callback(self, opt, opt->callback_data);
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
    for$eachp(opt, self->options, self->options_len)
    {
        if (opt->type != CexArgParseType__group) {
            if (reset) {
                opt->is_present = 0;
                if (!(opt->short_name || opt->long_name)) {
                    uassert(
                        (opt->short_name || opt->long_name) && "options both long/short_name NULL"
                    );
                    return Error.argument;
                }
                if (opt->value == NULL && opt->short_name != 'h') {
                    uassertf(
                        opt->value != NULL,
                        "option value [%c/%s] is null\n",
                        opt->short_name,
                        opt->long_name
                    );
                    return Error.argument;
                }
            } else {
                if (opt->required && !opt->is_present) {
                    fprintf(
                        stdout,
                        "Error: missing required option: -%c/--%s\n",
                        opt->short_name,
                        opt->long_name
                    );
                    return Error.argsparse;
                }
            }
        }

        switch (opt->type) {
            case CexArgParseType__group:
                continue;
            case CexArgParseType__boolean:
            case CexArgParseType__string: {
                uassert(opt->convert == NULL && "unexpected to be set for strings/bools");
                uassert(opt->callback == NULL && "unexpected to be set for strings/bools");
                break;
            }
            case CexArgParseType__i8:
            case CexArgParseType__u8:
            case CexArgParseType__i16:
            case CexArgParseType__u16:
            case CexArgParseType__i32:
            case CexArgParseType__u32:
            case CexArgParseType__i64:
            case CexArgParseType__u64:
            case CexArgParseType__f32:
            case CexArgParseType__f64:
                uassert(opt->convert != NULL && "expected to be set for standard args");
                uassert(opt->callback == NULL && "unexpected to be set for standard args");
                continue;
            case CexArgParseType__generic:
                uassert(opt->convert == NULL && "unexpected to be set for generic args");
                uassert(opt->callback != NULL && "expected to be set for generic args");
                continue;
            default:
                uassertf(false, "wrong option type: %d", opt->type);
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
            return argparse__getvalue(self, options, false);
        }
    }
    return Error.not_found;
}

static Exception
argparse__long_opt(argparse_c* self, argparse_opt_s* options)
{
    for (u32 i = 0; i < self->options_len; i++, options++) {
        const char* rest;
        if (!options->long_name) {
            continue;
        }
        rest = argparse__prefix_skip(self->argv[0] + 2, options->long_name);
        if (!rest) {
            if (options->type != CexArgParseType__boolean) {
                continue;
            }
            rest = argparse__prefix_skip(self->argv[0] + 2 + 3, options->long_name);
            if (!rest) {
                continue;
            }
        }
        if (*rest) {
            if (*rest != '=') {
                continue;
            }
            self->_ctx.optvalue = rest + 1;
        }
        return argparse__getvalue(self, options, true);
    }
    return Error.not_found;
}


static Exception
argparse__report_error(argparse_c* self, Exc err)
{
    // invalidate argc
    self->argc = 0;

    if (err == Error.not_found) {
        if (self->options != NULL) {
            io.printf("error: unknown option `%s`\n", self->argv[0]);
        } else {
            fprintf(
                stdout,
                "error: command name expected, got option `%s`, try --help\n",
                self->argv[0]
            );
        }
    } else if (err == Error.integrity) {
        io.printf("error: option `%s` follows argument\n", self->argv[0]);
    }
    return Error.argsparse;
}

static Exception
argparse__parse_commands(argparse_c* self)
{
    uassert(self->_ctx.current_command == NULL);
    if (self->commands_len == 0) {
        argparse_cmd_s* _cmd = self->commands;
        while (_cmd != NULL) {
            if (_cmd->name == NULL) {
                break;
            }
            self->commands_len++;
            _cmd++;
        }
    }

    argparse_cmd_s* cmd = NULL;
    const char* cmd_arg = (self->argc > 0) ? self->argv[0] : NULL;

    if (str.eq(cmd_arg, "-h") || str.eq(cmd_arg, "--help")) {
        argparse_usage(self);
        return Error.argsparse;
    }

    for$eachp(c, self->commands, self->commands_len)
    {
        uassert(c->func != NULL && "missing command funcion");
        uassert(c->help != NULL && "missing command help message");
        if (cmd_arg != NULL) {
            if (str.eq(cmd_arg, c->name)) {
                cmd = c;
                break;
            }
        } else {
            if (c->is_default) {
                uassert(cmd == NULL && "multiple default commands in argparse_c");
                cmd = c;
            }
        }
    }
    if (cmd == NULL) {
        argparse_usage(self);
        io.printf("error: unknown command name '%s', try --help\n", (cmd_arg) ? cmd_arg : "");
        return Error.argsparse;
    }
    self->_ctx.current_command = cmd;
    self->_ctx.cpidx = 0;

    return EOK;
}

static Exception
argparse__parse_options(argparse_c* self)
{
    if (self->options_len == 0) {
        argparse_opt_s* _opt = self->options;
        while (_opt != NULL) {
            if (_opt->type == CexArgParseType__na) {
                break;
            }
            self->options_len++;
            _opt++;
        }
    }
    int initial_argc = self->argc + 1;
    e$except_silent(err, argparse__options_check(self, true))
    {
        return err;
    }

    for (; self->argc; self->argc--, self->argv++) {
        const char* arg = self->argv[0];
        if (arg[0] != '-' || !arg[1]) {
            self->_ctx.has_argument = true;

            if (self->commands != 0) {
                self->argc--;
                self->argv++;
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
            self->argc--;
            self->argv++;
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

    self->argv = self->_ctx.out + self->_ctx.cpidx + 1; // excludes 1st argv[0], program_name
    self->argc = initial_argc - self->_ctx.cpidx - 1;
    self->_ctx.cpidx = 0;

    return EOK;
}

static Exception
argparse_parse(argparse_c* self, int argc, char** argv)
{
    if (self->options != NULL && self->commands != NULL) {
        uassert(false && "options and commands are mutually exclusive");
        return Error.integrity;
    }
    uassert(argc > 0);
    uassert(argv != NULL);
    uassert(argv[0] != NULL);

    if (self->program_name == NULL) {
        self->program_name = argv[0];
    }

    // reset if we have several runs
    memset(&self->_ctx, 0, sizeof(self->_ctx));

    self->argc = argc - 1;
    self->argv = argv + 1;
    self->_ctx.out = argv;

    if (self->commands) {
        return argparse__parse_commands(self);
    } else if (self->options) {
        return argparse__parse_options(self);
    }
    return Error.ok;
}

static const char*
argparse_next(argparse_c* self)
{
    uassert(self != NULL);
    uassert(self->argv != NULL && "forgot argparse.parse() call?");

    var result = self->argv[0];
    if (self->argc > 0) {

        if (self->_ctx.cpidx > 0) {
            // we have --opt=foo, return 'foo' part
            result = self->argv[0] + self->_ctx.cpidx + 1;
            self->_ctx.cpidx = 0;
        } else {
            if (str.starts_with(result, "--")) {
                char* eq = str.find(result, "=");
                if (eq) {
                    static char part_arg[128]; // temp buffer sustained after scope exit
                    self->_ctx.cpidx = eq - result;
                    if ((usize)self->_ctx.cpidx + 1 >= sizeof(part_arg)) {
                        return NULL;
                    }
                    if (str.copy(part_arg, result, sizeof(part_arg))) {
                        return NULL;
                    }
                    part_arg[self->_ctx.cpidx] = '\0';
                    return part_arg;
                }
            }
        }
        self->argc--;
        self->argv++;
    }

    if (unlikely(self->argc == 0)) {
        // After reaching argc=0, argv getting stack-overflowed (ASAN issues), we set to fake NULL
        static char* null_argv[] = { NULL };
        // reset NULL every call, because static null_argv may be overwritten in user code maybe
        null_argv[0] = NULL;
        self->argv = null_argv;
    }
    return result;
}

static Exception
argparse_run_command(argparse_c* self, void* user_ctx)
{
    uassert(self->_ctx.current_command != NULL && "not parsed/parse error?");
    if (self->argc == 0) {
        // seems default command (with no args)
        const char* dummy_args[] = { self->_ctx.current_command->name };
        return self->_ctx.current_command->func(1, (char**)dummy_args, user_ctx);
    } else {
        return self->_ctx.current_command->func(self->argc, (char**)self->argv, user_ctx);
    }
}


const struct __cex_namespace__argparse argparse = {
    // Autogenerated by CEX
    // clang-format off
    
    .next = argparse_next,
    .parse = argparse_parse,
    .run_command = argparse_run_command,
    .usage = argparse_usage,
    
    // clang-format on
};



/*
*                          src/_subprocess.c
*/


/*
*                          src/os.c
*/


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

static Exception
os__fs__mkpath(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
    str_s dir = os.path.split(path, true);
    char dir_path[PATH_MAX] = {0};
    e$ret(str.slice.copy(dir_path, dir, sizeof(dir_path)));
    if (os.path.exists(dir_path)) {
        return EOK;
    }
    usize dir_path_len = 0;

    for$iter(str_s, it, str.slice.iter_split(dir, "\\/", &it.iterator)) {
        if (dir_path_len > 0) {
            uassert(dir_path_len < sizeof(dir_path)-2);
            dir_path[dir_path_len] = os$PATH_SEP;
            dir_path_len++;
            dir_path[dir_path_len] = '\0';
        }
        e$ret(str.slice.copy(dir_path + dir_path_len, it.val, sizeof(dir_path) - dir_path_len));
        dir_path_len += it.val.len;
        e$ret(os.fs.mkdir(dir_path));
    }
    return EOK;
}

static os_fs_stat_s
os__fs__stat(const char* path)
{
    os_fs_stat_s result = { .error = Error.argument };
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
            result.error = Error.not_found;
        } else {
            result.error = strerror(errno);
        }
        return result;
    }
    result.is_valid = true;
    result.error = EOK;
    result.is_other = true;
    result.mtime = statbuf.st_mtime;

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
                result.error = Error.not_found;
            } else {
                result.error = strerror(errno);
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
        errno = 0;
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

        var ftype = os.fs.stat(path_buf);
        if (!ftype.is_valid) {
            result = ftype.error;
            goto end;
        }

        if (is_recursive && ftype.is_directory && !ftype.is_symlink) {
            e$except_silent(err, os__fs__dir_walk(path_buf, is_recursive, callback_fn, user_ctx))
            {
                result = err;
                goto end;
            }

        } 
        // After recursive call make a callback on a directory itself
        e$except_silent(err, callback_fn(path_buf, ftype, user_ctx))
        {
            result = err;
            goto end;
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

struct _os_fs_find_ctx_s
{
    const char* pattern;
    arr$(char*) result;
    IAllocator allc;
};

static Exception
_os__fs__remove_tree_walker(const char* path, os_fs_stat_s ftype, void* user_ctx)
{
    (void)user_ctx;
    (void)ftype;
    e$except_silent(err, os__fs__remove(path)){
        return err;
    }
    return EOK;
}
static Exception
os__fs__remove_tree(const char* path)
{
    if (path == NULL || path[0] == '\0') {
        return Error.argument;
    }
    if (!os.path.exists(path)){
        return EOK;
    }
    e$except_silent(err, os__fs__dir_walk(path, true, _os__fs__remove_tree_walker, NULL))
    {
        return err;
    }
    e$except_silent(err, os__fs__remove(path)){
        return err;
    }
    return EOK;
}

static Exception
_os__fs__find_walker(const char* path, os_fs_stat_s ftype, void* user_ctx)
{
    (void)ftype;
    struct _os_fs_find_ctx_s* ctx = (struct _os_fs_find_ctx_s*)user_ctx;
    uassert(ctx->result != NULL);
    if (ftype.is_directory) {
        return EOK; // skip, only supports finding files!
    }
    if (ftype.is_symlink) {
        return EOK; // does not follow symlinks!
    }

    str_s file_part = os.path.split(path, false);
    if (!str.match(file_part.buf, ctx->pattern)) {
        return EOK; // just skip when patten not matched
    }

    // allocate new string because path is stack allocated buffer in os__fs__dir_walk()
    char* new_path = str.clone(path, ctx->allc);
    if (new_path == NULL) {
        return Error.memory;
    }

    // Doing graceful memory check, otherwise arr$push will assert
    if (!arr$grow_check(ctx->result, 1)) {
        return Error.memory;
    }
    arr$push(ctx->result, new_path);
    return EOK;
}

static arr$(char*) os__fs__find(const char* path, bool is_recursive, IAllocator allc)
{

    if (unlikely(allc == NULL)) {
        return NULL;
    }


    str_s dir_part = os.path.split(path, true);
    if (dir_part.buf == NULL) {
#if defined(CEXTEST) || defined(CEXBUILD)
        (void)e$raise(Error.argument, "Bad path: os.fn.find('%s')", path);
#endif
        return NULL;
    }

    if (!is_recursive) {
        var f = os.fs.stat(path);
        if (f.is_valid && f.is_file) {
            // Find used with exact file path, we still have to return array + allocated path copy
            arr$(char*) result = arr$new(result, allc);
            char* it = str.clone(path, allc);
            arr$push(result, it);
            return result;
        }
    }

    char path_buf[PATH_MAX + 2] = { 0 };
    if (dir_part.len > 0 && str.slice.copy(path_buf, dir_part, sizeof(path_buf))) {
        uassert(dir_part.len < PATH_MAX);
        return NULL;
    }
    if (str.copy(
            path_buf + dir_part.len + 1,
            path + dir_part.len,
            sizeof(path_buf) - dir_part.len - 1
        )) {
        uassert(dir_part.len < PATH_MAX);
        return NULL;
    }
    char* dir_name = (dir_part.len > 0) ? path_buf : ".";
    char* pattern = path_buf + dir_part.len + 1;
    if (*pattern == os$PATH_SEP) {
        pattern++;
    }
    if (*pattern == '\0') {
        pattern = "*";
    }

    struct _os_fs_find_ctx_s ctx = { .result = arr$new(ctx.result, allc),
                                     .pattern = pattern,
                                     .allc = allc };
    if (unlikely(ctx.result == NULL)) {
        return NULL;
    }

    e$except_silent(err, os__fs__dir_walk(dir_name, is_recursive, _os__fs__find_walker, &ctx))
    {
        for$each(it, ctx.result)
        {
            mem$free(allc, it); // each individual item was allocated too
        }
        if (ctx.result != NULL) {
            arr$free(ctx.result);
        }
        ctx.result = NULL;
    }
    return ctx.result;
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

static bool
os__path__exists(const char* file_path)
{
    var ftype = os.fs.stat(file_path);
    return ftype.is_valid;
}

static char*
os__path__join(const char** parts, u32 parts_len, IAllocator allc)
{
    char sep[2] = { os$PATH_SEP, '\0' };
    return str.join(parts, parts_len, sep, allc);
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
        if (args[i] == NULL || args[i][0] == '\0' ) {
            return e$raise(
                Error.argument,
                "`args` item[%d] is NULL/empty, which may indicate string operation failure",
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
            log$error("Could not exec child process: %s\n", strerror(errno));
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


const struct __cex_namespace__os os = {
    // Autogenerated by CEX
    // clang-format off
    .sleep = os_sleep,

    .fs = {  // sub-module .fs >>>
        .rename = os__fs__rename,
        .mkdir = os__fs__mkdir,
        .mkpath = os__fs__mkpath,
        .stat = os__fs__stat,
        .remove = os__fs__remove,
        .dir_walk = os__fs__dir_walk,
        .remove_tree = os__fs__remove_tree,
        .find = os__fs__find,
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



/*
*                          src/test.c
*/
#ifdef CEXTEST
#include <math.h>

enum _cex_test_eq_op_e
{
    _cex_test_eq_op__na,
    _cex_test_eq_op__eq,
    _cex_test_eq_op__ne,
    _cex_test_eq_op__lt,
    _cex_test_eq_op__le,
    _cex_test_eq_op__gt,
    _cex_test_eq_op__ge,
};

static Exc __attribute__((noinline))
_check_eq_int(i64 a, i64 b, int line, enum _cex_test_eq_op_e op)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    bool passed = false;
    char* ops = "?";
    switch (op) {
        case _cex_test_eq_op__na:
            unreachable("bad op");
            break;
        case _cex_test_eq_op__eq:
            passed = a == b;
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = a != b;
            ops = "==";
            break;
        case _cex_test_eq_op__lt:
            passed = a < b;
            ops = ">=";
            break;
        case _cex_test_eq_op__le:
            passed = a <= b;
            ops = ">";
            break;
        case _cex_test_eq_op__gt:
            passed = a > b;
            ops = "<=";
            break;
        case _cex_test_eq_op__ge:
            passed = a >= b;
            ops = "<";
            break;
    }
    if (!passed) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            sizeof(_cex_test__mainfn_state.str_buf),
            "%s:%d -> %ld %s %ld",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            ops,
            b
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eq_almost(f64 a, f64 b, f64 delta, int line)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    bool passed = false;
    f64 abdelta = a - b;
    if (isnan(a) && isnan(b)) {
        passed = true;
    } else if (isinf(a) && isinf(b)) {
        passed = (signbit(a) == signbit(b));
    } else {
        passed = fabs(abdelta) <= ((delta != 0) ? delta : (f64)0.0000001);
    }
    if (!passed) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            sizeof(_cex_test__mainfn_state.str_buf),
            "%s:%d -> %f != %f (delta: %f, diff: %f)",
            _cex_test__mainfn_state.suite_file,
            line,
            (a),
            (b),
            delta,
            abdelta
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eq_f32(f64 a, f64 b, int line, enum _cex_test_eq_op_e op)
{
    (void)op;
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    bool passed = false;
    char* ops = "?";
    f64 delta = a - b;
    bool is_equal = false;

    if (isnan(a) && isnan(b)) {
        is_equal = true;
    } else if (isinf(a) && isinf(b)) {
        is_equal = (signbit(a) == signbit(b));
    } else {
        is_equal = fabs(delta) <= (f64)0.0000001;
    }
    switch (op) {
        case _cex_test_eq_op__na:
            unreachable("bad op");
            break;
        case _cex_test_eq_op__eq:
            passed = is_equal;
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = !is_equal;
            ops = "==";
            break;
        case _cex_test_eq_op__lt:
            passed = a < b;
            ops = ">=";
            break;
        case _cex_test_eq_op__le:
            passed = a < b || is_equal;
            ops = ">";
            break;
        case _cex_test_eq_op__gt:
            passed = a > b;
            ops = "<=";
            break;
        case _cex_test_eq_op__ge:
            passed = a >= b || is_equal;
            ops = "<";
            break;
    }
    if (!passed) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            sizeof(_cex_test__mainfn_state.str_buf),
            "%s:%d -> %f %s %f (delta: %f)",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            ops,
            b,
            delta
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eq_str(const char* a, const char* b, int line, enum _cex_test_eq_op_e op)
{
    bool passed = false;
    char* ops = "";
    switch (op) {
        case _cex_test_eq_op__eq:
            passed = str.eq(a, b);
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = !str.eq(a, b);
            ops = "==";
            break;
        default:
            unreachable("bad op or unsupported for strings");
    }
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (!passed) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            CEXTEST_AMSG_MAX_LEN - 1,
            "%s:%d -> '%s' %s '%s'",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            ops,
            b
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eq_err(const char* a, const char* b, int line)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (!str.eq(a, b)) {
        const char* ea = (a == EOK) ? "Error.ok" : a;
        const char* eb = (b == EOK) ? "Error.ok" : b;
        snprintf(
            _cex_test__mainfn_state.str_buf,
            CEXTEST_AMSG_MAX_LEN - 1,
            "%s:%d -> Exc mismatch '%s' != '%s'",
            _cex_test__mainfn_state.suite_file,
            line,
            ea,
            eb
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}


static Exc __attribute__((noinline))
_check_eq_ptr(const void* a, const void* b, int line)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (a != b) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            CEXTEST_AMSG_MAX_LEN - 1,
            "%s:%d -> %p != %p (ptr_diff: %ld)",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            b,
            (a - b)
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc __attribute__((noinline))
_check_eqs_slice(str_s a, str_s b, int line, enum _cex_test_eq_op_e op)
{
    bool passed = false;
    char* ops = "";
    switch (op) {
        case _cex_test_eq_op__eq:
            passed = str.slice.eq(a, b);
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = !str.slice.eq(a, b);
            ops = "==";
            break;
        default:
            unreachable("bad op or unsupported for strings");
    }
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (!passed) {
        if (str.sprintf(
                _cex_test__mainfn_state.str_buf,
                CEXTEST_AMSG_MAX_LEN - 1,
                "%s:%d -> '%S' %s '%S'",
                _cex_test__mainfn_state.suite_file,
                line,
                a,
                ops,
                b
            )) {
        }
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static void __attribute__((noinline))
cex_test_mute()
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    struct _cex_test_context_s* ctx = &_cex_test__mainfn_state;
    if (ctx->out_stream) {
        fflush(stdout);
        io.rewind(ctx->out_stream);
        fflush(ctx->out_stream);
        dup2(fileno(ctx->out_stream), STDOUT_FILENO);
    }
}
static void __attribute__((noinline))
cex_test_unmute(Exc test_result)
{
    (void)test_result;
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    struct _cex_test_context_s* ctx = &_cex_test__mainfn_state;
    if (ctx->out_stream) {
        fflush(stdout);
        putc('\0', stdout);
        fflush(stdout);
        isize flen = ftell(ctx->out_stream);
        io.rewind(ctx->out_stream);
        dup2(ctx->orig_stdout_fd, STDOUT_FILENO);

        if (test_result != EOK && flen > 1) {
            fflush(stdout);
            fflush(stderr);
            fprintf(stderr, "\n============== TEST OUTPUT >>>>>>>=============\n\n");
            int c;
            while ((c = fgetc(ctx->out_stream)) != EOF && c != '\0') {
                putc(c, stderr);
            }
            fprintf(stderr, "\n============== <<<<<< TEST OUTPUT =============\n");
        }
    }
}


static int __attribute__((noinline))
cex_test_main_fn(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    extern struct _cex_test_context_s _cex_test__mainfn_state;

    struct _cex_test_context_s* ctx = &_cex_test__mainfn_state;
    if (ctx->test_cases == NULL) {
        fprintf(stderr, "No test$case() in the test file: %s\n", __FILE__);
        return 1;
    }
    u32 max_name = 0;
    for$each(t, ctx->test_cases)
    {
        if (max_name < strlen(t.test_name) + 2) {
            max_name = strlen(t.test_name) + 2;
        }
    }
    max_name = (max_name < 70) ? 70 : max_name;

    ctx->quiet_mode = false;
    // FIX: after migrating to CEX c build system
    ctx->has_ansi = true; // io.is_atty();

    argparse_opt_s options[] = {
        argparse$opt_help(),
        argparse$opt(&ctx->case_filter, 'f', "filter", .help = "execute cases with filter"),
        argparse$opt(&ctx->quiet_mode, 'q', "quiet", .help = "run test in quiet_mode"),
        argparse$opt(&ctx->breakpoint, 'b', "breakpoint", .help = "breakpoint on tassert failure"),
        argparse$opt(
            &ctx->no_stdout_capture,
            'o',
            "no-capture",
            .help = "prints all stdout as test goes"
        ),
    };

    argparse_c args = {
        .options = options,
        .options_len = arr$len(options),
        .description = "Test runner program",
    };

    e$except_silent(err, argparse.parse(&args, argc, argv))
    {
        return 1;
    }

    if (!ctx->no_stdout_capture) {
        ctx->out_stream = tmpfile();
        if (ctx->out_stream == NULL) {
            fprintf(stderr, "Failed opening temp output for capturing tests\n");
            uassert(false && "TODO: test this");
        }
    }
    // TODO: win32
    // ctx->orig_stdout_fd = _dup(_fileno(stdout));
    // ctx->orig_stderr_fd = _dup(_fileno(stderr));

    ctx->orig_stdout_fd = dup(fileno(stdout));
    ctx->orig_stderr_fd = dup(fileno(stderr));

    mem$scope(tmem$, _)
    {
        void* data = mem$malloc(_, 120);
        (void)data;
        uassert(data != NULL && "priming temp allocator failed");
    }

    if (!ctx->quiet_mode) {
        fprintf(stderr, "-------------------------------------\n");
        fprintf(stderr, "Running Tests: %s\n", argv[0]);
        fprintf(stderr, "-------------------------------------\n\n");
    }
    if (ctx->setup_suite_fn) {
        Exc err = NULL;
        if ((err = ctx->setup_suite_fn())) {
            fprintf(
                stderr,
                "[%s] test$setup_suite() failed with %s (suite %s stopped)\n",
                ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                err,
                __FILE__
            );
            return 1;
        }
    }

    if (ctx->quiet_mode) {
        fprintf(stderr, "%s ", ctx->suite_file);
    }

    for$each(t, ctx->test_cases)
    {
        ctx->case_name = t.test_name;
        ctx->tests_run++;
        if (ctx->case_filter && !str.find(t.test_name, ctx->case_filter)) {
            continue;
        }

        if (!ctx->quiet_mode) {
            fprintf(stderr, "%s", t.test_name);
            for (u32 i = 0; i < max_name - strlen(t.test_name) + 2; i++) {
                putc('.', stderr);
            }
            if (ctx->no_stdout_capture) {
                putc('\n', stderr);
            }
        }

#ifndef NDEBUG
        uassert_enable(); // unconditionally enable previously disabled asserts
#endif
        Exc err = EOK;
        if (ctx->setup_case_fn && (err = ctx->setup_case_fn()) != EOK) {
            fprintf(
                stderr,
                "[%s] test$setup() failed with '%s' (suite %s stopped)\n",
                ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                err,
                __FILE__
            );
            return 1;
        }

        cex_test_mute();
        AllocatorHeap_c* alloc_heap = (AllocatorHeap_c*)mem$;
        alloc_heap->stats.n_allocs = 0;
        alloc_heap->stats.n_free = 0;
        err = t.test_fn();
        if (ctx->quiet_mode && err != EOK) {
            fprintf(stdout, "[%s] %s\n", ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL", err);
            fprintf(stdout, "Test suite: %s case: %s\n", ctx->suite_file, t.test_name);
        }
        cex_test_unmute(err);

        if (err == EOK) {
            if (ctx->quiet_mode) {
                fprintf(stderr, ".");
            } else {
                fprintf(stderr, "[%s]\n", ctx->has_ansi ? io$ansi("PASS", "32") : "PASS");
            }
        } else {
            ctx->tests_failed++;
            if (!ctx->quiet_mode) {
                fprintf(
                    stderr,
                    "[%s] %s (%s)\n",
                    ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                    err,
                    t.test_name
                );
            } else {
                fprintf(stderr, "F");
            }
        }
        if (ctx->teardown_case_fn && (err = ctx->teardown_case_fn()) != EOK) {
            fprintf(
                stderr,
                "[%s] test$teardown() failed with %s (suite %s stopped)\n",
                ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                err,
                __FILE__
            );
            return 1;
        }
        if (err == EOK && alloc_heap->stats.n_allocs != alloc_heap->stats.n_free) {
            if (!ctx->quiet_mode) {
                fprintf(stderr, "%s", t.test_name);
                for (u32 i = 0; i < max_name - strlen(t.test_name) + 2; i++) {
                    putc('.', stderr);
                }
            } else {
                putc('\n', stderr);
            }
            fprintf(
                stderr,
                "[%s] %s:%d Possible memory leak: allocated %d != free %d\n",
                ctx->has_ansi ? io$ansi("LEAK", "33") : "LEAK",
                ctx->suite_file,
                t.test_line,
                alloc_heap->stats.n_allocs,
                alloc_heap->stats.n_free
            );
            ctx->tests_failed++;
        }
    }
    if (ctx->out_stream) {
        fclose(ctx->out_stream);
        ctx->out_stream = NULL;
    }

    if (ctx->teardown_suite_fn) {
        e$except(err, ctx->teardown_suite_fn())
        {
            fprintf(
                stderr,
                "[%s] test$teardown_suite() failed with %s (suite %s stopped)\n",
                ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                err,
                __FILE__
            );
            return 1;
        }
    }

    if (!ctx->quiet_mode) {
        fprintf(stderr, "\n-------------------------------------\n");
        fprintf(
            stderr,
            "Total: %d Passed: %d Failed: %d\n",
            ctx->tests_run,
            ctx->tests_run - ctx->tests_failed,
            ctx->tests_failed
        );
        fprintf(stderr, "-------------------------------------\n");
    } else {
        fprintf(stderr, "\n");

        if (ctx->tests_failed) {
            fprintf(
                stderr,
                "\n[%s] %s %d tests failed\n",
                (ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL"),
                ctx->suite_file,
                ctx->tests_failed
            );
        }
    } // Return code, logic is inversed
    return ctx->tests_run == 0 || ctx->tests_failed > 0;
}
#endif // ifdef CEXTEST



/*
*                          src/cex_code_gen.c
*/
#ifdef CEXBUILD

void
cex_codegen_indent(cex_codegen_s* cg)
{
    if (unlikely(cg->error != EOK)) {
        return;
    }
    for (u32 i = 0; i < cg->indent; i++) {
        Exc err = sbuf.append(cg->buf, " ");
        if (unlikely(err != EOK && cg->error != EOK)) {
            cg->error = err;
        }
    }
}

#define cg$printva(cg) /* temp macro! */                                                           \
    do {                                                                                           \
        va_list va;                                                                                \
        va_start(va, format);                                                                      \
        Exc err = sbuf.appendfva(cg->buf, format, va);                                             \
        if (unlikely(err != EOK && cg->error != EOK)) {                                            \
            cg->error = err;                                                                       \
        }                                                                                          \
        va_end(va);                                                                                \
    } while (0)

void
cex_codegen_print(cex_codegen_s* cg, bool rep_new_line, const char* format, ...)
{
    if (unlikely(cg->error != EOK)) {
        return;
    }
    if (rep_new_line) {
        usize slen = sbuf.len(cg->buf);
        if (slen && cg->buf[0][slen - 1] == '\n') {
            sbuf.shrink(cg->buf, slen - 1);
        }
    }
    cg$printva(cg);
}

void
cex_codegen_print_line(cex_codegen_s* cg, const char* format, ...)
{
    if (unlikely(cg->error != EOK)) {
        return;
    }
    if (format[0] != '\n'){
        cex_codegen_indent(cg);
    }
    cg$printva(cg);
}

cex_codegen_s*
cex_codegen_print_scope_enter(cex_codegen_s* cg, const char* format, ...)
{
    usize slen = sbuf.len(cg->buf);
    if (slen && cg->buf[0][slen - 1] == '\n') {
        cex_codegen_indent(cg);
    }
    cg$printva(cg);
    cex_codegen_print(cg, false, "%c\n", '{');
    cg->indent += 4;
    return cg;
}

void
cex_codegen_print_scope_exit(cex_codegen_s** cgptr)
{
    uassert(*cgptr != NULL);
    cex_codegen_s* cg = *cgptr;

    if (cg->indent >= 4) {
        cg->indent -= 4;
    }
    cex_codegen_indent(cg);
    cex_codegen_print(cg, false, "%c\n", '}');
}


cex_codegen_s*
cex_codegen_print_case_enter(cex_codegen_s* cg, const char* format, ...)
{
    cex_codegen_indent(cg);
    cg$printva(cg);
    cex_codegen_print(cg, false, ": %c\n", '{');
    cg->indent += 4;
    return cg;
}

void
cex_codegen_print_case_exit(cex_codegen_s** cgptr)
{
    uassert(*cgptr != NULL);
    cex_codegen_s* cg = *cgptr;

    if (cg->indent >= 4) {
        cg->indent -= 4;
    }
    cex_codegen_indent(cg);
    cex_codegen_print_line(cg, "break;\n", '}');
    cex_codegen_indent(cg);
    cex_codegen_print(cg, false, "%c\n", '}');
}

#undef cg$printva
#endif // #ifdef CEXBUILD



/*
*                          src/cexy.c
*/
#if defined(CEXBUILD)

static void
cexy_build_self(int argc, char** argv, const char* cex_source)
{
    mem$scope(tmem$, _)
    {
        uassert(str.ends_with(argv[0], "cex"));
        char* bin_path = argv[0];
#ifdef CEX_SELF_BUILD
        log$trace("Checking self build for executable: %s\n", bin_path);
        if (!cexy.src_include_changed(bin_path, cex_source, NULL)) {
            log$trace("%s unchanged, skipping self build\n", cex_source);
            // cex.c/cex.h are up to date no rebuild needed
            return;
        }
#endif

        log$info("Rebuilding self: %s -> %s\n", cex_source, bin_path);
        char* old_name = str.fmt(_, "%s.old", bin_path);
        if (os.path.exists(bin_path)) {
            if (os.fs.remove(old_name)) {
            }
            e$except(err, os.fs.rename(bin_path, old_name))
            {
                goto err;
            }
        }
        arr$(const char*) args = arr$new(args, _);
        arr$pushm(args, cexy$cc, "-DCEX_SELF_BUILD", "-g", "-o", bin_path, cex_source, NULL);
        _os$args_print("CMD:", args, arr$len(args));
        os_cmd_c _cmd = { 0 };
        e$except(err, os.cmd.run(args, arr$len(args), &_cmd))
        {
            goto fail_recovery;
        }
        e$except(err, os.cmd.join(&_cmd, 0, NULL))
        {
            goto fail_recovery;
        }

        // All good new build successful, remove old binary
        if (os.fs.remove(old_name)) {
        }

        // run rebuilt cex executable again
        arr$clear(args);
        arr$pushm(args, bin_path);
        arr$pusha(args, &argv[1], argc - 1);
        arr$pushm(args, NULL);
        _os$args_print("CMD:", args, arr$len(args));
        e$except(err, os.cmd.run(args, arr$len(args), &_cmd))
        {
            goto err;
        }
        if (os.cmd.join(&_cmd, 0, NULL)) {
            goto err;
        }
        exit(0); // unconditionally exit after build was successful
    fail_recovery:
        if (os.path.exists(old_name)) {
            e$except(err, os.fs.rename(old_name, bin_path))
            {
                goto err;
            }
        }
        goto err;
    err:
        exit(1);
    }
}

static bool
cexy_src_include_changed(const char* target_path, const char* src_path, arr$(char*) alt_include_path)
{
    if (unlikely(target_path == NULL)) {
        log$error("target_path is NULL\n");
        return false;
    }
    if (unlikely(src_path == NULL)) {
        log$error("src_path is NULL\n");
        return false;
    }

    var src_meta = os.fs.stat(src_path);
    if (!src_meta.is_valid) {
        (void)e$raise(src_meta.error, "Error src: %s", src_path);
        return false;
    } else if (!src_meta.is_file || src_meta.is_symlink) {
        (void)e$raise("Bad type", "src is not a file: %s", src_path);
        return false;
    }

    var target_meta = os.fs.stat(target_path);
    if (!target_meta.is_valid) {
        if (target_meta.error == Error.not_found) {
            return true;
        } else {
            (void)e$raise(target_meta.error, "target_path: '%s'", target_path);
            return false;
        }
    } else if (!target_meta.is_file || target_meta.is_symlink) {
        (void)e$raise("Bad type", "target_path is not a file: %s", target_path);
        return false;
    }

    if (src_meta.mtime > target_meta.mtime) {
        // File itself changed
        log$debug("Src changed: %s\n", src_path);
        return true;
    }

    mem$scope(tmem$, _)
    {
        arr$(const char*) incl_path = arr$new(incl_path, _);
        if (arr$len(alt_include_path) > 0) {
            for$each(p, alt_include_path)
            {
                arr$push(incl_path, p);
                if (!os.path.exists(p)) {
                    log$warn("alt_include_path not exists: %s\n", p);
                }
            }
        } else {
            const char* def_incl_path[] = { cexy$cc_include };
            for$each(p, def_incl_path)
            {
                const char* clean_path = p;
                if (str.starts_with(p, "-I")) {
                    clean_path = p + 2;
                } else if (str.starts_with(p, "-iquote=")) {
                    clean_path = p + strlen("-iquote=");
                }
                if (!os.path.exists(clean_path)) {
                    log$warn("cexy$cc_include not exists: %s\n", clean_path);
                }
                arr$push(incl_path, clean_path);
            }
        }

        char* code = io.file.load(src_path, _);
        if (code == NULL) {
            (void)e$raise("IOError", "src is not a file: '%s'", src_path);
            return false;
        }

        (void)CexTkn_str;
        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        while ((t = CexParser.next_token(&lx)).type) {
            if (t.type != CexTkn__preproc) {
                continue;
            }
            if (str.slice.starts_with(t.value, str$s("include"))) {
                str_s incf = str.slice.sub(t.value, strlen("include"), 0);
                incf = str.slice.strip(incf);
                if (incf.len <= 4) { // <.h>
                    log$warn(
                        "Bad include in: %s (item: %S at line: %d)\n",
                        src_path,
                        t.value,
                        lx.line
                    );
                    continue;
                }
                log$trace("Processing include: '%S'\n", incf);
                if (!str.slice.match(incf, "\"*.[hc]\"")) {
                    // system includes skipped
                    log$trace("Skipping include: '%S'\n", incf);
                    continue;
                }
                incf = str.slice.sub(incf, 1, -1);

                mem$scope(tmem$, _)
                {
                    char* inc_fn = str.slice.clone(incf, _);
                    uassert(inc_fn != NULL);
                    for$each(inc_dir, incl_path)
                    {
                        char* try_path = os$path_join(_, inc_dir, inc_fn);
                        uassert(try_path != NULL);
                        var src_meta = os.fs.stat(try_path);
                        log$trace("Probing include: %s\n", try_path);
                        if (src_meta.is_valid && src_meta.mtime > target_meta.mtime) {
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

static bool
cexy_src_changed(const char* target_path, arr$(char*) src_array)
{
    usize src_array_len = arr$len(src_array);
    if (unlikely(src_array == NULL || src_array_len == 0)) {
        if (src_array == NULL) {
            log$error("src_array is NULL, which may indicate error\n");
        } else {
            log$error("src_array is empty\n");
        }
        return false;
    }
    if (unlikely(target_path == NULL)) {
        log$error("target_path is NULL\n");
        return false;
    }
    var target_ftype = os.fs.stat(target_path);
    if (!target_ftype.is_valid) {
        if (target_ftype.error == Error.not_found) {
            log$trace("Target '%s' not exists, needs build.\n", target_path);
            return true;
        } else {
            (void)e$raise(target_ftype.error, "target_path: '%s'", target_path);
            return false;
        }
    } else if (!target_ftype.is_file || target_ftype.is_symlink) {
        (void)e$raise("Bad type", "target_path is not a file: %s", target_path);
        return false;
    }

    bool has_error = false;
    for$each(p, src_array, src_array_len)
    {
        var ftype = os.fs.stat(p);
        if (!ftype.is_valid) {
            (void)e$raise(ftype.error, "Error src: %s", p);
            has_error = true;
        } else if (!ftype.is_file || ftype.is_symlink) {
            (void)e$raise("Bad type", "src is not a file: %s", p);
            has_error = true;
        } else {
            if (has_error) {
                continue;
            }
            if (ftype.mtime > target_ftype.mtime) {
                log$trace("Source changed '%s'\n", p);
                return true;
            }
        }
    }

    return false;
}

static char*
cexy_target_make(
    const char* src_path,
    const char* build_dir,
    const char* name_or_extension,
    IAllocator allocator
)
{
    uassert(allocator != NULL);

    if (src_path == NULL || src_path[0] == '\0') {
        log$error("src_path is NULL or empty\n");
        return NULL;
    }
    if (build_dir == NULL) {
        log$error("build_dir is NULL\n");
        return NULL;
    }
    if (name_or_extension == NULL || name_or_extension[0] == '\0') {
        log$error("name_or_extension is NULL or empty\n");
        return NULL;
    }
    if (!os.path.exists(src_path)) {
        log$error("src_path does not exist: '%s'\n", src_path);
        return NULL;
    }

    char* result = NULL;
    if (name_or_extension[0] == '.') {
        // starts_with .ext, make full path as following: build_dir/src_path/src_file.ext
        result = str.fmt(allocator, "%s%c%s%s", build_dir, os$PATH_SEP, src_path, name_or_extension);
        uassert(result != NULL && "memory error");
    } else {
        // probably a program name, make full path: build_dir/name_or_extension
        result = str.fmt(allocator, "%s%c%s", build_dir, os$PATH_SEP, name_or_extension);
        uassert(result != NULL && "memory error");
    }
    e$except(err, os.fs.mkpath(result))
    {
        mem$free(allocator, result);
    }

    return result;
}

Exception
cexy__test__create(const char* target)
{
    if (os.path.exists(target)) {
        return e$raise(Error.exists, "Test file already exists: %s", target);
    }
    if (str.eq(target, "all") || str.find(target, "*")) {
        return e$raise(
            Error.argument,
            "You must pass exact file path, not pattern, got: %s",
            target
        );
    }

    mem$scope(tmem$, _)
    {
        sbuf_c buf = sbuf.create(1024 * 10, _);
        cg$init(&buf);
        $pn("#define CEX_IMPLEMENTATION");
        $pn("#include \"cex.h\"");
        $pn("");
        $pn("//test$setup_case() {return EOK;}");
        $pn("//test$teardown_case() {return EOK;}");
        $pn("//test$setup_suite() {return EOK;}");
        $pn("//test$teardown_suite() {return EOK;}");
        $pn("");
        $scope("test$case(%s)", "my_test_case")
        {
            $pn("tassert_eq(1, 0);");
            $pn("return EOK;");
        }
        $pn("");
        $pn("test$main();");

        e$ret(io.file.save(target, buf));
    }
    return EOK;
}

Exception
cexy__test__clean(const char* target)
{
    if (os.path.exists(target)) {
        return e$raise(Error.exists, "Test file already exists: %s", target);
    }

    if (str.eq(target, "all")) {
        log$info("Cleaning all tests\n");
        e$ret(os.fs.remove_tree(cexy$build_dir "/tests/"));
    } else {
        log$info("Cleaning target: %s\n", target);
        e$ret(os.fs.remove(target));
    }
    return EOK;
}

Exception
cexy__test__make_target_pattern(const char** target)
{
    if (target == NULL) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            *target
        );
    }
    if (str.eq(*target, "all")) {
        *target = "tests/test_*.c";
    }

    if (!str.match(*target, "*test*.c")) {
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or tests/test_some_file.c",
            *target
        );
    }
    return EOK;
}

Exception
cexy__test__run(const char* target, bool is_debug, int argc, char** argv)
{
    Exc result = EOK;
    u32 n_tests = 0;
    u32 n_failed = 0;
    mem$scope(tmem$, _)
    {
        for$each(test_src, os.fs.find(target, true, _))
        {
            n_tests++;
            char* test_target = cexy.target_make(test_src, cexy$build_dir, ".test", _);
            arr$(char*) args = arr$new(args, _);
            if (is_debug) {
                if (str.ends_with(target, "test_*.c")) {
                    return e$raise(
                        Error.argument,
                        "Debug expect direct file path, i.e. tests/test_some_file.c, got: %s",
                        target
                    );
                }
                arr$pushm(args, cexy$debug_cmd);
            }
            arr$pushm(args, test_target, );
            if (str.ends_with(target, "test_*.c")) {
                arr$push(args, "--quiet");
            }
            arr$pusha(args, argv, argc);
            arr$push(args, NULL);
            if (os$cmda(args)) {
                log$error("<<<<<<<<<<<<<<<<<< Test failed: %s\n", test_target);
                n_failed++;
                result = Error.runtime;
            }
        }
    }
    if (str.ends_with(target, "test_*.c")) {
        log$info(
            "Test run completed: %d tests, %d passed, %d failed\n",
            n_tests,
            n_tests - n_failed,
            n_failed
        );
    }
    return result;
}

static int
cexy__decl_comparator(const void* a, const void* b)
{
    cex_decl_s** _a = (cex_decl_s**)a;
    cex_decl_s** _b = (cex_decl_s**)b;

    // Makes str__sub__ to be placed after
    isize isub_a = str.slice.index_of(_a[0]->name, str$s("__"));
    isize isub_b = str.slice.index_of(_b[0]->name, str$s("__"));
    if (unlikely(isub_a != isub_b)) {
        if (isub_a != -1) {
            return 1;
        } else {
            return -1;
        }
    }

    return str.slice.cmp(_a[0]->name, _b[0]->name);
}
static str_s
cexy__process_make_brief_docs(cex_decl_s* decl)
{
    str_s brief_str = { 0 };
    if (!decl->docs.buf) {
        return brief_str;
    }

    if (str.slice.starts_with(decl->docs, str$s("///"))) {
        // doxygen style ///
        brief_str = str.slice.strip(str.slice.sub(decl->docs, 3, 0));
    } else if (str.slice.match(decl->docs, "/\\*[\\*!]*")) {
        // doxygen style /** or /*!
        for$iter(
            str_s,
            it,
            str.slice.iter_split(str.slice.sub(decl->docs, 3, -2), "\n", &it.iterator)
        )
        {
            str_s line = str.slice.strip(it.val);
            if (line.len == 0) {
                continue;
            }
            brief_str = line;
            break;
        }
    }
    isize bridx = (str.slice.index_of(brief_str, str$s("@brief")));
    if (bridx == -1) {
        bridx = str.slice.index_of(brief_str, str$s("\\brief"));
    }
    if (bridx != -1) {
        brief_str = str.slice.strip(str.slice.sub(brief_str, bridx + strlen("@brief"), 0));
    }

    return brief_str;
}

static Exception
cexy__process_gen_struct(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
{
    cg$init(out_buf);
    str_s subn = { 0 };

    $scope("struct __cex_namespace__%S ", ns_prefix)
    {
        $pn("// Autogenerated by CEX");
        $pn("// clang-format off");
        $pn("");
        for$each(it, decls)
        {
            str_s clean_name = str.slice.sub(it->name, ns_prefix.len + 1, 0);
            if (str.slice.starts_with(clean_name, str$s("_"))) {
                // sub-namespace
                isize ns_end = str.slice.index_of(clean_name, str$s("__"));
                e$assert(ns_end >= 0);
                str_s new_ns = str.slice.sub(clean_name, 1, ns_end);
                if (!str.slice.eq(new_ns, subn)) {
                    if (subn.buf != NULL) {
                        // End previous sub-namespace
                        cg$dedent();
                        $pf("} %S;", subn);
                    }

                    // new subnamespace begin
                    $pn("");
                    $pf("struct {", subn);
                    cg$indent();
                    subn = new_ns;
                }
                clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
            }
            str_s brief_str = cexy__process_make_brief_docs(it);
            if (brief_str.len) {
                $pf("/// %S", brief_str);
            }
            $pf("%-15s (*%S)(%s);", it->ret_type, clean_name, it->args);
        }

        if (subn.buf != NULL) {
            // End previous sub-namespace
            cg$dedent();
            $pf("} %S;", subn);
        }
        $pn("");
        $pn("// clang-format on");
    }
    $pa("%s", ";");

    if (!cg$is_valid()) {
        return e$raise(Error.runtime, "Code generation error occured\n");
    }
    return EOK;
}

static Exception
cexy__process_gen_var_def(str_s ns_prefix, arr$(cex_decl_s*) decls, sbuf_c* out_buf)
{
    cg$init(out_buf);
    str_s subn = { 0 };

    $scope("const struct __cex_namespace__%S %S = ", ns_prefix, ns_prefix)
    {
        $pn("// Autogenerated by CEX");
        $pn("// clang-format off");
        $pn("");
        for$each(it, decls)
        {
            str_s clean_name = str.slice.sub(it->name, ns_prefix.len + 1, 0);
            if (str.slice.starts_with(clean_name, str$s("_"))) {
                // sub-namespace
                isize ns_end = str.slice.index_of(clean_name, str$s("__"));
                e$assert(ns_end >= 0);
                str_s new_ns = str.slice.sub(clean_name, 1, ns_end);
                if (!str.slice.eq(new_ns, subn)) {
                    if (subn.buf != NULL) {
                        // End previous sub-namespace
                        cg$dedent();
                        $pn("},");
                    }

                    // new subnamespace begin
                    $pn("");
                    $pf(".%S = {", new_ns);
                    cg$indent();
                    subn = new_ns;
                }
                clean_name = str.slice.sub(clean_name, 1 + subn.len + 2, 0);
            }
            $pf(".%S = %S,", clean_name, it->name);
        }

        if (subn.buf != NULL) {
            // End previous sub-namespace
            cg$dedent();
            $pf("},", subn);
        }
        $pn("");
        $pn("// clang-format on");
    }
    $pa("%s", ";");

    if (!cg$is_valid()) {
        return e$raise(Error.runtime, "Code generation error occured\n");
    }
    return EOK;
}

static Exception
cexy__process_update_code(
    const char* code_file,
    bool only_update,
    sbuf_c cex_h_struct,
    sbuf_c cex_h_var_decl,
    sbuf_c cex_c_var_def
)
{
    mem$scope(tmem$, _)
    {
        char* code = io.file.load(code_file, _);
        e$assert(code && "failed loading code");

        bool is_header = str.ends_with(code_file, ".h");

        CexParser_c lx = CexParser.create(code, 0, true);
        cex_token_s t;
        sbuf_c new_code = sbuf.create(10 * 1024 + (lx.content_end - lx.content), _);
        CexParser.reset(&lx);

        str_s code_buf = { .buf = lx.content, .len = 0 };
        bool has_module_def = false;
        bool has_module_decl = false;
        bool has_module_struct = false;
        arr$(cex_token_s) items = arr$new(items, _);

#define $dump_prev()                                                                               \
    code_buf.len = t.value.buf - code_buf.buf - ((t.value.buf > lx.content) ? 1 : 0);              \
    if (code_buf.buf != NULL)                                                                      \
        e$ret(sbuf.appendf(&new_code, "%S\n", code_buf));                                          \
    code_buf = (str_s){ 0 }
#define $dump_prev_comment()                                                                       \
    for$each(it, items)                                                                            \
    {                                                                                              \
        if (it.type == CexTkn__comment_single || it.type == CexTkn__comment_multi) {               \
            e$ret(sbuf.appendf(&new_code, "%S\n", it.value));                                      \
        } else {                                                                                   \
            break;                                                                                 \
        }                                                                                          \
    }

        while ((t = CexParser.next_entity(&lx, &items)).type) {
            if (t.type == CexTkn__cex_module_def) {
                e$assert(!is_header && "expected in .c file buf got header");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_def) {
                    e$ret(sbuf.appendf(&new_code, "%s\n", cex_c_var_def));
                }
                has_module_def = true;
            } else if (t.type == CexTkn__cex_module_decl) {
                e$assert(is_header && "expected in .h file buf got source");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_decl) {
                    e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_var_decl));
                }
                has_module_decl = true;
            } else if (t.type == CexTkn__cex_module_struct) {
                e$assert(is_header && "expected in .h file buf got source");
                $dump_prev();
                $dump_prev_comment();
                if (!has_module_struct) {
                    e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_struct));
                }
                has_module_struct = true;
            } else {
                if (code_buf.buf == NULL) {
                    code_buf.buf = t.value.buf;
                }
                code_buf.len = t.value.buf - code_buf.buf + t.value.len;
            }
        }
        if (code_buf.len) {
            e$ret(sbuf.appendf(&new_code, "%S\n", code_buf));
        }
        if (!is_header) {
            if (!has_module_def) {
                if (only_update) {
                    return EOK;
                }
                e$ret(sbuf.appendf(&new_code, "\n%s\n", cex_c_var_def));
            }
            e$ret(io.file.save(code_file, new_code));
        } else {
            if (!has_module_struct) {
                if (only_update) {
                    return EOK;
                }
                e$ret(sbuf.appendf(&new_code, "\n%s\n", cex_h_struct));
            }
            if (!has_module_decl) {
                if (only_update) {
                    return EOK;
                }
                e$ret(sbuf.appendf(&new_code, "%s\n", cex_h_var_decl));
            }
            e$ret(io.file.save(code_file, new_code));
        }
    }

#undef $dump_prev
#undef $dump_prev_comment
    return EOK;
}

static Exception
cexy__cmd__process(int argc, char** argv, void* user_ctx)
{
    (void)user_ctx;

    // clang-format off
    const char* process_help = ""
    "process command intended for building CEXy interfaces from your source code\n\n"
    "For example: you can create foo_fun1(), foo_fun2(), foo__bar__fun3(), foo__bar__fun4()\n"
    "   these functions will be processed and wrapped to a `foo` namespace, so you can     \n"
    "   access them via foo.fun1(), foo.fun2(), foo.bar.fun3(), foo.bar.fun4()\n\n" 

    "Requirements / caveats: \n"
    "1. You must have foo.c and foo.h in the same folder\n"
    "2. Filename must start with `foo` - namespace prefix\n"
    "3. Each function in foo.c that you'd like to add to namespace must start with `foo_`\n"
    "4. For adding sub-namespace use `foo__subname__` prefix\n"
    "5. Only one level of sub-namespace is allowed\n"
    "6. You may not declare function signature in header, and ony use .c static functions\n"
    "7. Functions with `static inline` are not included into namespace\n" 
    "8. Functions with prefix `foo__some` are considered internal and not included\n"
    "9. New namespace is created when you use exact foo.c argument, `all` just for updates\n"

    ;
    // clang-format on

    const char* ignore_kw = cexy$process_ignore_kw;
    argparse_c cmd_args = {
        .program_name = "./cex",
        .usage = "process [options] all|path/some_file.c",
        .description = process_help,
        .epilog = "Use `all` for updates, and exact path/some_file.c for creating new\n",
        .options =
            (argparse_opt_s[]){
                argparse$opt_group("Options"),
                argparse$opt_help(),
                argparse$opt(
                    &ignore_kw,
                    'i',
                    "ignore",
                    .help = "ignores `keyword` or `keyword()` from processed function signatures\n  uses cexy$process_ignore_kw"
                ),
                { 0 },
            }
    };
    e$ret(argparse.parse(&cmd_args, argc, argv));
    const char* target = argparse.next(&cmd_args);

    if (target == NULL) {
        argparse.usage(&cmd_args);
        return e$raise(
            Error.argsparse,
            "Invalid target: '%s', expected all or path/some_file.c",
            target
        );
    }


    bool only_update = true;
    if (str.eq(target, "all")) {
        target = "*.c";
    } else {
        if (!os.path.exists(target)) {
            return e$raise(Error.not_found, "Target file not exists: '%s'", target);
        }
        only_update = false;
    }


    mem$scope(tmem$, _)
    {
        for$each(src_fn, os.fs.find(target, true, _))
        {
            char* basename = os.path.basename(src_fn, _);
            if (str.starts_with(basename, "test") || str.eq(basename, "cex.c")) {
                continue;
            }
            mem$scope(tmem$, _)
            {
                e$assert(str.ends_with(src_fn, ".c") && "file must end with .c");

                char* hdr_fn = str.clone(src_fn, _);
                hdr_fn[str.len(hdr_fn) - 1] = 'h'; // .c -> .h

                str_s ns_prefix = str.sub(os.path.basename(src_fn, _), 0, -2); // src.c -> src
                char* fn_sub_pattern = str.fmt(_, "%S__[a-zA-Z0-9+]__*", ns_prefix);
                char* fn_pattern = str.fmt(_, "%S_*", ns_prefix);
                char* fn_private = str.fmt(_, "%S__*", ns_prefix);

                log$debug(
                    "Cex Processing src: '%s' hdr: '%s' prefix: '%S'\n",
                    src_fn,
                    hdr_fn,
                    ns_prefix
                );
                if (!os.path.exists(hdr_fn)) {
                    if (only_update) {
                        log$debug("CEX skipped (no .h file for: %s)\n", src_fn);
                        continue;
                    } else {
                        return e$raise(Error.not_found, "Header file not exists: '%s'", hdr_fn);
                    }
                }
                char* code = io.file.load(src_fn, _);
                e$assert(code && "failed loading code");
                arr$(cex_token_s) items = arr$new(items, _);
                arr$(cex_decl_s*) decls = arr$new(decls, _, .capacity = 128);

                CexParser_c lx = CexParser.create(code, 0, true);
                cex_token_s t;
                while ((t = CexParser.next_entity(&lx, &items)).type) {
                    if (t.type == CexTkn__error) {
                        return e$raise(
                            Error.integrity,
                            "Error parsing file %s, at line: %d, cursor: %d",
                            src_fn,
                            lx.line,
                            (i32)(lx.cur - lx.content)
                        );
                    }
                    cex_decl_s* d = CexParser.decl_parse(t, items, cexy$process_ignore_kw, _);
                    if (d == NULL) {
                        continue;
                    }
                    if (d->type != CexTkn__func_def) {
                        continue;
                    }
                    if (d->is_inline && d->is_static) {
                        continue;
                    }
                    if (str.slice.match(d->name, fn_sub_pattern)) {
                        // OK use it!
                    } else if (str.slice.match(d->name, fn_private) ||
                               !str.slice.match(d->name, fn_pattern)) {
                        continue;
                    }
                    log$trace("FN: %S ret_type: '%s' args: '%s'\n", d->name, d->ret_type, d->args);
                    arr$push(decls, d);
                }
                if (arr$len(decls) == 0) {
                    log$info("CEX skipped (no cex decls found in : %s)\n", src_fn);
                    continue;
                }

                qsort(decls, arr$len(decls), sizeof(*decls), cexy__decl_comparator);

                sbuf_c cex_h_struct = sbuf.create(10 * 1024, _);
                sbuf_c cex_h_var_decl = sbuf.create(1024, _);
                sbuf_c cex_c_var_def = sbuf.create(10 * 1024, _);

                e$ret(sbuf.appendf(
                    &cex_h_var_decl,
                    "__attribute__((visibility(\"hidden\"))) extern const struct __cex_namespace__%S %S;\n",
                    ns_prefix,
                    ns_prefix
                ));
                e$ret(cexy__process_gen_struct(ns_prefix, decls, &cex_h_struct));
                e$ret(cexy__process_gen_var_def(ns_prefix, decls, &cex_c_var_def));
                e$ret(cexy__process_update_code(
                    src_fn,
                    only_update,
                    cex_h_struct,
                    cex_h_var_decl,
                    cex_c_var_def
                ));
                e$ret(cexy__process_update_code(
                    hdr_fn,
                    only_update,
                    cex_h_struct,
                    cex_h_var_decl,
                    cex_c_var_def
                ));

                // log$info("cex_h_struct: \n%s\n", cex_h_struct);
                log$info("CEX processed: %s\n", src_fn);
            }
        }
    }
    return EOK;
}


const struct __cex_namespace__cexy cexy = {
    // Autogenerated by CEX
    // clang-format off

    .build_self = cexy_build_self,
    .src_changed = cexy_src_changed,
    .src_include_changed = cexy_src_include_changed,
    .target_make = cexy_target_make,

    .cmd = {
        .process = cexy__cmd__process,
    },

    .test = {
        .clean = cexy__test__clean,
        .create = cexy__test__create,
        .make_target_pattern = cexy__test__make_target_pattern,
        .run = cexy__test__run,
    },

    // clang-format on
};
#endif // defined(CEXBUILD)



/*
*                          src/CexParser.c
*/

// NOTE: lx$ are the temporary macro (will be #undef at the end of this file)
#define lx$next(lx)                                                                                \
    ({                                                                                             \
        if (*lx->cur == '\n') {                                                                    \
            lx->line++;                                                                            \
        }                                                                                          \
        *(lx->cur++);                                                                              \
    })
#define lx$rewind(lx)                                                                              \
    ({                                                                                             \
        if (lx->cur > lx->content) {                                                               \
            lx->cur--;                                                                             \
            if (*lx->cur == '\n') {                                                                \
                lx->line++;                                                                        \
            }                                                                                      \
        }                                                                                          \
    })
#define lx$peek(lx) (lx->cur < lx->content_end) ? *lx->cur : '\0'
#define lx$skip_space(lx, c)                                                                       \
    while (c && isspace((c))) {                                                                    \
        lx$next(lx);                                                                               \
        (c) = lx$peek(lx);                                                                         \
    }

CexParser_c
CexParser_create(char* content, u32 content_len, bool fold_scopes)
{
    if (content_len == 0) {
        content_len = str.len(content);
    }
    CexParser_c lx = { .content = content,
                       .content_end = content + content_len,
                       .cur = content,
                       .fold_scopes = fold_scopes };
    return lx;
}

void
CexParser_reset(CexParser_c* lx)
{
    uassert(lx != NULL);
    lx->cur = lx->content;
    lx->line = 0;
}

static cex_token_s
CexParser__scan_ident(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__ident, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        if (!(isalnum(c) || c == '_' || c == '$')) {
            lx$rewind(lx);
            break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexParser__scan_number(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__number, .value = { .buf = lx->cur, .len = 0 } };
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
            case '/':
            case '*':
            case '+':
            case '-':
                lx$rewind(lx);
                return t;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexParser__scan_string(CexParser_c* lx)
{
    cex_token_s t = { .type = (*lx->cur == '"' ? CexTkn__string : CexTkn__char),
                      .value = { .buf = lx->cur + 1, .len = 0 } };
    lx$next(lx);
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case '\\': // escape char, unconditionally skip next
                lx$next(lx);
                t.value.len++;
                break;
            case '\'':
                if (t.type == CexTkn__char) {
                    return t;
                }
                break;
            case '"':
                if (t.type == CexTkn__string) {
                    return t;
                }
                break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexParser__scan_comment(CexParser_c* lx)
{
    cex_token_s t = { .type = lx->cur[1] == '/' ? CexTkn__comment_single : CexTkn__comment_multi,
                      .value = { .buf = lx->cur, .len = 2 } };
    lx$next(lx);
    lx$next(lx);
    char c;
    while ((c = lx$next(lx))) {
        switch (c) {
            case '\n':
                if (t.type == CexTkn__comment_single) {
                    lx$rewind(lx);
                    return t;
                }
                break;
            case '*':
                if (t.type == CexTkn__comment_multi) {
                    if (lx->cur[0] == '/') {
                        lx$next(lx);
                        t.value.len += 2;
                        return t;
                    }
                }
                break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexParser__scan_preproc(CexParser_c* lx)
{
    lx$next(lx);
    char c = *lx->cur;
    lx$skip_space(lx, c);
    cex_token_s t = { .type = CexTkn__preproc, .value = { .buf = lx->cur, .len = 0 } };

    while ((c = lx$next(lx))) {
        switch (c) {
            case '\n':
                return t;
            case '\\':
                // next line concat for #define
                lx$next(lx);
                t.value.len++;
                break;
        }
        t.value.len++;
    }
    return t;
}

static cex_token_s
CexParser__scan_scope(CexParser_c* lx)
{
    cex_token_s t = { .type = CexTkn__unk, .value = { .buf = lx->cur, .len = 0 } };

    if (!lx->fold_scopes) {
        char c = lx$peek(lx);
        switch (c) {
            case '(':
                t.type = CexTkn__lparen;
                break;
            case '[':
                t.type = CexTkn__lbracket;
                break;
            case '{':
                t.type = CexTkn__lbrace;
                break;
            default:
                unreachable();
        }
        t.value.len = 1;
        lx$next(lx);
        return t;
    } else {
        char scope_stack[128] = { 0 };
        u32 scope_depth = 0;

#define scope$push(c) /* temp macro! */                                                            \
    if (++scope_depth < sizeof(scope_stack)) {                                                     \
        scope_stack[scope_depth - 1] = c;                                                          \
    }
#define scope$pop_if(c) /* temp macro! */                                                          \
    if (scope_depth > 0 && scope_depth <= sizeof(scope_stack) &&                                   \
        scope_stack[scope_depth - 1] == c) {                                                       \
        scope_stack[--scope_depth] = '\0';                                                         \
    }

        char c = lx$peek(lx);
        switch (c) {
            case '(':
                t.type = CexTkn__paren_block;
                break;
            case '[':
                t.type = CexTkn__bracket_block;
                break;
            case '{':
                t.type = CexTkn__brace_block;
                break;
            default:
                unreachable();
        }

        while ((c = lx$peek(lx))) {
            switch (c) {
                case '{':
                    scope$push(c);
                    break;
                case '[':
                    scope$push(c);
                    break;
                case '(':
                    scope$push(c);
                    break;
                case '}':
                    scope$pop_if('{');
                    break;
                case ']':
                    scope$pop_if('[');
                    break;
                case ')':
                    scope$pop_if('(');
                    break;
                case '"':
                case '\'': {
                    var s = CexParser__scan_string(lx);
                    t.value.len += s.value.len + 2;
                    continue;
                }
                case '/': {
                    if (lx->cur[1] == '/' || lx->cur[1] == '*') {
                        var s = CexParser__scan_comment(lx);
                        t.value.len += s.value.len;
                        continue;
                    }
                    break;
                }
                case '#': {
                    char* ppstart = lx->cur;
                    var s = CexParser__scan_preproc(lx);
                    if (s.value.buf) {
                        t.value.len += s.value.len + (s.value.buf - ppstart) + 1;
                    }
                    continue;
                }
            }
            t.value.len++;
            lx$next(lx);

            if (scope_depth == 0) {
                return t;
            }
        }

#undef scope$push
#undef scope$pop_if

        if (scope_depth != 0) {
            t.type = CexTkn__error;
            t.value.buf = NULL;
            t.value.len = 0;
        }
        return t;
    }
}

cex_token_s
CexParser_next_token(CexParser_c* lx)
{

#define tok$new(tok_type)                                                                          \
    ({                                                                                             \
        lx$next(lx);                                                                               \
        (cex_token_s){ .type = tok_type, .value = { .buf = lx->cur - 1, .len = 1 } };              \
    })

    char c;
    while ((c = lx$peek(lx))) {
        lx$skip_space(lx, c);
        if (!c) {
            break;
        }

        if (isalpha(c) || c == '_' || c == '$') {
            return CexParser__scan_ident(lx);
        }
        if (isdigit(c)) {
            return CexParser__scan_number(lx);
        }

        switch (c) {
            case '\'':
            case '"':
                return CexParser__scan_string(lx);
            case '/':
                if (lx->cur[1] == '/' || lx->cur[1] == '*') {
                    return CexParser__scan_comment(lx);
                } else {
                    break;
                }
                break;
            case '*':
                return tok$new(CexTkn__star);
            case '.':
                return tok$new(CexTkn__dot);
            case ',':
                return tok$new(CexTkn__comma);
            case ';':
                return tok$new(CexTkn__eos);
            case ':':
                return tok$new(CexTkn__colon);
            case '?':
                return tok$new(CexTkn__question);
            case '=': {
                if (lx->cur > lx->content) {
                    switch (lx->cur[-1]) {
                        case '=':
                        case '*':
                        case '/':
                        case '~':
                        case '!':
                        case '+':
                        case '-':
                        case '>':
                        case '<':
                            goto unkn;
                        default:
                            break;
                    }
                }
                switch (lx->cur[1]) {
                    case '=':
                        goto unkn;
                    default:
                        break;
                }

                return tok$new(CexTkn__eq);
            }
            case '{':
            case '(':
            case '[':
                return CexParser__scan_scope(lx);
            case '}':
                return tok$new(CexTkn__rbrace);
            case ')':
                return tok$new(CexTkn__rparen);
            case ']':
                return tok$new(CexTkn__rbracket);
            case '#':
                return CexParser__scan_preproc(lx);
            default:
                break;
        }


    unkn:
        return tok$new(CexTkn__unk);
    }

#undef tok$new
    return (cex_token_s){ 0 }; // EOF
}

cex_token_s
CexParser_next_entity(CexParser_c* lx, arr$(cex_token_s) * children)
{
    uassert(lx->fold_scopes && "CexParser must be with fold_scopes=true");
    uassert(children != NULL && "non initialized arr$");
    uassert(*children != NULL && "non initialized arr$");
    cex_token_s result = { 0 };

    arr$clear(*children);
    cex_token_s t;
    cex_token_s* prev_t = NULL;
    bool has_cex_namespace = false;
    u32 i = 0;
    (void)i;
    while ((t = CexParser_next_token(lx)).type) {
#ifdef CEXTEST
        log$trace("%02d: %-15s %S\n", i, CexTkn_str[t.type], t.value);
#endif
        if (unlikely(t.type == CexTkn__error)) {
            arr$clear(*children);
            return t;
        }

        if (unlikely(!result.type)) {
            result.type = CexTkn__global_misc;
            result.value = t.value;
            if (t.type == CexTkn__preproc) {
                result.value.buf--;
                result.value.len++;
            }
        } else {
            // Extend token text
            result.value.len = t.value.buf - result.value.buf + t.value.len;
            uassert(result.value.len < 1024 * 1024 && "token diff it too high, bad pointers?");
        }
        arr$push(*children, t);
        switch (t.type) {
            case CexTkn__preproc: {
                if (str.slice.match(t.value, "define *\\(*\\)*")) {
                    result.type = CexTkn__macro_func;
                } else if (str.slice.match(t.value, "define *")) {
                    result.type = CexTkn__macro_const;
                } else {
                    result.type = CexTkn__preproc;
                }
                goto end;
            }
            case CexTkn__paren_block: {
                if (result.type == CexTkn__var_decl) {
                    if (!str.slice.match(t.value, "\\(\\(*\\)\\)")) {
                        result.type = CexTkn__func_decl; // Check if not __attribute__(())
                    }
                } else {
                    if (prev_t && prev_t->type == CexTkn__ident) {
                        if (!str.slice.match(prev_t->value, "__attribute__")) {
                            result.type = CexTkn__func_decl;
                        }
                    }
                }
                break;
            }
            case CexTkn__brace_block: {
                if (result.type == CexTkn__func_decl) {
                    result.type = CexTkn__func_def;
                    goto end;
                }
                break;
            }
            case CexTkn__eq: {
                if (has_cex_namespace) {
                    result.type = CexTkn__cex_module_def;
                } else {
                    result.type = CexTkn__var_def;
                }
                break;
            }

            case CexTkn__ident: {
                if (str.slice.match(t.value, "(typedef|struct|enum|union)")) {
                    if (result.type != CexTkn__var_decl) {
                        result.type = CexTkn__typedef;
                    }
                } else if (str.slice.match(t.value, "extern")) {
                    result.type = CexTkn__var_decl;
                } else if (str.slice.match(t.value, "__cex_namespace__*")) {
                    has_cex_namespace = true;
                    if (result.type == CexTkn__var_decl) {
                        result.type = CexTkn__cex_module_decl;
                    } else if (result.type == CexTkn__typedef) {
                        result.type = CexTkn__cex_module_struct;
                    }
                }
                break;
            }
            case CexTkn__eos: {
                goto end;
            }
            default: {
            }
        }
        i++;
        prev_t = &(*children)[arr$len(*children) - 1];
    }
end:
    return result;
}

void
CexParser_decl_free(cex_decl_s* decl, IAllocator alloc)
{
    (void)alloc; // maybe used in the future
    if (decl) {
        sbuf.destroy(&decl->args);
        sbuf.destroy(&decl->ret_type);
    }
}

cex_decl_s*
CexParser_decl_parse(
    cex_token_s decl_token,
    arr$(cex_token_s) children,
    const char* ignore_keywords_pattern,
    IAllocator alloc
)
{
    (void)children;
    switch (decl_token.type) {
        case CexTkn__func_decl:
        case CexTkn__func_def:
        case CexTkn__macro_func:
        case CexTkn__macro_const:
            break;
        default:
            return NULL;
    }
    cex_decl_s* result = mem$new(alloc, cex_decl_s);
    if (decl_token.type == CexTkn__func_decl || decl_token.type == CexTkn__func_def ||
        decl_token.type == CexTkn__macro_func) {
        result->args = sbuf.create(128, alloc);
        result->ret_type = sbuf.create(128, alloc);
    }
    result->type = decl_token.type;
    const char* ignore_pattern =
        "(__attribute__|static|inline|__asm__|extern|volatile|restrict|register|__declspec|noreturn|_Noreturn)";


    cex_token_s prev_t = { 0 };
    bool prev_skipped = false;
    isize name_idx = -1;
    isize args_idx = -1;
    isize idx = -1;
    isize prev_idx = -1;

    for$each(it, children)
    {
        idx++;
        switch (it.type) {
            case CexTkn__ident: {
                if (str.slice.eq(it.value, str$s("static"))) {
                    result->is_static = true;
                }
                if (str.slice.eq(it.value, str$s("inline"))) {
                    result->is_inline = true;
                }
                if (str.slice.match(it.value, ignore_pattern)) {
                    prev_skipped = true;
                    continue;
                }
                if (ignore_keywords_pattern && str.slice.match(it.value, ignore_keywords_pattern)) {
                    prev_skipped = true;
                    continue;
                }
                prev_skipped = false;
                break;
            }
            case CexTkn__bracket_block: {
                if (str.slice.starts_with(it.value, str$s("[["))) {
                    // Clang/C23 attribute
                    continue;
                }
                break;
            }
            case CexTkn__preproc: {
                if (decl_token.type == CexTkn__macro_func) {
                    args_idx = -1;
                    uassert(str.slice.starts_with(it.value, str$s("define")));
                    isize iname_start = str.slice.index_of(it.value, str$s(" "));
                    isize iarg_start = str.slice.index_of(it.value, str$s("("));
                    isize iarg_end = str.slice.index_of(it.value, str$s(")"));

                    if (iname_start < 0 || iarg_start < 0 || iarg_end < 0 ||
                        iname_start > iarg_start || iarg_start > iarg_end) {
#if defined(CEXTEST)
                        log$error("bad macro function: %S\n", it.value);
#endif
                        goto fail;
                    }
                    result->name = str.slice.sub(it.value, iname_start + 1, iarg_start);
                    var margs = str.slice.sub(it.value, iarg_start + 1, iarg_end);
                    if (margs.len > 0) {
                        e$goto(sbuf.appendf(&result->args, "%S", margs), fail);
                    }
                } else if (decl_token.type == CexTkn__macro_const) {
                    uassert(str.slice.starts_with(it.value, str$s("define")));
                    isize iname_start = str.slice.index_of(it.value, str$s(" "));
                    if (iname_start < 0) {
                        goto fail;
                    }
                    var mconst = str.slice.sub(it.value, iname_start + 1, 0);
                    iname_start = str.slice.index_of(mconst, str$s(" "));
                    if (iname_start < 0) {
                        goto fail;
                    }
                    result->name = str.slice.sub(mconst, 0, iname_start);
                }
                break;
            }
            case CexTkn__comment_multi:
            case CexTkn__comment_single: {
                if (result->name.buf == NULL && sbuf.len(&result->ret_type) == 0 &&
                    sbuf.len(&result->args) == 0) {
                    result->docs = it.value;
                }
                break;
            }
            case CexTkn__brace_block: {
                result->body = it.value;
                fallthrough();
            }
            case CexTkn__eos: {
                if (prev_t.type == CexTkn__paren_block) {
                    args_idx = prev_idx;
                }
                break;
            }
            case CexTkn__paren_block: {
                if (prev_skipped) {
                    prev_skipped = false;
                    continue;
                }

                if (prev_t.type == CexTkn__ident) {
                    if (result->name.buf == NULL) {
                        if (str.slice.match(it.value, "\\(\\**\\)")) {
#if defined(CEXTEST)
                            // this looks a function returning function pointer,
                            // we intentionally don't support this, use typedef func ptr
                            log$error(
                                "Skipping function (returns raw fn pointer, use typedef): \n%S\n",
                                decl_token.value
                            );
#endif
                            goto fail;
                        }
                    }
                    // NOTE: name_idx may change until last {} or ;
                    // because in C is common to use MACRO(foo) which may look as args block
                    // we'll check it after the end of entity block
                    result->name = prev_t.value;
                    name_idx = idx;
                }

                break;
            }
            default:
                break;
        }

        prev_skipped = false;
        prev_t = it;
        prev_idx = idx;
    }

#define $append_fmt(buf, tok) /* temp macro, formats return type and arguments */                  \
    switch ((tok).type) {                                                                          \
        case CexTkn__eof:                                                                          \
        case CexTkn__comma:                                                                        \
        case CexTkn__star:                                                                         \
        case CexTkn__dot:                                                                          \
        case CexTkn__rbracket:                                                                     \
        case CexTkn__lbracket:                                                                     \
        case CexTkn__lparen:                                                                       \
        case CexTkn__rparen:                                                                       \
        case CexTkn__paren_block:                                                                  \
        case CexTkn__bracket_block:                                                                \
            break;                                                                                 \
        default:                                                                                   \
            if (sbuf.len(&(buf)) > 0) {                                                            \
                e$goto(sbuf.append(&(buf), " "), fail);                                            \
            }                                                                                      \
    }                                                                                              \
    e$goto(sbuf.appendf(&(buf), "%S", (tok).value), fail);
    //  <<<<<  #define $append_fmt

    // NOTE: parsing return type

    if (name_idx > 0) {
        prev_skipped = false;
        for$each(it, children, name_idx - 1)
        {
            switch (it.type) {
                case CexTkn__ident: {
                    if (str.slice.match(it.value, ignore_pattern)) {
                        prev_skipped = true;
                        // GCC attribute
                        continue;
                    }
                    if (ignore_keywords_pattern &&
                        str.slice.match(it.value, ignore_keywords_pattern)) {
                        prev_skipped = true;
                        // GCC attribute
                        continue;
                    }
                    $append_fmt(result->ret_type, it);
                    break;
                }
                case CexTkn__bracket_block: {
                    if (str.slice.starts_with(it.value, str$s("[["))) {
                        // Clang/C23 attribute
                        continue;
                    }
                    break;
                }
                case CexTkn__comment_multi:
                case CexTkn__comment_single:
                    continue;
                case CexTkn__paren_block: {
                    if (prev_skipped) {
                        prev_skipped = false;
                        continue;
                    }
                    fallthrough();
                }
                default:
                    $append_fmt(result->ret_type, it);
            }
            prev_skipped = false;
        }
    }

    // NOTE: parsing arguments of a function
    if (args_idx >= 0) {
        prev_t = children[args_idx];
        str_s clean_paren_block = str.slice.sub(str.slice.strip(prev_t.value), 1, -1);
        CexParser_c lx = CexParser_create(clean_paren_block.buf, clean_paren_block.len, true);
        cex_token_s t = { 0 };
        bool skip_next = false;
        while ((t = CexParser_next_token(&lx)).type) {
#ifdef CEXTEST
            log$trace("arg token: type: %s '%S'\n", CexTkn_str[t.type], t.value);
#endif
            switch (t.type) {
                case CexTkn__ident: {
                    if (str.slice.match(t.value, ignore_pattern)) {
                        skip_next = true;
                        continue;
                    }
                    if (ignore_keywords_pattern &&
                        str.slice.match(t.value, ignore_keywords_pattern)) {
                        skip_next = true;
                        continue;
                    }
                    break;
                }
                case CexTkn__paren_block: {
                    if (skip_next) {
                        skip_next = false;
                        continue;
                    }
                    break;
                }
                case CexTkn__comment_multi:
                case CexTkn__comment_single:
                    continue;
                case CexTkn__bracket_block: {
                    if (str.slice.starts_with(t.value, str$s("[["))) {
                        continue;
                    }
                    fallthrough();
                }
                default: {
                    break;
                }
            }
            $append_fmt(result->args, t);
            skip_next = false;
        }
    }
#undef $append_fmt
    return result;

fail:
    CexParser_decl_free(result, alloc);
    return NULL;
}


#undef lx$next
#undef lx$peek
#undef lx$skip_space

const struct __cex_namespace__CexParser CexParser = {
    // Autogenerated by CEX
    // clang-format off
    
    .create = CexParser_create,
    .decl_free = CexParser_decl_free,
    .decl_parse = CexParser_decl_parse,
    .next_entity = CexParser_next_entity,
    .next_token = CexParser_next_token,
    .reset = CexParser_reset,
    
    // clang-format on
};



#endif // ifndef CEX_IMPLEMENTATION


#endif // ifndef CEX_HEADER_H
