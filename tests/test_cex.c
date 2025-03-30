#include <cex/all.c>

test$NOOPT Exception
foo(int condition)
{
    if (condition == 0) {
        return e$raise(Error.io, "condition == 0");
    }
    if (condition == 2) {
        return Error.memory;
    }

    return EOK;
}

test$NOOPT int
sys_func(int condition)
{
    if (condition == -1) {
        errno = 999;
    }
    return condition;
}

test$NOOPT void*
void_ptr_func(int condition)
{
    if (condition == -1) {
        return NULL;
    }
    return &errno;
}


test$case(test_sysfunc)
{

    int ret = 0;

    errno = 777;
    u32 nit = 0;
    e$except_errno(ret = sys_func(-1))
    {
        log$error("Except: ret=%d errno=%d\n", ret, errno);
        tassert_eq(errno, 999);
        tassert_eq(ret, -1);
        nit++;
    }
    tassert_eq(nit, 1);

    errno = 777;
    nit = 0;
    e$except_errno(ret = sys_func(100))
    {
        tassert(false && "not expected");
        nit++;
    }
    tassert_eq(nit, 0);
    tassert_eq(ret, 100);
    return EOK;
}

Exception
check(int condition)
{
    if (condition == -1) {
        return Error.memory;
    }
    return EOK;
}

Exception
check_with_dollar(int condition)
{
    e$ret(check(condition));
    return EOK;
}

Exception
check_with_assert(int condition)
{
    e$assert(condition != -1);

    return EOK;
}

Exception
check_optimized(int e)
{

    int ret = 0;

    errno = 777;
    u32 nit = 0;
    e$except_errno(ret = sys_func(-1))
    {
        return Error.io;
    }
    (void)nit;
    (void)e;

    e$ret(check(e));

    return EOK;
}

test$case(test_e_dollar_macro)
{
    tassert_eq(EOK, check_with_dollar(true));
    tassert_eq(Error.memory, check_with_dollar(-1));
    return EOK;
}

test$case(test_e_dollar_macro_goto)
{
    Exc result = EOK;
    // On fail jumps to 'fail' label + sets the result to returned by check_with_dollar()
    e$goto(check_with_dollar(-1), setresult);
    tassert(false && "unreacheble, the above must fail");

setresult:
    // e$goto() - previous jump didn't change result
    tassert_er(Error.ok, result);

    // we can explicitly set result value to the error of the call
    e$goto(result = check_with_dollar(-1), fail);
    tassert(false && "unreacheble, the above must fail");

fail:
    tassert_er(Error.memory, result);

    return EOK;
}


test$case(test_null_ptr)
{
    void* res = NULL;
    e$except_null(res = void_ptr_func(1))
    {
        tassert(false && "not expected");
    }
    tassert(res != NULL);

    e$except_null(res = void_ptr_func(-1))
    {
        tassert(res == NULL);
    }
    tassert(res == NULL);

    e$except_null(void_ptr_func(-1))
    {
        tassert(res == NULL);
    }

    return EOK;
}

test$case(test_nested_excepts)
{

    e$except_silent(err, foo(0))
    {
        tassert_er(err, Error.io);

        e$except_silent(err, foo(2))
        {
            tassert_er(err, Error.memory);
        }

        // err, back after nested handling!
        tassert_er(err, Error.io);
    return EOK;
    }

    tassert(false && "unreachable");
    return EOK;
}


test$case(test_all_errors_set)
{
    const Exc* e = &Error.ok;
    tassert(Error.ok == NULL);
    tassert(EOK == NULL);
    tassert(e[0] == NULL && "EOK!");

    for (u32 i = 1; i < sizeof(Error) / sizeof(Error.ok); i++) {
        tassertf(e[i] != NULL, "Error. %d-th member is NULL", i); // Empty error
        tassertf(strlen(e[i]) > 2, "Exception is empty or too short: [%d]: %s", i, e[i]);
    }

    return EOK;
}
test$case(test_eassert)
{
    Exc result = EOK;

    // On fail jumps to 'fail' label + sets the result to returned by check_with_dollar()
    e$goto(result = check_with_assert(-1), fail);

    tassert(false && "unreacheble, the above must fail");

fail:
    tassert_er(Error.assert, result);

    return EOK;
}

