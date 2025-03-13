#include "cex.h"

#define mem$next_pow2(s) ({ \
s;\
})

#define mem$is_power_of2(s) (((s) != 0) && (((s) & ((s) - 1)) == 0))

#define mem$aligned_round(size, alignment) ({ \
    _Static_assert(alignment > 0, "alignment must be positive");\
    _Static_assert(alignment <= 1024, "alignment is too high");\
    _Static_assert(mem$is_power_of2(alignment), "must be pow2");\
    usize s = (usize)(size); \
    alignment * ((s + alignment - 1) / alignment);\
})

#define mem$aligned_pointer(p, alignment) (void*)mem$aligned_round(p, alignment)

#define mem$platform() __SIZEOF_SIZE_T__*8

#define mem$addressof(typevar, value)  ((typeof(typevar)[1]){ (value) })

#define mem$offsetof(var, field) ((char*)&(var)->field - (char*)(var))
