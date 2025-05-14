#include "src/all.c"

/*
 *
 *   TEST SUITE
 *
 */

test$case(test_cstr)
{
    char* cstr = "hello";


    str_s s = str.sstr(cstr);
    tassert_eq(s.buf, cstr);
    tassert_eq(s.len, 5); // lazy init until str.length() is called
    tassert(s.buf == cstr);
    tassert_eq(str.len(s.buf), 5);


    tassert_eq(s.len, 5); // now s.len is set

    str_s snull = str.sstr(NULL);
    tassert_eq(snull.buf, NULL);
    tassert_eq(snull.len, 0);
    tassert_eq(str.len(snull.buf), 0);

    str_s sempty = str.sstr("");
    tassert_eq(sempty.buf, "");
    tassert_eq(sempty.len, 0);
    tassert_eq(str.len(sempty.buf), 0);
    tassert_eq(sempty.len, 0);
    return EOK;
}

test$case(test_cstr_sdollar)
{
    char* cstr = "hello";

    sbuf_c sb = sbuf.create(10, mem$);
    tassert_eq(EOK, sbuf.append(&sb, "hello"));
    sbuf.destroy(&sb);

    str_s s2 = str$s("hello");
    tassert_eq(s2.buf, cstr);
    tassert_eq(s2.len, 5); // lazy init until str.length() is called
    tassert(s2.buf == cstr);


    char* str_null = NULL;
    str_s snull = str.sstr(str_null);
    tassert_eq(snull.buf, NULL);
    tassert_eq(snull.len, 0);

    str_s sempty = str$s("");
    tassert_eq(sempty.buf, "");
    tassert_eq(sempty.len, 0);
    tassert_eq(sempty.len, 0);

    // // WARNING: buffers for str$s() are extremely bad idea, they may not be null term!
    char buf[30] = { 0 };
    var s = str.sstr(buf);
    tassert_eq(s.buf, buf);
    tassert_eq(s.len, 0); // lazy init until str.length() is called

    memset(buf, 'z', arr$len(buf));

    // WARNING: if no null term
    // s = str$s(buf); // !!!! STACK OVERFLOW, no null term!
    //
    // Consider using str.sbuf() - it limits length by buffer size
    s = str.sbuf(buf, arr$len(buf));
    tassert_eq(s.len, 30);
    tassert(s.buf == buf);

    return EOK;
}

test$case(test_str_len)
{
    char* cstr = "hello";
    tassert_eq(5, str.len(cstr));
    tassert_eq(2, str.len("wa"));
    tassert_eq(0, str.len(""));

    char a[1] = { 0 };
    tassert_eq(0, str.len(a));
    tassert_eq(0, str.len(NULL));

    return EOK;
}

test$case(test_copy)
{
    char buf[8];
    memset(buf, 'a', arr$len(buf));


    memset(buf, 'a', arr$len(buf));
    tassert_eq(EOK, str.copy(buf, "1234567", arr$len(buf)));
    tassert_eq("1234567", buf);
    tassert_eq(strlen(buf), 7);
    tassert_eq(buf[7], '\0');

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.argument, str.copy(NULL, "1234567", arr$len(buf)));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0);
    tassert_eq(Error.argument, str.copy(buf, "1234567", 0)); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.ok, str.copy(buf, "", arr$len(buf)));
    tassert_eq("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.argument, str.copy(buf, NULL, arr$len(buf)));
    // buffer reset to "" string
    tassert_eq("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.overflow, str.copy(buf, "12345678", arr$len(buf)));
    // string is truncated
    tassert_eq(buf[7], '\0');
    tassert_eq("1234567", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.overflow, str.copy(buf, "1234567812309812308", arr$len(buf)));
    // string is truncated
    tassert_eq(buf[7], '\0');
    tassert_eq("1234567", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.ok, str.copy(buf, "1234", arr$len(buf)));
    tassert_eq("1234", buf);
    return EOK;
}

test$case(test_slice_copy)
{
    char buf[8];
    memset(buf, 'a', arr$len(buf));


    memset(buf, 'a', arr$len(buf));
    tassert_eq(EOK, str.slice.copy(buf, str$s("1234567"), arr$len(buf)));
    tassert_eq("1234567", buf);
    tassert_eq(strlen(buf), 7);
    tassert_eq(buf[7], '\0');

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.argument, str.slice.copy(NULL, str$s("1234567"), arr$len(buf)));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0);
    tassert_eq(Error.argument, str.slice.copy(buf, str$s("1234567"), 0)); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.ok, str.slice.copy(buf, str$s(""), arr$len(buf)));
    tassert_eq("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.argument, str.slice.copy(buf, (str_s){ 0 }, arr$len(buf)));
    // buffer reset to "" string
    tassert_eq("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.overflow, str.slice.copy(buf, str$s("12345678"), arr$len(buf)));
    tassert_eq("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eq(Error.ok, str.slice.copy(buf, str$s("1234"), arr$len(buf)));
    tassert_eq("1234", buf);
    return EOK;
}

test$case(test_sub_positive_start)
{

    str_s sub;
    str_s s = str.sstr("123456");
    tassert_eq(s.len, 6);

    sub = str.sub("123456", 0, s.len);
    tassert(sub.buf);
    tassert(memcmp(sub.buf, "123456", s.len) == 0);
    tassert_eq(sub.len, 6);
    tassert_eq(s.len, 6);


    sub = str.slice.sub(s, 1, 3);
    tassert(sub.buf);
    tassert_eq(sub.len, 2);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "23", sub.len) == 0);

    sub = str.slice.sub(s, 1, 2);
    tassert(sub.buf);
    tassert_eq(sub.len, 1);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "2", sub.len) == 0);

    sub = str.slice.sub(s, s.len - 1, s.len);
    tassert(sub.buf);
    tassert_eq(sub.len, 1);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "6", sub.len) == 0);

    sub = str.slice.sub(s, 2, 0);
    tassert(sub.buf);
    tassert_eq(sub.len, 4);
    tassert(memcmp(sub.buf, "3456", sub.len) == 0);

    sub = str.slice.sub(s, 0, s.len + 1);
    tassert(sub.buf);

    sub = str.slice.sub(s, 2, 1);
    tassert(sub.buf == NULL);

    sub = str.slice.sub(s, s.len, s.len + 1);
    tassert(sub.buf == NULL);

    s = str.sstr(NULL);
    sub = str.slice.sub(s, 1, 2);
    tassert(sub.buf == NULL);

    s = str.sstr("");
    tassert(s.buf);

    sub = str.slice.sub(s, 0, 0);
    tassert(sub.buf == NULL);
    tassert_eq(sub.len, 0);
    tassert_eq(sub.buf, NULL);

    sub = str.slice.sub(s, 1, 2);
    tassert(sub.buf == NULL);

    s = str.sstr("A");
    sub = str.slice.sub(s, 1, 2);
    tassert(sub.buf == NULL);

    s = str.sstr("A");
    sub = str.slice.sub(s, 0, 2);
    tassert(sub.buf);
    tassert_eq(sub.len, 1);
    tassert(memcmp(sub.buf, "A", sub.len) == 0);

    s = str.sstr("A");
    sub = str.slice.sub(s, 0, 0);
    tassert(sub.buf);
    tassert(memcmp(sub.buf, "A", sub.len) == 0);
    tassert_eq(sub.len, 1);
    return EOK;
}

test$case(test_sub_negative_start)
{

    str_s s = str.sstr("123456");
    tassert_eq(s.len, 6);
    tassert(s.buf);

    str_s s_empty = { 0 };
    var sub = str.slice.sub(s_empty, 0, 2);
    tassert_eq(sub.len, 0);
    tassert(sub.buf == NULL);
    tassert(sub.buf == NULL);

    sub = str.slice.sub(s, -3, -1);
    tassert(sub.buf);
    tassert_eq(sub.len, 2);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "45", sub.len) == 0);

    sub = str.slice.sub(s, -6, 0);
    tassert(sub.buf);
    tassert_eq(sub.len, 6);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);


    sub = str.slice.sub(s, -3, -2);
    tassert(sub.buf);
    tassert_eq(sub.len, 1);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "4", sub.len) == 0);

    sub = str.slice.sub(s, -3, -1);
    tassert(sub.buf);
    tassert_eq(sub.len, 2);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "45", sub.len) == 0);

    var slice = str$sslice(s, -3, -1);
    tassert_eq(slice.len, 2);
    tassert(memcmp(slice.arr, "45", slice.len) == 0);

    var slice2 = str$sslice(s, .start = -3, .end = -1);
    tassert_eq(slice2.len, 2);
    tassert(memcmp(slice2.arr, "45", slice2.len) == 0);

    sub = str.slice.sub(s, -3, -400);
    tassert(sub.buf == NULL);

    s = str.sstr("");
    sub = str.slice.sub(s, 0, 0);
    tassert(sub.buf == NULL);
    tassert_eq(sub.len, 0);

    s = str.sstr("");
    sub = str.slice.sub(s, -1, 0);
    tassert(sub.buf == NULL);

    s = str.sstr("123456");
    sub = str.slice.sub(s, -6, 0);
    tassert(sub.buf);
    tassert_eq(sub.len, 6);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = str.sstr("123456");
    sub = str.slice.sub(s, -6, 6);
    tassert(sub.buf);
    tassert_eq(sub.len, 6);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = str.sstr("123456");
    sub = str.slice.sub(s, -7, 0);
    tassert(sub.buf);
    tassert_eq(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = str.sstr("()");
    sub = str.slice.sub(s, 1, -1);
    tassert_eq(sub.buf, NULL);
    tassert_eq(sub.len, 0);

    return EOK;
}

