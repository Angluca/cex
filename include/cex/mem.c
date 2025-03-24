#include "all.h"

void
_cex_allocator_memscope_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    (*allc)->scope_exit(*allc);
}
