#include <cex/os/os.c>
#include <cex/io.c>
#include <limits.h>
#include <cex/all.c>
#include <cex/test.h>


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
    sbuf_c listdir = { 0 };
    tassert_eqe(EOK, sbuf.create(&listdir, 1024, mem$));

    tassert_eqe(EOK, os.listdir(str$("tests/data/"), &listdir));
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
    for$iter(str_c, it, sbuf.iter_split(&listdir, "\n", &it.iterator))
    {
        // io.printf("%d: %S\n", it.idx.i, *it.val);
        tassert_eqi(it.idx.i, nit);

        bool is_found = false;
        for$array(itf, expected, arr$len(expected))
        {
            if (str.cmp(*it.val, str.cstr(*itf.val)) == 0) {
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
    tassert_eqe(Error.not_found, os.listdir(str$("tests/data/unknownfolder"), &listdir));
    tassert_eqi(sbuf.len(&listdir), 0);

    sbuf.destroy(&listdir);
    return EOK;
}

test$case(test_os_getcwd)
{
    sbuf_c s = { 0 };
    tassert_eqe(EOK, sbuf.create(&s, 10, mem$));
    tassert_eqe(EOK, os.getcwd(&s));
    io.printf("'%S'\n", sbuf.to_str(&s));
    tassert_eqi(true, str.ends_with(sbuf.to_str(&s), str$("cex")));

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_os_path_exists)
{
    tassert_eqe(Error.argument, os.path.exists(str.cstr(NULL)));
    tassert_eqe(Error.argument, os.path.exists(str.cstr("")));
    tassert_eqe(EOK, os.path.exists(str$(".")));
    tassert_eqe(EOK, os.path.exists(str$("..")));
    tassert_eqe(EOK, os.path.exists(str$("./tests")));
    tassert_eqe(EOK, os.path.exists(str$("./tests/test_os_posix.c")));
    tassert_eqe(Error.not_found, os.path.exists(str$("./tests/test_os_posix.cpp")));

    tassert_eqe(EOK, os.path.exists(str$("tests/")));

    // substrings also work
    tassert_eqe(EOK, os.path.exists(str.sub(str$("tests/asldjalsdj"), 0, 6)));

    char buf[PATH_MAX + 10];
    memset(buf, 'a', arr$len(buf));
    buf[PATH_MAX + 8] = '\0';
    str_c s = str.cbuf(buf, arr$len(buf));
    s.len++;

    // Path is too long, and exceeds PATH_MAX buffer size
    tassert_eqe(Error.overflow, os.path.exists(s));

    s.len--;
    tassert_eqe("File name too long", os.path.exists(s));

    return EOK;
}

test$case(test_os_path_join)
{
    sbuf_c s = { 0 };
    tassert_eqe(EOK, sbuf.create(&s, 10, mem$));
    tassert_eqe(EOK, os.path.join(&s, "%S/%s_%d.txt", str$("cexstr"), "foo", 10));
    tassert_eqs("cexstr/foo_10.txt", s);
    sbuf.destroy(&s);
    return EOK;
}

test$case(test_os_setenv)
{
    // get non existing
    tassert_eqs(os.getenv("test_os_posix", NULL),  NULL);
    // get non existing, with default
    tassert_eqs(os.getenv("test_os_posix", "envdef"),  "envdef");

    // set env
    os.setenv("test_os_posix", "foo", true);
    tassert_eqs(os.getenv("test_os_posix", NULL),  "foo");

    // set without replacing
    os.setenv("test_os_posix", "bar", false);
    tassert_eqs(os.getenv("test_os_posix", NULL),  "foo");

    // set with replacing
    os.setenv("test_os_posix", "bar", true);
    tassert_eqs(os.getenv("test_os_posix", NULL),  "bar");

    // unset env
    os.unsetenv("test_os_posix");
    tassert_eqs(os.getenv("test_os_posix", NULL),  NULL);

    return EOK;
}
test$case(test_os_path_splitext)
{
    tassert_eqi(str.cmp(os.path.splitext(str$("foo.bar"), true), str$(".bar")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("foo.bar"), false), str$("foo")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("foo"), true), str$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("foo"), false), str$("foo")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("foo.bar.exe"), true), str$(".exe")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("foo.bar.exe"), false), str$("foo.bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("foo/bar.exe"), true), str$(".exe")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("foo/bar.exe"), false), str$("foo/bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar.exe"), true), str$(".exe")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar.exe"), false), str$("bar.foo/bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar"), true), str$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar"), false), str$("bar.foo/bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$(".gitingnore"), true), str$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$(".gitingnore"), false), str$(".gitingnore")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("...gitingnore"), true), str$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("...gitingnore"), false), str$("...gitingnore")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar..."), true), str$(".")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar..."), false), str$("bar.foo/bar..")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar."), true), str$(".")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar."), false), str$("bar.foo/bar")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("."), true), str$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("."), false), str$(".")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$(".."), true), str$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$(".."), false), str$("..")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar..exe"), true), str$(".exe")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$("bar.foo/bar..exe"), false), str$("bar.foo/bar.")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str$(""), true), str$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str$(""), false), str$("")), 0);

    tassert_eqi(str.cmp(os.path.splitext(str.cstr(NULL), false), str$("")), 0);
    tassert_eqi(str.cmp(os.path.splitext(str.cstr(NULL), true), str$("")), 0);
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