test$case(test_eq)
{
    tassert_eq(str.eq("", ""), 1);
    tassert_eq(str.eq(NULL, ""), 0);
    tassert_eq(str.eq("", NULL), 0);
    tassert_eq(str.eq("1", "1"), 1);
    tassert_eq(str.eq("123456", "123456"), 1);
    tassert_eq(str.eq("123456", "12345"), 0);
    tassert_eq(str.eq("12345", "123456"), 0);
    tassert_eq(str.eq("12345", ""), 0);
    tassert_eq(str.eq("", "123456"), 0);

    tassert_eq(str.slice.eq(str$s(""), str$s("")), 1);
    tassert_eq(str.slice.eq((str_s){ 0 }, str$s("")), 0);
    tassert_eq(str.slice.eq(str$s(""), (str_s){ 0 }), 0);
    tassert_eq(str.slice.eq(str$s("1"), str$s("1")), 1);
    tassert_eq(str.slice.eq(str$s("123456"), str$s("123456")), 1);
    tassert_eq(str.slice.eq(str$s("123456"), str$s("12345")), 0);
    tassert_eq(str.slice.eq(str$s("12345"), str$s("123456")), 0);
    tassert_eq(str.slice.eq(str$s("12345"), str$s("")), 0);
    tassert_eq(str.slice.eq(str$s(""), str$s("123456")), 0);
    return EOK;
}

test$case(test_eqi)
{
    tassert_eq(str.eqi("", ""), 1);
    tassert_eq(str.eqi(NULL, ""), 0);
    tassert_eq(str.eqi("", NULL), 0);
    tassert_eq(str.eqi("1", "1"), 1);
    tassert_eq(str.eqi("123456", "123456"), 1);
    tassert_eq(str.eqi("123456", "12345"), 0);
    tassert_eq(str.eqi("12345", "123456"), 0);
    tassert_eq(str.eqi("12345", ""), 0);
    tassert_eq(str.eqi("", "123456"), 0);

    char* s = "abcdefghijklmnopqrstuvwxyz123!@#тест";
    char* s2 = "ABCDEFGHIJKLMNOPQRSTUVWXYZ123!@#тест";
    tassert(str.eqi(s, s2));

    tassert(!str.eqi(s, s2 + 1));
    tassert(!str.eqi(s + 1, s2));

    return EOK;
}

test$case(test_iter_split)
{

    str_s s = str.sstr("123456");
    u32 nit = 0;

    nit = 0;
    char* expected1[] = {
        "123456",
    };
    for$iter(str_s, it, str.slice.iter_split(s, ",", &it.iterator))
    {
        tassert(it.val.buf);
        // tassert_eq(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eq(str.slice.eq(it.val, str.sstr(expected1[nit])), 1);
        nit++;
    }
    tassert_eq(nit, 1);

    nit = 0;
    s = str.sstr("123,456");
    char* expected2[] = {
        "123",
        "456",
    };
    for$iter(str_s, it, str.slice.iter_split(s, ",", &it.iterator))
    {
        tassert(it.val.buf);
        // tassert_eq(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eq(str.slice.eq(it.val, str.sstr(expected2[nit])), 1);
        nit++;
    }
    tassert_eq(nit, 2);

    nit = 0;
    s = str.sstr("123,456,88,99");
    char* expected3[] = {
        "123",
        "456",
        "88",
        "99",
    };
    for$iter(str_s, it, str.slice.iter_split(s, ",", &it.iterator))
    {
        tassert(it.val.buf);
        // tassert_eq(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eq(str.slice.eq(it.val, str.sstr(expected3[nit])), 1);
        nit++;
    }
    tassert_eq(nit, 4);

    nit = 0;
    char* expected4[] = {
        "123",
        "456",
        "88",
        "",
    };
    s = str.sstr("123,456,88,");
    for$iter(str_s, it, str.slice.iter_split(s, ",", &it.iterator))
    {
        tassert(it.val.buf);
        // tassert_eq(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eq(str.slice.eq(it.val, str.sstr(expected4[nit])), 1);
        tassert_eq(it.idx.i, nit);
        nit++;
    }
    tassert_eq(nit, 4);

    nit = 0;
    s = str.sstr("123,456#88@99");
    char* expected5[] = {
        "123",
        "456",
        "88",
        "99",
    };
    for$iter(str_s, it, str.slice.iter_split(s, ",@#", &it.iterator))
    {
        tassert(it.val.buf);
        // tassert_eq(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eq(str.slice.eq(it.val, str.sstr(expected5[nit])), 1);
        tassert_eq(it.idx.i, nit);
        nit++;
    }
    tassert_eq(nit, 4);

    nit = 0;
    char* expected6[] = {
        "123",
        "456",
        "",
    };
    s = str.sstr("123\n456\n");
    for$iter(str_s, it, str.slice.iter_split(s, "\n", &it.iterator))
    {
        tassert(it.val.buf);
        // tassert_eq(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eq(str.slice.eq(it.val, str.sstr(expected6[nit])), 1);
        nit++;
    }
    tassert_eq(nit, 3);

    nit = 0;
    char* expected7[] = {
        "123",
        "",
        "",
        "456",
        "",
    };
    s = str.sstr("123\n\n\n456\n");
    for$iter(str_s, it, str.slice.iter_split(s, "\n", &it.iterator))
    {
        tassert(it.val.buf);
        tassert_eq(it.val, str.sstr(expected7[nit]));
        nit++;
    }
    tassert_eq(nit, 5);


    s = str.sstr("");
    nit = 0;
    for$iter(str_s, it, str.slice.iter_split(s, "\n", &it.iterator))
    {
        tassert(false && "should not happen");
    }
    tassert_eq(nit, 0);

    nit = 0;
    char* expected8[] = {
        "",
        "123",
        "456",
        "",
    };
    s = str.sstr("\n123\n456\n");
    for$iter(str_s, it, str.slice.iter_split(s, "\n", &it.iterator))
    {
        tassert(it.val.buf);
        tassert_eq(it.val, str.sstr(expected8[nit]));
        nit++;
    }
    tassert_eq(nit, 4);
    return EOK;
}

test$case(test_find)
{

    char* s = "123456";

    // match first
    tassert(s == str.find(s, "1"));

    // match full
    tassert(s == str.find(s, "123456"));

    // needle overflow s
    tassert(NULL == str.find(s, "1234567"));

    // match
    tassert(&s[1] == str.find(s, "23"));

    // empty needle
    tassert(NULL == str.find(s, ""));

    // empty string
    tassert(NULL == str.find("", "23"));

    // bad string
    tassert(NULL == str.find(NULL, "23"));

    // starts after match
    tassert(&s[1] == str.find(s, "23"));

    // bad needle
    tassert(NULL == str.find(s, NULL));

    // no match
    tassert(NULL == str.find(s, "foo"));

    // match at the end
    tassert(&s[4] == str.find(s, "56"));
    tassert(&s[5] == str.find(s, "6"));

    // middle
    tassert(&s[3] == str.find(s, "45"));

    // starts from left
    s = "123123123";
    tassert(s == str.find("123123123", "123"));

    return EOK;
}

test$case(test_rfind)
{

    char* s = "123456";

    // match first
    tassert(&s[0] == str.findr(s, "1"));

    // match last
    tassert(&s[5] == str.findr(s, "6"));

    // match full
    tassert(&s[0] == str.findr(s, "123456"));

    // needle overflow s
    tassert(NULL == str.findr(s, "1234567"));

    // match
    tassert(&s[1] == str.findr(s, "23"));

    // empty needle
    tassert(NULL == str.findr(s, ""));

    // empty string
    tassert(NULL == str.findr("", "23"));

    // bad string
    tassert(NULL == str.findr(NULL, "23"));

    // starts after match
    tassert(&s[1] == str.findr(s, "23"));

    // bad needle
    tassert(NULL == str.findr(s, NULL));

    // no match
    tassert(NULL == str.findr(s, "foo"));

    // match at the end
    tassert(&s[4] == str.findr(s, "56"));
    tassert(&s[5] == str.findr(s, "6"));

    // middle
    tassert(&s[3] == str.findr(s, "45"));

    // ends more than length ok
    tassert(&s[4] == str.findr(s, "56"));

    // starts from right
    s = "123123123";
    tassert(&s[6] == str.findr("123123123", "123"));

    return EOK;
}