test$case(test_log)
{
    log$error("log error test\n");
    log$warn("log warn test\n");
    log$info("log info test\n");
    log$debug("log debug test\n");

    return EOK;
}

test$case(test_arrays_and_slices)
{
    i32 a[] = { 1, 2, 3, 4 };
    tassert_eq(arr$len(a), 4);

    // GOOD: Compiler error, when we use pointers
    i32* p = a;
    (void)p;
    // tassert_eq(arr$len(p), 0);

    slice$define(i32) s;
    _Static_assert(sizeof(s) == sizeof(usize) * 2, "sizeof");

    // slice$define(*a) s2 = (typeof(s2)){.arr = a, .len = 3};

    i32 a_empty[] = {};
    tassert_eq(arr$len(a_empty), 0);
    var s2 = arr$slice(a_empty, .start = -1);
    tassert_eq(s2.len, 0);
    tassert(s2.arr == NULL);

    // GOOD: _start is used in arr$slice as temp var, no conflict!
    isize _start = 2;
    (void)_start;
    tassert_eq(arr$slice(a, 1, 3).arr[1], 3);

    var s3 = arr$slice(a, 1, 3);
    tassert_eq(s3.len, 2);
    tassert_eq(s3.arr[0], 2);
    tassert_eq(s3.arr[1], 3);

    tassert_eq(arr$slice(a, 1, 3).len, 2);

    var s4 = arr$slice(a, 1);
    tassert_eq(s4.len, 3);
    tassert_eq(s4.arr[0], 2);
    tassert_eq(s4.arr[1], 3);
    tassert_eq(s4.arr[2], 4);

    var s5 = arr$slice(a, .end = -2);
    tassert_eq(s5.len, 2);
    tassert_eq(s5.arr[0], 1);
    tassert_eq(s5.arr[1], 2);

    var s6 = arr$slice(a, .start = 1, .end = -2);
    tassert_eq(s6.len, 1);
    tassert_eq(s6.arr[0], 2);

    // NOTE: slice_py is auto-generate by python script, in order to replicate pytnon's
    //       slice behavior. If end=0, it treated as a[start:] slice operation.
    struct slice_py
    {
        i32 start;
        i32 end;
        usize len;
        i32 slice[4];
    } slice_expected[] = {
        { -10, -10, 0, {} },
        { -10, -9, 0, {} },
        { -10, -8, 0, {} },
        { -10, -7, 0, {} },
        { -10, -6, 0, {} },
        { -10, -5, 0, {} },
        { -10, -4, 0, {} },
        { -10, -3, 1, { 1 } },
        { -10, -2, 2, { 1, 2 } },
        { -10, -1, 3, { 1, 2, 3 } },
        { -10, 0, 4, { 1, 2, 3, 4 } },
        { -10, 1, 1, { 1 } },
        { -10, 2, 2, { 1, 2 } },
        { -10, 3, 3, { 1, 2, 3 } },
        { -10, 4, 4, { 1, 2, 3, 4 } },
        { -10, 5, 4, { 1, 2, 3, 4 } },
        { -10, 6, 4, { 1, 2, 3, 4 } },
        { -10, 7, 4, { 1, 2, 3, 4 } },
        { -10, 8, 4, { 1, 2, 3, 4 } },
        { -10, 9, 4, { 1, 2, 3, 4 } },
        { -10, 10, 4, { 1, 2, 3, 4 } },
        { -9, -10, 0, {} },
        { -9, -9, 0, {} },
        { -9, -8, 0, {} },
        { -9, -7, 0, {} },
        { -9, -6, 0, {} },
        { -9, -5, 0, {} },
        { -9, -4, 0, {} },
        { -9, -3, 1, { 1 } },
        { -9, -2, 2, { 1, 2 } },
        { -9, -1, 3, { 1, 2, 3 } },
        { -9, 0, 4, { 1, 2, 3, 4 } },
        { -9, 1, 1, { 1 } },
        { -9, 2, 2, { 1, 2 } },
        { -9, 3, 3, { 1, 2, 3 } },
        { -9, 4, 4, { 1, 2, 3, 4 } },
        { -9, 5, 4, { 1, 2, 3, 4 } },
        { -9, 6, 4, { 1, 2, 3, 4 } },
        { -9, 7, 4, { 1, 2, 3, 4 } },
        { -9, 8, 4, { 1, 2, 3, 4 } },
        { -9, 9, 4, { 1, 2, 3, 4 } },
        { -9, 10, 4, { 1, 2, 3, 4 } },
        { -8, -10, 0, {} },
        { -8, -9, 0, {} },
        { -8, -8, 0, {} },
        { -8, -7, 0, {} },
        { -8, -6, 0, {} },
        { -8, -5, 0, {} },
        { -8, -4, 0, {} },
        { -8, -3, 1, { 1 } },
        { -8, -2, 2, { 1, 2 } },
        { -8, -1, 3, { 1, 2, 3 } },
        { -8, 0, 4, { 1, 2, 3, 4 } },
        { -8, 1, 1, { 1 } },
        { -8, 2, 2, { 1, 2 } },
        { -8, 3, 3, { 1, 2, 3 } },
        { -8, 4, 4, { 1, 2, 3, 4 } },
        { -8, 5, 4, { 1, 2, 3, 4 } },
        { -8, 6, 4, { 1, 2, 3, 4 } },
        { -8, 7, 4, { 1, 2, 3, 4 } },
        { -8, 8, 4, { 1, 2, 3, 4 } },
        { -8, 9, 4, { 1, 2, 3, 4 } },
        { -8, 10, 4, { 1, 2, 3, 4 } },
        { -7, -10, 0, {} },
        { -7, -9, 0, {} },
        { -7, -8, 0, {} },
        { -7, -7, 0, {} },
        { -7, -6, 0, {} },
        { -7, -5, 0, {} },
        { -7, -4, 0, {} },
        { -7, -3, 1, { 1 } },
        { -7, -2, 2, { 1, 2 } },
        { -7, -1, 3, { 1, 2, 3 } },
        { -7, 0, 4, { 1, 2, 3, 4 } },
        { -7, 1, 1, { 1 } },
        { -7, 2, 2, { 1, 2 } },
        { -7, 3, 3, { 1, 2, 3 } },
        { -7, 4, 4, { 1, 2, 3, 4 } },
        { -7, 5, 4, { 1, 2, 3, 4 } },
        { -7, 6, 4, { 1, 2, 3, 4 } },
        { -7, 7, 4, { 1, 2, 3, 4 } },
        { -7, 8, 4, { 1, 2, 3, 4 } },
        { -7, 9, 4, { 1, 2, 3, 4 } },
        { -7, 10, 4, { 1, 2, 3, 4 } },
        { -6, -10, 0, {} },
        { -6, -9, 0, {} },
        { -6, -8, 0, {} },
        { -6, -7, 0, {} },
        { -6, -6, 0, {} },
        { -6, -5, 0, {} },
        { -6, -4, 0, {} },
        { -6, -3, 1, { 1 } },
        { -6, -2, 2, { 1, 2 } },
        { -6, -1, 3, { 1, 2, 3 } },
        { -6, 0, 4, { 1, 2, 3, 4 } },
        { -6, 1, 1, { 1 } },
        { -6, 2, 2, { 1, 2 } },
        { -6, 3, 3, { 1, 2, 3 } },
        { -6, 4, 4, { 1, 2, 3, 4 } },
        { -6, 5, 4, { 1, 2, 3, 4 } },
        { -6, 6, 4, { 1, 2, 3, 4 } },
        { -6, 7, 4, { 1, 2, 3, 4 } },
        { -6, 8, 4, { 1, 2, 3, 4 } },
        { -6, 9, 4, { 1, 2, 3, 4 } },
        { -6, 10, 4, { 1, 2, 3, 4 } },
        { -5, -10, 0, {} },
        { -5, -9, 0, {} },
        { -5, -8, 0, {} },
        { -5, -7, 0, {} },
        { -5, -6, 0, {} },
        { -5, -5, 0, {} },
        { -5, -4, 0, {} },
        { -5, -3, 1, { 1 } },
        { -5, -2, 2, { 1, 2 } },
        { -5, -1, 3, { 1, 2, 3 } },
        { -5, 0, 4, { 1, 2, 3, 4 } },
        { -5, 1, 1, { 1 } },
        { -5, 2, 2, { 1, 2 } },
        { -5, 3, 3, { 1, 2, 3 } },
        { -5, 4, 4, { 1, 2, 3, 4 } },
        { -5, 5, 4, { 1, 2, 3, 4 } },
        { -5, 6, 4, { 1, 2, 3, 4 } },
        { -5, 7, 4, { 1, 2, 3, 4 } },
        { -5, 8, 4, { 1, 2, 3, 4 } },
        { -5, 9, 4, { 1, 2, 3, 4 } },
        { -5, 10, 4, { 1, 2, 3, 4 } },
        { -4, -10, 0, {} },
        { -4, -9, 0, {} },
        { -4, -8, 0, {} },
        { -4, -7, 0, {} },
        { -4, -6, 0, {} },
        { -4, -5, 0, {} },
        { -4, -4, 0, {} },
        { -4, -3, 1, { 1 } },
        { -4, -2, 2, { 1, 2 } },
        { -4, -1, 3, { 1, 2, 3 } },
        { -4, 0, 4, { 1, 2, 3, 4 } },
        { -4, 1, 1, { 1 } },
        { -4, 2, 2, { 1, 2 } },
        { -4, 3, 3, { 1, 2, 3 } },
        { -4, 4, 4, { 1, 2, 3, 4 } },
        { -4, 5, 4, { 1, 2, 3, 4 } },
        { -4, 6, 4, { 1, 2, 3, 4 } },
        { -4, 7, 4, { 1, 2, 3, 4 } },
        { -4, 8, 4, { 1, 2, 3, 4 } },
        { -4, 9, 4, { 1, 2, 3, 4 } },
        { -4, 10, 4, { 1, 2, 3, 4 } },
        { -3, -10, 0, {} },
        { -3, -9, 0, {} },
        { -3, -8, 0, {} },
        { -3, -7, 0, {} },
        { -3, -6, 0, {} },
        { -3, -5, 0, {} },
        { -3, -4, 0, {} },
        { -3, -3, 0, {} },
        { -3, -2, 1, { 2 } },
        { -3, -1, 2, { 2, 3 } },
        { -3, 0, 3, { 2, 3, 4 } },
        { -3, 1, 0, {} },
        { -3, 2, 1, { 2 } },
        { -3, 3, 2, { 2, 3 } },
        { -3, 4, 3, { 2, 3, 4 } },
        { -3, 5, 3, { 2, 3, 4 } },
        { -3, 6, 3, { 2, 3, 4 } },
        { -3, 7, 3, { 2, 3, 4 } },
        { -3, 8, 3, { 2, 3, 4 } },
        { -3, 9, 3, { 2, 3, 4 } },
        { -3, 10, 3, { 2, 3, 4 } },
        { -2, -10, 0, {} },
        { -2, -9, 0, {} },
        { -2, -8, 0, {} },
        { -2, -7, 0, {} },
        { -2, -6, 0, {} },
        { -2, -5, 0, {} },
        { -2, -4, 0, {} },
        { -2, -3, 0, {} },
        { -2, -2, 0, {} },
        { -2, -1, 1, { 3 } },
        { -2, 0, 2, { 3, 4 } },
        { -2, 1, 0, {} },
        { -2, 2, 0, {} },
        { -2, 3, 1, { 3 } },
        { -2, 4, 2, { 3, 4 } },
        { -2, 5, 2, { 3, 4 } },
        { -2, 6, 2, { 3, 4 } },
        { -2, 7, 2, { 3, 4 } },
        { -2, 8, 2, { 3, 4 } },
        { -2, 9, 2, { 3, 4 } },
        { -2, 10, 2, { 3, 4 } },
        { -1, -10, 0, {} },
        { -1, -9, 0, {} },
        { -1, -8, 0, {} },
        { -1, -7, 0, {} },
        { -1, -6, 0, {} },
        { -1, -5, 0, {} },
        { -1, -4, 0, {} },
        { -1, -3, 0, {} },
        { -1, -2, 0, {} },
        { -1, -1, 0, {} },
        { -1, 0, 1, { 4 } },
        { -1, 1, 0, {} },
        { -1, 2, 0, {} },
        { -1, 3, 0, {} },
        { -1, 4, 1, { 4 } },
        { -1, 5, 1, { 4 } },
        { -1, 6, 1, { 4 } },
        { -1, 7, 1, { 4 } },
        { -1, 8, 1, { 4 } },
        { -1, 9, 1, { 4 } },
        { -1, 10, 1, { 4 } },
        { 0, -10, 0, {} },
        { 0, -9, 0, {} },
        { 0, -8, 0, {} },
        { 0, -7, 0, {} },
        { 0, -6, 0, {} },
        { 0, -5, 0, {} },
        { 0, -4, 0, {} },
        { 0, -3, 1, { 1 } },
        { 0, -2, 2, { 1, 2 } },
        { 0, -1, 3, { 1, 2, 3 } },
        { 0, 0, 4, { 1, 2, 3, 4 } },
        { 0, 1, 1, { 1 } },
        { 0, 2, 2, { 1, 2 } },
        { 0, 3, 3, { 1, 2, 3 } },
        { 0, 4, 4, { 1, 2, 3, 4 } },
        { 0, 5, 4, { 1, 2, 3, 4 } },
        { 0, 6, 4, { 1, 2, 3, 4 } },
        { 0, 7, 4, { 1, 2, 3, 4 } },
        { 0, 8, 4, { 1, 2, 3, 4 } },
        { 0, 9, 4, { 1, 2, 3, 4 } },
        { 0, 10, 4, { 1, 2, 3, 4 } },
        { 1, -10, 0, {} },
        { 1, -9, 0, {} },
        { 1, -8, 0, {} },
        { 1, -7, 0, {} },
        { 1, -6, 0, {} },
        { 1, -5, 0, {} },
        { 1, -4, 0, {} },
        { 1, -3, 0, {} },
        { 1, -2, 1, { 2 } },
        { 1, -1, 2, { 2, 3 } },
        { 1, 0, 3, { 2, 3, 4 } },
        { 1, 1, 0, {} },
        { 1, 2, 1, { 2 } },
        { 1, 3, 2, { 2, 3 } },
        { 1, 4, 3, { 2, 3, 4 } },
        { 1, 5, 3, { 2, 3, 4 } },
        { 1, 6, 3, { 2, 3, 4 } },
        { 1, 7, 3, { 2, 3, 4 } },
        { 1, 8, 3, { 2, 3, 4 } },
        { 1, 9, 3, { 2, 3, 4 } },
        { 1, 10, 3, { 2, 3, 4 } },
        { 2, -10, 0, {} },
        { 2, -9, 0, {} },
        { 2, -8, 0, {} },
        { 2, -7, 0, {} },
        { 2, -6, 0, {} },
        { 2, -5, 0, {} },
        { 2, -4, 0, {} },
        { 2, -3, 0, {} },
        { 2, -2, 0, {} },
        { 2, -1, 1, { 3 } },
        { 2, 0, 2, { 3, 4 } },
        { 2, 1, 0, {} },
        { 2, 2, 0, {} },
        { 2, 3, 1, { 3 } },
        { 2, 4, 2, { 3, 4 } },
        { 2, 5, 2, { 3, 4 } },
        { 2, 6, 2, { 3, 4 } },
        { 2, 7, 2, { 3, 4 } },
        { 2, 8, 2, { 3, 4 } },
        { 2, 9, 2, { 3, 4 } },
        { 2, 10, 2, { 3, 4 } },
        { 3, -10, 0, {} },
        { 3, -9, 0, {} },
        { 3, -8, 0, {} },
        { 3, -7, 0, {} },
        { 3, -6, 0, {} },
        { 3, -5, 0, {} },
        { 3, -4, 0, {} },
        { 3, -3, 0, {} },
        { 3, -2, 0, {} },
        { 3, -1, 0, {} },
        { 3, 0, 1, { 4 } },
        { 3, 1, 0, {} },
        { 3, 2, 0, {} },
        { 3, 3, 0, {} },
        { 3, 4, 1, { 4 } },
        { 3, 5, 1, { 4 } },
        { 3, 6, 1, { 4 } },
        { 3, 7, 1, { 4 } },
        { 3, 8, 1, { 4 } },
        { 3, 9, 1, { 4 } },
        { 3, 10, 1, { 4 } },
        { 4, -10, 0, {} },
        { 4, -9, 0, {} },
        { 4, -8, 0, {} },
        { 4, -7, 0, {} },
        { 4, -6, 0, {} },
        { 4, -5, 0, {} },
        { 4, -4, 0, {} },
        { 4, -3, 0, {} },
        { 4, -2, 0, {} },
        { 4, -1, 0, {} },
        { 4, 0, 0, {} },
        { 4, 1, 0, {} },
        { 4, 2, 0, {} },
        { 4, 3, 0, {} },
        { 4, 4, 0, {} },
        { 4, 5, 0, {} },
        { 4, 6, 0, {} },
        { 4, 7, 0, {} },
        { 4, 8, 0, {} },
        { 4, 9, 0, {} },
        { 4, 10, 0, {} },
        { 5, -10, 0, {} },
        { 5, -9, 0, {} },
        { 5, -8, 0, {} },
        { 5, -7, 0, {} },
        { 5, -6, 0, {} },
        { 5, -5, 0, {} },
        { 5, -4, 0, {} },
        { 5, -3, 0, {} },
        { 5, -2, 0, {} },
        { 5, -1, 0, {} },
        { 5, 0, 0, {} },
        { 5, 1, 0, {} },
        { 5, 2, 0, {} },
        { 5, 3, 0, {} },
        { 5, 4, 0, {} },
        { 5, 5, 0, {} },
        { 5, 6, 0, {} },
        { 5, 7, 0, {} },
        { 5, 8, 0, {} },
        { 5, 9, 0, {} },
        { 5, 10, 0, {} },
        { 6, -10, 0, {} },
        { 6, -9, 0, {} },
        { 6, -8, 0, {} },
        { 6, -7, 0, {} },
        { 6, -6, 0, {} },
        { 6, -5, 0, {} },
        { 6, -4, 0, {} },
        { 6, -3, 0, {} },
        { 6, -2, 0, {} },
        { 6, -1, 0, {} },
        { 6, 0, 0, {} },
        { 6, 1, 0, {} },
        { 6, 2, 0, {} },
        { 6, 3, 0, {} },
        { 6, 4, 0, {} },
        { 6, 5, 0, {} },
        { 6, 6, 0, {} },
        { 6, 7, 0, {} },
        { 6, 8, 0, {} },
        { 6, 9, 0, {} },
        { 6, 10, 0, {} },
        { 7, -10, 0, {} },
        { 7, -9, 0, {} },
        { 7, -8, 0, {} },
        { 7, -7, 0, {} },
        { 7, -6, 0, {} },
        { 7, -5, 0, {} },
        { 7, -4, 0, {} },
        { 7, -3, 0, {} },
        { 7, -2, 0, {} },
        { 7, -1, 0, {} },
        { 7, 0, 0, {} },
        { 7, 1, 0, {} },
        { 7, 2, 0, {} },
        { 7, 3, 0, {} },
        { 7, 4, 0, {} },
        { 7, 5, 0, {} },
        { 7, 6, 0, {} },
        { 7, 7, 0, {} },
        { 7, 8, 0, {} },
        { 7, 9, 0, {} },
        { 7, 10, 0, {} },
        { 8, -10, 0, {} },
        { 8, -9, 0, {} },
        { 8, -8, 0, {} },
        { 8, -7, 0, {} },
        { 8, -6, 0, {} },
        { 8, -5, 0, {} },
        { 8, -4, 0, {} },
        { 8, -3, 0, {} },
        { 8, -2, 0, {} },
        { 8, -1, 0, {} },
        { 8, 0, 0, {} },
        { 8, 1, 0, {} },
        { 8, 2, 0, {} },
        { 8, 3, 0, {} },
        { 8, 4, 0, {} },
        { 8, 5, 0, {} },
        { 8, 6, 0, {} },
        { 8, 7, 0, {} },
        { 8, 8, 0, {} },
        { 8, 9, 0, {} },
        { 8, 10, 0, {} },
        { 9, -10, 0, {} },
        { 9, -9, 0, {} },
        { 9, -8, 0, {} },
        { 9, -7, 0, {} },
        { 9, -6, 0, {} },
        { 9, -5, 0, {} },
        { 9, -4, 0, {} },
        { 9, -3, 0, {} },
        { 9, -2, 0, {} },
        { 9, -1, 0, {} },
        { 9, 0, 0, {} },
        { 9, 1, 0, {} },
        { 9, 2, 0, {} },
        { 9, 3, 0, {} },
        { 9, 4, 0, {} },
        { 9, 5, 0, {} },
        { 9, 6, 0, {} },
        { 9, 7, 0, {} },
        { 9, 8, 0, {} },
        { 9, 9, 0, {} },
        { 9, 10, 0, {} },
        { 10, -10, 0, {} },
        { 10, -9, 0, {} },
        { 10, -8, 0, {} },
        { 10, -7, 0, {} },
        { 10, -6, 0, {} },
        { 10, -5, 0, {} },
        { 10, -4, 0, {} },
        { 10, -3, 0, {} },
        { 10, -2, 0, {} },
        { 10, -1, 0, {} },
        { 10, 0, 0, {} },
        { 10, 1, 0, {} },
        { 10, 2, 0, {} },
        { 10, 3, 0, {} },
        { 10, 4, 0, {} },
        { 10, 5, 0, {} },
        { 10, 6, 0, {} },
        { 10, 7, 0, {} },
        { 10, 8, 0, {} },
        { 10, 9, 0, {} },
        { 10, 10, 0, {} },
    };
    tassert_eq(441, arr$len(slice_expected));

    for$eachp(it, slice_expected, arr$len(slice_expected))
    {
        var s4 = arr$slice(a, it->start, it->end);
        tassertf(
            s4.len == it->len,
            "slice.len mismatch: start: %d end: %d, slice.len: %zu expected.len %zu",
            it->start,
            it->end,
            s4.len,
            it->len
        );

        for(usize i = 0; i < s4.len; i++){
            tassert_eq(s4.arr[i], it->slice[i]);
        }
        if (s4.len == 0){
            tassert(s4.arr == NULL && "must be NULL when empty");
        }
    }

    return EOK;
}

test$main();
