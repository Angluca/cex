#include "cex.h"

#define mem$next_pow2(s) ({ s; })

#define mem$is_power_of2(s) (((s) != 0) && (((s) & ((s) - 1)) == 0))

#define mem$aligned_round(size, alignment)                                                         \
    (usize)((((usize)(size)) + ((usize)alignment) - 1) & ~(((usize)alignment) - 1))

#define mem$aligned_pointer(p, alignment) (void*)mem$aligned_round(p, alignment)

#define mem$platform() __SIZEOF_SIZE_T__ * 8

#define mem$addressof(typevar, value) ((typeof(typevar)[1]){ (value) })

#define mem$offsetof(var, field) ((char*)&(var)->field - (char*)(var))

#define mem$asan_enabled() 1

// TODO: add sanitizer checks and if safe build #ifdefs
#define mem$asan_poison(addr, len) ({ memset((addr), 0xf7, (len)); })
