#include <stdio.h>
#define CEX_IMPLEMENTATION
#include "cex.h"

int 
main(int argc, char** argv) {
    io.printf("hello\n"); 
    os$cmd("gcc", "-o", "cex2", "cex.c2");
    return 0;
}
