#define TBUILDDIR "tests/build/cexytest/"
#define CEX_LOG_LVL 4
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#define cexy$cex_self_cc "cc"
#include "src/all.c"

test$setup_case()
{
    e$ret(os.fs.remove_tree(TBUILDDIR));
    return EOK;
}
test$teardown_case()
{
    e$ret(os.fs.remove_tree(TBUILDDIR));
    return EOK;
}

test$case(test_make_new_project)
{
    tassert_er(EOK, cexy.utils.make_new_project(TBUILDDIR));
    tassert(os.path.exists(TBUILDDIR "cex.h"));
    tassert(os.path.exists(TBUILDDIR "tests"));
    tassert(os.path.exists(TBUILDDIR "lib/mylib.c"));
    tassert(os.path.exists(TBUILDDIR "lib/mylib.h"));
    tassert(os.path.exists(TBUILDDIR "src/myapp.c"));

    return EOK;
}

test$main();
