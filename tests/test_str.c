#include <cex/all.c>
#include <cex/all.h>
#include <cex/test.h>
#include <stdio.h>

/*
 * SUITE INIT / SHUTDOWN
 */
test$teardown()
{
    return EOK;
}

test$setup()
{
    uassert_enable();
    return EOK;
}

/*
 *
 *   TEST SUITE
 *
 */

test$case(test_cstr)
{
    const char* cstr = "hello";


    str_s s = str.sstr(cstr);
    tassert_eqs(s.buf, cstr);
    tassert_eqi(s.len, 5); // lazy init until str.length() is called
    tassert(s.buf == cstr);
    tassert_eqi(str.len(s.buf), 5);


    tassert_eqi(s.len, 5); // now s.len is set

    str_s snull = str.sstr(NULL);
    tassert_eqs(snull.buf, NULL);
    tassert_eqi(snull.len, 0);
    tassert_eqi(str.len(snull.buf), 0);

    str_s sempty = str.sstr("");
    tassert_eqs(sempty.buf, "");
    tassert_eqi(sempty.len, 0);
    tassert_eqi(str.len(sempty.buf), 0);
    tassert_eqi(sempty.len, 0);
    return EOK;
}

test$case(test_cstr_sdollar)
{
    const char* cstr = "hello";

    sbuf_c sb = sbuf.create(10, mem$);
    tassert_eqs(EOK, sbuf.append(&sb, "hello"));
    sbuf.destroy(&sb);

    str_s s2 = str$s("hello");
    tassert_eqs(s2.buf, cstr);
    tassert_eqi(s2.len, 5); // lazy init until str.length() is called
    tassert(s2.buf == cstr);


    char* str_null = NULL;
    str_s snull = str.sstr(str_null);
    tassert_eqs(snull.buf, NULL);
    tassert_eqi(snull.len, 0);

    str_s sempty = str$s("");
    tassert_eqs(sempty.buf, "");
    tassert_eqi(sempty.len, 0);
    tassert_eqi(sempty.len, 0);

    // // WARNING: buffers for str$s() are extremely bad idea, they may not be null term!
    char buf[30] = { 0 };
    var s = str.sstr(buf);
    tassert_eqs(s.buf, buf);
    tassert_eqi(s.len, 0); // lazy init until str.length() is called

    memset(buf, 'z', arr$len(buf));

    // WARNING: if no null term
    // s = str$s(buf); // !!!! STACK OVERFLOW, no null term!
    //
    // Consider using str.sbuf() - it limits length by buffer size
    s = str.sbuf(buf, arr$len(buf));
    tassert_eqi(s.len, 30);
    tassert(s.buf == buf);

    return EOK;
}

test$case(test_copy)
{
    char buf[8];
    memset(buf, 'a', arr$len(buf));


    memset(buf, 'a', arr$len(buf));
    tassert_eqs(EOK, str.copy(buf, "1234567", arr$len(buf)));
    tassert_eqs("1234567", buf);
    tassert_eqi(strlen(buf), 7);
    tassert_eqi(buf[7], '\0');

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.argument, str.copy(NULL, "1234567", arr$len(buf)));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0);
    tassert_eqs(Error.argument, str.copy(buf, "1234567", 0)); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.ok, str.copy(buf, "", arr$len(buf)));
    tassert_eqs("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.argument, str.copy(buf, NULL, arr$len(buf)));
    // buffer reset to "" string
    tassert_eqs("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.overflow, str.copy(buf, "12345678", arr$len(buf)));
    // string is truncated
    tassert_eqs("1234567", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.ok, str.copy(buf, "1234", arr$len(buf)));
    tassert_eqs("1234", buf);
    return EOK;
}

test$case(test_slice_copy)
{
    char buf[8];
    memset(buf, 'a', arr$len(buf));


    memset(buf, 'a', arr$len(buf));
    tassert_eqs(EOK, str.slice.copy(buf, str$s("1234567"), arr$len(buf)));
    tassert_eqs("1234567", buf);
    tassert_eqi(strlen(buf), 7);
    tassert_eqi(buf[7], '\0');

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.argument, str.slice.copy(NULL, str$s("1234567"), arr$len(buf)));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert(memcmp(buf, "aaaaaaaa", arr$len(buf)) == 0);
    tassert_eqs(Error.argument, str.slice.copy(buf, str$s("1234567"), 0)); // untouched

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.ok, str.slice.copy(buf, str$s(""), arr$len(buf)));
    tassert_eqs("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.argument, str.slice.copy(buf, (str_s){ 0 }, arr$len(buf)));
    // buffer reset to "" string
    tassert_eqs("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.overflow, str.slice.copy(buf, str$s("12345678"), arr$len(buf)));
    tassert_eqs("", buf);

    memset(buf, 'a', arr$len(buf));
    tassert_eqs(Error.ok, str.slice.copy(buf, str$s("1234"), arr$len(buf)));
    tassert_eqs("1234", buf);
    return EOK;
}

test$case(test_sub_positive_start)
{

    str_s sub;
    str_s s = str.sstr("123456");
    tassert_eqi(s.len, 6);

    sub = str.sub("123456", 0, s.len);
    tassert(sub.buf);
    tassert(memcmp(sub.buf, "123456", s.len) == 0);
    tassert_eqi(sub.len, 6);
    tassert_eqi(s.len, 6);


    sub = str.slice.sub(s, 1, 3);
    tassert(sub.buf);
    tassert_eqi(sub.len, 2);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "23", sub.len) == 0);

    sub = str.slice.sub(s, 1, 2);
    tassert(sub.buf);
    tassert_eqi(sub.len, 1);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "2", sub.len) == 0);

    sub = str.slice.sub(s, s.len - 1, s.len);
    tassert(sub.buf);
    tassert_eqi(sub.len, 1);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "6", sub.len) == 0);

    sub = str.slice.sub(s, 2, 0);
    tassert(sub.buf);
    tassert_eqi(sub.len, 4);
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
    tassert_eqi(sub.len, 0);
    tassert_eqs(NULL, sub.buf);

    sub = str.slice.sub(s, 1, 2);
    tassert(sub.buf == NULL);

    s = str.sstr("A");
    sub = str.slice.sub(s, 1, 2);
    tassert(sub.buf == NULL);

    s = str.sstr("A");
    sub = str.slice.sub(s, 0, 2);
    tassert(sub.buf);
    tassert_eqi(sub.len, 1);
    tassert(memcmp(sub.buf, "A", sub.len) == 0);

    s = str.sstr("A");
    sub = str.slice.sub(s, 0, 0);
    tassert(sub.buf);
    tassert(memcmp(sub.buf, "A", sub.len) == 0);
    tassert_eqi(sub.len, 1);
    return EOK;
}

