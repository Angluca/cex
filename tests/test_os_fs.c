#include <cex/all.c>

#define TBUILDDIR "tests/build/"

Exception
test_dir_walk(const char* path, os_fs_filetype_s ftype, void* user_ctx)
{
    u32* cnt = (u32*)user_ctx;
    uassert(cnt != NULL);
    printf("path walker: %s, is_dir: %d, user_ctx: %p\n", path, ftype.is_directory, user_ctx);
    *cnt += 1;
    return EOK;
}

test$case(test_os_dir_walk_print)
{
    u32 nfiles = 0;

    // recursive walk (only files called)
    tassert_eqe(EOK, os.fs.dir_walk("tests/data/dir1", true, test_dir_walk, &nfiles));
    tassert_eqi(4, nfiles);

    // non-recursive
    nfiles = 0;
    tassert_eqe(EOK, os.fs.dir_walk("tests/data/dir1", false, test_dir_walk, &nfiles));
    tassert_eqi(3, nfiles);

    return EOK;
}

test$case(test_os_listdir)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.dir_list("tests/data/", false, _);
        tassert(files != NULL);
        tassert(arr$len(files) > 0);

        const char* expected[] = {
            "tests/data/allocator_fopen.txt",       "tests/data/text_file_50b.txt",
            "tests/data/text_file_empty.txt",       "tests/data/text_file_fprintf.txt",
            "tests/data/text_file_line_4095.txt",   "tests/data/text_file_only_newline.txt",
            "tests/data/text_file_win_newline.txt", "tests/data/text_file_write.txt",
            "tests/data/text_file_zero_byte.txt",   "tests/data/dir1",
        };
        u32 nit = 0;
        for$each(it, files)
        {
            bool is_found = false;
            for$each(itf, expected, arr$len(expected))
            {
                if (str.eq(it, itf)) {
                    is_found = true;
                    break;
                }
            }
            if (!is_found) {
                io.printf("Not found in expected: %s\n", it);
                tassert(is_found && "file not found in expected");
            }
            nit++;
        }
        tassert_eqi(nit, arr$len(expected));
    }

    //
    // tassert(sbuf.len(&listdir) > 0);
    // tassert_eqe(Error.not_found, os.fs.listdir("tests/data/unknownfolder", &listdir));
    // tassert_eqi(sbuf.len(&listdir), 0);
    //
    // sbuf.destroy(&listdir);
    return EOK;
}

