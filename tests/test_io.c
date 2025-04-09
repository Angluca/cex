#include "include/cex/all.c"

test$case(test_io)
{
    FILE* file = (void*)221;
    tassert_eq(Error.not_found, io.fopen(&file, "test_not_exist.txt", "r"));
    tassert(file == NULL);

    uassert_disable();
    tassert_eq(Error.argument, io.fopen(&file, "test_not_exist.txt", NULL));
    tassert_eq(Error.argument, io.fopen(&file, NULL, "r"));
    tassert_eq(Error.argument, io.fopen(NULL, "test.txt", "r"));
    return EOK;
}

test$case(test_readall)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    str_s content;
    tassert_eq(Error.ok, io.fread_all(file, &content, mem$));

    tassert_eq(50, io.file.size(file));
    tassert_eq(
        "000000001\n"
        "000000002\n"
        "000000003\n"
        "000000004\n"
        "000000005\n",
        content.buf
    );
    tassert_eq(content.len, 50);

    mem$free(mem$, content.buf);

    tassert(file != NULL);
    io.fclose(&file);
    tassert(file == NULL);
    return EOK;
}


test$case(test_read_all_empty)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r"));
    str_s content;
    tassert_eq(Error.ok, io.fread_all(file, &content, mem$));
    tassert_eq(0, io.file.size(file));
    tassert(content.buf != NULL);
    tassert(content.len == 0);
    tassert_eq(content.buf, "");

    mem$free(mem$, content.buf);

    io.fclose(&file);
    return EOK;
}

test$case(test_read_all_stdin)
{
    tassert_eq(0, io.file.size(stdin));

    str_s content;
    tassert_eq(
        "io.fread_all() not allowed for pipe/socket/std[in/out/err]",
        io.fread_all(stdin, &content, mem$)
    );
    return EOK;
}

test$case(test_read_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    str_s content;
    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000001");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000002");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    // filesize doesn't wreck file internal cursor
    tassert_eq(50, io.file.size(file));

    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000003");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000004");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    tassert_eq(Error.ok, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, "000000005");
    tassert_eq(content.len, 9);
    mem$free(mem$, content.buf);

    tassert_eq(Error.eof, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, NULL);
    tassert_eq(content.len, 0);

    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_stream)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));


    mem$scope(tmem$, _)
    {
        tassert_eq(io.file.readln(file, _), "000000001");
        tassert_eq(io.file.readln(file, _), "000000002");
        tassert_eq(io.file.readln(file, _), "000000003");
        tassert_eq(io.file.readln(file, _), "000000004");
        tassert_eq(io.file.readln(file, _), "000000005");
        tassert_eq(io.file.readln(file, _), NULL);
        tassert_eq(io.file.readln(file, _), NULL);
    }
    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_empty_file)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r"));

    str_s content;
    tassert_eq(Error.eof, io.fread_line(file, &content, mem$));
    tassert_eq(content.buf, NULL);
    tassert_eq(content.len, 0);

    tassert_eq(io.file.readln(file, mem$), NULL);

    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_binary_file_with_zero_char)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_zero_byte.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000001");
        tassert_eq(content.len, 9);

        tassert_eq(Error.integrity, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_fread_line_binary_file_with_zero_char)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_zero_byte.txt", "r"));

    mem$scope(tmem$, _)
    {
        tassert_eq("000000001", io.file.readln(file, _));
        tassert_eq(io.file.readln(file, _), NULL);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_win_new_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_win_newline.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000001");
        tassert_eq(content.len, 9);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000002");
        tassert_eq(content.len, 9);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000003");
        tassert_eq(content.len, 9);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000004");
        tassert_eq(content.len, 9);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000005");
        tassert_eq(content.len, 9);

        tassert_eq(Error.eof, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_fread_line_win_new_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_win_newline.txt", "r"));

    mem$scope(tmem$, _)
    {
        tassert_eq("000000001", io.file.readln(file, _));
        tassert_eq("000000002", io.file.readln(file, _));
        tassert_eq("000000003", io.file.readln(file, _));
        tassert_eq("000000004", io.file.readln(file, _));
        tassert_eq("000000005", io.file.readln(file, _));

        tassert_eq(io.file.readln(file, _), NULL);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_line_only_new_lines)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_only_newline.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "");
        tassert_eq(content.len, 0);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "");
        tassert_eq(content.len, 0);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "");
        tassert_eq(content.len, 0);

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "");
        tassert_eq(content.len, 0);

        tassert_eq(Error.eof, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_all_then_read_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_all(file, &content, _));
        tassert_eq(50, io.file.size(file));

        tassert_eq(Error.eof, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);

        tassert_eq(Error.ok, io.fseek(file, 0, SEEK_SET));
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert_eq(content.buf, "000000001");
        tassert_eq(content.len, 9);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_long_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(4096 + 4095 + 2, io.file.size(file));

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert(str.slice.starts_with(content, str.sstr("4095")));
        tassert_eq(content.len, 4095);
        tassert_eq(0, content.buf[content.len]); // null term

        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert(str.slice.starts_with(content, str.sstr("4096")));
        tassert_eq(content.len, 4096);
        tassert_eq(0, content.buf[content.len]); // null term


        tassert_eq(Error.eof, io.fread_line(file, &content, _));
        tassert_eq(content.buf, NULL);
        tassert_eq(content.len, 0);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_fread_long_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r"));
    tassert_eq(4096 + 4095 + 2, io.file.size(file));

    mem$scope(tmem$, _)
    {
        char* line = io.file.readln(file, _);
        tassert(line);
        tassert(str.starts_with(line, "4095"));
        tassert_eq(str.len(line), 4095);

        line = io.file.readln(file, _);
        tassert(line);
        tassert(str.starts_with(line, "4096"));
        tassert_eq(str.len(line), 4096);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read_all_realloc)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r"));

    str_s content;
    tassert_eq(4096 + 4095 + 2, io.file.size(file));

    mem$scope(tmem$, _)
    {
        tassert_eq(Error.ok, io.fread_line(file, &content, _));
        tassert(str.slice.starts_with(content, str.sstr("4095")));
        tassert_eq(content.len, 4095);
        tassert_eq(0, content.buf[content.len]); // null term

        io.rewind(file);

        tassert_eq(Error.ok, io.fread_all(file, &content, _));
        tassert(str.slice.starts_with(content, str.sstr("4095")));
        tassert_eq(content.len, 4095 + 4096 + 2);
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_read)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_line_4095.txt", "r"));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    usize read_len = 4;
    tassert_eq(Error.ok, io.fread(file, buf, 2, &read_len));
    tassert_eq(memcmp(buf, "40950000", 8), 0);
    tassert_eq(read_len, 4);

    io.fclose(&file);
    return EOK;
}

