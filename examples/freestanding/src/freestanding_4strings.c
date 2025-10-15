/*
*  Freestanding implementation for linux platform (no libc, memory allocators, file system, etc)
*/

// Disables all CEX modules except base types, each module have to be re-enabled
#define cex$enable_minimal 
#define cex$enable_mem
#define cex$enable_ds
#define cex$enable_str


#define CEX_IMPLEMENTATION
#include "cex.h"

#include "platform.c"
#include "platform_mem.c"
#include "platform_str.c"

int main(int argc, char** argv){
    (void)argc;
    (void)argv;

    log$info("This is Freestanding with strings\n");

    mem$scope(tmem$, _) {
        char* dyn_str = str.fmt(_, "str.fmt: argv[0]=%s", argv[0]);
        log$info("%s\n", dyn_str);

        sbuf_c sb = sbuf.create(100, _);
        sbuf.appendf(&sb, "sbuf.appendf: 'dyn_str=%s'", dyn_str);
        sbuf.append(&sb, " + static append\n");
        log$info("%s\n", sb);
    }
    log$info("Done\n");

    return 0;
}
