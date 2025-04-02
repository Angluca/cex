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
    tassert_er(EOK, os.fs.dir_walk("tests/data/dir1", true, test_dir_walk, &nfiles));
    tassert_eq(4, nfiles);

    // non-recursive
    nfiles = 0;
    tassert_er(EOK, os.fs.dir_walk("tests/data/dir1", false, test_dir_walk, &nfiles));
    tassert_eq(3, nfiles);

    return EOK;
}

test$case(test_os_find)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find("tests/data/dir1/", false, _);
        tassert(files != NULL);
        tassert(arr$len(files) > 0);
        ;
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, "tests/data/dir1/file1.csv", true);
        hm$set(exp_files, "tests/data/dir1/file3.txt", true);

        u32 nit = 0;
        for$each(it, files)
        {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

   return EOK;
}

test$case(test_os_find_no_trailing_slash)
{

    mem$scope(tmem$, _)
    {
        // NOTE: when dir1 is a directory, and input path is without a pattern
        //       dir1 considered as a pattern
        arr$(char*) files = os.fs.find("tests/data/dir1", false, _);
        tassert(files != NULL);
        tassert_eq(arr$len(files), 0);
    }

   return EOK;
}

test$case(test_os_find_pattern)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find("tests/data/dir1/*.txt", false, _);
        tassert(files != NULL);
        tassert(arr$len(files) > 0);

        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, "tests/data/dir1/file3.txt", true);

        u32 nit = 0;
        for$each(it, files)
        {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

   return EOK;
}

test$case(test_os_find_pattern_glob)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find("tests/data/dir1/file?.(txt|csv)", false, _);
        tassert(files != NULL);
        tassert(arr$len(files) > 0);
        ;
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, "tests/data/dir1/file1.csv", true);
        hm$set(exp_files, "tests/data/dir1/file3.txt", true);

        u32 nit = 0;
        for$each(it, files)
        {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

   return EOK;
}

test$case(test_os_find_pattern_with_relative_non_norm_path)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find("tests/data/../../tests/data/dir1/*.txt", false, _);
        tassert(files != NULL);
        tassert_eq(arr$len(files), 1);
    }

   return EOK;
}

test$case(test_os_find_recursive)
{

    mem$scope(tmem$, _)
    {
        arr$(char*) files = os.fs.find("tests/data/dir1/*.csv", true, _);
        tassert(files != NULL);
        tassert(arr$len(files) > 0);
        ;
        hm$(char*, bool) exp_files = hm$new(exp_files, _);
        hm$set(exp_files, "tests/data/dir1/file1.csv", true);
        hm$set(exp_files, "tests/data/dir1/dir2/dir3/file4.csv", true);
        hm$set(exp_files, "tests/data/dir1/dir2/file2.csv", true);

        u32 nit = 0;
        for$each(it, files)
        {
            log$debug("found file: %s\n", it);
            tassert(NULL != hm$getp(exp_files, it));
            nit++;
        }
        tassert_eq(nit, hm$len(exp_files));
    }

   return EOK;
}

test$case(test_os_getcwd)
{
    sbuf_c s = sbuf.create(10, mem$);
    tassert_er(EOK, os.fs.getcwd(&s));
    tassert_eq(true, str.slice.ends_with(str.sstr(s), str$s("cex")));

    sbuf.destroy(&s);
    return EOK;
}

test$case(test_os_path_exists)
{
    tassert_er(Error.argument, os.path.exists(NULL));
    tassert_er(Error.argument, os.path.exists(""));
    tassert_er(EOK, os.path.exists("."));
    tassert_er(EOK, os.path.exists(".."));
    tassert_er(EOK, os.path.exists("./tests"));
    tassert_er(EOK, os.path.exists(__FILE__));
    tassert_er(Error.not_found, os.path.exists("./tests/test_os_posix.cpp"));

    tassert_er(EOK, os.path.exists("tests/"));

    char buf[PATH_MAX + 10];
    memset(buf, 'a', arr$len(buf));
    buf[PATH_MAX + 8] = '\0';
    uassert_disable();

    // Path is too long, and exceeds PATH_MAX buffer size
    tassert_er(Error.argument, os.path.exists(buf));


    return EOK;
}

