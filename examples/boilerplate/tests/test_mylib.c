#define CEX_IMPLEMENTATION
#include "cex.h"
#include "lib/mylib.c"

test$case(test_make_new_project)
{
    tassert_eq(mylib_add(1,2), 3);
    return EOK;
}

test$main();
