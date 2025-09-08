#include <ctype.h>
#include <math.h>

#ifdef _WIN32
#    define WIN32_LEAN_AND_MEAN
#    include "windows.h"
#    include <direct.h>
#else
#    include <dirent.h>
#    include <fcntl.h>
#    include <limits.h>
#    include <sys/stat.h>
#    include <sys/types.h>
#    include <unistd.h>
#endif
