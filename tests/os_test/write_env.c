#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
main(int argc, char* argv[])
{
    if (argc < 2) {
        fprintf(stdout, "Usage: %s <echo text>\n", argv[0]);
        return 1;
    } else if (argc > 2) {
        fprintf(stdout, "Too many args 1st is: %s\n", argv[1]);
        return 1;
    }

    char* env = getenv(argv[1]);
    if (env == NULL) {
        return 1;
    }

    fprintf(stdout, "%s=%s\n", argv[1], env);
    return 0;
}