test$case(test_sub_negative_start)
{

    str_s s = str.sstr("123456");
    tassert_eqi(s.len, 6);
    tassert(s.buf);

    str_s s_empty = { 0 };
    var sub = str.slice.sub(s_empty, 0, 2);
    tassert_eqi(sub.len, 0);
    tassert(sub.buf == NULL);
    tassert(sub.buf == NULL);

    sub = str.slice.sub(s, -3, -1);
    tassert(sub.buf);
    tassert_eqi(sub.len, 2);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "45", sub.len) == 0);

    sub = str.slice.sub(s, -6, 0);
    tassert(sub.buf);
    tassert_eqi(sub.len, 6);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);


    sub = str.slice.sub(s, -3, -2);
    tassert(sub.buf);
    tassert_eqi(sub.len, 1);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "4", sub.len) == 0);

    sub = str.slice.sub(s, -3, -1);
    tassert(sub.buf);
    tassert_eqi(sub.len, 2);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "45", sub.len) == 0);

    var slice = str$sslice(s, -3, -1);
    tassert_eqi(slice.len, 2);
    tassert(memcmp(slice.arr, "45", slice.len) == 0);

    var slice2 = str$sslice(s, .start = -3, .end = -1);
    tassert_eqi(slice2.len, 2);
    tassert(memcmp(slice2.arr, "45", slice2.len) == 0);

    sub = str.slice.sub(s, -3, -400);
    tassert(sub.buf == NULL);

    s = str.sstr("");
    sub = str.slice.sub(s, 0, 0);
    tassert(sub.buf == NULL);
    tassert_eqi(sub.len, 0);

    s = str.sstr("");
    sub = str.slice.sub(s, -1, 0);
    tassert(sub.buf == NULL);

    s = str.sstr("123456");
    sub = str.slice.sub(s, -6, 0);
    tassert(sub.buf);
    tassert_eqi(sub.len, 6);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = str.sstr("123456");
    sub = str.slice.sub(s, -6, 6);
    tassert(sub.buf);
    tassert_eqi(sub.len, 6);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);

    s = str.sstr("123456");
    sub = str.slice.sub(s, -7, 0);
    tassert(sub.buf);
    tassert_eqi(s.len, 6);
    tassert(memcmp(sub.buf, "123456", sub.len) == 0);

    return EOK;
}

test$case(test_eq)
{
    tassert_eqi(str.eq("", ""), 1);
    tassert_eqi(str.eq(NULL, ""), 0);
    tassert_eqi(str.eq("", NULL), 0);
    tassert_eqi(str.eq("1", "1"), 1);
    tassert_eqi(str.eq("123456", "123456"), 1);
    tassert_eqi(str.eq("123456", "12345"), 0);
    tassert_eqi(str.eq("12345", "123456"), 0);
    tassert_eqi(str.eq("12345", ""), 0);
    tassert_eqi(str.eq("", "123456"), 0);

    tassert_eqi(str.slice.eq(str$s(""), str$s("")), 1);
    tassert_eqi(str.slice.eq((str_s){ 0 }, str$s("")), 0);
    tassert_eqi(str.slice.eq(str$s(""), (str_s){ 0 }), 0);
    tassert_eqi(str.slice.eq(str$s("1"), str$s("1")), 1);
    tassert_eqi(str.slice.eq(str$s("123456"), str$s("123456")), 1);
    tassert_eqi(str.slice.eq(str$s("123456"), str$s("12345")), 0);
    tassert_eqi(str.slice.eq(str$s("12345"), str$s("123456")), 0);
    tassert_eqi(str.slice.eq(str$s("12345"), str$s("")), 0);
    tassert_eqi(str.slice.eq(str$s(""), str$s("123456")), 0);
    return EOK;
}

