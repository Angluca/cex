#include <stdio.h>

int main(int argc, char *argv[]) {
    // Check if a filename is provided
    if (argc < 2) {
        fprintf(stdout, "Usage: %s <filename>\n", argv[0]);
        return 1;
    }

    // Open the file for reading
    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        fprintf(stdout, "Error opening file: %s\n", argv[1]);
        return 1;
    }

    // Read and print the file contents character by character
    int ch;
    while ((ch = fgetc(file)) != EOF) {
        putchar(ch);
    }

    // Close the file
    fclose(file);

    return 0;
}