test$case(test_os_mkdir)
{
    tassert_er(Error.argument, os.fs.mkdir(NULL));
    tassert_er(Error.argument, os.fs.mkdir(""));

    if (os.fs.remove(TBUILDDIR "mytestdir")) {
    }

    var ftype = os.fs.file_type(TBUILDDIR "mytestdir");
    tassert_er(ftype.result, Error.not_found);
    tassert_eq(ftype.is_valid, 0);
    tassert_eq(ftype.is_directory, 0);
    tassert_eq(ftype.is_file, 0);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);

    tassert_er(Error.not_found, os.path.exists(TBUILDDIR "mytestdir"));
    tassert_er(Error.ok, os.fs.mkdir(TBUILDDIR "mytestdir"));
    tassert_er(Error.ok, os.path.exists(TBUILDDIR "mytestdir"));
    ftype = os.fs.file_type(TBUILDDIR "mytestdir");
    tassert_er(ftype.result, Error.ok);
    tassert_eq(ftype.is_valid, 1);
    tassert_eq(ftype.is_directory, 1);
    tassert_eq(ftype.is_file, 0);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);

    // Already exists no error
    tassert_er(Error.ok, os.fs.mkdir(TBUILDDIR "mytestdir"));

    tassert_er(Error.ok, os.fs.remove(TBUILDDIR "mytestdir"));
    tassert_er(Error.not_found, os.path.exists(TBUILDDIR "mytestdir"));

    ftype = os.fs.file_type("tests/data/dir1/dir2_symlink");
    tassert_er(ftype.result, Error.ok);
    tassert_eq(ftype.is_valid, 1);
    tassert_eq(ftype.is_directory, 1);
    tassert_eq(ftype.is_file, 0);
    tassert_eq(ftype.is_symlink, 1);
    tassert_eq(ftype.is_other, 0);
    tassert_eq(ftype.is_other, 0);

    return EOK;
}

test$case(test_os_fs_file_type)
{

    tassert_er(EOK, os.path.exists(__FILE__));
    var ftype = os.fs.file_type(__FILE__);
    tassert_er(ftype.result, Error.ok);
    tassert_eq(ftype.is_valid, 1);
    tassert_eq(ftype.is_directory, 0);
    tassert_eq(ftype.is_file, 1);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);

    return EOK;
}

test$case(test_os_rename_dir)
{
    if (os.fs.remove(TBUILDDIR "mytestdir")) {
    }

    tassert_er(Error.not_found, os.path.exists(TBUILDDIR "mytestdir"));
    tassert_er(Error.ok, os.fs.mkdir(TBUILDDIR "mytestdir"));
    tassert_er(Error.ok, os.path.exists(TBUILDDIR "mytestdir"));

    tassert_er(Error.ok, os.path.exists(TBUILDDIR "mytestdir"));

    tassert_er(Error.argument, os.fs.rename("", "foo"));
    tassert_er(Error.argument, os.fs.rename(NULL, "foo"));
    tassert_er(Error.argument, os.fs.rename("foo", ""));
    tassert_er(Error.argument, os.fs.rename("foo", NULL));

    tassert_er(Error.ok, os.fs.rename(TBUILDDIR "mytestdir", TBUILDDIR "mytestdir2"));
    tassert_er(Error.not_found, os.path.exists(TBUILDDIR "mytestdir"));
    tassert_er(Error.ok, os.path.exists(TBUILDDIR "mytestdir2"));
    var ftype = os.fs.file_type(TBUILDDIR "mytestdir2");
    tassert_er(ftype.result, EOK);
    tassert_eq(ftype.is_valid, 1);
    tassert_eq(ftype.is_directory, 1);
    tassert_eq(ftype.is_file, 0);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);

    tassert_er(EOK, os.fs.remove(TBUILDDIR "mytestdir2"));


    return EOK;
}

