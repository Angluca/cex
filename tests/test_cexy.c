#include "cex/test.h"
#include <cex/all.c>
#define TBUILDDIR "tests/build/cexytest/"

test$setup_case(){
    e$ret(os.fs.mkdir("tests/build"));
    e$assert(!os.path.exists(TBUILDDIR) && "must not exist!");
    e$ret(os.fs.mkdir(TBUILDDIR));
    e$assert(os.path.exists(TBUILDDIR) && "must exist!");
    return EOK;
}
test$teardown_case(){
    mem$scope(tmem$, _) {
        arr$(char*) files = os.fs.find(TBUILDDIR "/*", true, _);
        for$each(f, files) {
            e$ret(os.fs.remove(f));
        }
        e$ret(os.fs.remove(TBUILDDIR));
    }
    return EOK;
}

test$case(test_needs_build_invalid){
    mem$scope(tmem$, _) {
        tassert_eq(0, cexy$needs_build(NULL, (const char*[]){}));
        tassert_eq(0, cexy$needs_build("", NULL));

        char* tgt = TBUILDDIR " my_tgt"; 
        const char* src[] = {TBUILDDIR " my_tgt"}; 
        tassert_eq(0, cexy$needs_build("",  src));
        
        e$ret(io.file.save(src[0], "// hello"));
        tassert(os.path.exists(src[0]));

        e$ret(io.file.save(tgt, ""));
        tassert(os.path.exists(tgt));
    }
    return EOK;
}
test$case(test_needs_build){
    mem$scope(tmem$, _) {
        char* tgt = TBUILDDIR " my_tgt"; 
        const char* src[] = {TBUILDDIR "my_tgt.c"}; 
        
        e$ret(io.file.save(src[0], "// hello"));
        tassert(os.path.exists(src[0]));

        tassert_eq(1, cexy$needs_build(tgt,  src));

        e$ret(io.file.save(tgt, ""));
        tassert(os.path.exists(tgt));
        tassert_eq(0, cexy$needs_build(tgt,  src));
        os.sleep(1500);
        tassert_eq(0, cexy$needs_build(tgt,  src));
        e$ret(io.file.save(src[0], "// world"));
        tassert_eq(1, cexy$needs_build(tgt,  src));
    }
    return EOK;
}

test$case(test_needs_build_many_files_not_exists){
    mem$scope(tmem$, _) {
        char* tgt = TBUILDDIR " my_tgt"; 
        const char* src[] = {TBUILDDIR "my_src1.c", TBUILDDIR "my_src2"}; 
        
        e$ret(io.file.save(tgt, ""));
        tassert(os.path.exists(tgt));

        e$ret(io.file.save(src[0], "// hello"));
        tassert(os.path.exists(src[0]));

        tassert_eq(0, cexy$needs_build(tgt,  src));
    }
    return EOK;
}

test$case(test_needs_build_many_files){
    mem$scope(tmem$, _) {
        char* tgt = TBUILDDIR " my_tgt"; 
        const char* src[] = {TBUILDDIR "my_src1.c", TBUILDDIR "my_src2"}; 
        e$ret(io.file.save(src[0], "// hello"));
        e$ret(io.file.save(src[1], "// world"));
        e$ret(io.file.save(tgt, ""));
        tassert_eq(0, cexy$needs_build(tgt,  src));

        os.sleep(1500);
        tassert_eq(0, cexy$needs_build(tgt,  src));
        e$ret(io.file.save(src[1], "// world again"));
        tassert_eq(1, cexy$needs_build(tgt,  src));

    }
    return EOK;
}


test$main();
