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
    Exc os; /**< Generic OS error */
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
    do {                                                                                           \
        if (unlikely(!((A)))) {                                                                    \
            __cex__fprintf(stdout, "[ASSERT] ", __FILE_NAME__, __LINE__, __func__, "%s\n", #A);    \
            return Error.assert;                                                                   \
        }                                                                                          \
    } while (0)


#define e$assertf(A, format, ...)                                                                  \
    do {                                                                                           \
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
    } while (0)
#else // #if CEX_LOG_LVL > 0
#define __cex__traceback(uerr, fail_func) __cex__fprintf_dummy()
#define e$assert(A)                                                                                \
    do {                                                                                           \
        if (unlikely(!((A)))) {                                                                    \
            return Error.assert;                                                                   \
        }                                                                                          \
    } while (0)


#define e$assertf(A, format, ...)                                                                  \
    do {                                                                                           \
        if (unlikely(!((A)))) {                                                                    \
            return Error.assert;                                                                   \
        }                                                                                          \
    } while (0)
#endif // #if CEX_LOG_LVL > 0


/**
 *                 ASSERTIONS MACROS
 */


#ifdef NDEBUG
#define uassertf(cond, format, ...) ((void)(0))
#define uassert(cond) ((void)(0))
#else


#ifdef __SANITIZE_ADDRESS__
// This should be linked when gcc sanitizer enabled
void __sanitizer_print_stack_trace();
#define sanitizer_stack_trace() __sanitizer_print_stack_trace()
#else
#define sanitizer_stack_trace() ((void)(0))
#endif

#ifndef __cex__assert
#define __cex__assert()                                                                            \
    do {                                                                                           \
        fflush(stdout);                                                                            \
        fflush(stderr);                                                                            \
        sanitizer_stack_trace();                                                                   \
        abort();                                                                                   \
    } while (0);
#endif

#ifdef CEXTEST
// this prevents spamming on stderr (i.e. cextest.h output stream in silent mode)
#define __CEX_OUT_STREAM stdout

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
#endif

/**
 * @def uassert(A)
 * @brief Custom assertion, with support of sanitizer call stack printout at failure.
 */
#define uassert(A)                                                                                 \
    do {                                                                                           \
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
    } while (0)

#define uassertf(A, format, ...)                                                                   \
    do {                                                                                           \
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
    } while (0)
#endif


// __CEX_TMPNAME - internal macro for generating temporary variable names (unique__line_num)
#define __CEX_CONCAT_INNER(a, b) a##b
#define __CEX_CONCAT(a, b) __CEX_CONCAT_INNER(a, b)
#define __CEX_TMPNAME(base) __CEX_CONCAT(base, __LINE__)

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
    for (Exc __CEX_TMPNAME(__cex_err_traceback_) = _func; unlikely(                                \
             (__CEX_TMPNAME(__cex_err_traceback_) != EOK) &&                                       \
             (__cex__traceback(__CEX_TMPNAME(__cex_err_traceback_), #_func))                       \
         );                                                                                        \
         __CEX_TMPNAME(__cex_err_traceback_) = EOK)                                                \
    return __CEX_TMPNAME(__cex_err_traceback_)

#define e$goto(_func, _label)                                                                      \
    for (Exc __CEX_TMPNAME(__cex_err_traceback_) = _func; unlikely(                                \
             (__CEX_TMPNAME(__cex_err_traceback_) != EOK) &&                                       \
             (__cex__traceback(__CEX_TMPNAME(__cex_err_traceback_), #_func))                       \
         );                                                                                        \
         __CEX_TMPNAME(__cex_err_traceback_) = EOK)                                                \
    goto _label


/*
 *                  ARRAYS / SLICES / ITERATORS INTERFACE
 */
typedef struct
{
    void* arr;
    usize len;
} slice_generic_s;
_Static_assert(sizeof(slice_generic_s) == sizeof(usize) * 2, "sizeof");

#define slice$define(eltype)                                                                       \
    union                                                                                          \
    {                                                                                              \
        struct                                                                                     \
        {                                                                                          \
            typeof(eltype)* arr;                                                                   \
            usize len;                                                                             \
        };                                                                                         \
        slice_generic_s base;                                                                      \
    }

#define _arr$slice_get(slice, array, array_len, start, end)                                        \
    {                                                                                              \
        uassert(array != NULL);                                                                    \
        isize _start = start;                                                                      \
        isize _end = end;                                                                          \
        isize _len = array_len;                                                                    \
        if (unlikely(_start < 0))                                                                  \
            _start += _len;                                                                        \
        if (_end == 0) /* _end=0 equivalent of python's arr[_star:] */                             \
            _end = _len;                                                                           \
        else if (unlikely(_end < 0))                                                               \
            _end += _len;                                                                          \
        _end = _end < _len ? _end : _len;                                                          \
        _start = _start > 0 ? _start : 0;                                                          \
        /*log$debug("instart: %d, inend: %d, start: %ld, end: %ld", start, end, _start, _end); */  \
        if (_start < _end) {                                                                       \
            slice.arr = &((array)[_start]);                                                        \
            slice.len = (usize)(_end - _start);                                                    \
        }                                                                                          \
    }


#define arr$slice(array, start, end)                                                               \
    ({                                                                                             \
        slice$define(*array) s = { .arr = NULL, .len = 0 };                                        \
        _arr$slice_get(s, array, arr$len(array), start, end);                                      \
        s;                                                                                         \
    })

#define arr$len(arr) (sizeof(arr) / sizeof(arr[0]))

/**
 * @brief Iterates through array: itvar is struct {eltype* val, usize index}
 */
#define for$array(it, array, length)                                                               \
    struct __CEX_TMPNAME(__cex_arriter_)                                                           \
    {                                                                                              \
        typeof(*array)* val;                                                                       \
        usize idx;                                                                                 \
    };                                                                                             \
    usize __CEX_TMPNAME(__cex_arriter__length) = (length); /* prevents multi call of (length)*/    \
    for (struct __CEX_TMPNAME(__cex_arriter_) it = { .val = array, .idx = 0 };                     \
         it.idx < __CEX_TMPNAME(__cex_arriter__length);                                            \
         it.val++, it.idx++)

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
    union __CEX_TMPNAME(__cex_iter_)                                                               \
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
    for (union __CEX_TMPNAME(__cex_iter_) it = { .val = (iter_func) }; it.val != NULL;             \
         it.val = (iter_func))


/*
 *                 RESOURCE ALLOCATORS
 */

typedef struct Allocator_i
{
    // >>> cacheline
    void* (*malloc)(usize size);
    void* (*calloc)(usize nmemb, usize size);
    void* (*realloc)(void* ptr, usize new_size);
    void* (*malloc_aligned)(usize alignment, usize size);
    void* (*realloc_aligned)(void* ptr, usize alignment, usize new_size);
    void (*free)(void* ptr);
    FILE* (*fopen)(const char* filename, const char* mode);
    int (*open)(const char* pathname, int flags, unsigned int mode);
    //<<< 64 byte cacheline
    int (*fclose)(FILE* stream);
    int (*close)(int fd);
} Allocator_i;
_Static_assert(alignof(Allocator_i) == alignof(usize), "size");
_Static_assert(sizeof(Allocator_i) == sizeof(usize) * 10, "size");


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
