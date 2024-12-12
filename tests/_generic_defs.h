#include <cex/list.h>
#include <cex/deque/deque.h>

// NOTE: generic list_i32 definition (object must be declared in test)
list$typedef(list_i32, i32, false /* false - means separate implementation */);

// You should place the following line into .c file
// list$impl(list_i32);


deque$typedef(deque_int, int, false);
// You should place the following line into .c file
// deque$impl(deque_int);