test$case(test_iter_split)
{

    str_s s = str.sstr("123456");
    u32 nit = 0;

    nit = 0;
    const char* expected1[] = {
        "123456",
    };
    for$iter(str_s, it, str.slice.iter_split(s, ",", &it.iterator))
    {
        tassert(it.val->buf);
        // tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.slice.cmp(*it.val, str.sstr(expected1[nit])), 0);
        nit++;
    }
    tassert_eqi(nit, 1);

    nit = 0;
    s = str.sstr("123,456");
    const char* expected2[] = {
        "123",
        "456",
    };
    for$iter(str_s, it, str.slice.iter_split(s, ",", &it.iterator))
    {
        tassert(it.val->buf);
        // tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.slice.cmp(*it.val, str.sstr(expected2[nit])), 0);
        nit++;
    }
    tassert_eqi(nit, 2);

    nit = 0;
    s = str.sstr("123,456,88,99");
    const char* expected3[] = {
        "123",
        "456",
        "88",
        "99",
    };
    for$iter(str_s, it, str.slice.iter_split(s, ",", &it.iterator))
    {
        tassert(it.val->buf);
        // tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.slice.cmp(*it.val, str.sstr(expected3[nit])), 0);
        nit++;
    }
    tassert_eqi(nit, 4);

    nit = 0;
    const char* expected4[] = {
        "123",
        "456",
        "88",
        "",
    };
    s = str.sstr("123,456,88,");
    for$iter(str_s, it, str.slice.iter_split(s, ",", &it.iterator))
    {
        tassert(it.val->buf);
        // tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.slice.cmp(*it.val, str.sstr(expected4[nit])), 0);
        tassert_eqi(it.idx.i, nit);
        nit++;
    }
    tassert_eqi(nit, 4);

    nit = 0;
    s = str.sstr("123,456#88@99");
    const char* expected5[] = {
        "123",
        "456",
        "88",
        "99",
    };
    for$iter(str_s, it, str.slice.iter_split(s, ",@#", &it.iterator))
    {
        tassert(it.val->buf);
        // tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.slice.cmp(*it.val, str.sstr(expected5[nit])), 0);
        tassert_eqi(it.idx.i, nit);
        nit++;
    }
    tassert_eqi(nit, 4);

    nit = 0;
    const char* expected6[] = {
        "123",
        "456",
        "",
    };
    s = str.sstr("123\n456\n");
    for$iter(str_s, it, str.slice.iter_split(s, "\n", &it.iterator))
    {
        tassert(it.val->buf);
        // tassert_eqs(Error.ok, str.copy(*it.val, buf, arr$len(buf)));
        tassert_eqi(str.slice.cmp(*it.val, str.sstr(expected6[nit])), 0);
        nit++;
    }
    tassert_eqi(nit, 3);
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
    tassert_eqi(1, str.starts_with(s, "1"));
    tassert_eqi(1, str.starts_with(s, "1234"));
    tassert_eqi(1, str.starts_with(s, "123456"));
    tassert_eqi(0, str.starts_with(s, "1234567"));
    tassert_eqi(0, str.starts_with(s, "234567"));
    tassert_eqi(0, str.starts_with(s, "56"));
    tassert_eqi(0, str.starts_with(s, ""));
    tassert_eqi(0, str.starts_with(s, NULL));
    tassert_eqi(0, str.starts_with("", "1"));
    tassert_eqi(0, str.starts_with(NULL, "1"));


    tassert_eqi(1, str.ends_with(s, "6"));
    tassert_eqi(1, str.ends_with(s, "123456"));
    tassert_eqi(1, str.ends_with(s, "56"));
    tassert_eqi(0, str.ends_with(s, "1234567"));
    tassert_eqi(0, str.ends_with(s, "5"));
    tassert_eqi(0, str.ends_with(s, ""));
    tassert_eqi(0, str.ends_with(s, NULL));
    tassert_eqi(0, str.ends_with("", "1"));
    tassert_eqi(0, str.ends_with(NULL, "1"));

    return EOK;
}


test$case(test_contains_sclice_starts_ends)
{
    str_s s = str$s("123456");
    tassert_eqi(1, str.slice.starts_with(s, str.sstr("1")));
    tassert_eqi(1, str.slice.starts_with(s, str.sstr("1234")));
    tassert_eqi(1, str.slice.starts_with(s, str.sstr("123456")));
    tassert_eqi(0, str.slice.starts_with(s, str.sstr("1234567")));
    tassert_eqi(0, str.slice.starts_with(s, str.sstr("234567")));
    tassert_eqi(0, str.slice.starts_with(s, str.sstr("56")));
    tassert_eqi(0, str.slice.starts_with(s, str.sstr("")));
    tassert_eqi(0, str.slice.starts_with(s, str.sstr(NULL)));
    tassert_eqi(0, str.slice.starts_with(str$s("1"), str$s("")));
    tassert_eqi(0, str.slice.starts_with((str_s){ 0 }, str$s("1")));


    tassert_eqi(1, str.slice.ends_with(s, str.sstr("6")));
    tassert_eqi(1, str.slice.ends_with(s, str.sstr("123456")));
    tassert_eqi(1, str.slice.ends_with(s, str.sstr("123456")));
    tassert_eqi(1, str.slice.ends_with(s, str.sstr("56")));
    tassert_eqi(0, str.slice.ends_with(s, str.sstr("1234567")));
    tassert_eqi(0, str.slice.ends_with(s, str.sstr("567")));
    tassert_eqi(0, str.slice.ends_with(s, str.sstr("")));
    tassert_eqi(0, str.slice.ends_with(s, str.sstr(NULL)));
    tassert_eqi(0, str.slice.ends_with(str$s("1"), str$s("")));
    tassert_eqi(0, str.slice.ends_with((str_s){ 0 }, str$s("1")));

    return EOK;
}

test$case(test_remove_prefix)
{
    str_s out;

    out = str.slice.remove_prefix(str$s("prefix_str_prefix"), str$s("prefix"));
    tassert_eqi(str.slice.cmp(out, str$s("_str_prefix")), 0);

    // no exact match skipped
    out = str.slice.remove_prefix(str$s(" prefix_str_prefix"), str$s("prefix"));
    tassert_eqi(str.slice.cmp(out, str$s(" prefix_str_prefix")), 0);

    // empty prefix
    out = str.slice.remove_prefix(str$s("prefix_str_prefix"), str$s(""));
    tassert_eqi(str.slice.cmp(out, str$s("prefix_str_prefix")), 0);

    // bad prefix
    out = str.slice.remove_prefix(str$s("prefix_str_prefix"), str.sstr(NULL));
    tassert_eqi(str.slice.cmp(out, str$s("prefix_str_prefix")), 0);

    // no match
    out = str.slice.remove_prefix(str$s("prefix_str_prefix"), str$s("prefi_"));
    tassert_eqi(str.slice.cmp(out, str$s("prefix_str_prefix")), 0);

    return EOK;
}

test$case(test_remove_suffix)
{
    str_s out;

    out = str.slice.remove_suffix(str$s("suffix_str_suffix"), str$s("suffix"));
    tassert_eqi(str.slice.cmp(out, str$s("suffix_str_")), 0);

    // no exact match skipped
    out = str.slice.remove_suffix(str$s("suffix_str_suffix "), str$s("suffix"));
    tassert_eqi(str.slice.cmp(out, str$s("suffix_str_suffix ")), 0);

    // empty suffix
    out = str.slice.remove_suffix(str$s("suffix_str_suffix"), str$s(""));
    tassert_eqi(str.slice.cmp(out, str$s("suffix_str_suffix")), 0);

    // bad suffix
    out = str.slice.remove_suffix(str$s("suffix_str_suffix"), str.sstr(NULL));
    tassert_eqi(str.slice.cmp(out, str$s("suffix_str_suffix")), 0);

    // no match
    out = str.slice.remove_suffix(str$s("suffix_str_suffix"), str$s("_uffix"));
    tassert_eqi(str.slice.cmp(out, str$s("suffix_str_suffix")), 0);

    return EOK;
}

