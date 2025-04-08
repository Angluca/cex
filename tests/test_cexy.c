#define TBUILDDIR "tests/build/cexytest/"
#define cexy$cc_include "-I.","-I" TBUILDDIR
#include "cex/test.h"
#include <cex/all.c>

test$setup_case()
{
    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find(TBUILDDIR "/*", true, _);
        for$each(f, files)
        {
            e$ret(os.fs.remove(f));
        }
        e$ret(os.fs.remove(TBUILDDIR));
    }
    e$ret(os.fs.mkdir("tests/build"));
    e$assert(!os.path.exists(TBUILDDIR) && "must not exist!");
    e$ret(os.fs.mkdir(TBUILDDIR));
    e$assert(os.path.exists(TBUILDDIR) && "must exist!");
    return EOK;
}
test$teardown_case()
{
    return EOK;
}

test$case(test_needs_build_invalid)
{
    mem$scope(tmem$, _)
    {
        tassert_eq(0, cexy.src_changed(NULL, NULL));
        tassert_eq(0, cexy.src_changed("", NULL));

        char* tgt = TBUILDDIR " my_tgt";
        arr$(const char*) src = arr$new(src, _);
        arr$pushm(src, TBUILDDIR "my_tgt.c");
        tassert_eq(0, cexy.src_changed("", src));

        e$ret(io.file.save(src[0], "// hello"));
        tassert(os.path.exists(src[0]));

        e$ret(io.file.save(tgt, ""));
        tassert(os.path.exists(tgt));
    }
    return EOK;
}
test$case(test_needs_build)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR " my_tgt";
        arr$(const char*) src = arr$new(src, _);
        arr$pushm(src, TBUILDDIR "my_tgt.c");

        e$ret(io.file.save(src[0], "// hello"));
        tassert(os.path.exists(src[0]));

        tassert_eq(1, cexy.src_changed(tgt, src));

        e$ret(io.file.save(tgt, ""));
        tassert(os.path.exists(tgt));
        tassert_eq(0, cexy.src_changed(tgt, src));
        os.sleep(1500);
        tassert_eq(0, cexy.src_changed(tgt, src));
        e$ret(io.file.save(src[0], "// world"));
        tassert_eq(1, cexy.src_changed(tgt, src));
    }
    return EOK;
}

test$case(test_needs_build_many_files_not_exists)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR " my_tgt";
        arr$(const char*) src = arr$new(src, _);
        arr$pushm(src, TBUILDDIR "my_src1.c", TBUILDDIR "my_src2");

        e$ret(io.file.save(tgt, ""));
        tassert(os.path.exists(tgt));

        e$ret(io.file.save(src[0], "// hello"));
        tassert(os.path.exists(src[0]));

        tassert_eq(0, cexy.src_changed(tgt, src));
    }
    return EOK;
}

test$case(test_needs_build_many_files)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR " my_tgt";
        arr$(const char*) src = arr$new(src, _);
        arr$pushm(src, TBUILDDIR "my_src1.c", TBUILDDIR "my_src2.c");
        e$ret(io.file.save(src[0], "// hello"));
        e$ret(io.file.save(src[1], "// world"));
        e$ret(io.file.save(tgt, ""));
        tassert_eq(0, cexy.src_changed(tgt, src));

        os.sleep(1500);
        tassert_eq(0, cexy.src_changed(tgt, src));
        e$ret(io.file.save(src[1], "// world again"));
        tassert_eq(1, cexy.src_changed(tgt, src));
    }
    return EOK;
}

test$case(test_src_changed_include_direct_changes)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR " my_tgt";
        char* src = TBUILDDIR "my_src.c";
        tassert_eq(0, cexy.src_include_changed(NULL, src, NULL));
        tassert_eq(0, cexy.src_include_changed(tgt, NULL, NULL));
        tassert_eq(0, cexy.src_include_changed(TBUILDDIR, NULL, NULL)); // directory target
        tassert_eq(0, cexy.src_include_changed(TBUILDDIR, src, NULL));
        tassert_eq(0, cexy.src_include_changed(tgt, TBUILDDIR, NULL));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

        e$ret(io.file.save(src, "// world"));
        tassert_eq(1, cexy.src_include_changed(tgt, src, NULL));

        e$ret(io.file.save(tgt, ""));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

        os.sleep(1500);
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));
        e$ret(io.file.save(src, "// world again"));
        tassert_eq(1, cexy.src_include_changed(tgt, src, NULL));
    }
    return EOK;
}

test$case(test_src_changed_include)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR "my_tgt";
        char* src = TBUILDDIR "my_src.c";
        char* src2 = TBUILDDIR "my_src2.c";

        e$ret(io.file.save(tgt, ""));
        e$ret(io.file.save(src, "#include \"my_src2.c\""));
        e$ret(io.file.save(src2, "// I am include"));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

        os.sleep(1500);
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));
        e$ret(io.file.save(src2, "// I am include again"));
        tassert_eq(1, cexy.src_include_changed(tgt, src, NULL));
    }
    return EOK;
}

test$case(test_src_changed_include_skips_system)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR "my_tgt";
        char* src = TBUILDDIR "my_src.c";
        char* src2 = TBUILDDIR "my_src2.c";

        e$ret(io.file.save(tgt, ""));
        e$ret(io.file.save(src, "#include <my_src2.c>"));
        e$ret(io.file.save(src2, "// I am include"));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));

        os.sleep(1500);
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));
        e$ret(io.file.save(src2, "// I am include again"));
        tassert_eq(0, cexy.src_include_changed(tgt, src, NULL));
    }
    return EOK;
}


test$main();
