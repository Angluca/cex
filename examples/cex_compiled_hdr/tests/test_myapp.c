// NOTE: when using linking with cex.obj the CEX_IMPLEMENTATION is not needed
// #define CEX_IMPLEMENTATION

#define CEX_TEST
#include "cex.h"

//test$setup_case() {return EOK;}
//test$teardown_case() {return EOK;}
//test$setup_suite() {return EOK;}
//test$teardown_suite() {return EOK;}

test$case(my_test_case){
    tassert_eq(1, 1);
    return EOK;
}

test$main();