test$case(test_strip)
{
    str_s s = str.sstr("\n\t \r123\n\t\r 456 \r\n\t");
    (void)s;
    str_s out;

    // LEFT
    out = str.slice.lstrip(str.sstr(NULL));
    tassert_eqi(out.len, 0);
    tassert(out.buf == NULL);

    out = str.slice.lstrip(str.sstr(""));
    tassert_eqi(out.len, 0);
    tassert_eqs(out.buf, "");

    out = str.slice.lstrip(s);
    tassert_eqi(out.len, 14);
    tassert_eqs(out.buf, "123\n\t\r 456 \r\n\t");

    s = str.sstr("\n\t \r\r\n\t");
    out = str.slice.lstrip(s);
    tassert_eqi(out.len, 0);
    tassert_eqs("", out.buf);


    // RIGHT
    out = str.slice.rstrip(str.sstr(NULL));
    tassert_eqi(out.len, 0);
    tassert(out.buf == NULL);

    out = str.slice.rstrip(str.sstr(""));
    tassert_eqi(out.len, 0);
    tassert_eqs(out.buf, "");

    s = str.sstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = str.slice.rstrip(s);
    tassert_eqi(out.len, 14);
    tassert_eqi(memcmp(out.buf, "\n\t \r123\n\t\r 456", out.len), 0);

    s = str.sstr("\n\t \r\r\n\t");
    out = str.slice.rstrip(s);
    tassert_eqi(out.len, 0);
    tassert_eqi(str.slice.cmp(out, str$s("")), 0);

    // BOTH
    out = str.slice.strip(str.sstr(NULL));
    tassert_eqi(out.len, 0);
    tassert(out.buf == NULL);

    out = str.slice.strip(str.sstr(""));
    tassert_eqi(out.len, 0);
    tassert_eqs(out.buf, "");

    s = str.sstr("\n\t \r123\n\t\r 456 \r\n\t");
    out = str.slice.strip(s);
    tassert_eqi(out.len, 10);
    tassert_eqi(memcmp(out.buf, "123\n\t\r 456", out.len), 0);

    s = str.sstr("\n\t \r\r\n\t");
    out = str.slice.strip(s);
    tassert_eqi(out.len, 0);
    tassert_eqs("", out.buf);
    return EOK;
}

test$case(test_cmp)
{

    tassert_eqi(str.slice.cmp(str.sstr("123456"), str.sstr("123456")), 0);
    tassert_eqi(str.slice.cmp(str.sstr(NULL), str.sstr(NULL)), 0);
    tassert_eqi(str.slice.cmp(str.sstr(""), str.sstr("")), 0);
    tassert_eqi(str.slice.cmp(str.sstr("ABC"), str.sstr("AB")), 67);
#ifdef CEX_ENV32BIT
    tassert(str.slice.cmp(str.sstr("ABA"), str.sstr("ABZ")) < 0);
#else
    tassert_eqi(str.slice.cmp(str.sstr("ABA"), str.sstr("ABZ")), -25);
#endif
    tassert_eqi(str.slice.cmp(str.sstr("AB"), str.sstr("ABC")), -67);
    tassert_eqi(str.slice.cmp(str.sstr("A"), str.sstr("")), (int)'A');
    tassert_eqi(str.slice.cmp(str.sstr(""), str.sstr("A")), -1 * ((int)'A'));
    tassert_eqi(str.slice.cmp(str.sstr(""), str.sstr(NULL)), 1);
    tassert_eqi(str.slice.cmp(str.sstr(NULL), str.sstr("ABC")), -1);

    return EOK;
}

test$case(test_cmpi)
{

    tassert_eqi(str.slice.cmpi(str.sstr("123456"), str.sstr("123456")), 0);
    tassert_eqi(str.slice.cmpi(str.sstr(NULL), str.sstr(NULL)), 0);
    tassert_eqi(str.slice.cmpi(str.sstr(""), str.sstr("")), 0);

    tassert_eqi(str.slice.cmpi(str.sstr("ABC"), str.sstr("ABC")), 0);
    tassert_eqi(str.slice.cmpi(str.sstr("abc"), str.sstr("ABC")), 0);
    tassert_eqi(str.slice.cmpi(str.sstr("ABc"), str.sstr("ABC")), 0);
    tassert_eqi(str.slice.cmpi(str.sstr("ABC"), str.sstr("aBC")), 0);

    tassert_eqi(str.slice.cmpi(str.sstr("ABC"), str.sstr("AB")), 67);
    tassert_eqi(str.slice.cmpi(str.sstr("ABA"), str.sstr("ABZ")), -25);
    tassert_eqi(str.slice.cmpi(str.sstr("AB"), str.sstr("ABC")), -67);
    tassert_eqi(str.slice.cmpi(str.sstr("A"), str.sstr("")), (int)'A');
    tassert_eqi(str.slice.cmpi(str.sstr(""), str.sstr("A")), -1 * ((int)'A'));
    tassert_eqi(str.slice.cmpi(str.sstr(""), str.sstr(NULL)), 1);
    tassert_eqi(str.slice.cmpi(str.sstr(NULL), str.sstr("ABC")), -1);


    tassert(str.slice.cmpi(str.sstr("PFIRM"), str.sstr("PCOMM")) != 0);
    tassert(str.slice.cmpi(str.sstr("PCLASS"), str.sstr("PCLOSE")) != 0);

    return EOK;
}

