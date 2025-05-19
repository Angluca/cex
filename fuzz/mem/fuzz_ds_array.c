#define CEX_LOG_LVL 0 /* 0 (mute all) - 5 (log$trace) */
#define CEX_TEST
#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


typedef struct fuzz_s
{
    const u8* cur;
    const u8* end;
} fuzz_s;

fuzz_s
fuzz_create(const u8* data, usize size)
{
    return (fuzz_s){ .cur = data, .end = data + size };
}

bool
fuzz_has(fuzz_s* f, isize size)
{
    uassert(size > 0);
    if (f->end - f->cur < size) {
        f->cur = f->end;
        return false;
    } else {
        return true;
    }
}

u16
fuzz_u16(fuzz_s* f, u16 def)
{
    if (!fuzz_has(f, sizeof(u16))) { return def; }
    u16 res = { 0 };
    memcpy(&res, f->cur, sizeof(u16));
    f->cur += sizeof(u16);
    return res;
}

int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    fuzz_s f = fuzz_create(data, size);


    mem$scope(tmem$, _)
    {
        u16 n = 0;
        arr$(u32) items = arr$new(items, _);

        while ((n = fuzz_u16(&f, 0))) {
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
