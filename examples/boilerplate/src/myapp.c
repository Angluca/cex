#define CEX_IMPLEMENTATION
#include "cex.h"
#include "src/mylib.c"

int main(int argc, char** argv) {
    io.printf("MOCCA - Make Old C Cexy Again!\n");
    io.printf("1 + 2 = %d\n", mylib_add(1, 2));
    return 0;
}
