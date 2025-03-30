#pragma once
#include "all.h"
#include "argparse.h"

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

static void
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
static void
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
            fprintf(stderr, "\n============== TEST OUTPUT >>>>>>>=============\n\n");
            int c;
            while ((c = fgetc(ctx->out_stream)) != EOF && c != '\0') {
                putc(c, stderr);
            }
            fprintf(stderr, "\n============== <<<<<< TEST OUTPUT =============\n");
        }
    }
}


static int
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
    ctx->has_ansi = true; // io.has_ansi_support();

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
        argparse.usage(&args);
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
        e$except(err, ctx->setup_suite_fn())
        {
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
        cex_test_mute();
        AllocatorHeap_c* alloc_heap = (AllocatorHeap_c*)mem$;
        alloc_heap->stats.n_allocs = 0;
        alloc_heap->stats.n_free = 0;
        Exc err = EOK;
        if (ctx->setup_case_fn && (ctx->setup_case_fn() != EOK)) {
            fprintf(
                stderr,
                "[%s] test$setup() failed with %s (suite %s stopped)\n",
                ctx->has_ansi ? io$ansi("FAIL", "31") : "FAIL",
                err,
                __FILE__
            );
            return 1;
        }
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
        if (ctx->teardown_case_fn && (ctx->teardown_case_fn() != EOK)) {
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
        Exc err = NULL;                                                                            \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((err = genf((a), (b), __LINE__, _cex_test_eq_op__eq))) {                               \
            _test$tassert_breakpoint();                                                            \
            return err;                                                                            \
        }                                                                                          \
    })

#define tassert_er(a, b)                                                                           \
    ({                                                                                             \
        Exc err = NULL;                                                                            \
        if ((err = _check_eq_err((a), (b), __LINE__))) {                                           \
            _test$tassert_breakpoint();                                                            \
            return err;                                                                            \
        }                                                                                          \
    })

#define tassert_eq_almost(a, b, delta)                                                             \
    ({                                                                                             \
        Exc err = NULL;                                                                            \
        if ((err = _check_eq_almost((a), (b), (delta), __LINE__))) {                               \
            _test$tassert_breakpoint();                                                            \
            return err;                                                                            \
        }                                                                                          \
    })
#define tassert_eq_ptr(a, b)                                                                       \
    ({                                                                                             \
        Exc err = NULL;                                                                            \
        if ((err = _check_eq_ptr((a), (b), __LINE__))) {                                           \
            _test$tassert_breakpoint();                                                            \
            return err;                                                                            \
        }                                                                                          \
    })
#define tassert_eq_mem(a, b...)                                                                        \
    ({                                                                                              \
        var _a = (a);                                                                              \
        var _b = (b);                                                                              \
        _Static_assert(__builtin_types_compatible_p(__typeof__(_a), __typeof__(_b)), "incompatible"); \
        _Static_assert(sizeof(_a) == sizeof(_b), "different size");                                   \
        if (memcmp(&_a, &_b, sizeof(_a)) != 0) {                                                \
            _test$tassert_breakpoint();                                                             \
            if (str.sprintf(                                                                        \
                    _cex_test__mainfn_state.str_buf,                                                \
                    CEXTEST_AMSG_MAX_LEN - 1,                                                       \
                    _test$log_err("a and b are not binary equal")                                  \
                )) {                                                                                \
            }                                                                                       \
            return _cex_test__mainfn_state.str_buf;                                                 \
        }                                                                                           \
    })

#define tassert_eq_arr(a, b...)                                                                       \
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
        Exc err = NULL;                                                                            \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((err = genf((a), (b), __LINE__, _cex_test_eq_op__ne))) {                               \
            _test$tassert_breakpoint();                                                            \
            return err;                                                                            \
        }                                                                                          \
    })

#define tassert_le(a, b)                                                                           \
    ({                                                                                             \
        Exc err = NULL;                                                                            \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((err = genf((a), (b), __LINE__, _cex_test_eq_op__le))) {                               \
            _test$tassert_breakpoint();                                                            \
            return err;                                                                            \
        }                                                                                          \
    })

#define tassert_lt(a, b)                                                                           \
    ({                                                                                             \
        Exc err = NULL;                                                                            \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((err = genf((a), (b), __LINE__, _cex_test_eq_op__lt))) {                               \
            _test$tassert_breakpoint();                                                            \
            return err;                                                                            \
        }                                                                                          \
    })

#define tassert_ge(a, b)                                                                           \
    ({                                                                                             \
        Exc err = NULL;                                                                            \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((err = genf((a), (b), __LINE__, _cex_test_eq_op__ge))) {                               \
            _test$tassert_breakpoint();                                                            \
            return err;                                                                            \
        }                                                                                          \
    })

#define tassert_gt(a, b)                                                                           \
    ({                                                                                             \
        Exc err = NULL;                                                                            \
        var genf = _test$tassert_fn((a), (b));                                                     \
        if ((err = genf((a), (b), __LINE__, _cex_test_eq_op__gt))) {                               \
            _test$tassert_breakpoint();                                                            \
            return err;                                                                            \
        }                                                                                          \
    })
