#define TBUILDDIR "tests/build/cexytest/"
#define cexy$cc_include "-I.", "-I" TBUILDDIR
#include "src/all.c"

test$setup_case()
{
    e$ret(os.fs.remove_tree(TBUILDDIR));
    e$assert(!os.path.exists(TBUILDDIR) && "must not exist!");
    e$ret(os.fs.mkpath(TBUILDDIR));
    e$assert(os.path.exists(TBUILDDIR) && "must exist!");
    return EOK;
}
test$teardown_case()
{
    e$ret(os.fs.remove_tree(TBUILDDIR));
    return EOK;
}

test$case(test_target_make)
{
    mem$scope(tmem$, _)
    {
        char* tgt = TBUILDDIR "my_tgt_dir/my_tgt";
        char* src = TBUILDDIR "my_src.c";
        e$assert(!os.path.exists(TBUILDDIR "my_tgt_dir/") && "must not exist!");

        e$ret(io.file.save(src, "#include <my_src2.c>"));
        char* tgt_file = cexy.target_make(src, TBUILDDIR "my_tgt_dir", "my_tgt", _);
        e$assert(os.path.exists(TBUILDDIR "my_tgt_dir/") && "must exist!");
        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(tgt_file, TBUILDDIR "my_tgt_dir\\my_tgt.exe");
        } else {
            tassert_eq(tgt_file, tgt);
        }
        e$assert(!os.path.exists(tgt_file) && "must not exist!");
    }
    return EOK;
}

test$case(test_target_make_with_ext)
{
    mem$scope(tmem$, _)
    {
        char* src = TBUILDDIR "my_src/my_src.c";
        e$assert(!os.path.exists(TBUILDDIR "my_src/") && "must not exist!");
        e$assert(!os.path.exists(TBUILDDIR "my_tgt/") && "must not exist!");
        e$ret(os.fs.mkpath(src));


        e$ret(io.file.save(src, "#include <my_src2.c>"));
        tassert(!os.path.exists(TBUILDDIR "my_tgt_dir/"));
        char* tgt_file = cexy.target_make(src, TBUILDDIR "my_tgt_dir", ".test", _);
        tassert(tgt_file != NULL);
        tassert(os.path.exists(TBUILDDIR "my_tgt_dir/") && "must exist!");
        if (os.platform.current() == OSPlatform__win) {
            tassert_eq(tgt_file, TBUILDDIR "my_tgt_dir\\" TBUILDDIR "my_src/my_src.c.test.exe");
        } else {
            tassert_eq(tgt_file, TBUILDDIR "my_tgt_dir/" TBUILDDIR "my_src/my_src.c.test");
        }
        e$assert(!os.path.exists(tgt_file) && "must not exist!");
    }
    return EOK;
}

test$case(test_needs_build_invalid)
{
    mem$scope(tmem$, _)
    {
        tassert_eq(0, cexy.src_changed(NULL, NULL));
        tassert_eq(0, cexy.src_changed("", NULL));

        char* tgt = TBUILDDIR " my_tgt";
        arr$(char*) src = arr$new(src, _);
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
        arr$(char*) src = arr$new(src, _);
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
        arr$(char*) src = arr$new(src, _);
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
        arr$(char*) src = arr$new(src, _);
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
        (void)_;
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
        (void)_;
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
    return EOK;
}


test$main();
