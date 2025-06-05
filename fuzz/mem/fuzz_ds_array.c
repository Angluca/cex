#define CEX_LOG_LVL 0 /* 0 (mute all) - 5 (log$trace) */
#define CEX_TEST
#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

int
fuzz$case(const u8* data, usize size)
{
    if (size < 8) { return 0; }
    fuzz$dnew(data, size);

    mem$scope(tmem$, _)
    {
        u16 n = 0;
        arr$(u32) items = arr$new(items, _);

        while (fuzz$dget(&n)) {
            if (n == 0) { continue; }
            arr$push(items, n);
            for$eachp (it, items) { *it = 0xf0; }

            u8* d = mem$malloc(_, n);
            if (d) { memset(d, n % 255, n); }

            arr$push(items, n);
            for$eachp (it, items) { *it = 0xf0; }
        }
    }

    return 0;
}

fuzz$main();
