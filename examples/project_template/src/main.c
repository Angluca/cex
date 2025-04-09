// #define CEX_LOG_LVL 1
#include "include/cex/all.c"
#include <cex/argparse/argparse.c>
#include <stdlib.h>
#include "lib/mylib.c"
#include "App.c"

int
main(int argc, char* argv[])
{
    i32 ret_code = EXIT_FAILURE;
    App_c app = { 0 };
    var allocator = mem$;

    e$except_silent(err, App.create(&app, argc, argv, allocator))
    {
        //App.create() may fail with argsparse error, don't print anything
        // let it show args help or it's internal error
        if (err != Error.argsparse) {
            // print only unhandled 
            log$error("App.create(&app, argc, argv, allocator)) %s", err);
        }
        goto shutdown;
    }

    e$goto(App.main(&app, allocator), shutdown);

    ret_code = EXIT_SUCCESS;

shutdown:
    App.destroy(&app, allocator);
    AllocatorGeneric.destroy();
    return ret_code;
}