test$case(test_read_empty)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_empty.txt", "r"));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    usize read_len = 4;
    tassert_eq(Error.eof, io.fread(file, buf, 2, &read_len));
    tassert_eq(memcmp(buf, "zzzzzzzz", 8), 0); // untouched!
    tassert_eq(read_len, 0);

    io.fclose(&file);
    return EOK;
}

test$case(test_read_not_all)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_50b.txt", "r"));

    char buf[128];
    memset(buf, 'z', arr$len(buf));

    usize read_len = 100;
    tassert_eq(Error.ok, io.fread(file, buf, 1, &read_len));
    tassert_eq(read_len, 50);

    // NOTE: read method does not null terminate!
    tassert_eq(buf[read_len], 'z');

    buf[read_len] = '\0'; // null terminate to compare string result below
    tassert_eq(
        "000000001\n"
        "000000002\n"
        "000000003\n"
        "000000004\n"
        "000000005\n",
        buf
    );

    tassert_eq(Error.eof, io.fread(file, buf, 1, &read_len));
    tassert_eq(read_len, 0);

    io.fclose(&file);
    return EOK;
}


test$case(test_fprintf_to_file)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_fprintf.txt", "w+"));

    char buf[4] = { "1234" };
    str_s s1 = str.sbuf(buf, 4);
    tassert_eq(s1.len, 4);
    tassert_eq(s1.buf[3], '4');

    tassert_eq(EOK, io.fprintf(file, "io.fprintf: str_c: %S\n", s1));

    str_s content;
    io.rewind(file);

    tassert_eq(EOK, io.fread_all(file, &content, mem$));
    tassert_eq(1, str.slice.eq(content, str$s("io.fprintf: str_c: 1234\n")));

    mem$free(mem$, content.buf);
    io.fclose(&file);
    return EOK;
}

test$case(test_write)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_write.txt", "w+"));

    char buf[4] = { "1234" };

    tassert_eq(EOK, io.fwrite(file, buf, sizeof(char), arr$len(buf)));

    str_s content;
    io.rewind(file);
    tassert_eq(EOK, io.fread_all(file, &content, mem$));
    tassert_eq(0, str.slice.cmp(content, str$s("1234")));

    mem$free(mem$, content.buf);
    io.fclose(&file);
    return EOK;
}

test$case(test_fload_save)
{
    tassert_eq(Error.ok, io.file.save("tests/data/text_file_write.txt", "Hello from CEX!\n"));
    char* content = io.file.load("tests/data/text_file_write.txt", mem$);
    tassert(content);
    tassert_eq(content, "Hello from CEX!\n");
    mem$free(mem$, content);
    return EOK;
}

test$case(test_fload_not_found)
{
    tassert_eq("Is a directory", io.file.save("tests/", "Hello from CEX!\n"));
    return EOK;
}

test$case(test_write_line)
{
    FILE* file;
    tassert_eq(Error.ok, io.fopen(&file, "tests/data/text_file_write.txt", "w+"));

    str_s content;
    mem$scope(tmem$, _)
    {
        tassert_eq(EOK, io.file.writeln(file, "hello"));
        tassert_eq(EOK, io.file.writeln(file, "world"));

        io.rewind(file);
        tassert_eq("hello", io.file.readln(file, _));
        tassert_er(EOK, io.fread_line(file, &content, _));
        tassert(str.slice.eq(content, str$s("world")));
    }

    io.fclose(&file);
    return EOK;
}

test$case(test_fload)
{
    char* content = io.file.load("tests/data/text_file_line_4095.txt", mem$);
    tassert(str.starts_with(content, "409500000000"));
    mem$free(mem$, content);

    content = io.file.load("tests/data/text_file_empty.txt", mem$);
    tassert_eq("", content);
    mem$free(mem$, content);

    content = io.file.load("tests/data/asdjaldhashdajlkhuci.txt", mem$);
    tassert_eq(content, NULL);
    tassert_eq(ENOENT, errno);
    tassert_eq("No such file or directory", strerror(errno));

    content = io.file.load(NULL, mem$);
    tassert_eq(content, NULL);
    tassert_eq(EINVAL, errno);

    content = io.file.load("tests/data/text_file_empty.txt", mem$);
    tassert_eq("", content);
    mem$free(mem$, content);

    content = io.file.load("tests/data/text_file_zero_byte.txt", mem$);
    tassert_eq("000000001\n0", content);
    mem$free(mem$, content);

    // TODO: this may fail on windows
    content = io.file.load("/dev/console", mem$);
    tassert_eq("Permission denied", strerror(errno));
    tassert_eq(EACCES, errno);
    tassert_eq(content, NULL);
    mem$free(mem$, content);

    return EOK;
}

test$main();
