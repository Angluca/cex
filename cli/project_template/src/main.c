// WARNING: CEX advocates #include.c principle, for small projects
#include "../include/mylib.c"
#include "cex.h"
#include <cex.c>

Exception
read_number(i32* num){

    char num1[5];
    fgets(num1, sizeof(num1), stdin);

    // Making a read-only CEX string view from num1 buffer
    str_c s1 = str.cbuf(num1, sizeof(num1));

    // fgets appends new line, strip it before parsing an int
    s1 = str.strip(s1);
    
    if (EOK != str.to_i32(s1, num)) {
        io.printf("You entered '%S', bad number try again\n", s1);
        return Error.io;
    }

    return EOK;
}

int
main(int argc, char** argv)
{
    (void)argc;
    (void)argv;


    while (true) {

        printf("Simple multiplication calculator: num1 * num2 = ?\n");

        printf("Enter num1: ");
        i32 n1 = 0;
        if(read_number(&n1) != EOK){
            continue;
        }

        printf("Enter num2: ");
        i32 n2 = 0;
        if(read_number(&n2) != EOK){
            continue;
        }

        i32 result = mylib.mul(n1, n2);
        printf("%d * %d = %d\n", n1, n2, result);
    }

    return 0;
}
