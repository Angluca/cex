#include <stdio.h>
#include <string.h>
#include <sys/syscall.h>

//
// Minimal base constants
//
static FILE _stdin = { 0 };
static FILE _stdout = { 0 };
static FILE _stderr = { 0 };

FILE* stdin = &_stdin;
FILE* stdout = &_stdout;
FILE* stderr = &_stderr;

void sys_write(int fd, const char* buf, size_t count);
void sys_exit(int status);
void write_stdout(const char* s);
int main(int argc, char** argv);

void
_start()
{
    char msg[] = "Hello World, from Freestanding linux!\n";

    // Write to stdout
    sys_write(1, msg, sizeof(msg) - 1); // -1 to exclude null terminator

    write_stdout("This is _start()\n");

    long* stack = (long*)__builtin_frame_address(0);
    long argc = stack[1];
    char** argv = (char**)(stack + 2);

    int rc = main(argc, argv);

    sys_exit(rc);
}


//
// Platform specific functions
//
static inline long
syscall3(long n, long a1, long a2, long a3)
{
    long ret;
    asm volatile("syscall"
                 : "=a"(ret)
                 : "a"(n), "D"(a1), "S"(a2), "d"(a3)
                 : "rcx", "r11", "memory");
    return ret;
}

void
write_stdout(const char* s)
{
    syscall3(SYS_write, 1, (long)s, strlen(s));
}

// System call wrapper for write
void
sys_write(int fd, const char* buf, size_t count)
{
    asm volatile("mov $1, %%rax\n"
                 "syscall\n"
                 :
                 : "D"(fd), "S"(buf), "d"(count)
                 : "rax", "rcx", "r11", "memory");
}

// System call wrapper for exit
void
sys_exit(int status)
{
    asm volatile("mov $60, %%rax\n"
                 "syscall\n"
                 :
                 : "D"(status)
                 : "rax", "rcx", "r11");
}


//
// USED BY CEX
//    function below are just for illustration purposes, simply vibe coded :) 
// Don't use in production
//

void
abort(void)
{
    __builtin_trap();
}

int
strcmp(const char* s1, const char* s2)
{
    while (*s1 && *s1 == *s2) {
        s1++;
        s2++;
    }

    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

size_t
strlen(const char* s)
{
    size_t len = 0;
    while (s[len]) { len++; }
    return len;
}

void*
memcpy(void* dest, const void* src, size_t n)
{
    char* d = dest;
    const char* s = src;
    while (n--) { *d++ = *s++; }
    return dest;
}

void*
memset(void* s, int c, size_t n)
{
    unsigned char* p = s;
    unsigned char value = (unsigned char)c;

    for (size_t i = 0; i < n; i++) { p[i] = value; }

    return s;
}

int
memcmp(const void* s1, const void* s2, size_t n)
{
    const unsigned char* p1 = s1;
    const unsigned char* p2 = s2;

    for (size_t i = 0; i < n; i++) {
        if (p1[i] != p2[i]) { return (int)p1[i] - (int)p2[i]; }
    }

    return 0;
}


void*
memmove(void* dest, const void* src, size_t n)
{
    if (n == 0) { return dest; }

    unsigned char* d = dest;
    const unsigned char* s = src;

    // Check for overlap
    if (d > s && d < s + n) {
        // Overlap detected, copy backwards
        d += n;
        s += n;
        while (n--) { *--d = *--s; }
    } else {
        // No overlap or dest before src, copy forwards
        while (n--) { *d++ = *s++; }
    }

    return dest;
}

size_t
fwrite(const void* ptr, size_t size, size_t nmemb, FILE* stream)
{
    if (stream == stdout) {
        syscall3(SYS_write, 1, (long)ptr, size * nmemb);
    } else if (stream == stderr) {
        syscall3(SYS_write, 2, (long)ptr, size * nmemb);
    } else if (stream == stdin) {
        syscall3(SYS_write, 0, (long)ptr, size * nmemb);
    } else {
        return 0; // Oops, maybe assert here?
    }

    return size * nmemb;
}

int
fflush(FILE* stream)
{
    (void)stream;
    return 0;
}
