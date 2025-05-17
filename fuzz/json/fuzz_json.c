#include "src/all.c"
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>


int
LLVMFuzzerTestOneInput(const uint8_t* data, size_t size)
{
    if (size == 0) { return -1; }
    json_iter_c js;

    // permissive mode
    if (json.iter.create(&js, (char*)data, size, false)) { return 0; }
    while (json.iter.next(&js)) {}

    // strict mode
    if (json.iter.create(&js, (char*)data, size, true)) { return 0; }
    while (json.iter.next(&js)) {}

    return 0;
}
