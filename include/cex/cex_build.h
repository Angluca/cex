#include "all.h"

#if defined(CEX_BUILD)

#ifndef cex$cc
#if defined(__clang__)
#define cex$cc "clang"
#elif defined(__GNUC__)
#define cex$cc "gcc"
#else
# #error "Compiler type is not supported"
#endif
#endif // #ifndef cex$cc


#define cex$run
#define cex$needs_rebuild
#define cex$initialize

#endif // #if defined(CEX_BUILD)
