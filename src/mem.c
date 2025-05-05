#include "all.h"

void
_cex_allocator_memscope_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    (*allc)->scope_exit(*allc);
}

void
_cex_allocator_arena_cleanup(IAllocator* allc)
{
    uassert(allc != NULL);
    AllocatorArena.destroy(*allc);
}

// NOTE: destructor(101) - 101 lowest priority for destructors
__attribute__((destructor(101))) 
void _cex_global_allocators_destructor() {
    AllocatorArena_c* allc = (AllocatorArena_c*)tmem$;
    allocator_arena_page_s* page = allc->last_page;
    while (page) {
        var tpage = page->prev_page;
        mem$free(mem$, page);
        page = tpage;
    }

}
