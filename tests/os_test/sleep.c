#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

int
main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stdout, "Usage: %s <sleep_time_sec>\n", argv[0]);
        return 1;
    }

    int sleep_time = atoi(argv[1]);
    if (sleep_time <= 0) {
        fprintf(stdout, "sleep_time must be a positive integer.\n");
        return 1;
    }

#ifdef _WIN32
    Sleep(sleep_time * 1000);
#else
    usleep(sleep_time * 1000 * 1000); 
#endif

    return 0;
}
