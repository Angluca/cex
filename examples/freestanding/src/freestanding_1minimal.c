/*
*  Freestanding implementation for linux platform (no libc, memory allocators, file system, etc)
*/

#include "platform.c"
// Disables all CEX modules except base types, each module have to be re-enabled
#define cex$enable_minimal 
#define CEX_IMPLEMENTATION
#include "cex.h"

Exception other_func(i32 arg) {
    e$assert(arg > 0);
    e$assert(arg < 0);
    return EOK;
}
Exception some_func(i32 arg) {
    if (arg == 0) {
        return EOK;
    } else {
        e$ret(other_func(arg));
        return EOK;
    }
}

int main(int argc, char** argv){
    (void)argc;
    (void)argv;
    write_stdout("It's main()\n");
    write_stdout("argv[0]: ");
    write_stdout(argv[0]);
    write_stdout("\n");

    if(some_func(argc)) {
        write_stdout("Opps error\n");
    }

    log$info("This is CEX log!\n");
    uassert(true && "oops");

    return 0;
}
