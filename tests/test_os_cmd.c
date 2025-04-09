#include "src/all.h"
#include "src/all.c"


test$case(os_cmd_create)
{
    os_cmd_c c = { 0 };
    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/write_lines", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert(str.starts_with(output, "Usage: "));
        int err_code = 0;
        tassert_er(Error.runtime, os.cmd.join(&c, 0, &err_code));
        tassert_eq(err_code, 1);
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        tassert(os.cmd.stderr(&c) == c._subpr.stderr_file);
        tassert(os.cmd.stdout(&c) == c._subpr.stdout_file);
        tassert(os.cmd.stdin(&c) == c._subpr.stdin_file);

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        tassert(str.starts_with(output, "Usage: "));
        int err_code = 0;
        tassert_er(Error.runtime, os.cmd.join(&c, 0, &err_code));
        tassert_eq(err_code, 1);
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        int err_code = 1;
        tassert_er(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eq(err_code, 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) {
            tassert_eq(str.fmt(_, "%09d", i), lines[i]);
        }

        tassert_eq(strlen(output) / 10, arr$len(lines));
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        int err_code = 1;
        tassert_er(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eq(err_code, 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 100000);
        for (u32 i = 0; i < arr$len(lines); i++) {
            tassert_eq(str.fmt(_, "%09d", i), lines[i]);
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* line;
        u32 lcnt = 0;
        while ((line = os.cmd.read_line(&c, _))) {
            tassert_eq(str.fmt(_, "%09d", lcnt), line);
            lcnt++;
        }
        int err_code = 1;
        tassert_er(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eq(err_code, 0);

        tassert_eq(lcnt, 100000);
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        tassert_eq(output, "");

        int err_code = 1;
        tassert_er(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eq(err_code, 0);
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, &(os_cmd_flags_s){ .combine_stdouterr = 1 }));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        int err_code = 1;
        tassert_er(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eq(err_code, 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) {
            tassert_eq(str.fmt(_, "%09d", i), lines[i]);
        }

        tassert_eq(strlen(output) / 10, arr$len(lines));
    }
    return EOK;
}

test$case(os_cmd_join_timeout)
{
    os_cmd_c c = { 0 };
    // WTF: if we call subprocess_terminate() on empty struct it will kill self!
    tassert_eq(0, os.cmd.is_alive(&c));
    e$ret(os.cmd.kill(&c));

    mem$scope(tmem$, _)
    {
        arr$(char*) args = arr$new(args, _);
        arr$pushm(args, "tests/build/os_test/sleep", "2", NULL);
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));
        tassert_eq(1, os.cmd.is_alive(&c));

        int err_code = 0;
        tassert_er(Error.timeout, os.cmd.join(&c, 1, &err_code));
        tassert_eq(err_code, -1);

        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        err_code = 777;
        tassert_er(Error.ok, os.cmd.join(&c, 3, &err_code));
        tassert_eq(err_code, 0);
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        int err_code = 1;
        tassert_er(Error.timeout, os.cmd.join(&c, 1, &err_code));
        tassert_eq(err_code, -1);
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        int err_code = 1;
        tassert_er(Error.timeout, os.cmd.join(&c, 1, &err_code));
        tassert_eq(err_code, -1);
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));
        tassert(os.cmd.is_alive(&c));

        tassert_eq("welcome to echo server", os.cmd.read_line(&c, _));
        tassert_er(EOK, os.cmd.write_line(&c, "hello"));
        tassert_eq("out: hello", os.cmd.read_line(&c, _));
        tassert_er(EOK, os.cmd.write_line(&c, "world"));
        tassert_eq("out: world", os.cmd.read_line(&c, _));
        tassert_er(EOK, os.cmd.write_line(&c, "end"));

        tassert_er(Error.ok, os.cmd.join(&c, 1, NULL));
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
        tassert_er(EOK, os.cmd.create(&c, args, NULL, NULL));

        char* output = os.cmd.read_all(&c, _);
        tassert(output != NULL);
        // printf("%s\n", output);
        int err_code = 1;
        tassert_er(Error.ok, os.cmd.join(&c, 0, &err_code));
        tassert_eq(err_code, 0);

        arr$(char*) lines = str.split_lines(output, _);
        tassert_eq(arr$len(lines), 10);
        for (u32 i = 0; i < arr$len(lines); i++) {
            tassert_eq(str.fmt(_, "%09d", i), lines[i]);
        }

        tassert_eq(strlen(output) / 10, arr$len(lines));
    }
    return EOK;
}

test$main();
