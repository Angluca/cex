/**
 * @file cex/cex.h
 * @brief CEX core file
 */
#pragma once
#include "cex_header.h"

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
typedef char* Exc;

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
    Exc permission;
    Exc try_again;
} Error;

#ifndef __cex__fprintf

// NOTE: you may try to define our own fprintf
#    define __cex__fprintf(stream, prefix, filename, line, func, format, ...)                      \
        io.fprintf(stream, "%s ( %s:%d %s() ) " format, prefix, filename, line, func, ##__VA_ARGS__)

static inline bool
__cex__fprintf_dummy(void)
{
    return true; // WARN: must always return true!
}

#endif

#ifndef __FILE_NAME__
#    define __FILE_NAME__                                                                          \
        (__builtin_strrchr(__FILE__, '/') ? __builtin_strrchr(__FILE__, '/') + 1 : __FILE__)
#endif

#ifndef _WIN32
#define CEX_NAMESPACE __attribute__((visibility("hidden"))) extern const
#else
#define CEX_NAMESPACE extern const
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
#    define CEX_LOG_LVL 4
#endif

#if CEX_LOG_LVL > 0
#    define log$error(format, ...)                                                                 \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[ERROR]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$error(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 1
#    define log$warn(format, ...)                                                                  \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[WARN]   ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$warn(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 2
#    define log$info(format, ...)                                                                  \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[INFO]   ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$info(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 3
#    define log$debug(format, ...)                                                                 \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[DEBUG]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$debug(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 4
#    define log$trace(format, ...)                                                                 \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[TRACE]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            format,                                                                                \
            ##__VA_ARGS__                                                                          \
        ))
#else
#    define log$trace(format, ...) __cex__fprintf_dummy()
#endif

#if CEX_LOG_LVL > 0
#    define __cex__traceback(uerr, fail_func)                                                      \
        (__cex__fprintf(                                                                           \
            stdout,                                                                                \
            "[^STCK]  ",                                                                           \
            __FILE_NAME__,                                                                         \
            __LINE__,                                                                              \
            __func__,                                                                              \
            "^^^^^ [%s] in function call `%s`\n",                                                  \
            uerr,                                                                                  \
            fail_func                                                                              \
        ))

/**
 * @brief Non disposable assert, returns Error.assert CEX exception when failed
 */
#    define e$assert(A)                                                                             \
        ({                                                                                          \
            if (unlikely(!((A)))) {                                                                 \
                __cex__fprintf(stdout, "[ASSERT] ", __FILE_NAME__, __LINE__, __func__, "%s\n", #A); \
                return Error.assert;                                                                \
            }                                                                                       \
        })


#    define e$assertf(A, format, ...)                                                              \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                __cex__fprintf(                                                                    \
                    stdout,                                                                        \
                    "[ASSERT] ",                                                                   \
                    __FILE_NAME__,                                                                 \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    format "\n",                                                                   \
                    ##__VA_ARGS__                                                                  \
                );                                                                                 \
                return Error.assert;                                                               \
            }                                                                                      \
        })
#else // #if CEX_LOG_LVL > 0
#    define __cex__traceback(uerr, fail_func) __cex__fprintf_dummy()
#    define e$assert(A)                                                                            \
        ({                                                                                         \
            if (unlikely(!((A)))) { return Error.assert; }                                         \
        })


#    define e$assertf(A, format, ...)                                                              \
        ({                                                                                         \
            if (unlikely(!((A)))) { return Error.assert; }                                         \
        })
#endif // #if CEX_LOG_LVL > 0


/**
 *                 ASSERTIONS MACROS
 */
#ifndef mem$asan_enabled
#    if defined(__has_feature)
#        if __has_feature(address_sanitizer)
#            define mem$asan_enabled() 1
#        else
#            define mem$asan_enabled() 0
#        endif
#    else
#        if defined(__SANITIZE_ADDRESS__)
#            define mem$asan_enabled() 1
#        else
#            define mem$asan_enabled() 0
#        endif
#    endif
#endif // mem$asan_enabled

#if mem$asan_enabled()
// This should be linked when gcc sanitizer enabled
void __sanitizer_print_stack_trace();
#    define sanitizer_stack_trace() __sanitizer_print_stack_trace()
#else
#    define sanitizer_stack_trace() ((void)(0))
#endif

#ifdef NDEBUG
#    define uassertf(cond, format, ...) ((void)(0))
#    define uassert(cond) ((void)(0))
#    define uassert_disable() ((void)0)
#    define uassert_enable() ((void)0)
#    define __cex_test_postmortem_exists() 0
#else

#    ifdef CEX_TEST
// this prevents spamming on stderr (i.e. cextest.h output stream in silent mode)
int __cex_test_uassert_enabled = 1;
#        define uassert_disable() __cex_test_uassert_enabled = 0
#        define uassert_enable() __cex_test_uassert_enabled = 1
#        define uassert_is_enabled() (__cex_test_uassert_enabled)
#    else
#        define uassert_disable()                                                                  \
            _Static_assert(false, "uassert_disable() allowed only when compiled with -DCEX_TEST")