test$case(test_os_getcwd)
{
    sbuf_c s = sbuf.create(10, mem$);
    tassert_eqe(EOK, os.fs.getcwd(&s));
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
    tassert_eqe(EOK, os.path.exists(__FILE__));
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

test$case(test_os_mkdir)
{
    tassert_eqe(Error.argument, os.fs.mkdir(NULL));
    tassert_eqe(Error.argument, os.fs.mkdir(""));

    if (os.fs.remove(TBUILDDIR "mytestdir")) {
    }

    var ftype = os.fs.file_type(TBUILDDIR "mytestdir");
    tassert_eqe(ftype.result, Error.not_found);
    tassert_eqi(ftype.is_valid, 0);
    tassert_eqi(ftype.is_directory, 0);
    tassert_eqi(ftype.is_file, 0);
    tassert_eqi(ftype.is_symlink, 0);
    tassert_eqi(ftype.is_other, 0);

    tassert_eqe(Error.not_found, os.path.exists(TBUILDDIR "mytestdir"));
    tassert_eqe(Error.ok, os.fs.mkdir(TBUILDDIR "mytestdir"));
    tassert_eqe(Error.ok, os.path.exists(TBUILDDIR "mytestdir"));
    ftype = os.fs.file_type(TBUILDDIR "mytestdir");
    tassert_eqe(ftype.result, Error.ok);
    tassert_eqi(ftype.is_valid, 1);
    tassert_eqi(ftype.is_directory, 1);
    tassert_eqi(ftype.is_file, 0);
    tassert_eqi(ftype.is_symlink, 0);
    tassert_eqi(ftype.is_other, 0);

    // Already exists no error
    tassert_eqe(Error.ok, os.fs.mkdir(TBUILDDIR "mytestdir"));

    tassert_eqe(Error.ok, os.fs.remove(TBUILDDIR "mytestdir"));
    tassert_eqe(Error.not_found, os.path.exists(TBUILDDIR "mytestdir"));

    ftype = os.fs.file_type("tests/data/dir1/dir2_symlink");
    tassert_eqe(ftype.result, Error.ok);
    tassert_eqi(ftype.is_valid, 1);
    tassert_eqi(ftype.is_directory, 1);
    tassert_eqi(ftype.is_file, 0);
    tassert_eqi(ftype.is_symlink, 1);
    tassert_eqi(ftype.is_other, 0);

    return EOK;
}

test$case(test_os_fs_file_type)
{

    tassert_eqe(EOK, os.path.exists(__FILE__));
    var ftype = os.fs.file_type(__FILE__);
    tassert_eqe(ftype.result, Error.ok);
    tassert_eqi(ftype.is_valid, 1);
    tassert_eqi(ftype.is_directory, 0);
    tassert_eqi(ftype.is_file, 1);
    tassert_eqi(ftype.is_symlink, 0);
    tassert_eqi(ftype.is_other, 0);

    return EOK;
}

test$case(test_os_rename_dir)
{
    if (os.fs.remove(TBUILDDIR "mytestdir")) {
    }

    tassert_eqe(Error.not_found, os.path.exists(TBUILDDIR "mytestdir"));
    tassert_eqe(Error.ok, os.fs.mkdir(TBUILDDIR "mytestdir"));
    tassert_eqe(Error.ok, os.path.exists(TBUILDDIR "mytestdir"));

    tassert_eqe(Error.ok, os.path.exists(TBUILDDIR "mytestdir"));

    tassert_eqe(Error.argument, os.fs.rename("", "foo"));
    tassert_eqe(Error.argument, os.fs.rename(NULL, "foo"));
    tassert_eqe(Error.argument, os.fs.rename("foo", ""));
    tassert_eqe(Error.argument, os.fs.rename("foo", NULL));

    tassert_eqe(Error.ok, os.fs.rename(TBUILDDIR "mytestdir", TBUILDDIR "mytestdir2"));
    tassert_eqe(Error.not_found, os.path.exists(TBUILDDIR "mytestdir"));
    tassert_eqe(Error.ok, os.path.exists(TBUILDDIR "mytestdir2"));
    var ftype = os.fs.file_type(TBUILDDIR "mytestdir2");
    tassert_eqe(ftype.result, EOK);
    tassert_eqi(ftype.is_valid, 1);
    tassert_eqi(ftype.is_directory, 1);
    tassert_eqi(ftype.is_file, 0);
    tassert_eqi(ftype.is_symlink, 0);
    tassert_eqi(ftype.is_other, 0);

    tassert_eqe(EOK, os.fs.remove(TBUILDDIR "mytestdir2"));


    return EOK;
}

test$case(test_os_rename_file)
{
    if (os.fs.remove(TBUILDDIR "mytestfile.txt")) {
    }
    if (os.fs.remove(TBUILDDIR "mytestfile.txt2")) {
    }

    tassert_eqe(Error.not_found, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_eqe(Error.ok, io.file.save(TBUILDDIR "mytestfile.txt", "foo"));
    tassert_eqe(Error.ok, os.path.exists(TBUILDDIR "mytestfile.txt"));

    tassert_eqe(Error.ok, os.fs.rename(TBUILDDIR "mytestfile.txt", TBUILDDIR "mytestfile.txt2"));
    tassert_eqe(Error.not_found, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_eqe(Error.ok, os.path.exists(TBUILDDIR "mytestfile.txt2"));
    var ftype = os.fs.file_type(TBUILDDIR "mytestfile.txt2");
    tassert_eqe(ftype.result, EOK);
    tassert_eqi(ftype.is_valid, 1);
    tassert_eqi(ftype.is_directory, 0);
    tassert_eqi(ftype.is_file, 1);
    tassert_eqi(ftype.is_symlink, 0);
    tassert_eqi(ftype.is_other, 0);

    tassert_eqe(EOK, os.fs.remove(TBUILDDIR "mytestfile.txt2"));


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
    tassert_eqs(os.env.get("test_os_posix", NULL), NULL);
    // get non existing, with default
    tassert_eqs(os.env.get("test_os_posix", "envdef"), "envdef");

    // set env
    os.env.set("test_os_posix", "foo", true);
    tassert_eqs(os.env.get("test_os_posix", NULL), "foo");

    // set without replacing
    os.env.set("test_os_posix", "bar", false);
    tassert_eqs(os.env.get("test_os_posix", NULL), "foo");

    // set with replacing
    os.env.set("test_os_posix", "bar", true);
    tassert_eqs(os.env.get("test_os_posix", NULL), "bar");

    // unset env
    os.env.unset("test_os_posix");
    tassert_eqs(os.env.get("test_os_posix", NULL), NULL);

    return EOK;
}
test$case(test_os_path_split)
{

    // dir part
    tassert_eqi(str.slice.cmp(os.path.split("foo.bar", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("foo", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.split(".", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("..", true), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("./", true), str$s(".")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("../", true), str$s("..")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("/my", true), str$s("/")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("/", true), str$s("/")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("asd/a", true), str$s("asd")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("\\foo\\bar\\baz", true), str$s("\\foo\\bar")), 0);

    // file part
    tassert_eqi(str.slice.cmp(os.path.split("foo.bar", false), str$s("foo.bar")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("foo", false), str$s("foo")), 0);
    tassert_eqi(str.slice.cmp(os.path.split(".", false), str$s(".")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("..", false), str$s("..")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("./", false), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("../", false), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("/my", false), str$s("my")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("/", false), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("asd/", false), str$s("")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("asd/a", false), str$s("a")), 0);
    tassert_eqi(str.slice.cmp(os.path.split("\\foo\\bar\\baz", false), str$s("baz")), 0);

    return EOK;
}

test$case(test_os_path_basename_dirname)
{
    tassert_eqs(NULL, os.path.basename(NULL, mem$));
    tassert_eqs(NULL, os.path.basename("", mem$));
    tassert_eqs(NULL, os.path.dirname(NULL, mem$));
    tassert_eqs(NULL, os.path.dirname("", mem$));

    mem$scope(tmem$, _)
    {
        tassert_eqs("baz.txt", os.path.basename("foo/bar/baz.txt", _));
        tassert_eqs(".txt", os.path.basename("foo/bar/.txt", _));
        tassert_eqs("txt", os.path.basename("foo/bar/txt", _));
        tassert_eqs("", os.path.basename("foo/bar/", _));

        tassert_eqs("foo/bar", os.path.dirname("foo/bar/baz.txt", _));
        tassert_eqs("/", os.path.dirname("/foo", _));
    }

    return EOK;
}

test$main();
