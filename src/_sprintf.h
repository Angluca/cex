// stb_sprintf - v1.10 - public domain snprintf() implementation
// originally by Jeff Roberts / RAD Game Tools, 2015/10/20
// http://github.com/nothings/stb
//
// allowed types:  sc uidBboXx p AaGgEef n
// lengths      :  hh h ll j z t I64 I32 I
//
// Contributors:
//    Fabian "ryg" Giesen (reformatting)
//    github:aganm (attribute format)
//
// Contributors (bugfixes):
//    github:d26435
//    github:trex78
//    github:account-login
//    Jari Komppa (SI suffixes)
//    Rohit Nirmal
//    Marcin Wojdyr
//    Leonard Ritter
//    Stefano Zanotti
//    Adam Allison
//    Arvid Gerstmann
//    Markus Kolb
//
// LICENSE:
//
//   See end of file for license information.


/*
Single file sprintf replacement.

Originally written by Jeff Roberts at RAD Game Tools - 2015/10/20.
Hereby placed in public domain.

This is a full sprintf replacement that supports everything that
the C runtime sprintfs support, including float/double, 64-bit integers,
hex floats, field parameters (%*.*d stuff), length reads backs, etc.

It compiles to roughly 8K with float support, and 4K without.
As a comparison, when using MSVC static libs, calling sprintf drags
in 16K.


FLOATS/DOUBLES:
===============
This code uses a internal float->ascii conversion method that uses
doubles with error correction (double-doubles, for ~105 bits of
precision).  This conversion is round-trip perfect - that is, an atof
of the values output here will give you the bit-exact double back.

If you don't need float or doubles at all, define STB_SPRINTF_NOFLOAT
and you'll save 4K of code space.

64-BIT INTS:
============
This library also supports 64-bit integers and you can use MSVC style or
GCC style indicators (%I64d or %lld).  It supports the C99 specifiers
for size_t and ptr_diff_t (%jd %zd) as well.

EXTRAS:
=======
Like some GCCs, for integers and floats, you can use a ' (single quote)
specifier and commas will be inserted on the thousands: "%'d" on 12345
would print 12,345.

For integers and floats, you can use a "$" specifier and the number
will be converted to float and then divided to get kilo, mega, giga or
tera and then printed, so "%$d" 1000 is "1.0 k", "%$.2d" 2536000 is
"2.53 M", etc. For byte values, use two $:s, like "%$$d" to turn
2536000 to "2.42 Mi". If you prefer JEDEC suffixes to SI ones, use three
$:s: "%$$$d" -> "2.42 M". To remove the space between the number and the
suffix, add "_" specifier: "%_$d" -> "2.53M".

In addition to octal and hexadecimal conversions, you can print
integers in binary: "%b" for 256 would print 100.
*/
#pragma once
#include "all.h"

// SETTINGS

// #define CEX_SPRINTF_NOFLOAT // disables floating point code (2x less in size)
#ifndef CEX_SPRINTF_MIN
#define CEX_SPRINTF_MIN 512 // size of stack based buffer for small strings
#endif

// #define CEXSP_STATIC   // makes all functions static

#ifdef CEXSP_STATIC
#define CEXSP__PUBLICDEC static
#define CEXSP__PUBLICDEF static
#else
#define CEXSP__PUBLICDEC extern
#define CEXSP__PUBLICDEF
#endif

#define CEXSP__ATTRIBUTE_FORMAT(fmt, va) __attribute__((format(printf, fmt, va)))

typedef char* cexsp_callback_f(char* buf, void* user, u32 len);

typedef struct cexsp__context
{
    char* buf;
    FILE* file;
    IAllocator allc;
    u32 capacity;
    u32 length;
    u32 has_error;
    char tmp[CEX_SPRINTF_MIN];
} cexsp__context;

// clang-format off
CEXSP__PUBLICDEF int cexsp__vfprintf(FILE* stream, const char* format, va_list va);
CEXSP__PUBLICDEF int cexsp__fprintf(FILE* stream, const char* format, ...);
CEXSP__PUBLICDEC int cexsp__vsnprintf(char* buf, int count, char const* fmt, va_list va);
CEXSP__PUBLICDEC int cexsp__snprintf(char* buf, int count, char const* fmt, ...) CEXSP__ATTRIBUTE_FORMAT(3, 4);
CEXSP__PUBLICDEC int cexsp__vsprintfcb(cexsp_callback_f* callback, void* user, char* buf, char const* fmt, va_list va);
CEXSP__PUBLICDEC void cexsp__set_separators(char comma, char period);
// clang-format on
