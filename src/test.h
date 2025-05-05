#pragma once
#include "all.h"
#include "argparse.h"

typedef Exception (*_cex_test_case_f)(void);

#define CEX_TEST_AMSG_MAX_LEN 512
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
    char str_buf[CEX_TEST_AMSG_MAX_LEN];
};

#if defined(__clang__)
#    define test$NOOPT __attribute__((optnone))
#elif defined(__GNUC__) || defined(__GNUG__)
#    define test$NOOPT __attribute__((optimize("O0")))
#elif defined(_MSC_VER)
#    error "MSVC deprecated"
#endif

#define _test$log_err(msg) __FILE__ ":" cex$stringize(__LINE__) " -> " msg
#define _test$tassert_breakpoint()                                                                 \
    ({                                                                                             \
        if (_cex_test__mainfn_state.breakpoint) {                                                  \
            fprintf(stderr, "[BREAK] %s\n", _test$log_err("breakpoint hit"));                      \
            breakpoint();                                                                          \
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


#ifndef CEX_TEST
#    define test$env_check()                                                                       \
        fprintf(stderr, "CEX_TEST was not defined, pass -DCEX_TEST or #define CEX_TEST");          \
        exit(1);
#else
#    define test$env_check() (void)0
#endif

#define test$main()                                                                                \
    _Pragma("GCC diagnostic push"); /* Mingw64:  warning: visibility attribute not supported */    \
    _Pragma("GCC diagnostic ignored \"-Wattributes\"");                                            \
    struct _cex_test_context_s _cex_test__mainfn_state = { .suite_file = __FILE__ };               \
    int main(int argc, char** argv)                                                                \
    {                                                                                              \
        test$env_check();                                                                          \
        argv[0] = __FILE__;                                                                        \
        int ret_code = cex_test_main_fn(argc, argv);                                               \
        if (_cex_test__mainfn_state.test_cases) {                                                  \
            arr$free(_cex_test__mainfn_state.test_cases);                                          \
        }                                                                                          \
        if (_cex_test__mainfn_state.orig_stdout_fd) {                                              \
            close(_cex_test__mainfn_state.orig_stdout_fd);\
        }                                                                                          \
        if (_cex_test__mainfn_state.orig_stderr_fd) {                                              \
            close(_cex_test__mainfn_state.orig_stderr_fd);\
        }                                                                                          \
        return ret_code;                                                                           \
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
                   CEX_TEST_AMSG_MAX_LEN - 1,                                                      \
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
                    CEX_TEST_AMSG_MAX_LEN - 1,                                                     \
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
                    CEX_TEST_AMSG_MAX_LEN - 1,                                                     \
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
                            CEX_TEST_AMSG_MAX_LEN - 1,                                             \
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
