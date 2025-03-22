#include <stdio.h>
#include <string.h>

#define MAX_INPUT_SIZE 100

int main() {
    char input[MAX_INPUT_SIZE];
    printf("welcome to echo server\n");
    fflush(stdout);

    while (1) {
        // Read a line from stdin
        if (fgets(input, MAX_INPUT_SIZE, stdin) == NULL) {
            return 1;
        }

        // Remove the newline character at the end of the input
        input[strcspn(input, "\n")] = '\0';

        // Check if the input is "end"
        if (strcmp(input, "end") == 0) {
            printf("exit: %s\n", input);
            break;
        }

        // Print the input with the prefix "out: "
        printf("out: %s\n", input);
        fflush(stdout);
    }
    return 0;
}
