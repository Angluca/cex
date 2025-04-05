#include "all.h"

#if defined(CEXBUILD)

#ifndef cexy$cc
#if defined(__clang__)
#define cexy$cc "clang"
#elif defined(__GNUC__)
#define cexy$cc "gcc"
#else
# #error "Compiler type is not supported"
#endif
#endif // #ifndef cexy$cc


#define cexy$run
#define cexy$initialize
#define cexy$needs_build(target, src_array)                                                      \
    ({ cexy_needs_build((target), (src_array), arr$len(src_array), false); })

#endif // #if defined(CEXBUILD)