test$case(test_contains_starts_ends)
{
    char* s = "123456";
    tassert_eq(1, str.starts_with(s, "1"));
    tassert_eq(1, str.starts_with(s, "1234"));
    tassert_eq(1, str.starts_with(s, "123456"));
    tassert_eq(0, str.starts_with(s, "1234567"));
    tassert_eq(0, str.starts_with(s, "234567"));
    tassert_eq(0, str.starts_with(s, "56"));
    tassert_eq(0, str.starts_with(s, ""));
    tassert_eq(0, str.starts_with(s, NULL));
    tassert_eq(0, str.starts_with("", "1"));
    tassert_eq(0, str.starts_with(NULL, "1"));


    tassert_eq(1, str.ends_with(s, "6"));
    tassert_eq(1, str.ends_with(s, "123456"));
    tassert_eq(1, str.ends_with(s, "56"));
    tassert_eq(0, str.ends_with(s, "1234567"));
    tassert_eq(0, str.ends_with(s, "5"));
    tassert_eq(0, str.ends_with(s, ""));
    tassert_eq(0, str.ends_with(s, NULL));
    tassert_eq(0, str.ends_with("", "1"));
    tassert_eq(0, str.ends_with(NULL, "1"));

    return EOK;
}


test$case(test_contains_sclice_starts_ends)
{
    str_s s = str$s("123456");
    tassert_eq(1, str.slice.starts_with(s, str.sstr("1")));
    tassert_eq(1, str.slice.starts_with(s, str.sstr("1234")));
    tassert_eq(1, str.slice.starts_with(s, str.sstr("123456")));
    tassert_eq(0, str.slice.starts_with(s, str.sstr("1234567")));
    tassert_eq(0, str.slice.starts_with(s, str.sstr("234567")));
    tassert_eq(0, str.slice.starts_with(s, str.sstr("56")));
    tassert_eq(0, str.slice.starts_with(s, str.sstr("")));
    tassert_eq(0, str.slice.starts_with(s, str.sstr(NULL)));
    tassert_eq(0, str.slice.starts_with(str$s("1"), str$s("")));
    tassert_eq(0, str.slice.starts_with((str_s){ 0 }, str$s("1")));


    tassert_eq(1, str.slice.ends_with(s, str.sstr("6")));
    tassert_eq(1, str.slice.ends_with(s, str.sstr("123456")));
    tassert_eq(1, str.slice.ends_with(s, str.sstr("123456")));
    tassert_eq(1, str.slice.ends_with(s, str.sstr("56")));
    tassert_eq(0, str.slice.ends_with(s, str.sstr("1234567")));
    tassert_eq(0, str.slice.ends_with(s, str.sstr("567")));
    tassert_eq(0, str.slice.ends_with(s, str.sstr("")));
    tassert_eq(0, str.slice.ends_with(s, str.sstr(NULL)));
    tassert_eq(0, str.slice.ends_with(str$s("1"), str$s("")));
    tassert_eq(0, str.slice.ends_with((str_s){ 0 }, str$s("1")));

    return EOK;
}

test$case(test_remove_prefix)
{
    str_s out;

    out = str.slice.remove_prefix(str$s("prefix_str_prefix"), str$s("prefix"));
    tassert_eq(str.slice.eq(out, str$s("_str_prefix")), 1);

    // no exact match skipped
    out = str.slice.remove_prefix(str$s(" prefix_str_prefix"), str$s("prefix"));
    tassert_eq(str.slice.eq(out, str$s(" prefix_str_prefix")), 1);

    // empty prefix
    out = str.slice.remove_prefix(str$s("prefix_str_prefix"), str$s(""));
    tassert_eq(str.slice.eq(out, str$s("prefix_str_prefix")), 1);

    // bad prefix
    out = str.slice.remove_prefix(str$s("prefix_str_prefix"), str.sstr(NULL));
    tassert_eq(str.slice.eq(out, str$s("prefix_str_prefix")), 1);

    // no match
    out = str.slice.remove_prefix(str$s("prefix_str_prefix"), str$s("prefi_"));
    tassert_eq(str.slice.eq(out, str$s("prefix_str_prefix")), 1);

    return EOK;
}

test$case(test_remove_suffix)
{
    str_s out;

    out = str.slice.remove_suffix(str$s("suffix_str_suffix"), str$s("suffix"));
    tassert_eq(str.slice.eq(out, str$s("suffix_str_")), 1);

    // no exact match skipped
    out = str.slice.remove_suffix(str$s("suffix_str_suffix "), str$s("suffix"));
    tassert_eq(str.slice.eq(out, str$s("suffix_str_suffix ")), 1);

    // empty suffix
    out = str.slice.remove_suffix(str$s("suffix_str_suffix"), str$s(""));
    tassert_eq(str.slice.eq(out, str$s("suffix_str_suffix")), 1);

    // bad suffix
    out = str.slice.remove_suffix(str$s("suffix_str_suffix"), str.sstr(NULL));
    tassert_eq(str.slice.eq(out, str$s("suffix_str_suffix")), 1);

    // no match
    out = str.slice.remove_suffix(str$s("suffix_str_suffix"), str$s("_uffix"));
    tassert_eq(str.slice.eq(out, str$s("suffix_str_suffix")), 1);

    return EOK;
}

test$case(test_strip)
{
    str_s s = str.sstr("\n\t \r123\n\t\r 456 \r\n\t");
    (void)s;
    str_s out;

    // LEFT
    out = str.slice.lstrip(str.sstr(NULL));
    tassert_eq(out.len, 0);
    tassert(out.buf == NULL);

    out = str.slice.lstrip(str.sstr(""));
    tassert_eq(out.len, 0);
    tassert_eq(out.buf, "");

    out = str.slice.lstrip(s);
    tassert_eq(out.len, 14);
    tassert_eq(out.buf, "123\n\t\r 456 \r\n\t");

    s = str.sstr("\n\t \r\r\n\t");
    out = str.slice.lstrip(s);
    tassert_eq(out.len, 0);
    tassert_eq("", out.buf);


    // RIGHT
    out = str.slice.rstrip(str.sstr(NULL));
    tassert_eq(out.len, 0);
    tassert(out.buf == NULL);

    out = str.slice.rstrip(str.sstr(""));
    tassert_eq(out.len, 0);
    tassert_eq(out.buf, "");

    s = str.sstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = str.slice.rstrip(s);
    tassert_eq(out.len, 14);
    tassert_eq(memcmp(out.buf, "\n\t \r123\n\t\r 456", out.len), 0);

    s = str.sstr("\n\t \r\r\n\t");
    out = str.slice.rstrip(s);
    tassert_eq(out.len, 0);
    tassert_eq(str.slice.eq(out, str$s("")), 1);

    // BOTH
    out = str.slice.strip(str.sstr(NULL));
    tassert_eq(out.len, 0);
    tassert(out.buf == NULL);

    out = str.slice.strip(str.sstr(""));
    tassert_eq(out.len, 0);
    tassert_eq(out.buf, "");

    s = str.sstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = str.slice.strip(s);
    tassert_eq(out.len, 10);
    tassert_eq(memcmp(out.buf, "123\n\t\r 456", out.len), 0);

    s = str.sstr("\n\t \r\r\n\t");
    out = str.slice.strip(s);
    tassert_eq(out.len, 0);
    tassert_eq("", out.buf);
    return EOK;
}