test$case(str_to__signed_num)
{
    i64 num;
    str_s s;

    num = 0;
    s = str$s("1");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 1);

    num = 0;
    s = str$s(" ");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s(" -");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s(" +");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("+");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 100);

    num = 0;
    s = str$s("      -100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -100);

    num = 0;
    s = str$s("      +100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 100);

    num = 0;
    s = str$s("-100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -100);

    num = 10;
    s = str$s("0");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("0xf");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 15);

    num = 0;
    s = str$s("-0xf");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -15);


    num = 0;
    s = str$s("0x0F");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 15);


    num = 0;
    s = str$s("0x0A");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 10);

    num = 0;
    s = str$s("0x0a");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 10);

    num = 0;
    s = str$s("127");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, INT8_MAX);

    num = 0;
    s = str$s("-127");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -127);

    num = 0;
    s = str$s("-128");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, INT8_MIN);

    num = 0;
    s = str$s("-129");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("128");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("-0x80");
    tassert_eqe(Error.ok, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -128);
    tassert_eqi(num, INT8_MIN);

    num = 0;
    s = str$s("-0x");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = str$s("0x7f");
    tassert_eqe(Error.ok, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, 127);
    tassert_eqi(num, INT8_MAX);

    num = 0;
    s = str$s("0x80");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = str$s("-100");
    tassert_eqe(EOK, str__to_signed_num(s, &num, -100, 100));
    tassert_eqi(num, -100);

    num = 0;
    s = str$s("-101");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, -100, 100));

    num = 0;
    s = str$s("101");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, -100, 100));

    num = 0;
    s = str$s("-000000000127    ");
    tassert_eqe(EOK, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));
    tassert_eqi(num, -127);

    num = 0;
    s = str$s("    ");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = str$s("12 2");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = str$s("-000000000127a");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = str$s("-000000000127h");
    tassert_eqe(Error.argument, str__to_signed_num(s, &num, INT8_MIN, INT8_MAX));

    num = 0;
    s = str$s("-9223372036854775807");
    tassert_eqe(Error.ok, str__to_signed_num(s, &num, INT64_MIN + 1, INT64_MAX));
    tassert_eql(num, -9223372036854775807L);

    num = 0;
    s = str$s("-9223372036854775808");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT64_MIN + 1, INT64_MAX));

    num = 0;
    s = str$s("9223372036854775807");
    tassert_eqe(Error.ok, str__to_signed_num(s, &num, INT64_MIN + 1, INT64_MAX));
    tassert_eql(num, 9223372036854775807L);

    num = 0;
    s = str$s("9223372036854775808");
    tassert_eqe(Error.overflow, str__to_signed_num(s, &num, INT64_MIN + 1, INT64_MAX));

    return EOK;
}

test$case(test_str_to_i8)
{
    i8 num;
    str_s s;

    num = 0;
    s = str$s("127");
    tassert_eqe(EOK, str.convert.to_i8(s, &num));
    tassert_eqi(num, 127);

    num = 0;
    s = str$s("128");
    tassert_eqe(Error.overflow, str.convert.to_i8(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("0");
    tassert_eqe(Error.ok, str.convert.to_i8(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("-128");
    tassert_eqe(Error.ok, str.convert.to_i8(s, &num));
    tassert_eqi(num, -128);


    num = 0;
    s = str$s("-129");
    tassert_eqe(Error.overflow, str.convert.to_i8(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}


test$case(test_str_to_i16)
{
    i16 num;
    str_s s;

    num = 0;
    s = str$s("-32768");
    tassert_eqe(EOK, str.convert.to_i16(s, &num));
    tassert_eqi(num, -32768);

    num = 0;
    s = str$s("-32769");
    tassert_eqe(Error.overflow, str.convert.to_i16(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("32767");
    tassert_eqe(Error.ok, str.convert.to_i16(s, &num));
    tassert_eqi(num, 32767);

    num = 0;
    s = str$s("32768");
    tassert_eqe(Error.overflow, str.convert.to_i16(s, &num));
    tassert_eqi(num, 0);

    return EOK;
}

test$case(test_str_to_i32)
{
    i32 num;
    str_s s;

    num = 0;
    s = str$s("-2147483648");
    tassert_eqe(EOK, str.convert.to_i32(s, &num));
    tassert_eqi(num, -2147483648);

    num = 0;
    s = str$s("-2147483649");
    tassert_eqe(Error.overflow, str.convert.to_i32(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("2147483647");
    tassert_eqe(Error.ok, str.convert.to_i32(s, &num));
    tassert_eqi(num, 2147483647);


    num = 0;
    s = str$s("2147483648");
    tassert_eqe(Error.overflow, str.convert.to_i32(s, &num));
    tassert_eqi(num, 0);

    return EOK;
}

test$case(test_str_to_i64)
{
    i64 num;
    str_s s;

    num = 0;
    s = str$s("-9223372036854775807");
    tassert_eqe(Error.ok, str.convert.to_i64(s, &num));
    tassert_eql(num, -9223372036854775807);

    num = 0;
    s = str$s("-9223372036854775808");
    tassert_eqe(Error.overflow, str.convert.to_i64(s, &num));
    // tassert_eql(num, -9223372036854775808UL);

    num = 0;
    s = str$s("9223372036854775807");
    tassert_eqe(Error.ok, str.convert.to_i64(s, &num));
    tassert_eql(num, 9223372036854775807);

    num = 0;
    s = str$s("9223372036854775808");
    tassert_eqe(Error.overflow, str.convert.to_i64(s, &num));
    // tassert_eql(num, 9223372036854775807);

    return EOK;
}

test$case(str_to__unsigned_num)
{
    u64 num;
    str_s s;

    num = 0;
    s = str$s("1");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 1);

    num = 0;
    s = str$s(" ");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s(" -");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s(" +");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("+");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("100");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 100);

    num = 0;
    s = str$s("      -100");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = str$s("      +100");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 100);

    num = 0;
    s = str$s("-100");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 10;
    s = str$s("0");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("0xf");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 15);

    num = 0;
    s = str$s("0x");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = str$s("-0xf");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));


    num = 0;
    s = str$s("0x0F");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 15);


    num = 0;
    s = str$s("0x0A");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 10);

    num = 0;
    s = str$s("0x0a");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 10);

    num = 0;
    s = str$s("127");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, INT8_MAX);

    num = 0;
    s = str$s("-127");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = str$s("-128");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = str$s("255");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 255);

    num = 0;
    s = str$s("256");
    tassert_eqe(Error.overflow, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("0xff");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 255);
    tassert_eqi(num, UINT8_MAX);


    num = 0;
    s = str$s("100");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, 100));
    tassert_eqi(num, 100);

    num = 0;
    s = str$s("101");
    tassert_eqe(Error.overflow, str__to_unsigned_num(s, &num, 100));


    num = 0;
    s = str$s("000000000127    ");
    tassert_eqe(EOK, str__to_unsigned_num(s, &num, UINT8_MAX));
    tassert_eqi(num, 127);

    num = 0;
    s = str$s("    ");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = str$s("12 2");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = str$s("000000000127a");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = str$s("000000000127h");
    tassert_eqe(Error.argument, str__to_unsigned_num(s, &num, UINT8_MAX));

    num = 0;
    s = str$s("9223372036854775807");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT64_MAX));
    tassert_eql(num, 9223372036854775807L);

    num = 0;
    s = str$s("9223372036854775807");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT64_MAX));
    tassert_eql(num, 9223372036854775807L);

    num = 0;
    s = str$s("18446744073709551615");
    tassert_eqe(Error.ok, str__to_unsigned_num(s, &num, UINT64_MAX));
    tassert(num == __UINT64_C(18446744073709551615));

    num = 0;
    s = str$s("18446744073709551616");
    tassert_eqe(Error.overflow, str__to_unsigned_num(s, &num, UINT64_MAX));

    return EOK;
}


