#pragma once
#include <cex/cex.h>

typedef struct App_c {
    bool is_flag;
    i64 num_arg;
    char* name_arg;
    char** args;
    u32 args_count;
    const Allocator_i* allocator;
} App_c;

