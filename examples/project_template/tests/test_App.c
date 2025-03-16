#include <cex/all.c>
#include <cex/test/fff.h>
#include <cex/test/test.h>
#include "src/App.c"
#include "lib/mylib.c"

IAllocator allocator;

test$teardown(){
    allocator = AllocatorGeneric.destroy(); // this also nullifies allocator
    return EOK;
}

test$setup(){
    uassert_enable(); // re-enable if you disabled it in some test case
    allocator = mem$;
    return EOK;
}

test$case(app_create_args_empty)
{
    App_c app;
    char* argv[] = {
        "app_name", // first argv is always program name
    };
    u32 argc = arr$len(argv);
    tassert_eqe(Error.argsparse, App.create(&app, argc, argv, allocator));
    tassert_eqi(app.is_flag, 0);

    return EOK;
}

test$case(app_create_args)
{
    App_c app;
    char* argv[] = {
        "app_name", // first argv is always program name
        "-f", 
        "--name=cex",
        "foo", 
        "bar",
    };
    u32 argc = arr$len(argv);
    tassert_eqe(Error.ok, App.create(&app, argc, argv, allocator));
    tassert_eqi(app.is_flag, 1);
    tassert_eqi(app.num_arg, 234);
    tassert_eqs(app.name_arg, "cex");

    return EOK;
}
test$case(app_main)
{
    App_c app;
    char* argv[] = {
        "app_name", // first argv is always program name
        "-f", 
        "--name=cex",
        "foo", 
        "bar",
    };
    u32 argc = arr$len(argv);
    tassert_eqe(Error.ok, App.create(&app, argc, argv, allocator));
    tassert_eqe(Error.ok, App.main(&app, allocator));

    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(app_create_args_empty);
    test$run(app_create_args);
    test$run(app_main);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