test$case(str_to__signed_num)
{
    i64 num;
    char* s;

    num = 0;
    s = "1";
    tassert_er(EOK, cex_str__to_signed_num(s,0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 1);

    num = 0;
    s = " ";
    tassert_er(Error.argument, cex_str__to_signed_num(s,0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = " -";
    tassert_er(Error.argument, cex_str__to_signed_num(s,0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = " +";
    tassert_er(Error.argument, cex_str__to_signed_num(s,0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = "+";
    tassert_er(Error.argument, cex_str__to_signed_num(s,0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = "100";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 100);

    num = 0;
    s = "      -100";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, -100);

    num = 0;
    s = "      +100";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 100);

    num = 0;
    s = "-100";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, -100);

    num = 10;
    s = "0";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = "0xf";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 15);

    num = 0;
    s = "-0xf";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, -15);


    num = 0;
    s = "0x0F";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 15);


    num = 0;
    s = "0x0A";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 10);

    num = 0;
    s = "0x0a";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 10);

    num = 0;
    s = "127";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, INT8_MAX);

    num = 0;
    s = "-127";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, -127);

    num = 0;
    s = "-128";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, INT8_MIN);

    num = 0;
    s = "-129";
    tassert_er(Error.overflow, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = "128";
    tassert_er(Error.overflow, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = "-0x80";
    tassert_er(Error.ok, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, -128);
    tassert_eq(num, INT8_MIN);

    num = 0;
    s = "-0x";
    tassert_er(Error.argument, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = "0x7f";
    tassert_er(Error.ok, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, 127);
    tassert_eq(num, INT8_MAX);

    num = 0;
    s = "0x80";
    tassert_er(Error.overflow, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = "-100";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, -100, 100));
    tassert_eq(num, -100);

    num = 0;
    s = "-101";
    tassert_er(Error.overflow, cex_str__to_signed_num(s, 0, &num, -100, 100));

    num = 0;
    s = "101";
    tassert_er(Error.overflow, cex_str__to_signed_num(s, 0, &num, -100, 100));

    num = 0;
    s = "-000000000127    ";
    tassert_er(EOK, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));
    tassert_eq(num, -127);

    num = 0;
    s = "    ";
    tassert_er(Error.argument, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = "12 2";
    tassert_er(Error.argument, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = "-000000000127a";
    tassert_er(Error.argument, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = "-000000000127h";
    tassert_er(Error.argument, cex_str__to_signed_num(s, 0, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = "-9223372036854775807";
    tassert_er(Error.ok, cex_str__to_signed_num(s, 0, &num, INT64_MIN + 1, INT64_MAX));
    tassert_eq(num, -9223372036854775807L);

    num = 0;
    s = "-9223372036854775808";
    tassert_er(Error.overflow, cex_str__to_signed_num(s, 0, &num, INT64_MIN + 1, INT64_MAX));

    num = 0;
    s = "9223372036854775807";
    tassert_er(Error.ok, cex_str__to_signed_num(s, 0, &num, INT64_MIN + 1, INT64_MAX));
    tassert_eq(num, 9223372036854775807L);

    num = 0;
    s = "9223372036854775808";
    tassert_er(Error.overflow, cex_str__to_signed_num(s, 0, &num, INT64_MIN + 1, INT64_MAX));

    return EOK;
}

test$case(test_str_to_i8)
{
    i8 num;
    char* s;

    num = 0;
    s = "127";
    tassert_er(EOK, str.convert.to_i8(s, &num));
    tassert_eq(num, 127);

    num = 0;
    s = "128";
    tassert_er(Error.overflow, str.convert.to_i8(s, &num));
    tassert_eq(num, 0);

    num = 0;
    s = "0";
    tassert_er(Error.ok, str.convert.to_i8(s, &num));
    tassert_eq(num, 0);

    num = 0;
    s = "-128";
    tassert_er(Error.ok, str.convert.to_i8(s, &num));
    tassert_eq(num, -128);


    num = 0;
    s = "-129";
    tassert_er(Error.overflow, str.convert.to_i8(s, &num));
    tassert_eq(num, 0);


    return EOK;
}


test$case(test_str_to_i16)
{
    i16 num;
    char* s;

    num = 0;
    s = "-32768";
    tassert_er(EOK, str.convert.to_i16(s, &num));
    tassert_eq(num, -32768);

    num = 0;
    s = "-32769";
    tassert_er(Error.overflow, str.convert.to_i16(s, &num));
    tassert_eq(num, 0);

    num = 0;
    s = "32767";
    tassert_er(Error.ok, str.convert.to_i16(s, &num));
    tassert_eq(num, 32767);

    num = 0;
    s = "32768";
    tassert_er(Error.overflow, str.convert.to_i16(s, &num));
    tassert_eq(num, 0);

    return EOK;
}

test$case(test_str_to_i32)
{
    i32 num;
    char* s;

    num = 0;
    s = "-2147483648";
    tassert_er(EOK, str.convert.to_i32(s, &num));
    tassert_eq(num, -2147483648);

    num = 0;
    s = "-2147483649";
    tassert_er(Error.overflow, str.convert.to_i32(s, &num));
    tassert_eq(num, 0);

    num = 0;
    s = "2147483647";
    tassert_er(Error.ok, str.convert.to_i32(s, &num));
    tassert_eq(num, 2147483647);


    num = 0;
    s = "2147483648";
    tassert_er(Error.overflow, str.convert.to_i32(s, &num));
    tassert_eq(num, 0);

    return EOK;
}

test$case(test_str_to_i64)
{
    i64 num;
    char* s;

    num = 0;
    s = "-9223372036854775807";
    tassert_er(Error.ok, str.convert.to_i64(s, &num));
    tassert_eq(num, -9223372036854775807);

    num = 0;
    s = "-9223372036854775808";
    tassert_er(Error.overflow, str.convert.to_i64(s, &num));
    // tassert_eq(num, -9223372036854775808UL);

    num = 0;
    s = "9223372036854775807";
    tassert_er(Error.ok, str.convert.to_i64(s, &num));
    tassert_eq(num, 9223372036854775807);

    num = 0;
    s = "9223372036854775808";
    tassert_er(Error.overflow, str.convert.to_i64(s, &num));
    // tassert_eq(num, 9223372036854775807);

    return EOK;
}

test$case(str_to__unsigned_num)
{
    u64 num;
    char* s;

    num = 0;
    s = "1";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 1);

    num = 0;
    s = " ";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = " -";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = " +";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = "+";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = "100";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 100);

    num = 0;
    s = "      -100";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));

    num = 0;
    s = "      +100";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 100);

    num = 0;
    s = "-100";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));

    num = 10;
    s = "0";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = "0xf";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 15);

    num = 0;
    s = "0x";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));

    num = 0;
    s = "-0xf";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));


    num = 0;
    s = "0x0F";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 15);


    num = 0;
    s = "0x0A";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 10);

    num = 0;
    s = "0x0a";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 10);

    num = 0;
    s = "127";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, INT8_MAX);

    num = 0;
    s = "-127";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));

    num = 0;
    s = "-128";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));

    num = 0;
    s = "255";
    tassert_er(Error.ok, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 255);

    num = 0;
    s = "256";
    tassert_er(Error.overflow, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 0);

    num = 0;
    s = "0xff";
    tassert_er(Error.ok, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 255);
    tassert_eq(num, UINT8_MAX);


    num = 0;
    s = "100";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, 100));
    tassert_eq(num, 100);

    num = 0;
    s = "101";
    tassert_er(Error.overflow, cex_str__to_unsigned_num(s, 0, &num, 100));


    num = 0;
    s = "000000000127    ";
    tassert_er(EOK, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));
    tassert_eq(num, 127);

    num = 0;
    s = "    ";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));

    num = 0;
    s = "12 2";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));

    num = 0;
    s = "000000000127a";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));

    num = 0;
    s = "000000000127h";
    tassert_er(Error.argument, cex_str__to_unsigned_num(s, 0, &num, UINT8_MAX));

    num = 0;
    s = "9223372036854775807";
    tassert_er(Error.ok, cex_str__to_unsigned_num(s, 0, &num, UINT64_MAX));
    tassert_eq(num, 9223372036854775807L);

    num = 0;
    s = "9223372036854775807";
    tassert_er(Error.ok, cex_str__to_unsigned_num(s, 0, &num, UINT64_MAX));
    tassert_eq(num, 9223372036854775807L);

    num = 0;
    s = "18446744073709551615";
    tassert_er(Error.ok, cex_str__to_unsigned_num(s, 0, &num, UINT64_MAX));
    tassert(num == UINT64_C(18446744073709551615));

    num = 0;
    s = "18446744073709551616";
    tassert_er(Error.overflow, cex_str__to_unsigned_num(s, 0, &num, UINT64_MAX));

    return EOK;
}


test$case(test_str_to_u8)
{
    u8 num;
    char* s;

    num = 0;
    s = "255";
    tassert_er(EOK, str.convert.to_u8(s, &num));
    tassert_eq(num, 255);
    tassert(num == UINT8_MAX);

    num = 0;
    s = "256";
    tassert_er(Error.overflow, str.convert.to_u8(s, &num));
    tassert_eq(num, 0);

    num = 0;
    s = "0";
    tassert_er(Error.ok, str.convert.to_u8(s, &num));
    tassert_eq(num, 0);

    num = 0;
    s = "0";
    tassert_er(Error.ok, str$convert(s, &num));
    tassert_eq(num, 0);

    return EOK;
}


test$case(test_str_to_u16)
{
    u16 num;
    char* s;

    num = 0;
    s = "65535";
    tassert_er(EOK, str.convert.to_u16(s, &num));
    tassert_eq(num, 65535);
    tassert(num == UINT16_MAX);

    num = 0;
    s = "65536";
    tassert_er(Error.overflow, str.convert.to_u16(s, &num));
    tassert_eq(num, 0);

    num = 0;
    s = "0";
    tassert_er(Error.ok, str.convert.to_u16(s, &num));
    tassert_eq(num, 0);


    return EOK;
}


test$case(test_str_to_u32)
{
    u32 num;
    char* s;

    num = 0;
    s = "4294967295";
    tassert_er(EOK, str.convert.to_u32(s, &num));
    tassert_eq(num, 4294967295U);
    tassert(num == UINT32_MAX);

    num = 0;
    s = "4294967296";
    tassert_er(Error.overflow, str.convert.to_u32(s, &num));
    tassert_eq(num, 0);

    num = 0;
    s = "0";
    tassert_er(Error.ok, str.convert.to_u32(s, &num));
    tassert_eq(num, 0);


    return EOK;
}

