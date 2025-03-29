#include <cex/all.c>

test$case(foo){
    tassert(true);
    tassertf(1, "hi %d", 2);
    tassert_eqi(1, 1);
    tassert_eqs("s", "s");
    tassert_eqe(EOK, "ook");
    tassert_eqf(NAN, NAN);
    tassert_eql(2, 3);
    return EOK;
}


test$case(bar)
{
    fprintf(stdout, "stdout suppressed\n");
    return EOK;
}

test$case(bar_print)
{
    // fprintf(stderr, "stderr msg\n");
    fprintf(stdout, "stdout msg\n");
    fprintf(stdout, "stdout captured\n");
    return Error.runtime;
}


test$case(bar_no_print)
{
    return "some fail";
}

test$case(asan_fail)
{
    // uassert(false && "fuck");
    // char b[1] = {0};
    // char* pb = b;
    // printf("%c", pb[30]);
    return EOK;
}

test$main();
