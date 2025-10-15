/*
*  Freestanding implementation for linux platform (no libc, memory allocators, file system, etc)
*/

// Disables all CEX modules except base types, each module have to be re-enabled
#define cex$enable_minimal 
#define cex$enable_mem
#define cex$enable_ds


#define CEX_IMPLEMENTATION
#include "cex.h"

#include "platform.c"
#include "platform_mem.c"

int main(int argc, char** argv){
    (void)argc;
    (void)argv;

    log$info("This is Freestanding with data structures\n");

    mem$scope(tmem$, _) {
        arr$(i32) arr = arr$new(arr, mem$);

        for(u32 i = 0; i < 100; i++)
        {
            arr$push(arr, i);
        }

        for$eachp(it, arr) {
            isize idx = it - arr;
            log$info("arr[%zd] = %d\n", idx, *it);
        }
    }
    log$info("Done\n");

    return 0;
}