test$case(test_str_to_u8)
{
    u8 num;
    str_s s;

    num = 0;
    s = str$s("255");
    tassert_eqe(EOK, str.convert.to_u8(s, &num));
    tassert_eqi(num, 255);
    tassert(num == UINT8_MAX);

    num = 0;
    s = str$s("256");
    tassert_eqe(Error.overflow, str.convert.to_u8(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("0");
    tassert_eqe(Error.ok, str.convert.to_u8(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}


test$case(test_str_to_u16)
{
    u16 num;
    str_s s;

    num = 0;
    s = str$s("65535");
    tassert_eqe(EOK, str.convert.to_u16(s, &num));
    tassert_eqi(num, 65535);
    tassert(num == UINT16_MAX);

    num = 0;
    s = str$s("65536");
    tassert_eqe(Error.overflow, str.convert.to_u16(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("0");
    tassert_eqe(Error.ok, str.convert.to_u16(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}


test$case(test_str_to_u32)
{
    u32 num;
    str_s s;

    num = 0;
    s = str$s("4294967295");
    tassert_eqe(EOK, str.convert.to_u32(s, &num));
    tassert_eql(num, 4294967295U);
    tassert(num == UINT32_MAX);

    num = 0;
    s = str$s("4294967296");
    tassert_eqe(Error.overflow, str.convert.to_u32(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("0");
    tassert_eqe(Error.ok, str.convert.to_u32(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}

test$case(test_str_to_u64)
{
    u64 num;
    str_s s;

    num = 0;
    s = str$s("18446744073709551615");
    tassert_eqe(EOK, str.convert.to_u64(s, &num));
    tassert(num == __UINT64_C(18446744073709551615));
    tassert(num == UINT64_MAX);

    num = 0;
    s = str$s("18446744073709551616");
    tassert_eqe(Error.overflow, str.convert.to_u64(s, &num));
    tassert_eqi(num, 0);

    num = 0;
    s = str$s("0");
    tassert_eqe(Error.ok, str.convert.to_u64(s, &num));
    tassert_eqi(num, 0);


    return EOK;
}
test$case(str_to__double)
{
    double num;
    char* s;

    num = 0;
    s = "1";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 1.0);

    num = 0;
    s = "1.5";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 1.5);

    num = 0;
    s = "-1.5";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, -1.5);

    num = 0;
    s = "1e3";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 1000);

    num = 0;
    s = "1e-3";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 1e-3);

    num = 0;
    s = "123e-30";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 123e-30);

    num = 0;
    s = "123e+30";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 123e+30);

    num = 0;
    s = "123.321E+30";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 123.321e+30);

    num = 0;
    s = "-0.";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, -0.0);

    num = 0;
    s = "+.5";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 0.5);

    num = 0;
    s = ".0e10";
    tassert_eqe(EOK, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 0.0);

    num = 0;
    s = ".";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 0.0);

    num = 0;
    s = ".e10";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 0.0);

    num = 0;
    s = "00000000001.e00000010";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 10000000000.0);

    num = 0;
    s = "e10";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "10e";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "10e0.3";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "10a";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "10.0a";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "10.0e-a";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "      10.5     ";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, 10.5);

    num = 0;
    s = "      10.5     z";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "n";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "na";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "nan";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, NAN);

    num = 0;
    s = "   NAN    ";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, NAN);

    num = 0;
    s = "   NaN    ";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, NAN);

    num = 0;
    s = "   nan    y";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "   nanny";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "inf";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, INFINITY);

    num = 0;
    s = "INF";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, INFINITY);
    tassert(isinf(num));

    num = 0;
    s = "-iNf";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, -INFINITY);
    tassert(isinf(num));

    num = 0;
    s = "-infinity";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, -INFINITY);
    tassert(isinf(num));

    num = 0;
    s = "   INFINITY   ";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));
    tassert_eqf(num, INFINITY);
    tassert(isinf(num));

    num = 0;
    s = "INFINITY0";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "INFO";
    tassert_eqe(Error.argument, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "1e100";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "1e101";
    tassert_eqe(Error.overflow, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "1e-100";
    tassert_eqe(Error.ok, str__to_double(s, &num, -100, 100));

    num = 0;
    s = "1e-101";
    tassert_eqe(Error.overflow, str__to_double(s, &num, -100, 100));

    return EOK;
}
test$case(test_str_to_f32)
{
    f32 num;
    char* s;

    num = 0;
    s = "1.4";
    tassert_eqe(EOK, str.convert.to_f32(s, &num));
    tassert_eqf(num, 1.4f);


    num = 0;
    s = "nan";
    tassert_eqe(EOK, str.convert.to_f32(s, &num));
    tassert_eqf(num, NAN);

    num = 0;
    s = "1e38";
    tassert_eqe(EOK, str.convert.to_f32(s, &num));

    num = 0;
    s = "1e39";
    tassert_eqe(Error.overflow, str.convert.to_f32(s, &num));


    num = 0;
    s = "1e-37";
    tassert_eqe(EOK, str.convert.to_f32(s, &num));

    num = 0;
    s = "1e-38";
    tassert_eqe(Error.overflow, str.convert.to_f32(s, &num));


    return EOK;
}

test$case(test_str_to_f64)
{
    f64 num;
    char* s;

    num = 0;
    s = "1.4";
    tassert_eqe(EOK, str.convert.to_f64(s, &num));
    tassert_eqf(num, 1.4);


    num = 0;
    s = "nan";
    tassert_eqe(EOK, str.convert.to_f64(s, &num));
    tassert_eqf(num, NAN);

    num = 0;
    s = "1e308";
    tassert_eqe(EOK, str.convert.to_f64(s, &num));

    num = 0;
    s = "1e309";
    tassert_eqe(Error.overflow, str.convert.to_f64(s, &num));


    num = 0;
    s = "1e-307";
    tassert_eqe(EOK, str.convert.to_f64(s, &num));

    num = 0;
    s = "1e-308";
    tassert_eqe(Error.overflow, str.convert.to_f64(s, &num));


    return EOK;
}

test$case(test_str_sprintf)
{
    char buffer[10] = { 0 };

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(EOK, str.sprintf(buffer, sizeof(buffer), "%s", "1234"));
    tassert_eqs("1234", buffer);
    tassert_eqi('\0', buffer[arr$len(buffer) - 1]);


    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(EOK, str.sprintf(buffer, sizeof(buffer), "%s", "123456789"));
    tassert_eqs("123456789", buffer);
    tassert_eqi('\0', buffer[arr$len(buffer) - 1]);

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(Error.overflow, str.sprintf(buffer, sizeof(buffer), "%s", "1234567890"));
    tassert_eqs("", buffer);
    tassert_eqi('\0', buffer[arr$len(buffer) - 1]);

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(
        Error.overflow,
        str.sprintf(buffer, sizeof(buffer), "%s", "12345678900129830128308")
    );
    tassert_eqs("", buffer);
    tassert_eqi('\0', buffer[arr$len(buffer) - 1]);

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(EOK, str.sprintf(buffer, sizeof(buffer), "%s", ""));
    tassert_eqs("", buffer);
    tassert_eqi('\0', buffer[arr$len(buffer) - 1]);


    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(Error.argument, str.sprintf(NULL, sizeof(buffer), "%s", ""));
    tassert_eqi(buffer[0], 'z'); // untouched!

    memset(buffer, 'z', sizeof(buffer));
    tassert_eqe(Error.argument, str.sprintf(buffer, 0, "%s", ""));
    tassert_eqi(buffer[0], 'z'); // untouched!

    return EOK;
}

test$case(test_s_macros)
{

    str_s s = str.sstr("123456");
    tassert_eqi(s.len, 6);
    tassert(s.buf);

    str_s s2 = str$s("fofo");
    tassert_eqi(s2.len, 4);
    tassert(s2.buf);

    s2 = str$s("");
    tassert(s2.buf);
    tassert_eqi(s2.len, 0);
    tassert_eqi(str.slice.cmp(s2, str$s("")), 0);

    str_s m = str$s("\
foo\n\
bar\n\
zoo\n");

    tassert(m.buf);
    tassert_eqs("foo\nbar\nzoo\n", m.buf);

    return EOK;
}

test$case(test_fmt)
{
    mem$scope(tmem$, _)
    {
        // loading this test!
        char* fcont = io.fload(__FILE__, _);
        str_s fcontent = str.sstr(fcont);
        tassert(str.len(fcont) > CEX_SPRINTF_MIN * 2);

        // NOTE: even smalls strings were allocated on mem$
        char* s = str.fmt(mem$, "%s", "foo");
        tassert(s != NULL);
        tassert_eqi(3, strlen(s));
        tassert_eqs("foo", s);
        mem$free(mem$, s);

        char* s2 = str.fmt(mem$, "%S", fcontent);
        tassert(s2 != NULL);
        tassert_eqi(strlen(s2), fcontent.len);
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
        char* fcont = io.fload(__FILE__, _);
        str_s fcontent = str.sstr(fcont);
        tassert(str.len(fcont) > CEX_SPRINTF_MIN * 2);

        char* s = str.fmt(mem$, "%s", "foo");
        tassert_eqs("foo", s);
        mem$free(mem$, s);

        str_s s2 = str.sstr(str.fmt(mem$, "%S", str.slice.sub(fcontent, 0, CEX_SPRINTF_MIN - 1)));
        tassert(s2.buf);
        tassert_eqi(s2.len, CEX_SPRINTF_MIN - 1);
        for (u32 i = 0; i < s2.len; i++) {
            tassertf(
                s2.buf[i] == fcontent.buf[i],
                "i=%d s2['%c'] != fcontent['%c']",
                i,
                s2.buf[i],
                fcontent.buf[i]
            );
        }
        tassert_eqi(s2.buf[s2.len], '\0');
        mem$free(mem$, s2.buf);

        s2 = str.sstr(str.fmt(mem$, "%S", str.slice.sub(fcontent, 0, CEX_SPRINTF_MIN)));
        tassert(s2.buf);
        tassert_eqi(s2.len, CEX_SPRINTF_MIN);
        for (u32 i = 0; i < s2.len; i++) {
            tassertf(
                s2.buf[i] == fcontent.buf[i],
                "i=%d s2['%c'] != fcontent['%c']",
                i,
                s2.buf[i],
                fcontent.buf[i]
            );
        }
        tassert_eqi(s2.buf[s2.len], '\0');
        mem$free(mem$, s2.buf);

        s2 = str.sstr(str.fmt(mem$, "%S", str.slice.sub(fcontent, 0, CEX_SPRINTF_MIN + 1)));
        tassert(s2.buf);
        tassert_eqi(s2.len, CEX_SPRINTF_MIN + 1);
        for (u32 i = 0; i < s2.len; i++) {
            tassertf(
                s2.buf[i] == fcontent.buf[i],
                "i=%d s2['%c'] != fcontent['%c']",
                i,
                s2.buf[i],
                fcontent.buf[i]
            );
        }
        tassert_eqi(s2.buf[s2.len], '\0');
        mem$free(mem$, s2.buf);
    }

    return EOK;
}

test$case(test_slice_clone)
{
    mem$scope(tmem$, _)
    {
        char* fcont = io.fload(__FILE__, _);
        str_s fcontent = str.sstr(fcont);
        tassert(str.len(fcont) > CEX_SPRINTF_MIN * 2);

        var snew = str.slice.clone(fcontent, _);
        tassert(snew != NULL);
        tassert(snew != fcontent.buf);
        tassert_eqi(strlen(snew), fcontent.len);
        tassert(0 == strcmp(snew, fcontent.buf));
        tassert_eqi(snew[fcontent.len], 0);
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
        tassert_eqi(strlen(snew), strlen(foo));
        tassert(0 == strcmp(snew, foo));
        tassert_eqi(snew[strlen(snew)], 0);
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
        const char* expected1[] = {
            "123456",
        };
        res = str.split(s.buf, ",", _);
        tassert_eqi(arr$len(res), 1);

        for$arr(v, res)
        {
            tassert_eqi(strcmp(v, expected1[nit]), 0);
            nit++;
        }
    }

    mem$scope(tmem$, _)
    {
        u32 nit = 0;
        var s = str.sstr("123,456");
        const char* expected[] = {
            "123",
            "456",
        };
        arr$(char*) res = str.split(s.buf, ",", _);
        tassert(res != NULL);
        tassert_eqi(arr$len(res), 2);

        for$arr(v, res)
        {
            tassert_eqi(strcmp(v, expected[nit]), 0);
            nit++;
        }
    }

    mem$scope(tmem$, _)
    {
        u32 nit = 0;
        var s = str.sstr("123,456, 789");
        const char* expected[] = {
            "123",
            "456",
            " 789",
        };
        arr$(char*) res = str.split(s.buf, ",", _);
        tassert(res != NULL);
        tassert_eqi(arr$len(res), 3);

        for$arr(v, res)
        {
            tassert_eqs(v, expected[nit]);
            nit++;
        }
    }

    mem$scope(tmem$, _)
    {
        u32 nit = 0;
        var s = str.sstr("123,456, 789,");
        const char* expected[] = {
            "123",
            "456",
            " 789",
            "",
        };
        arr$(char*) res = str.split(s.buf, ",", _);
        tassert(res != NULL);
        tassert_eqi(arr$len(res), 4);

        for$arr(v, res)
        {
            tassert_eqs(v, expected[nit]);
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
        const char* expected[] = {
            "123",
            "456",
            "789",
        };
        arr$(char*) res = str.split_lines(s.buf, _);
        tassert(res != NULL);
        tassert_eqi(arr$len(res), 3);

        u32 nit = 0;
        for$arr(v, res)
        {
            tassert_eqs(v, expected[nit]);
            nit++;
        }
    }

    mem$scope(tmem$, _)
    {
        var s = str.sstr("ab c\n\nde fg\rkl\r\nfff\fvvv\v");
        const char* expected[] = {
            "ab c",
            "",
            "de fg",
            "kl",
            "fff",
            "vvv",
        };
        arr$(char*) res = str.split_lines(s.buf, _);
        tassert(res != NULL);
        tassert_eqi(arr$len(res), 6);
        tassert_eqi(arr$len(res), arr$len(expected));

        u32 nit = 0;
        for$arr(v, res)
        {
            tassert_eqs(v, expected[nit]);
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
        tassert_eqs("456456456", str.replace(s, "123", "456", _));
        tassert_eqs("787878", str.replace("456456456", "456", "78", _));
        tassert_eqs("321321321", str.replace("787878", "78", "321", _));
        tassert_eqs("111", str.replace("321321321", "32", "", _));
        tassert_eqs("223223223", str.replace(s, "1", "2", _));
        tassert_eqs("", str.replace("2222", "2", "", _));
        tassert_eqs("1111", str.replace("1111", "2", "", _));
        tassert_eqs("", str.replace("", "2", "", _));
        tassert_eqs(NULL, str.replace("11111", "", "", _));
        tassert_eqs(NULL, str.replace(NULL, "foo", "bar", _));
        tassert_eqs(NULL, str.replace("baz", NULL, "bar", _));
        tassert_eqs(NULL, str.replace("baz", "foo", NULL, _));
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

        char* joined = str.join(res, ",", _);
        tassert(joined != NULL);
        tassert(joined != res[0]); // new memory allocated
        tassert_eqs(joined, "foo");

        arr$pushm(res, "bar");
        joined = str.join(res, ", ", _);
        tassert(joined != NULL);
        tassert(joined != res[0]); // new memory allocated
        tassert_eqs(joined, "foo, bar");

        arr$pushm(res, "baz");
        joined = str.join(res, ", ", _);
        tassert(joined != NULL);
        tassert(joined != res[0]); // new memory allocated
        tassert_eqs(joined, "foo, bar, baz");
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
        tassert_eqs(s, "result is: hello there");
    }

    return EOK;
}
/*
 *

 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header();  // >>> all tests below
    
    test$run(test_cstr);
    test$run(test_cstr_sdollar);
    test$run(test_copy);
    test$run(test_slice_copy);
    test$run(test_sub_positive_start);
    test$run(test_sub_negative_start);
    test$run(test_eq);
    test$run(test_iter_split);
    test$run(test_find);
    test$run(test_rfind);
    test$run(test_contains_starts_ends);
    test$run(test_contains_sclice_starts_ends);
    test$run(test_remove_prefix);
    test$run(test_remove_suffix);
    test$run(test_strip);
    test$run(test_cmp);
    test$run(test_cmpi);
    test$run(str_to__signed_num);
    test$run(test_str_to_i8);
    test$run(test_str_to_i16);
    test$run(test_str_to_i32);
    test$run(test_str_to_i64);
    test$run(str_to__unsigned_num);
    test$run(test_str_to_u8);
    test$run(test_str_to_u16);
    test$run(test_str_to_u32);
    test$run(test_str_to_u64);
    test$run(str_to__double);
    test$run(test_str_to_f32);
    test$run(test_str_to_f64);
    test$run(test_str_sprintf);
    test$run(test_s_macros);
    test$run(test_fmt);
    test$run(test_fmt_edge);
    test$run(test_slice_clone);
    test$run(test_clone);
    test$run(test_tsplit);
    test$run(test_split_lines);
    test$run(test_str_replace);
    test$run(test_tjoin);
    test$run(test_str_chaining);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