test$case(test_str_to_u64)
{
    u64 num;
    char* s;

    num = 0;
    s = "18446744073709551615";
    tassert_er(EOK, str.convert.to_u64(s, &num));
    tassert(num == UINT64_C(18446744073709551615));
    tassert(num == UINT64_MAX);

    num = 0;
    s = "18446744073709551616";
    tassert_er(Error.overflow, str.convert.to_u64(s, &num));
    tassert_eq(num, 0);

    num = 0;
    s = "0";
    tassert_er(Error.ok, str.convert.to_u64(s, &num));
    tassert_eq(num, 0);


    return EOK;
}
test$case(str_to__double)
{
    double num;
    char* s;

    num = 0;
    s = "1";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 1.0);

    num = 0;
    s = "1.5";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 1.5);

    num = 0;
    s = "-1.5";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, -1.5);

    num = 0;
    s = "1e3";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 1000);

    num = 0;
    s = "1e-3";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 1e-3);

    num = 0;
    s = "123e-30";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 123e-30);

    num = 0;
    s = "123e+30";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 123e+30);

    num = 0;
    s = "123.321E+30";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 123.321e+30);

    num = 0;
    s = "-0.";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, -0.0);

    num = 0;
    s = "+.5";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 0.5);

    num = 0;
    s = ".0e10";
    tassert_er(EOK, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 0.0);

    num = 0;
    s = ".";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 0.0);

    num = 0;
    s = ".e10";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 0.0);

    num = 0;
    s = "00000000001.e00000010";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 10000000000.0);

    num = 0;
    s = "e10";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "10e";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "10e0.3";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "10a";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "10.0a";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "10.0e-a";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "      10.5     ";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, 10.5);

    num = 0;
    s = "      10.5     z";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "n";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "na";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "nan";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, NAN);

    num = 0;
    s = "   NAN    ";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, NAN);

    num = 0;
    s = "   NaN    ";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, NAN);

    num = 0;
    s = "   nan    y";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "   nanny";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "inf";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, INFINITY);

    num = 0;
    s = "INF";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, INFINITY);
    tassert(isinf(num));

    num = 0;
    s = "-iNf";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, -INFINITY);
    tassert(isinf(num));

    num = 0;
    s = "-infinity";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, -INFINITY);
    tassert(isinf(num));

    num = 0;
    s = "   INFINITY   ";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));
    tassert_eq(num, INFINITY);
    tassert(isinf(num));

    num = 0;
    s = "INFINITY0";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "INFO";
    tassert_er(Error.argument, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "1e100";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "1e101";
    tassert_er(Error.overflow, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "1e-100";
    tassert_er(Error.ok, cex_str__to_double(s, 0, &num, -100, 100));

    num = 0;
    s = "1e-101";
    tassert_er(Error.overflow, cex_str__to_double(s, 0, &num, -100, 100));

    return EOK;
}
test$case(test_str_to_f32)
{
    f32 num;
    char* s;

    num = 0;
    s = "1.4";
    tassert_er(EOK, str.convert.to_f32(s, &num));
    tassert_eq(num, 1.4f);


    num = 0;
    s = "nan";
    tassert_er(EOK, str.convert.to_f32(s, &num));
    tassert_eq(num, NAN);

    num = 0;
    s = "1e38";
    tassert_er(EOK, str.convert.to_f32(s, &num));

    num = 0;
    s = "1e39";
    tassert_er(Error.overflow, str.convert.to_f32(s, &num));


    num = 0;
    s = "1e-37";
    tassert_er(EOK, str.convert.to_f32(s, &num));

    num = 0;
    s = "1e-38";
    tassert_er(Error.overflow, str.convert.to_f32(s, &num));


    return EOK;
}

test$case(test_str_to_f64)
{
    f64 num;
    char* s;

    num = 0;
    s = "1.4";
    tassert_er(EOK, str.convert.to_f64(s, &num));
    tassert_eq(num, 1.4);


    num = 0;
    s = "nan";
    tassert_er(EOK, str.convert.to_f64(s, &num));
    tassert_eq(num, NAN);

    num = 0;
    s = "1e308";
    tassert_er(EOK, str.convert.to_f64(s, &num));

    num = 0;
    s = "1e309";
    tassert_er(Error.overflow, str.convert.to_f64(s, &num));


    num = 0;
    s = "1e-307";
    tassert_er(EOK, str.convert.to_f64(s, &num));

    num = 0;
    s = "1e-308";
    tassert_er(Error.overflow, str.convert.to_f64(s, &num));


    return EOK;
}

test$case(test_str_sprintf)
{
    char buffer[10] = { 0 };

    memset(buffer, 'z', sizeof(buffer));
    tassert_er(EOK, str.sprintf(buffer, sizeof(buffer), "%s", "1234"));
    tassert_eq("1234", buffer);
    tassert_eq('\0', buffer[arr$len(buffer) - 1]);


    memset(buffer, 'z', sizeof(buffer));
    tassert_er(EOK, str.sprintf(buffer, sizeof(buffer), "%s", "123456789"));
    tassert_eq("123456789", buffer);
    tassert_eq('\0', buffer[arr$len(buffer) - 1]);

    memset(buffer, 'z', sizeof(buffer));
    tassert_er(Error.overflow, str.sprintf(buffer, sizeof(buffer), "%s", "1234567890"));
    tassert_eq("123456789", buffer);
    tassert_eq('\0', buffer[arr$len(buffer) - 1]);

    memset(buffer, 'z', sizeof(buffer));
    tassert_er(Error.overflow, str.sprintf(buffer, sizeof(buffer), "%s", "12345678900129830128308"));
    tassert_eq("123456789", buffer);
    tassert_eq('\0', buffer[arr$len(buffer) - 1]);

    memset(buffer, 'z', sizeof(buffer));
    tassert_er(EOK, str.sprintf(buffer, sizeof(buffer), "%s", ""));
    tassert_eq("", buffer);
    tassert_eq('\0', buffer[arr$len(buffer) - 1]);


    memset(buffer, 'z', sizeof(buffer));
    tassert_er(Error.argument, str.sprintf(NULL, sizeof(buffer), "%s", ""));
    tassert_eq(buffer[0], 'z'); // untouched!

    memset(buffer, 'z', sizeof(buffer));
    tassert_er(Error.argument, str.sprintf(buffer, 0, "%s", ""));
    tassert_eq(buffer[0], 'z'); // untouched!

    return EOK;
}

test$case(test_s_macros)
{

    str_s s = str.sstr("123456");
    tassert_eq(s.len, 6);
    tassert(s.buf);

    str_s s2 = str$s("fofo");
    tassert_eq(s2.len, 4);
    tassert(s2.buf);

    s2 = str$s("");
    tassert(s2.buf);
    tassert_eq(s2.len, 0);
    tassert_eq(str.slice.eq(s2, str$s("")), 1);

    str_s m = str$s("\
foo\n\
bar\n\
zoo\n");

    tassert(m.buf);
    tassert_eq("foo\nbar\nzoo\n", m.buf);

    return EOK;
}

test$case(test_fmt)
{
    mem$scope(tmem$, _)
    {
        // loading this test!
        char* fcont = io.file.load(__FILE__, _);
        str_s fcontent = str.sstr(fcont);
        tassert(str.len(fcont) > CEX_SPRINTF_MIN * 2);

        // NOTE: even smalls strings were allocated on mem$
        char* s = str.fmt(mem$, "%s", "foo");
        tassert(s != NULL);
        tassert_eq(3, strlen(s));
        tassert_eq("foo", s);
        mem$free(mem$, s);

        // NOTE: %S format for slice is limited to 65k chars, using this trick to overcome it
        char* s2 = str.fmt(mem$, "%.*S", fcontent.len, fcontent);
        tassert(s2 != NULL);
        tassert_eq(strlen(s2), fcontent.len);
        for (u32 i = 0; i < fcontent.len; i++) {
            tassertf(
                s2[i] == fcontent.buf[i],
                "i=%d s2['%c'] != fcontent['%c']",
                i,
                s2[i],
                fcontent.buf[i]
            );
        }
        mem$free(mem$, s2);
    }

    return EOK;
}


test$case(test_fmt_edge)
{
    mem$scope(tmem$, _)
    {
        // loading this test!
        char* fcont = io.file.load(__FILE__, _);
        str_s fcontent = str.sstr(fcont);
        tassert(str.len(fcont) > CEX_SPRINTF_MIN * 2);

        char* s = str.fmt(mem$, "%s", "foo");
        tassert_eq("foo", s);
        mem$free(mem$, s);

        str_s s2 = str.sstr(str.fmt(mem$, "%S", str.slice.sub(fcontent, 0, CEX_SPRINTF_MIN - 1)));
        tassert(s2.buf);
        tassert_eq(s2.len, CEX_SPRINTF_MIN - 1);
        for (u32 i = 0; i < s2.len; i++) {
            tassertf(
                s2.buf[i] == fcontent.buf[i],
                "i=%d s2['%c'] != fcontent['%c']",
                i,
                s2.buf[i],
                fcontent.buf[i]
            );
        }
        tassert_eq(s2.buf[s2.len], '\0');
        mem$free(mem$, s2.buf);

        s2 = str.sstr(str.fmt(mem$, "%S", str.slice.sub(fcontent, 0, CEX_SPRINTF_MIN)));
        tassert(s2.buf);
        tassert_eq(s2.len, CEX_SPRINTF_MIN);
        for (u32 i = 0; i < s2.len; i++) {
            tassertf(
                s2.buf[i] == fcontent.buf[i],
                "i=%d s2['%c'] != fcontent['%c']",
                i,
                s2.buf[i],
                fcontent.buf[i]
            );
        }
        tassert_eq(s2.buf[s2.len], '\0');
        mem$free(mem$, s2.buf);

        s2 = str.sstr(str.fmt(mem$, "%S", str.slice.sub(fcontent, 0, CEX_SPRINTF_MIN + 1)));
        tassert(s2.buf);
        tassert_eq(s2.len, CEX_SPRINTF_MIN + 1);
        for (u32 i = 0; i < s2.len; i++) {
            tassertf(
                s2.buf[i] == fcontent.buf[i],
                "i=%d s2['%c'] != fcontent['%c']",
                i,
                s2.buf[i],
                fcontent.buf[i]
            );
        }
        tassert_eq(s2.buf[s2.len], '\0');
        mem$free(mem$, s2.buf);
    }

    return EOK;
}

test$case(test_slice_clone)
{
    mem$scope(tmem$, _)
    {
        char* fcont = io.file.load(__FILE__, _);
        str_s fcontent = str.sstr(fcont);
        tassert(str.len(fcont) > CEX_SPRINTF_MIN * 2);

        var snew = str.slice.clone(fcontent, _);
        tassert(snew != NULL);
        tassert(snew != fcontent.buf);
        tassert_eq(strlen(snew), fcontent.len);
        tassert(0 == strcmp(snew, fcontent.buf));
        tassert_eq(snew[fcontent.len], 0);
        mem$free(tmem$, snew); // assert if snew doesn't belong to tmem$

        snew = str.slice.clone((str_s){ 0 }, _);
        tassert(snew == NULL);
    }

    return EOK;
}

test$case(test_clone)
{
    mem$scope(tmem$, _)
    {

        char* foo = "foo";
        var snew = str.clone(foo, _);
        tassert(snew != NULL);
        tassert(snew != foo);
        tassert_eq(strlen(snew), strlen(foo));
        tassert(0 == strcmp(snew, foo));
        tassert_eq(snew[strlen(snew)], 0);
        mem$free(tmem$, snew); // assert if snew doesn't belong to tmem$

        snew = str.clone(NULL, _);
        tassert(snew == NULL);
    }


    return EOK;
}

test$case(test_tsplit)
{
    mem$scope(tmem$, _)
    {

        arr$(char*) res = str.split(NULL, ",", _);
        tassert(res == NULL);
        res = str.split("foo", NULL, _);
        tassert(res == NULL);

        str_s s = str.sstr("123456");
        u32 nit = 0;
        char* expected1[] = {
            "123456",
        };
        res = str.split(s.buf, ",", _);
        tassert_eq(arr$len(res), 1);

        for$each(v, res)
        {
            tassert_eq(strcmp(v, expected1[nit]), 0);
            nit++;
        }
    }

    mem$scope(tmem$, _)
    {
        u32 nit = 0;
        var s = str.sstr("123,456");
        char* expected[] = {
            "123",
            "456",
        };
        arr$(char*) res = str.split(s.buf, ",", _);
        tassert(res != NULL);
        tassert_eq(arr$len(res), 2);

        for$each(v, res)
        {
            tassert_eq(strcmp(v, expected[nit]), 0);
            nit++;
        }
    }

    mem$scope(tmem$, _)
    {
        u32 nit = 0;
        var s = str.sstr("123,456, 789");
        char* expected[] = {
            "123",
            "456",
            " 789",
        };
        arr$(char*) res = str.split(s.buf, ",", _);
        tassert(res != NULL);
        tassert_eq(arr$len(res), 3);

        for$each(v, res)
        {
            tassert_eq(v, expected[nit]);
            nit++;
        }
    }

    mem$scope(tmem$, _)
    {
        u32 nit = 0;
        var s = str.sstr("123,456, 789,");
        char* expected[] = {
            "123",
            "456",
            " 789",
            "",
        };
        arr$(char*) res = str.split(s.buf, ",", _);
        tassert(res != NULL);
        tassert_eq(arr$len(res), 4);

        for$each(v, res)
        {
            tassert_eq(v, expected[nit]);
            nit++;
        }
    }

    return EOK;
}
test$case(test_split_lines)
{
    mem$scope(tmem$, _)
    {
        var s = str.sstr("123\n456\r\n789\r");
        char* expected[] = {
            "123",
            "456",
            "789",
        };
        arr$(char*) res = str.split_lines(s.buf, _);
        tassert(res != NULL);
        tassert_eq(arr$len(res), 3);

        u32 nit = 0;
        for$each(v, res)
        {
            tassert_eq(v, expected[nit]);
            nit++;
        }
    }

    mem$scope(tmem$, _)
    {
        var s = str.sstr("ab c\n\nde fg\rkl\r\nfff\fvvv\v");
        char* expected[] = {
            "ab c", "", "de fg", "kl", "fff", "vvv",
        };
        arr$(char*) res = str.split_lines(s.buf, _);
        tassert(res != NULL);
        tassert_eq(arr$len(res), 6);
        tassert_eq(arr$len(res), arr$len(expected));

        u32 nit = 0;
        for$each(v, res)
        {
            tassert_eq(v, expected[nit]);
            nit++;
        }
    }

    return EOK;
}

test$case(test_str_replace)
{
    char* s = "123123123";
    mem$scope(tmem$, _)
    {
        tassert_eq("456456456", str.replace(s, "123", "456", _));
        tassert_eq("787878", str.replace("456456456", "456", "78", _));
        tassert_eq("321321321", str.replace("787878", "78", "321", _));
        tassert_eq("111", str.replace("321321321", "32", "", _));
        tassert_eq("223223223", str.replace(s, "1", "2", _));
        tassert_eq("", str.replace("2222", "2", "", _));
        tassert_eq("1111", str.replace("1111", "2", "", _));
        tassert_eq("", str.replace("", "2", "", _));
        tassert_eq(str.replace("11111", "", "", _), NULL);
        tassert_eq(str.replace(NULL, "foo", "bar", _), NULL);
        tassert_eq(str.replace("baz", NULL, "bar", _), NULL);
        tassert_eq(str.replace("baz", "foo", NULL, _), NULL);
    }

    return EOK;
}

test$case(test_tjoin)
{
    mem$scope(tmem$, _)
    {

        arr$(char*) res = arr$new(res, _);
        tassert(res != NULL);
        arr$pushm(res, "foo");

        char* joined = str.join(res, arr$len(res), ",", _);
        tassert(joined != NULL);
        tassert(joined != res[0]); // new memory allocated
        tassert_eq(joined, "foo");

        arr$pushm(res, "bar");
        joined = str.join(res, arr$len(res), ", ", _);
        tassert(joined != NULL);
        tassert(joined != res[0]); // new memory allocated
        tassert_eq(joined, "foo, bar");

        arr$pushm(res, "baz");
        joined = str.join(res, arr$len(res), ", ", _);
        tassert(joined != NULL);
        tassert(joined != res[0]); // new memory allocated
        tassert_eq(joined, "foo, bar, baz");

        char* s = "3";
        char s2[] = "4";
        joined = str$join(_, ", ", "1", "2", s, s2);
        tassert(joined != NULL);
        tassert_eq(joined, "1, 2, 3, 4");
    }

    return EOK;
}

test$case(test_str_chaining)
{
    mem$scope(tmem$, _)
    {
        char* s = str.fmt(_, "hi there");
        s = str.replace(s, "hi", "hello", _);
        s = str.fmt(_, "result is: %s", s);
        tassert_eq(s, "result is: hello there");
    }

    return EOK;
}

test$case(test_str_tolower)
{
    char* s = str.lower("ABCDEFGHIJKLMNOPQRSTUVWXYZ123!@#тест", mem$);
    tassert(s);
    tassert_eq(s, "abcdefghijklmnopqrstuvwxyz123!@#тест");
    mem$free(mem$, s);

    s = str.lower(NULL, mem$);
    tassert(s == NULL);

    return EOK;
}

test$case(test_str_toupper)
{
    char* s = str.upper("abcdefghijklmnopqrstuvwxyz123!@#тест", mem$);
    tassert(s);
    tassert_eq(s, "ABCDEFGHIJKLMNOPQRSTUVWXYZ123!@#тест");
    mem$free(mem$, s);

    s = str.upper(NULL, mem$);
    tassert(s == NULL);
    return EOK;
}

test$case(test_str_slice_eq)
{
    tassert(str.slice.eq(str.sstr("hello"), str$s("hello")));
    tassert(!str.slice.eq(str.sstr("hello"), str$s("hello ")));
    tassert(!str.slice.eq(str.sstr("hello "), str$s("hello")));
    tassert(str.slice.eq(str.sstr(""), str$s("")));
    tassert(!str.slice.eq(str.sstr(""), str$s(" ")));
    tassert(!str.slice.eq(str.sstr(NULL), str$s(" ")));
    tassert(!str.slice.eq(str.sstr(NULL), str$s(" ")));
    tassert(str.slice.eq(str.sstr(NULL), str.sstr(NULL)));

    return EOK;
}

test$case(test_str_match)
{
    uassert_disable();
    tassert(str.match("test", "*"));
    tassert(!str.match("", "*"));
    tassert(!str.match(NULL, "*"));
    tassert(str.match(".txt", "*.txt"));
    tassert(!str.match("test.txt", ""));
    tassert(str.match("test.txt", "*txt"));
    tassert(!str.match("test.txt", "*.tx"));
    tassert(str.match("test.txt", "*.txt"));
    tassert(str.match("test.txt", "*xt"));
    tassert(str.match("txt", "???"));
    tassert(str.match("txta", "???a"));
    tassert(!str.match("txt", "???a"));
    tassert(!str.match("txt", "??"));
    tassert(str.match("t", "?"));
    tassert(str.match("t", "*"));
    tassert(str.match("test1.txt", "test?.txt"));
    tassert(str.match("test.txt", "*.*"));
    tassert(str.match("test1.txt", "t*t?.txt"));
    tassert(str.match("test.txt", "*?txt"));
    tassert(str.match("test.txt", "*.???"));
    tassert(str.match("test.txt", "*?t?*"));
    tassert(str.match("a*b", "a\\*b"));
    tassert(str.match("backup.txt", "[!a]*.txt"));
    tassert(str.match("image.png", "image.[jp][pn]g"));
    tassert(!str.match("a", "[]"));
    tassert(str.match("a", "[!]"));
    tassert(!str.match("a", "[!"));
    tassert(!str.match("a", "["));
    tassert(str.match("a", "[ab]"));
    tassert(str.match("b", "[ab]"));
    tassert(str.match("b", "[abc]"));
    tassert(str.match("c", "[abc]"));
    tassert(str.match("a", "[a-c]"));
    tassert(str.match("b", "[a-c]"));
    tassert(str.match("c", "[a-c]"));
    tassert(str.match("A", "[a-cA-C]"));
    tassert(str.match("0", "[a-cA-C0-9]"));
    tassert(!str.match("D", "[a-cA-C0-9]"));
    tassert(!str.match("d", "[a-cA-C1-9]"));
    tassert(!str.match("0", "[a-cA-C1-9]"));
    tassert(str.match("_", "[_a-cd]"));
    tassert(str.match("a", "[_a-cd]"));
    tassert(str.match("b", "[_a-cd]"));
    tassert(str.match("c", "[_a-cd]"));
    tassert(str.match("d", "[_a-cd]"));
    tassert(str.match("_", "[a-c_A-C1-9]"));
    tassert(str.match("-", "[a-z-]"));
    tassert(str.match("*", "[a-c*]"));
    tassert(str.match("?", "[?]"));
    tassert(str.match("-", "[?-]"));
    tassert(str.match("*", "[*-]"));
    tassert(str.match(")", "[)]"));
    tassert(str.match("(", "[(]"));
    tassert(str.match("e$*", "*[*]"));
    tassert(!str.match("d", "[a-c*]")); // * - is literal
    tassert(str.match("*", "[*a-z]"));
    tassert(str.match("*", "[a-z\\*]"));
    tassert(str.match("]", "[\\]]"));
    tassert(str.match("[", "[\\[]"));
    tassert(str.match("\\", "[\\\\]"));
    tassert(!str.match("#abc=", "[a-c+]="));
    tassert(!str.match("#abc=", "[a-c+]*=*"));
    tassert(str.match("abc  =", "[a-c +]*=*"));
    tassert(str.match("abc=", "[a-c +]=*"));
    tassert(str.match("abc", "[a-c+]"));
    tassert(!str.match("zabc", "[a-c+]"));
    tassert(str.match("abcfed", "[a-c+]*"));
    tassert(str.match("abc@", "[a-c+]@"));
    tassert(str.match("abdef", "[a-c+][d-f+]"));
    tassert(!str.match("abcf", "[a-c+]"));
    tassert(str.match("abc+", "[+a-c+]"));
    tassert(str.match("abcf", "[a-c+]?"));
    tassert(!str.match("", "[a-c+]"));
    tassert(str.match("abcd", "[!d-f+]?"));
    tassert(str.match("abcf", "[!d-f+]?"));
    tassert(!str.match("abcff", "[!d-f+]?"));
    tassert(str.match("1234567890abcdefABCDEF", "[0-9a-fA-F+]"));
    tassert(str.match("f5fca082882b848dd28c470c4d1f111c995cb7bd", "[0-9a-fA-F+]"));
    tassert(str.slice.match(str$s("f5fca082882b848dd28c470c4d1f111c995cb7bd"), "[0-9a-fA-F+]"));

    tassert(!str.match("abc", "(\\"));
    tassert(str.match("abc", "(abc)"));
    tassert(str.match("def", "(abc|def)"));
    tassert(str.match("abcdef", "abc(abc|def)"));
    tassert(str.match("abXdef", "ab?(abc|def)"));
    tassert(str.match("abcdef", "(abc|def)(abc|def)"));
    tassert(str.match("ldkjdef", "*(abc|def)"));
    tassert(!str.match("adef", "(cdef)"));
    tassert(!str.match("bdef", "(cdef)"));
    tassert(!str.match("f", "(cdef)"));
    tassert(!str.match("f", "(cdef)"));
    tassert(!str.match("fas", "(fa)"));
    tassert(!str.match("fas", "(fa)"));
    tassert(str.match("fa@", "(fa)@"));
    tassert(str.match("faB", "(fa)B"));
    tassert(str.match("faZ", "(fa)?"));
    tassert(str.match("faZ", "(fa)*"));
    tassert(str.match("faZ", "(fa)[A-Z]"));
    tassert(!str.match("fas", "(fa|)"));
    tassert(str.match("fa", "(fa|)"));
    tassert(str.match("fa", "(|fa)"));
    tassert(!str.match("fa", "(|)"));
    tassert(!str.match("fa", "()"));
    tassert(!str.match("fa", "fa()"));
    tassert(str.match("fa)", "(fa\\))"));
    tassert(str.match("fa|", "(fa\\|)"));
    tassert(str.match("fa\\", "(fa\\\\)"));

    // NOTE: \\+ - makes pattern non repeating it will test only 'a' and fail
    tassert(!str.match("abc+", "[a-c\\+]"));
    tassert(str.match("a+", "a[a-c\\+]"));

    tassert(str.match("clean", "(clean)"));
    tassert(str.match("run", "(run|clean)"));
    tassert(str.match("build", "(run|build|clean)"));
    tassert(str.match("create", "(run|build|create|clean)"));
    tassert(str.match("build", "(run|build|create|clean)"));
    tassert(str.match("foo", "(run|build|create|clean|foo)"));
    tassert(str.match("clean", "(run|build|create|clean)"));
    tassert(str.match("clean", "(run|build|cleany|clean)"));
    return EOK;
}

test$case(test_str_slice_match)
{
    str_s src = str$s("my_test __String.txt");
    tassert(str.slice.match(src, "*"));
    tassert(str.slice.match(src, "*.txt"));
    tassert(str.slice.match(src, "*.txt*"));
    tassert(str.slice.match(src, "my_test*.txt"));
    tassert(str.slice.match(src, "my_test* *.txt"));
    tassert(!str.slice.match(str.slice.sub(src, 1, 0), "my_test* *.txt"));
    tassert(!str.slice.match(str.slice.sub(src, 0, 3), "my_t"));
    tassert(str.slice.match(str.slice.sub(src, 0, 4), "my_t"));
    tassert(str.slice.match(str.slice.sub(src, 0, 4), "my_t*"));
    tassert(str.slice.match(str.slice.sub(src, 0, 4), "my_?"));
    tassert(!str.slice.match(str.slice.sub(src, 0, 4), "my_t?"));

    str_s hex_slice = str$s("abcdef ");
    tassert_eq(str.slice.strip(hex_slice), str$s("abcdef"));

    tassert(str.slice.match(str.slice.strip(hex_slice), "*def"));
    tassert(str.slice.match(str$s("1234567890abcdef"), "*(def)"));
    tassert(str.slice.match(str.slice.strip(hex_slice), "*(def)"));
    tassert(str.slice.match(str.slice.strip(hex_slice), "[0-9a-fA-F+]"));
    tassert(str.slice.match(str.slice.strip(hex_slice), "abcdef"));
    tassert(str.slice.match(str.slice.strip(hex_slice), "abcde?"));
    tassert(str.slice.match(str.slice.strip(hex_slice), "abc(def)"));

    return EOK;
}

test$case(test_slice_index_of)
{

    str_s s = str$s("1234");
    tassert_eq(-1, str.slice.index_of(s, (str_s){0}));
    tassert_eq(-1, str.slice.index_of(s, (str_s){0, "sd"}));
    tassert_eq(-1, str.slice.index_of(s, (str_s){0, "sd"}));

    tassert_eq(-1, str.slice.index_of((str_s){0}, str$s("foo")));
    tassert_eq(-1, str.slice.index_of((str_s){0, "sd"}, str$s("foo")));
    tassert_eq(-1, str.slice.index_of((str_s){0, "sd"}, str$s("foo")));
    tassert_eq(-1, str.slice.index_of(str$s("fo"), str$s("foo")));

    tassert_eq(-1, str.slice.index_of(s, str$s("")));
    tassert_eq(0, str.slice.index_of(s, str$s("1")));
    tassert_eq(2, str.slice.index_of(s, str$s("3")));
    tassert_eq(3, str.slice.index_of(s, str$s("4")));
    tassert_eq(-1, str.slice.index_of(s, str$s("5")));
    

    tassert_eq(-1, str.slice.index_of(s, str$s("12345")));
    tassert_eq(1, str.slice.index_of(s, str$s("234")));
    tassert_eq(2, str.slice.index_of(s, str$s("34")));
    tassert_eq(0, str.slice.index_of(s, str$s("1234")));

    return EOK;
}
test$case(test_cmp_str)
{
    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "2", "1", "", "5");
        char* expected[] = {"", "1", "2", "5"};
        arr$sort(items, str.qscmp);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "2", NULL, "", "5");
        char* expected[] = {"", "2", "5", NULL};
        arr$sort(items, str.qscmp);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "2", "1", "", "");
        char* expected[] = {"", "", "1", "2"};
        arr$sort(items, str.qscmp);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "2", NULL, "", NULL, "foo");
        char* expected[] = {"", "2", "foo", NULL, NULL};
        arr$sort(items, str.qscmp);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "A", "1", "a", "Z", "z",);
        char* expected[] = {"1", "A", "Z", "a", "z"};
        arr$sort(items, str.qscmp);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "aaaAA", "aaaA", "aaaAAc");
        char* expected[] = {"aaaA", "aaaAA", "aaaAAc"};
        arr$sort(items, str.qscmp);
        tassert_eq_arr(items, expected);
    }

    return EOK;
}

