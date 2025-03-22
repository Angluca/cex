#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#endif

int
main(int argc, char* argv[])
{
    if (argc != 3) {
        fprintf(stdout, "Usage: %s stdout|stderr <out_count>\n", argv[0]);
        return 1;
    }
    FILE* outstream;
    if (strcmp(argv[1], "stdout") == 0) {
        outstream = stdout;
    } else if (strcmp(argv[1], "stderr") == 0) {
        outstream = stderr;
    } else {
        fprintf(stdout, "1st argument must be stdout|stderr, got: %s\n", argv[1]);
        return 1;
    }

    int out_count = atoi(argv[2]);
    if (out_count <= 0) {
        fprintf(stdout, "out_count must be a positive integer.\n");
        return 1;
    }

    for (int i = 0; i < out_count; i++) {
        fprintf(outstream, "%09d\n", i);
#ifdef _WIN32
    Sleep(10);
#else
    usleep(10 * 1000); 
#endif
    }
    return 0;
}
