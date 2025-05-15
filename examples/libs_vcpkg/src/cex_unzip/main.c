// NOTE: main.c serves as unity build container + detaching allows unit testing of app's code
#define CEX_IMPLEMENTATION
#include "cex.h"
#include "cex_unzip.c"
// TODO: add your app sources here (include .c)

int
main(int argc, char** argv)
{
    if (cex_unzip(argc, argv) != EOK) { return 1; }
    return 0;
}