#        define uassert_enable() (void)0
#        define uassert_is_enabled() true
#        define __cex_test_postmortem_ctx NULL
#        define __cex_test_postmortem_exists() 0
#        define __cex_test_postmortem_f(ctx)
#    endif // #ifdef CEX_TEST


/**
 * @def uassert(A)
 * @brief Custom assertion, with support of sanitizer call stack printout at failure.
 */
#    define uassert(A)                                                                             \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                __cex__fprintf(                                                                    \
                    (uassert_is_enabled() ? stderr : stdout),                                      \
                    "[ASSERT] ",                                                                   \
                    __FILE_NAME__,                                                                 \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    "%s\n",                                                                        \
                    #A                                                                             \
                );                                                                                 \
                if (uassert_is_enabled()) {                                                        \
                    sanitizer_stack_trace();\
                    __builtin_trap();                                                       \
                }                                                                                  \
            }                                                                                      \
        })

#    define uassertf(A, format, ...)                                                               \
        ({                                                                                         \
            if (unlikely(!((A)))) {                                                                \
                __cex__fprintf(                                                                    \
                    (uassert_is_enabled() ? stderr : stdout),                                      \
                    "[ASSERT] ",                                                                   \
                    __FILE_NAME__,                                                                 \
                    __LINE__,                                                                      \
                    __func__,                                                                      \
                    format "\n",                                                                   \
                    ##__VA_ARGS__                                                                  \
                );                                                                                 \
                if (uassert_is_enabled()) {                                                        \
                    sanitizer_stack_trace();\
                    __builtin_trap();                                                       \
                }                                                                                  \
            }                                                                                      \
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
        __builtin_unreachable();                                                                   \
    })

#if defined(_WIN32) || defined(_WIN64)
#    define breakpoint() __debugbreak()
#elif defined(__APPLE__)
#    define breakpoint() __builtin_debugtrap()
#elif defined(__linux__) || defined(__unix__)
#    define breakpoint() __builtin_trap()
#else
#    include <signal.h>
#    define breakpoint() raise(SIGTRAP)
#endif

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
         unlikely((_var_name != EOK) && (__cex__traceback(_var_name, #_func), 1));                 \
         _var_name = EOK)

#if defined(CEX_TEST) || defined(CEX_BUILD)
#    define e$except_silent(_var_name, _func) e$except (_var_name, _func)
#else
#    define e$except_silent(_var_name, _func)                                                      \
        for (Exc _var_name = _func; unlikely(_var_name != EOK); _var_name = EOK)
#endif

#define e$except_errno(_expression)                                                                \
    for (int _tmp_errno = 0; unlikely(                                                             \
             ((_tmp_errno == 0) && ((_expression) == -1) && ((_tmp_errno = errno), 1) &&           \
              (log$error(                                                                          \
                   "`%s` failed errno: %d, msg: %s\n",                                             \
                   #_expression,                                                                   \
                   _tmp_errno,                                                                     \
                   strerror(_tmp_errno)                                                            \
               ),                                                                                  \
               1) &&                                                                               \
              (errno = _tmp_errno, 1))                                                             \
         );                                                                                        \
         _tmp_errno = 1)

#define e$except_null(_expression)                                                                 \
    if (unlikely(((_expression) == NULL) && (log$error("`%s` returned NULL\n", #_expression), 1)))

#define e$except_true(_expression)                                                                 \
    if (unlikely(((_expression)) && (log$error("`%s` returned non zero\n", #_expression), 1)))

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
        if (unlikely(_slice.start < 0)) _slice.start += _len;                                      \
        if (_slice.end == 0) /* _end=0 equivalent of python's arr[_star:] */                       \
            _slice.end = _len;                                                                     \
        else if (unlikely(_slice.end < 0)) _slice.end += _len;                                     \
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
#define for$iter(it_val_type, it, iter_func)                                                       \
    struct cex$tmpname(__cex_iter_)                                                                \
    {                                                                                              \
        it_val_type val;                                                                           \
        union /* NOTE:  iterator above and this struct shadow each other */                        \
        {                                                                                          \
            cex_iterator_s iterator;                                                               \
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
    for (struct cex$tmpname(__cex_iter_) it = { .val = (iter_func) }; !it.iterator.stopped;        \
         it.val = (iter_func))


#if defined(__GNUC__) && !defined(__clang__)
// NOTE: GCC < 12, has some weird warnings for arr$len temp pragma push + missing-field-initializers
#    if (__GNUC__ < 12)
#        pragma GCC diagnostic ignored "-Wsizeof-pointer-div"
#        pragma GCC diagnostic ignored "-Wmissing-field-initializers"
#    endif
#endif
