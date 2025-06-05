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
    if (size < 4 || size > 64) { return -1; }

    fuzz$dnew(data, size);

    mem$scope(tmem$, _)
    {
        u8* prev_d = NULL;
        u16 prev_n = 0;
        u16 n = 0;

        while (fuzz$dget(&n)) {
            if (n == 0) { continue; }
            if (prev_d && prev_n) {
                for (u32 i = 0; i < prev_n; i++) { uassert(prev_d[i] == prev_n % 255); }
                if (fuzz$dprob(0.5)) {
                    prev_d = mem$realloc(_, prev_d, n);
                    if (prev_d) { memset(prev_d, n % 255, n); }
                    prev_n = n;
                } else if (fuzz$dprob(0.2)) {
                    prev_d = mem$free(_, prev_d);
                    uassert(prev_d == NULL);
                }
            }
            u8* d = mem$malloc(_, n);
            if (d) { memset(d, n % 255, n); }

            if (prev_d && prev_n) {
                for (u32 i = 0; i < prev_n; i++) { uassert(prev_d[i] == prev_n % 255); }
                if (prev_n % 2 == 0) {
                    if (fuzz$dprob(0.5)) {
                        prev_d = mem$realloc(_, prev_d, n);
                        if (prev_d) { memset(prev_d, n % 255, n); }
                    } else if (fuzz$dprob(0.3)) {
                        prev_d = mem$free(_, prev_d);
                        uassert(prev_d == NULL);
                    }
                }

                prev_d = d;
                prev_n = n;
            }
        }
    }

    return 0;
}

fuzz$main();
