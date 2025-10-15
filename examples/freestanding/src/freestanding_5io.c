/*
*  Freestanding implementation for linux platform (no libc, memory allocators, file system, etc)
*/

// Disables all CEX modules except base types, each module have to be re-enabled
#define cex$enable_minimal 
#define cex$enable_mem
#define cex$enable_ds
#define cex$enable_str
#define cex$enable_io


#define CEX_IMPLEMENTATION
#include "cex.h"

#include "platform.c"
#include "platform_mem.c"
#include "platform_str.c"
#include "platform_io.c"

int main(int argc, char** argv){
    (void)argc;
    (void)argv;

    log$info("This is Freestanding with IO\n");
    io.printf("io.printf() works: argv[0]=%s\n", argv[0]);

    log$info("Done\n");

    return 0;
}
