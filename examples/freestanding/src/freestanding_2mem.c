/*
*  Freestanding implementation for linux platform (no libc, memory allocators, file system, etc)
*/

// Disables all CEX modules except base types, each module have to be re-enabled
#define cex$enable_minimal 
#define cex$enable_mem
#define CEX_IMPLEMENTATION
#include "cex.h"

#include "platform.c"
#include "platform_mem.c"

int main(int argc, char** argv){
    (void)argc;
    (void)argv;

    log$info("This is Freestanding with mem\n");
    // Base allocator test
    u8* p = malloc(8);
    uassert(p);
    uassert((usize)p % 8 == 0);
    u8* p2 = malloc(8);
    uassert(p2);
    uassert((usize)p2 % 8 == 0);
    uassert(p2 - p == 16);

    memset(p, 0xbe, 8);
    uassert(p[0] == 0xbe);
    uassert(p[7] == 0xbe);
    uassert(p[8] != 0xbe); // out of allocation access 

    u8* old_p = p;
    p = realloc(p, 16);
    uassert(p);
    uassert(p != old_p);
    uassert(p[0] == 0xbe);
    uassert(p[7] == 0xbe);
    uassert(p[9] != 0xbe);

    free(p);
    uassert(p[0] == 0xaf);
    uassert(p[7] == 0xaf);

    mem$scope(tmem$, _) {
        char* s = mem$calloc(_, 1, 10);
        uassert(s && "memory error");
        char* p = s;
        for (char c = '1'; c < '9'; c++) {
            *p = c;
            p++;
        }
        log$info("alloc str: '%s'\n", s);
    }

    log$info("Done\n");

    return 0;
}
