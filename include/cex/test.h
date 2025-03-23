#pragma once
#include <cex/cex_base.h>
#include <float.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#ifndef NAN
#error "NAN is undefined on this platform"
#endif

#ifndef FLT_EPSILON
#error "FLT_EPSILON is undefined on this platform"
#endif

#define CEXTEST_AMSG_MAX_LEN 512
#define CEXTEST_CRED "\033[0;31m"
#define CEXTEST_CGREEN "\033[0;32m"
#define CEXTEST_CNONE "\033[0m"

#ifdef _WIN32
#define CEXTEST_NULL_DEVICE "NUL:"
#else
#define CEXTEST_NULL_DEVICE "/dev/null"
#endif

#if defined(__clang__)
#define test$NOOPT __attribute__((optnone))
#elif defined(__GNUC__) || defined(__GNUG__)
#define test$NOOPT __attribute__((optimize("O0")))
#elif defined(_MSC_VER)
#error "MSVC deprecated"
#endif

struct __CexTestContext_s
{
    FILE* out_stream; // by default uses stdout, however you can replace it by any FILE*
                      // stream
    int tests_run;    // number of tests run
    int tests_failed; // number of tests failed
    int verbosity;    // verbosity level: 0 - only stats, 1 - short test results (. or F) +
                      // stats, 2 - all tests results, 3 - include logs
    const char* in_test;
    // Internal buffer for aassertf() string formatting
    char _str_buf[CEXTEST_AMSG_MAX_LEN];
};


void* __cex_test_postmortem_ctx = NULL;
void (*__cex_test_postmortem_f)(void* ctx) = NULL;


void
__cex_test_run_postmortem()
{
    if (__cex_test_postmortem_exists()) {
        fprintf(stdout, "=========== POSTMORTEM ===========\n");
        fflush(stdout);
        fflush(stderr);
        __cex_test_postmortem_f(__cex_test_postmortem_ctx);
        fflush(stdout);
        fflush(stderr);
        fprintf(stdout, "=========== POSTMORTEM END =======\n");
    }
}
/*
 *
 *  ASSERTS
 *
 */
