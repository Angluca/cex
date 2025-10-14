#include "cex.h"

Exception
wasm_emscripten(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    io.printf("MOCCA - Make Old C Cexy Again!\n");
    log$info("Hi there\n");

    io.printf("test_str_sprintf\n");
    char buffer[10] = { 0 };

    memset(buffer, 'z', sizeof(buffer));

    e$ret(str.sprintf(buffer, sizeof(buffer), "%s", "1234"));

    printf("new buf: %s\n", buffer);

    return EOK;
}
