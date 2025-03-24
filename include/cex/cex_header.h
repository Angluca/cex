#include <ctype.h>
#include <errno.h>
#include <float.h>
#include <math.h>
#include <signal.h>
#include <stdalign.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <threads.h>
#include <unistd.h>

#ifdef _WIN32
#include <Windows.h>
#else
#include <dirent.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#endif

/*
 *                  GLOBAL CEX VARS / DEFINES
 */

/// disables all asserts and safety checks (fast release mode)
// #define NDEBUG

/// custom fprintf() function for asserts/logs/etc
// #define __cex__fprintf(stream, prefix, filename, line, func, format, ...)

/// customize abort() behavior
// #define __cex__abort()

// customize uassert() behavior
// #define __cex__assert()

// you may override this level to manage log$* verbosity
// #define CEX_LOG_LVL 5

/// disable ASAN memory poisoning and mem$asan_poison*
// #define CEX_DISABLE_POISON 1

/// size of stack based buffer for small strings
// #define CEX_SPRINTF_MIN 512

/// disables float printing for io.printf/et al functions (code size reduction)
// #define CEX_SPRINTF_NOFLOAT