test$case(test_os_rename_file)
{
    if (os.fs.remove(TBUILDDIR "mytestfile.txt")) {
    }
    if (os.fs.remove(TBUILDDIR "mytestfile.txt2")) {
    }

    tassert_er(Error.not_found, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_er(Error.ok, io.file.save(TBUILDDIR "mytestfile.txt", "foo"));
    tassert_er(Error.ok, os.path.exists(TBUILDDIR "mytestfile.txt"));

    tassert_er(Error.ok, os.fs.rename(TBUILDDIR "mytestfile.txt", TBUILDDIR "mytestfile.txt2"));
    tassert_er(Error.not_found, os.path.exists(TBUILDDIR "mytestfile.txt"));
    tassert_er(Error.ok, os.path.exists(TBUILDDIR "mytestfile.txt2"));
    var ftype = os.fs.file_type(TBUILDDIR "mytestfile.txt2");
    tassert_er(ftype.result, EOK);
    tassert_eq(ftype.is_valid, 1);
    tassert_eq(ftype.is_directory, 0);
    tassert_eq(ftype.is_file, 1);
    tassert_eq(ftype.is_symlink, 0);
    tassert_eq(ftype.is_other, 0);

    tassert_er(EOK, os.fs.remove(TBUILDDIR "mytestfile.txt2"));


    return EOK;
}

test$case(test_os_path_join)
{
    mem$scope(tmem$, ta)
    {
        arr$(char*) parts = arr$new(parts, ta);
        arr$pushm(parts, "cexstr", str.fmt(ta, "%s_%d.txt", "foo", 10));
        char* p = os.path.join(parts, ta);
        tassert_eq("cexstr/foo_10.txt", p);
    }
    return EOK;
}

test$case(test_os_setenv)
{
    // get non existing
    tassert_eq(os.env.get("test_os_posix", NULL), NULL);
    // get non existing, with default
    tassert_eq(os.env.get("test_os_posix", "envdef"), "envdef");

    // set env
    os.env.set("test_os_posix", "foo", true);
    tassert_eq(os.env.get("test_os_posix", NULL), "foo");

    // set without replacing
    os.env.set("test_os_posix", "bar", false);
    tassert_eq(os.env.get("test_os_posix", NULL), "foo");

    // set with replacing
    os.env.set("test_os_posix", "bar", true);
    tassert_eq(os.env.get("test_os_posix", NULL), "bar");

    // unset env
    os.env.unset("test_os_posix");
    tassert_eq(os.env.get("test_os_posix", NULL), NULL);

    return EOK;
}
test$case(test_os_path_split)
{

    // dir part
    tassert_eq(str.slice.cmp(os.path.split("foo.bar", true), str$s("")), 0);
    tassert_eq(str.slice.cmp(os.path.split("foo", true), str$s("")), 0);
    tassert_eq(str.slice.cmp(os.path.split(".", true), str$s("")), 0);
    tassert_eq(str.slice.cmp(os.path.split("..", true), str$s("")), 0);
    tassert_eq(str.slice.cmp(os.path.split("./", true), str$s(".")), 0);
    tassert_eq(str.slice.cmp(os.path.split("../", true), str$s("..")), 0);
    tassert_eq(str.slice.cmp(os.path.split("/my", true), str$s("/")), 0);
    tassert_eq(str.slice.cmp(os.path.split("/", true), str$s("/")), 0);
    tassert_eq(str.slice.cmp(os.path.split("asd/a", true), str$s("asd")), 0);
    tassert_eq(str.slice.cmp(os.path.split("\\foo\\bar\\baz", true), str$s("\\foo\\bar")), 0);

    // file part
    tassert_eq(str.slice.cmp(os.path.split("foo.bar", false), str$s("foo.bar")), 0);
    tassert_eq(str.slice.cmp(os.path.split("foo", false), str$s("foo")), 0);
    tassert_eq(str.slice.cmp(os.path.split(".", false), str$s(".")), 0);
    tassert_eq(str.slice.cmp(os.path.split("..", false), str$s("..")), 0);
    tassert_eq(str.slice.cmp(os.path.split("./", false), str$s("")), 0);
    tassert_eq(str.slice.cmp(os.path.split("../", false), str$s("")), 0);
    tassert_eq(str.slice.cmp(os.path.split("/my", false), str$s("my")), 0);
    tassert_eq(str.slice.cmp(os.path.split("/", false), str$s("")), 0);
    tassert_eq(str.slice.cmp(os.path.split("asd/", false), str$s("")), 0);
    tassert_eq(str.slice.cmp(os.path.split("asd/a", false), str$s("a")), 0);
    tassert_eq(str.slice.cmp(os.path.split("\\foo\\bar\\baz", false), str$s("baz")), 0);

    return EOK;
}

test$case(test_os_path_basename_dirname)
{
    tassert_eq(os.path.basename(NULL, mem$), NULL);
    tassert_eq(os.path.basename("", mem$), NULL);
    tassert_eq(os.path.dirname(NULL, mem$), NULL);
    tassert_eq(os.path.dirname("", mem$), NULL);

    mem$scope(tmem$, _)
    {
        tassert_eq("baz.txt", os.path.basename("foo/bar/baz.txt", _));
        tassert_eq(".txt", os.path.basename("foo/bar/.txt", _));
        tassert_eq("txt", os.path.basename("foo/bar/txt", _));
        tassert_eq("", os.path.basename("foo/bar/", _));

        tassert_eq("foo/bar", os.path.dirname("foo/bar/baz.txt", _));
        tassert_eq("/", os.path.dirname("/foo", _));
    }

    return EOK;
}

test$main();