//
// tassert( condition ) - simple test assert, fails when condition false
//
#define tassert(A)                                                                                 \
    ({                                                                                             \
        if (!(A)) {                                                                                \
            __cex_test_run_postmortem();                                                           \
            return __CEXTEST_LOG_ERR(#A);                                                          \
        }                                                                                          \
    })

//
// tassertf( condition, format_string, ...) - test only assert with printf() style formatting
// error message
//
#define tassertf(A, M, ...)                                                                        \
    ({                                                                                             \
        if (!(A)) {                                                                                \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR(M),                                                              \
                ##__VA_ARGS__                                                                      \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    })

//
// tassert_eqs(char * actual, char * expected) - compares strings via strcmp() (NULL tolerant)
//
#define tassert_eqs(_ac, _ex)                                                                      \
    ({                                                                                             \
        const char* ac = (_ac);                                                                    \
        const char* ex = (_ex);                                                                    \
        if ((ac) == NULL && (ex) != NULL) {                                                        \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("NULL != '%s'"),                                                 \
                (char*)(ex)                                                                        \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        } else if ((ac) != NULL && (ex) == NULL) {                                                 \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("'%s' != NULL"),                                                 \
                (char*)(ac)                                                                        \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        } else if (((ac) != NULL && (ex) != NULL) &&                                               \
                   (strcmp(__CEXTEST_NON_NULL((ac)), __CEXTEST_NON_NULL((ex))) != 0)) {            \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("'%s' != '%s'"),                                                 \
                (char*)(ac),                                                                       \
                (char*)(ex)                                                                        \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    })

//
// tassert_eqs(Exception actual, Exception expected) - compares Exceptions (Error.ok = NULL = EOK)
//
#define tassert_eqe(_ac, _ex)                                                                      \
    ({                                                                                             \
        const char* ac = (_ac);                                                                    \
        const char* ex = (_ex);                                                                    \
        if ((ac) == NULL && (ex) != NULL) {                                                        \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("Error.ok != '%s'"),                                             \
                (char*)(ex)                                                                        \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        } else if ((ac) != NULL && (ex) == NULL) {                                                 \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("'%s' != Error.ok"),                                             \
                (char*)(ac)                                                                        \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        } else if (((ac) != NULL && (ex) != NULL) &&                                               \
                   (strcmp(__CEXTEST_NON_NULL((ac)), __CEXTEST_NON_NULL((ex))) != 0)) {            \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("'%s' != '%s'"),                                                 \
                (char*)(ac),                                                                       \
                (char*)(ex)                                                                        \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    })

//
// tassert_eqi(int actual, int expected) - compares equality of signed integers
//
#define tassert_eqi(_ac, _ex)                                                                      \
    ({                                                                                             \
        long int ac = (_ac);                                                                       \
        long int ex = (_ex);                                                                       \
        if ((ac) != (ex)) {                                                                        \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("%ld != %ld"),                                                   \
                (ac),                                                                              \
                (ex)                                                                               \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    })

//
// tassert_eql(int actual, int expected) - compares equality of long signed integers
//
#define tassert_eql(_ac, _ex)                                                                      \
    ({                                                                                             \
        long long ac = (_ac);                                                                      \
        long long ex = (_ex);                                                                      \
        if ((ac) != (ex)) {                                                                        \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("%lld != %lld"),                                                 \
                (ac),                                                                              \
                (ex)                                                                               \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    })

//
// tassert_eqf(f64 actual, f64 expected) - compares equality of f64 / double numbers
// NAN friendly, i.e. tassert_eqf(NAN, NAN) -- passes
//
#define tassert_eqf(_ac, _ex)                                                                      \
    ({                                                                                             \
        f64 ac = (_ac);                                                                            \
        f64 ex = (_ex);                                                                            \
        int __at_assert_passed = 1;                                                                \
        if (isnan(ac) && !isnan(ex))                                                               \
            __at_assert_passed = 0;                                                                \
        else if (!isnan(ac) && isnan(ex))                                                          \
            __at_assert_passed = 0;                                                                \
        else if (isnan(ac) && isnan(ex))                                                           \
            __at_assert_passed = 1;                                                                \
        else if (fabs((ac) - (ex)) > (f64)FLT_EPSILON)                                             \
            __at_assert_passed = 0;                                                                \
        if (__at_assert_passed == 0) {                                                             \
            snprintf(                                                                              \
                __CexTestContext._str_buf,                                                         \
                CEXTEST_AMSG_MAX_LEN - 1,                                                          \
                __CEXTEST_LOG_ERR("%.10e != %.10e (diff: %0.10e epsilon: %0.10e)"),                \
                (ac),                                                                              \
                (ex),                                                                              \
                ((ac) - (ex)),                                                                     \
                (f64)FLT_EPSILON                                                                   \
            );                                                                                     \
            __cex_test_run_postmortem();                                                           \
            return __CexTestContext._str_buf;                                                      \
        }                                                                                          \
    })


/*
 *
 *  TEST FUNCTIONS
 *
 */
#ifndef CEXTEST
#define test$env_check()                                                                           \
    uassert(false && "CEXTEST was not defined, pass -DCEXTEST or #define CEXTEST")
#else
#define test$env_check() (void)0
#endif

#define test$setup()                                                                               \
    struct __CexTestContext_s __CexTestContext = {                                                 \
        .out_stream = NULL,                                                                        \
        .tests_run = 0,                                                                            \
        .tests_failed = 0,                                                                         \
        .verbosity = 3,                                                                            \
        .in_test = NULL,                                                                           \
    };                                                                                             \
    static test$NOOPT Exc cextest_setup_func()

#define test$teardown() static test$NOOPT Exc cextest_teardown_func()

//
// Typical test function template
//  MUST return Error.ok / EOK on test success, or char* with error message when failed!
//
#define test$case(test_case_name) static test$NOOPT Exc test_case_name()

#define test$set_postmortem(postmortem_func, func_ctx)                                             \
    {                                                                                              \
        __cex_test_postmortem_f = (postmortem_func);                                               \
        __cex_test_postmortem_ctx = (func_ctx);                                                    \
    }                                                                                              \
    while (0)

#define test$run(test_case_name)                                                                   \
    do {                                                                                           \
        if (argc >= 3 && strcmp(argv[2], #test_case_name) != 0) {                                  \
            break;                                                                                 \
        }                                                                                          \
        __CexTestContext.in_test = #test_case_name;                                                \
        Exc err = cextest_setup_func();                                                            \
        if (err != EOK) {                                                                          \
            fprintf(                                                                               \
                __CEX_TEST_STREAM,                                                                 \
                "[%s] %s in test$setup() before %s (suite %s stopped)\n",                          \
                CEXTEST_CRED "FAIL" CEXTEST_CNONE,                                                 \
                err,                                                                               \
                #test_case_name,                                                                   \
                __FILE__                                                                           \
            );                                                                                     \
            return 1;                                                                              \
        } else {                                                                                   \
            err = (test_case_name());                                                              \
        }                                                                                          \
                                                                                                   \
        __CexTestContext.tests_run++;                                                              \
        if (err == EOK) {                                                                          \
            if (__CexTestContext.verbosity > 0) {                                                  \
                if (__CexTestContext.verbosity == 1) {                                             \
                    fprintf(__CEX_TEST_STREAM, ".");                                               \
                } else {                                                                           \
                    fprintf(                                                                       \
                        __CEX_TEST_STREAM,                                                         \
                        "[%s] %s\n",                                                               \
                        CEXTEST_CGREEN "PASS" CEXTEST_CNONE,                                       \
                        #test_case_name                                                            \
                    );                                                                             \
                }                                                                                  \
            }                                                                                      \
        } else {                                                                                   \
            __CexTestContext.tests_failed++;                                                       \
            fprintf(                                                                               \
                __CEX_TEST_STREAM,                                                                 \
                "[%s] %s %s\n",                                                                    \
                CEXTEST_CRED "FAIL" CEXTEST_CNONE,                                                 \
                err,                                                                               \
                #test_case_name                                                                    \
            );                                                                                     \
        }                                                                                          \
        err = cextest_teardown_func();                                                             \
        if (err != EOK) {                                                                          \
            fprintf(                                                                               \
                __CEX_TEST_STREAM,                                                                 \
                "[%s] %s in test$teardown() after %s\n",                                           \
                CEXTEST_CRED "FAIL" CEXTEST_CNONE,                                                 \
                err,                                                                               \
                #test_case_name                                                                    \
            );                                                                                     \
        }                                                                                          \
        __CexTestContext.in_test = NULL;                                                           \
        fflush(__CEX_TEST_STREAM);                                                                 \
        fflush(stdout);                                                                            \
        fflush(stderr);                                                                            \
    } while (0)

//
//  Prints current test header
//
#define test$print_header()                                                                        \
    ({                                                                                             \
        setbuf(__CEX_TEST_STREAM, NULL);                                                           \
        if (__CexTestContext.verbosity > 0) {                                                      \
            fprintf(__CEX_TEST_STREAM, "-------------------------------------\n");                 \
            fprintf(__CEX_TEST_STREAM, "Running Tests: %s\n", __FILE__);                           \
            fprintf(__CEX_TEST_STREAM, "-------------------------------------\n\n");               \
        }                                                                                          \
        fflush(stdout);                                                                            \
        fflush(stderr);                                                                            \
    })

//
// Prints current tests stats total / passed / failed
//
#define test$print_footer()                                                                        \
    ({                                                                                             \
        if (__CexTestContext.verbosity > 0) {                                                      \
            fprintf(__CEX_TEST_STREAM, "\n-------------------------------------\n");               \
            fprintf(                                                                               \
                __CEX_TEST_STREAM,                                                                 \
                "Total: %d Passed: %d Failed: %d\n",                                               \
                __CexTestContext.tests_run,                                                        \
                __CexTestContext.tests_run - __CexTestContext.tests_failed,                        \
                __CexTestContext.tests_failed                                                      \
            );                                                                                     \
            fprintf(__CEX_TEST_STREAM, "-------------------------------------\n");                 \
        } else {                                                                                   \
            fprintf(                                                                               \
                __CEX_TEST_STREAM,                                                                 \
                "[%s] %-70s [%2d/%2d]\n",                                                          \
                __CexTestContext.tests_failed == 0 ? CEXTEST_CGREEN "PASS" CEXTEST_CNONE           \
                                                   : CEXTEST_CRED "FAIL" CEXTEST_CNONE,            \
                __FILE__,                                                                          \
                __CexTestContext.tests_run - __CexTestContext.tests_failed,                        \
                __CexTestContext.tests_run                                                         \
            );                                                                                     \
        }                                                                                          \
        fflush(__CEX_TEST_STREAM);                                                                 \
        fflush(stdout);                                                                            \
        fflush(stderr);                                                                            \
    })

//
//  Utility macro for returning main() exit code based on test failed/run, if no tests
//  run it's an error too
//
#define test$args_parse(argc, argv)                                                                \
    do {                                                                                           \
        test$env_check();                                                                          \
        if (argc == 1)                                                                             \
            break;                                                                                 \
        if (argc > 3) {                                                                            \
            fprintf(                                                                               \
                __CEX_TEST_STREAM,                                                                 \
                "Too many arguments: use test_name_exec vvv|q test_case_name\n"                    \
            );                                                                                     \
            return 1;                                                                              \
        }                                                                                          \
        char a;                                                                                    \
        int i = 0;                                                                                 \
        int _has_quiet = 0;                                                                        \
        int _verb = 0;                                                                             \
        while ((a = argv[1][i]) != '\0') {                                                         \
            if (a == 'q')                                                                          \
                _has_quiet = 1;                                                                    \
            if (a == 'v')                                                                          \
                _verb++;                                                                           \
            i++;                                                                                   \
        }                                                                                          \
        __CexTestContext.verbosity = _has_quiet ? 0 : _verb;                                       \
        if (__CexTestContext.verbosity == 0) {                                                     \
            freopen(CEXTEST_NULL_DEVICE, "w", stdout);                                             \
        }                                                                                          \
    } while (0)

#define test$exit_code() (-1 ? __CexTestContext.tests_run == 0 : __CexTestContext.tests_failed)

/*
 *
 * Utility non public macros
 *
 */
#define __CEX_TEST_STREAM                                                                          \
    (__CexTestContext.out_stream == NULL ? stderr : __CexTestContext.out_stream)
#define __STRINGIZE_DETAIL(x) #x
#define __STRINGIZE(x) __STRINGIZE_DETAIL(x)
#define __CEXTEST_LOG_ERR(msg) (__FILE__ ":" __STRINGIZE(__LINE__) " -> " msg)
#define __CEXTEST_NON_NULL(x) ((x != NULL) ? (x) : "")
