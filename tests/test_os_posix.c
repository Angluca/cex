#include <cex/all.c>
#include <cex/io.c>
#include <cex/os/os.c>
#include <cex/test.h>
#include <limits.h>


test$teardown()
{
    return EOK;
}

test$setup()
{
    uassert_enable(); // re-enable if you disabled it in some test case
    return EOK;
}

test$case(test_os_listdir)
{
    sbuf_c listdir = sbuf.create(1024, mem$);

    tassert_eqe(EOK, os.listdir("tests/data/", &listdir));
    tassert(sbuf.len(&listdir) > 0);

    u32 nit = 0;
    const char* expected[] = {
        "allocator_fopen.txt",       "text_file_50b.txt",
        "text_file_empty.txt",       "text_file_fprintf.txt",
        "text_file_line_4095.txt",   "text_file_only_newline.txt",
        "text_file_win_newline.txt", "text_file_write.txt",
        "text_file_zero_byte.txt",   "",
    };
    (void)expected;
    for$iter(str_s, it, str.slice.iter_split(str.sstr(listdir), "\n", &it.iterator))
    {
        // io.printf("%d: %S\n", it.idx.i, *it.val);
        tassert_eqi(it.idx.i, nit);

        bool is_found = false;
        for$array(itf, expected, arr$len(expected))
        {
            if (str.slice.cmp(*it.val, str.sstr(*itf.val)) == 0) {
                is_found = true;
                break;
            }
        }
        if (!is_found) {
            io.printf("Not found in expected: %S\n", *it.val);
            tassert(is_found && "file not found in expected");
        }
        nit++;
    }
    tassert_eqi(nit, arr$len(expected));

    tassert(sbuf.len(&listdir) > 0);
    tassert_eqe(Error.not_found, os.listdir("tests/data/unknownfolder", &listdir));
    tassert_eqi(sbuf.len(&listdir), 0);

    sbuf.destroy(&listdir);
    return EOK;
}

test$case(test_os_getcwd)
{
    sbuf_c s = sbuf.create(10, mem$);
    tassert_eqe(EOK, os.getcwd(&s));
    tassert_eqi(true, str.slice.ends_with(str.sstr(s), str$s("cex")));

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_os_path_exists)
{
    tassert_eqe(Error.argument, os.path.exists(NULL));
    tassert_eqe(Error.argument, os.path.exists(""));
    tassert_eqe(EOK, os.path.exists("."));
    tassert_eqe(EOK, os.path.exists(".."));
    tassert_eqe(EOK, os.path.exists("./tests"));
    tassert_eqe(EOK, os.path.exists("./tests/test_os_posix.c"));
    tassert_eqe(Error.not_found, os.path.exists("./tests/test_os_posix.cpp"));

    tassert_eqe(EOK, os.path.exists("tests/"));

    char buf[PATH_MAX + 10];
    memset(buf, 'a', arr$len(buf));
    buf[PATH_MAX + 8] = '\0';
    uassert_disable();

    // Path is too long, and exceeds PATH_MAX buffer size
    tassert_eqe(Error.argument, os.path.exists(buf));


    return EOK;
}

test$case(test_os_path_join)
{
    mem$scope(tmem$, ta)
    {
        arr$(char*) parts = arr$new(parts, ta);
        arr$pushm(parts, "cexstr", str.fmt(ta, "%s_%d.txt", "foo", 10));
        char* p = os.path.join(parts, ta);
        tassert_eqs("cexstr/foo_10.txt", p);
    }
    return EOK;
}

test$case(test_os_setenv)
{
    // get non existing
    tassert_eqs(os.getenv("test_os_posix", NULL), NULL);
    // get non existing, with default
    tassert_eqs(os.getenv("test_os_posix", "envdef"), "envdef");

    // set env
    os.setenv("test_os_posix", "foo", true);
    tassert_eqs(os.getenv("test_os_posix", NULL), "foo");

    // set without replacing
    os.setenv("test_os_posix", "bar", false);
    tassert_eqs(os.getenv("test_os_posix", NULL), "foo");

    // set with replacing
    os.setenv("test_os_posix", "bar", true);
    tassert_eqs(os.getenv("test_os_posix", NULL), "bar");

    // unset env
    os.unsetenv("test_os_posix");
    tassert_eqs(os.getenv("test_os_posix", NULL), NULL);

    return EOK;
}
test$case(test_os_path_splitext)
{
    tassert_eqi(str.slice.cmp(os.path.splitext("foo.bar", true), str$s(".bar")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("foo.bar", false), str$s("foo")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("foo", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("foo", false), str$s("foo")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("foo.bar.exe", true), str$s(".exe")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("foo.bar.exe", false), str$s("foo.bar")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("foo/bar.exe", true), str$s(".exe")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("foo/bar.exe", false), str$s("foo/bar")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("bar.foo/bar.exe", true), str$s(".exe")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("bar.foo/bar.exe", false), str$s("bar.foo/bar")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("bar.foo/bar", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("bar.foo/bar", false), str$s("bar.foo/bar")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext(".gitingnore", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext(".gitingnore", false), str$s(".gitingnore")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("...gitingnore", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("...gitingnore", false), str$s("...gitingnore")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("bar.foo/bar...", true), str$s(".")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("bar.foo/bar...", false), str$s("bar.foo/bar..")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("bar.foo/bar.", true), str$s(".")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("bar.foo/bar.", false), str$s("bar.foo/bar")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext(".", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext(".", false), str$s(".")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("..", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("..", false), str$s("..")), 0);

    tassert_eqi(str.slice.cmp(os.path.splitext("bar.foo/bar..exe", true), str$s(".exe")), 0);
    tassert_eqi(
        str.slice.cmp(os.path.splitext("bar.foo/bar..exe", false), str$s("bar.foo/bar.")),
        0
    );

    tassert_eqi(str.slice.cmp(os.path.splitext("", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.splitext("", false), str$s("")), 0);

    tassert(os.path.splitext(NULL, false).buf == NULL);
    tassert(os.path.splitext(NULL, true).buf == NULL);
    return EOK;
}

int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_os_listdir);
    test$run(test_os_getcwd);
    test$run(test_os_path_exists);
    test$run(test_os_path_join);
    test$run(test_os_setenv);
    test$run(test_os_path_splitext);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