test$case(test_cmp_str_ignore_case)
{
    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "2", "1", "", "5");
        char* expected[] = {"", "1", "2", "5"};
        arr$sort(items, str.qscmpi);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "2", NULL, "", "5");
        char* expected[] = {"", "2", "5", NULL};
        arr$sort(items, str.qscmpi);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "2", NULL, "", NULL, "foo");
        char* expected[] = {"", "2", "foo", NULL, NULL};
        arr$sort(items, str.qscmpi);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(char*) items = arr$new(items, _); 
        arr$pushm(items, "A", "1", "a", "Z", "z",);
        // char* expected[] = {"1", "A", "a", "Z", "z"};
        arr$sort(items, str.qscmpi);
        tassert_eq(items[0], "1");
        tassert(str.eqi(items[1], "a"));
        tassert(str.eqi(items[2], "a"));
        tassert(str.eqi(items[3], "z"));
        tassert(str.eqi(items[4], "z"));
    }
    return EOK;
}

test$case(test_cmp_slice)
{
    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, str$s("2"), str$s("1"), str$s(""), str$s("5"));
        str_s expected[] = {str$s(""), str$s("1"), str$s("2"), str$s("5")};
        arr$sort(items, str.slice.qscmp);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, {0}, str$s("1"), str$s(""), str$s("5"));
        str_s expected[] = {str$s(""), str$s("1"), str$s("5"), {0}};
        arr$sort(items, str.slice.qscmp);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, {0}, str$s("1"), {0}, str$s("5"));
        str_s expected[] = {str$s("1"), str$s("5"), {0}, {0}};
        arr$sort(items, str.slice.qscmp);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, str$s("A"), str$s("1"), str$s("z"), str$s("a"), str$s("Z"));
        str_s expected[] = {str$s("1"), str$s("A"), str$s("Z"), str$s("a"), str$s("z")};
        arr$sort(items, str.slice.qscmp);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, str$s("aaaAA"), str$s("aaaA"), str$s("aaaAAc"));
        str_s expected[] = {str$s("aaaA"), str$s("aaaAA"), str$s("aaaAAc")};
        arr$sort(items, str.slice.qscmp);
        tassert_eq_arr(items, expected);
    }
    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, str$s(""), str$s("1"), str$s("z"), str$s(""), str$s(""));
        str_s expected[] = {str$s(""), str$s(""), str$s(""), str$s("1"), str$s("z")};
        arr$sort(items, str.slice.qscmp);
        tassert_eq_arr(items, expected);
    }
    return EOK;
}

