/*
*  Freestanding implementation for linux platform (no libc, memory allocators, file system, etc)
*/

#include "platform.c"

int main(int argc, char** argv){
    (void)argc;
    (void)argv;
    write_stdout("It's main() no cex included\n");
    return 0;
}
