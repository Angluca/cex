#include "all.h"

#if defined(CEX_BUILD)

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
#define cexy$needs_rebuild
#define cexy$initialize

#endif // #if defined(CEX_BUILD)
