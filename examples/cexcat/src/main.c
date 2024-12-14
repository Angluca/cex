#include <cex/all.c>
#include <cex/argparse/argparse.c>
#include "App.c"
#include <stdlib.h>

int
main(int argc, char* argv[])
{
  i32 ret_code = EXIT_SUCCESS;
  App_c app = { 0 };
  var allocator = AllocatorGeneric.create();

  e$except_silent(err, App.create(&app, argc, argv, allocator))
  {
    if (err != Error.argsparse){
      log$error("App.create(&app, argc, argv, allocator)) %s", err);
    }
    ret_code = EXIT_FAILURE;
    goto shutdown;
  }

  e$except(err, App.main(&app, allocator))
  {
    ret_code = EXIT_FAILURE;
    goto shutdown;
  }

shutdown:
  App.destroy(&app, allocator);
  AllocatorGeneric.destroy();
  return ret_code;
}