test$case(test_cmp_slice_ignore_case)
{
    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, str$s("2"), str$s("1"), str$s(""), str$s("5"));
        str_s expected[] = {str$s(""), str$s("1"), str$s("2"), str$s("5")};
        arr$sort(items, str.slice.qscmpi);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, {0}, str$s("1"), str$s(""), str$s("5"));
        str_s expected[] = {str$s(""), str$s("1"), str$s("5"), {0}};
        arr$sort(items, str.slice.qscmpi);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, {0}, str$s("1"), {0}, str$s("5"));
        str_s expected[] = {str$s("1"), str$s("5"), {0}, {0}};
        arr$sort(items, str.slice.qscmpi);
        tassert_eq_arr(items, expected);
    }

    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, str$s("A"), str$s("1"), str$s("z"), str$s("a"), str$s("Z"));
        // str_s expected[] = {str$s("1"), str$s("A"), str$s("a"), str$s("z"), str$s("Z")};
        arr$sort(items, str.slice.qscmpi);

        tassert(str.slice.eqi(items[0], str$s("1")));
        tassert(str.slice.eqi(items[1], str$s("a")));
        tassert(str.slice.eqi(items[2], str$s("a")));
        tassert(str.slice.eqi(items[3], str$s("z")));
        tassert(str.slice.eqi(items[4], str$s("z")));
    }

    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, str$s("aaaAA"), str$s("aaaA"), str$s("aaaAAc"));
        str_s expected[] = {str$s("aaaA"), str$s("aaaAA"), str$s("aaaAAc")};
        arr$sort(items, str.slice.qscmpi);
        tassert_eq_arr(items, expected);
    }
    mem$scope(tmem$, _) {
        arr$(str_s) items = arr$new(items, _); 
        arr$pushm(items, str$s(""), str$s("1"), str$s("z"), str$s(""), str$s(""));
        str_s expected[] = {str$s(""), str$s(""), str$s(""), str$s("1"), str$s("z")};
        arr$sort(items, str.slice.qscmpi);
        tassert_eq_arr(items, expected);
    }
    return EOK;
}

