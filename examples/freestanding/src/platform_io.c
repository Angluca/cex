
// NOTE: linux GlibC specific errno implementation, very platform specific
#include <stdio.h>
#include <string.h>

int __my_platform_errno = 0;
int*
__errno_location(void)
{
    return &__my_platform_errno;
}

char*
strerror(int errnum)
{
    // errnum - is errno type of value
    if (errnum == 0) {
        return "NoError";
    } else {
        return "Some error stub";
    }
}


FILE*
fopen(const char* restrict pathname, const char* restrict mode)
{
    (void)mode;
    if (strcmp(pathname, "stdout") == 0) {
        return stdout;
    } else {
        return NULL;
    }
}

int
fseek(FILE* stream, long offset, int whence)
{
    (void)stream;
    (void)offset;
    (void)whence;
    return 0;
}
long
ftell(FILE* stream)
{
    (void)stream;
    return 0;
}

void
rewind(FILE* stream)
{
    (void)stream;
}

int
feof(FILE* stream)
{
    (void)stream;
    return 1;
}

int
ferror(FILE* stream)
{
    (void)stream;
    return 0;
}


size_t
fread(void* ptr, size_t size, size_t nmemb, FILE* restrict stream)
{
    (void)ptr;
    (void)size;
    (void)nmemb;
    (void)stream;
    return 0;
}

int
fgetc(FILE* stream)
{
    (void)stream;
    return 0;
}

int fclose(FILE *stream){
    (void)stream;
    return 0;
}

