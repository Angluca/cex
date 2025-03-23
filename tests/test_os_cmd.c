// clang-format off
#include "cex/test.h"
#include <cex/all.h>
#include <cex/all.c>
// clang-format on


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
    // FIX:  Add WIN32 versions of this test

    const char* command_line[] = { "echo", "\"Hello, world!\"", NULL };
    (void)command_line;
    // char* command_line[] = { "sleep", "5", NULL };
    // char* command_line[] = { "dd", "if=/dev/zero", "bs=1000k", "count=3000", NULL };
    // char *command_line[] = {"dd", "if=/dev/random", "bs=100k", "count=65", NULL};
    tassert_eqe(Error.argument, os$cmd(NULL));
    tassert_eqe(Error.argument, os$cmd("ls", "soo", NULL, "foo"));
    tassert_eqe(Error.runtime, os$cmd("sleepasdlkajdlja", "2"));
    os_cmd_c c = { 0 };
    e$ret(os.cmd.run(command_line, arr$len(command_line), &c));

    tassert_eqe(Error.runtime, os$cmd("tests/build/os_test/cat", "/asdljqlw/asdlkjasdlji"));

    mem$scope(tmem$, _)
    {
        tassert_eqe(EOK, os$cmd("echo", "hi there", str.fmt(_, "foo: %d", 1)));

        arr$(const char*) args = arr$new(args, _);
        arr$pusha(args, command_line);
        e$ret(os.cmd.run(args, arr$len(args), &c));
    }

    // Manual testing stdin works!
    // os$cmd("tests/build/os_test/echo_server");
    return EOK;
}

test$case(os_cmd_create)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert(str.starts_with(output, "Usage: "));
        int err_code = 0;
        tassert_eqe(Error.runtime, os.cmd.join(&c, 0, &err_code));
        tassert_eqi(err_code, 1);
    }
    return EOK;
}

test$case(os_cmd_file_handles)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        tassert(os.cmd.stderr(&c) == c._subpr.stderr_file);
        tassert(os.cmd.stdout(&c) == c._subpr.stdout_file);
        tassert(os.cmd.stdin(&c) == c._subpr.stdin_file);

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert(str.starts_with(output, "Usage: "));
        int err_code = 0;
        tassert_eqe(Error.runtime, os.cmd.join(&c, 0, &err_code));
        tassert_eqi(err_code, 1);
    }
    return EOK;
}

test$case(os_cmd_read_all_small)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", "stdout", "10", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        int err_code = 1;
        tassert_eqe(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eqi(err_code, 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eqi(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) {
            tassert_eqs(str.fmt(_, "%09d", i), lines[i]);
        }

        tassert_eqi(strlen(output) / 10, arr$len(lines));
    }
    return EOK;
}

test$case(os_cmd_read_all_huge)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", "stdout", "100000", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        int err_code = 1;
        tassert_eqe(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eqi(err_code, 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eqi(arr$len(lines), 100000);
        for (u32 i = 0; i < arr$len(lines); i++) {
            tassert_eqs(str.fmt(_, "%09d", i), lines[i]);
        }
    }
    return EOK;
}

test$case(os_cmd_read_line_huge)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", "stdout", "100000", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* line;
        u32 lcnt = 0;
        while ((line = os.cmd.read_line(&c, _))) {
            tassert_eqs(str.fmt(_, "%09d", lcnt), line);
            lcnt++;
        }
        int err_code = 1;
        tassert_eqe(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eqi(err_code, 0);

        tassert_eqi(lcnt, 100000);
    }
    return EOK;
}

test$case(os_cmd_read_all_only_stdout)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", "stderr", "10", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        tassert_eqs(output, "");

        int err_code = 1;
        tassert_eqe(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eqi(err_code, 0);
    }
    return EOK;
}


test$case(os_cmd_read_all_combined_stderr)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", "stderr", "10", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, &(os_cmd_flags_s){ .combine_stdouterr = 1 }));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        int err_code = 1;
        tassert_eqe(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eqi(err_code, 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eqi(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) {
            tassert_eqs(str.fmt(_, "%09d", i), lines[i]);
        }

        tassert_eqi(strlen(output) / 10, arr$len(lines));
    }
    return EOK;
}

test$case(os_cmd_join_timeout)
{
    os_cmd_c c = { 0 };
    // WTF: if we call subprocess_terminate() on empty struct it will kill self!
    tassert_eqi(0, os.cmd.is_alive(&c));
    e$ret(os.cmd.kill(&c));

    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/sleep", "2", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));
        tassert_eqi(1, os.cmd.is_alive(&c));

        int err_code = 0;
        tassert_eqe(Error.timeout, os.cmd.join(&c, 1, &err_code));
        tassert_eqi(err_code, -1);

        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        err_code = 777;
        tassert_eqe(Error.ok, os.cmd.join(&c, 3, &err_code));
        tassert_eqi(err_code, 0);
    }
    return EOK;
}

test$case(os_cmd_huge_join)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", "stdout", "100000", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        int err_code = 1;
        tassert_eqe(Error.timeout, os.cmd.join(&c, 1, &err_code));
        tassert_eqi(err_code, -1);
    }
    return EOK;
}

test$case(os_cmd_huge_join_stderr)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", "stderr", "100000", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        int err_code = 1;
        tassert_eqe(Error.timeout, os.cmd.join(&c, 1, &err_code));
        tassert_eqi(err_code, -1);
    }
    return EOK;
}

test$case(os_cmd_stdin_communucation)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/echo_server", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));
        tassert(os.cmd.is_alive(&c));

        tassert_eqs("welcome to echo server", os.cmd.read_line(&c, _));
        tassert_eqe(EOK, os.cmd.write_line(&c, "hello"));
        tassert_eqs("out: hello", os.cmd.read_line(&c, _));
        tassert_eqe(EOK, os.cmd.write_line(&c, "world"));
        tassert_eqs("out: world", os.cmd.read_line(&c, _));
        tassert_eqe(EOK, os.cmd.write_line(&c, "end"));

        tassert_eqe(Error.ok, os.cmd.join(&c, 1, NULL));
    }
    return EOK;
}

test$case(os_cmd_read_all_small_wdelay)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines_delay", "stdout", "10", NULL);
        tassert_eqe(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        int err_code = 1;
        tassert_eqe(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eqi(err_code, 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eqi(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) {
            tassert_eqs(str.fmt(_, "%09d", i), lines[i]);
        }

        tassert_eqi(strlen(output) / 10, arr$len(lines));
    }
    return EOK;
}
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(my_run);
    test$run(os_cmd_create);
    test$run(os_cmd_file_handles);
    test$run(os_cmd_read_all_small);
    test$run(os_cmd_read_all_huge);
    test$run(os_cmd_read_line_huge);
    test$run(os_cmd_read_all_only_stdout);
    test$run(os_cmd_read_all_combined_stderr);
    test$run(os_cmd_join_timeout);
    test$run(os_cmd_huge_join);
    test$run(os_cmd_huge_join_stderr);
    test$run(os_cmd_stdin_communucation);
    test$run(os_cmd_read_all_small_wdelay);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