test$case(test_str_convert_macro_u8)
{
    char b[] = {"200"};
    u8 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, 200);
    return EOK;
}

test$case(test_str_convert_macro_i8)
{
    char b[] = {"-20"};
    i8 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, -20);
    return EOK;
}

test$case(test_str_convert_macro_u16)
{
    char b[] = {"200"};
    u16 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, 200);
    return EOK;
}

test$case(test_str_convert_macro_i16)
{
    char b[] = {"-20"};
    i16 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, -20);
    return EOK;
}


test$case(test_str_convert_macro_u32)
{
    char b[] = {"200"};
    u32 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, 200);
    return EOK;
}

test$case(test_str_convert_macro_i32)
{
    char b[] = {"-20"};
    i32 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, -20);
    return EOK;
}


test$case(test_str_convert_macro_u64)
{
    char b[] = {"200"};
    u64 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, 200);
    return EOK;
}

test$case(test_str_convert_macro_i64)
{
    char b[] = {"-20"};
    i64 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, -20);
    return EOK;
}

test$case(test_str_convert_macro_f32)
{
    char b[] = {"200"};
    f32 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, 200);
    return EOK;
}

test$case(test_str_convert_macro_f64)
{
    char b[] = {"-20"};
    i64 num = 0;
    tassert_er(EOK, str$convert("8", &num));
    tassert_eq(num, 8);
    tassert_er(EOK, str$convert(str$s("9"), &num));
    tassert_eq(num, 9);
    tassert_er(EOK, str$convert(b, &num));
    tassert_eq(num, -20);
    return EOK;
}

test$main();
