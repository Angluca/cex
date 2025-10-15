
// Ultra-broken memory allocator stub
// (!) Do not use in production

#include "cex.h"
#include <string.h>

size_t
strnlen(const char* s, size_t maxlen)
{
    (void)s;
    (void)maxlen;
    uassert(false && "Not implemented");
    return 0;
}

char*
strstr(const char* haystack, const char* needle)
{
    uassert(false && "Not implemented");
    (void)haystack;
    (void)needle;
    return NULL;
}

char*
strcpy(char* restrict dst, const char* restrict src)
{
    uassert(false && "Not implemented, don't use it!");
    (void)dst;
    (void)src;
    return NULL;
}

int strncmp(const char* s1, const char* s2, size_t n)
{
    uassert(false && "Not implemented, don't use it!");
    (void)s1;
    (void)s2;
    (void)n;
    return 0;
}
