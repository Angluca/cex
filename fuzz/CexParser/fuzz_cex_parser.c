#    define CEX_LOG_LVL 0 /* 0 (mute all) - 5 (log$trace) */
#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


__attribute__((constructor(200))) void
init_corp(void)
{
    char* corpus_dir = "fuzz_cex_parser_corpus.tmp";

    uassert(os.path.exists(corpus_dir));
    uassert(os.path.exists("../../src") && "expected to be run from fuzz_cex_parser dir");

    mem$scope(tmem$, _)
    {
        for$each (fn, os.fs.find("../../src/*.[hc]", false, _)) {
            e$except (
                err,
                os.fs.copy(fn, str.fmt(_, "%s/%S", corpus_dir, os.path.split(fn, false)))
            );
        }
    }
}

int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size == 0) { return -1; }

    CexParser_c lx = CexParser.create((char*)data, size, true);
    cex_token_s t;
    while ((t = CexParser.next_token(&lx)).type) {}

    /*
    mem$scope(tmem$, _)
    {
        arr$(cex_token_s) items = arr$new(items, _);
        CexParser_c lx2 = CexParser.create((char*)data, size, true);
        cex_token_s t;
        while ((t = CexParser.next_entity(&lx2, &items)).type) {
            cex_decl_s* d = CexParser.decl_parse(&lx2, t, items, NULL, _);
            if (d == NULL) { continue; }
        }
    }
    */
    return 0;
}
