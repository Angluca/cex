#include <cex/all.h>
#include <cex/all.c>


test$teardown()
{
    return EOK;
}

test$setup()
{
    uassert_enable(); // re-enable if you disabled it in some test case
    return EOK;
}

test$case(my_run)
{
    const char* command_line[] = { "echo", "\"Hello, world!\"", NULL };
    (void)command_line;
    // char* command_line[] = { "sleep", "5", NULL };
    // char* command_line[] = { "dd", "if=/dev/zero", "bs=1000k", "count=3000", NULL };
    // char *command_line[] = {"dd", "if=/dev/random", "bs=100k", "count=65", NULL};
    tassert_eqe(Error.argument, os$cmd(NULL));
    tassert_eqe(Error.argument, os$cmd("ls", "soo", NULL, "foo"));
    tassert_eqe(Error.runtime, os$cmd("sleepasdlkajdlja", "2"));
    os_cmd_c c = {0};
    tassert_eqe(Error.empty, os.cmd.wait(&c));
    e$ret(os.cmd.run(command_line, arr$len(command_line), &c ));

    tassert_eqe(Error.runtime, os$cmd("cat", "/asdljqlw/asdlkjasdlji"));

    mem$scope(tmem$, _) {
        tassert_eqe(EOK, os$cmd("echo", "hi there", str.fmt(_, "foo: %d", 1)));

        arr$(const char*) args = arr$new(args, _);
        arr$pusha(args, command_line);
        e$ret(os.cmd.run(args, arr$len(args), &c ));
        e$ret(os.cmd.wait(&c));

        tassert_eqe(Error.empty, os.cmd.wait(&c));
    }
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(my_run);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
