#include <cex/all.c>
#include <math.h>

test$case(test_tassert_i8)
{
    i8 a = 0;
    i8 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = INT8_MAX;
    b = INT8_MIN;
    tassert_ne(a, b);

    a = INT8_MAX;
    b = INT8_MAX;
    tassert_eq(a, b);

    a = INT8_MIN;
    b = INT8_MIN;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_u8)
{
    u8 a = 0;
    u8 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = UINT8_MAX;
    b = 0;
    tassert_ne(a, b);

    a = UINT8_MAX;
    b = UINT8_MAX;
    tassert_eq(a, b);

    a = 0;
    b = 0;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_i16)
{
    i16 a = 0;
    i16 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = INT16_MAX;
    b = INT16_MIN;
    tassert_ne(a, b);

    a = INT16_MAX;
    b = INT16_MAX;
    tassert_eq(a, b);

    a = INT16_MIN;
    b = INT16_MIN;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_u16)
{
    u16 a = 0;
    u16 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = UINT16_MAX;
    b = 0;
    tassert_ne(a, b);

    a = UINT16_MAX;
    b = UINT16_MAX;
    tassert_eq(a, b);

    a = 0;
    b = 0;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_i32)
{
    i32 a = 0;
    i32 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = INT32_MAX;
    b = INT32_MIN;
    tassert_ne(a, b);

    a = INT32_MAX;
    b = INT32_MAX;
    tassert_eq(a, b);

    a = INT32_MIN;
    b = INT32_MIN;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_u32)
{
    u32 a = 0;
    u32 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = UINT32_MAX;
    b = 0;
    tassert_ne(a, b);

    a = UINT32_MAX;
    b = UINT32_MAX;
    tassert_eq(a, b);

    a = 0;
    b = 0;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_i64)
{
    i64 a = 0;
    i64 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = INT64_MAX;
    b = INT64_MIN;
    tassert_ne(a, b);

    a = INT64_MAX;
    b = INT64_MAX;
    tassert_eq(a, b);

    a = INT64_MIN;
    b = INT64_MIN;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_u64)
{
    u64 a = 0;
    u64 b = 0;

    a = 0;
    b = 0;
    tassert_eq(a, b);

    a = 1;
    b = 0;
    tassert_ne(a, b);

    a = 0;
    b = 1;
    tassert_ne(a, b);

    a = 0;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_ge(a, b);

    a = 1;
    b = 0;
    tassert_gt(a, b);

    a = 0;
    b = 0;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_le(a, b);

    a = 0;
    b = 1;
    tassert_lt(a, b);

    a = UINT64_MAX;
    b = 0;
    tassert_ne(a, b);

    a = UINT64_MAX;
    b = UINT64_MAX;
    tassert_eq(a, b);

    a = 0;
    b = 0;
    tassert_eq(a, b);

    return EOK;
}

test$case(test_tassert_f32)
{
    f32 nan = 0.0f / 0.0f;
    f32 a = 0.0f;
    f32 b = 0.0f;

    a = 0.0f;
    b = 0.0f;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = 1.0f;
    b = 0.0f;
    tassert_ne(a, b);

    a = 0.0f;
    b = 1.0f;
    tassert_ne(a, b);

    a = 0.0f;
    b = 0.0f;
    tassert_ge(a, b);

    a = 1.0f;
    b = 0.0f;
    tassert_ge(a, b);

    a = 1.0f;
    b = 0.0f;
    tassert_gt(a, b);

    a = 0.0f;
    b = 0.0f;
    tassert_le(a, b);

    a = 0.0f;
    b = 1.0f;
    tassert_le(a, b);

    a = 0.0f;
    b = 1.0f;
    tassert_lt(a, b);

    a = nan;
    b = nan;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = nan;
    b = 0;
    tassert_ne(a, b);

    a = 1.0f/3.0f;
    b = 1.0f/3.0f;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = HUGE_VALF;
    b = HUGE_VALF;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = -HUGE_VALF;
    b = -HUGE_VALF;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = HUGE_VALF;
    b = HUGE_VALF;
    tassert_ge(a, b);

    a = -HUGE_VALF;
    b = HUGE_VALF;
    tassert_lt(a, b);

    a = HUGE_VALF;
    b = -HUGE_VALF;
    tassert_gt(a, b);

    a = -HUGE_VALF;
    b = HUGE_VALF;
    tassert_ne(a, b);

    a = -HUGE_VALF;
    b = nan;
    tassert_ne(a, b);

    a = HUGE_VALF;
    b = nan;
    tassert_ne(a, b);

    return EOK;
}

test$case(test_tassert_f64)
{
    f64 nan = 0.0 / 0.0;
    f64 a = 0.0;
    f64 b = 0.0;

    a = 0.0;
    b = 0.0;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = 1.0;
    b = 0.0;
    tassert_ne(a, b);

    a = 0.0;
    b = 1.0;
    tassert_ne(a, b);

    a = 0.0;
    b = 0.0;
    tassert_ge(a, b);

    a = 1.0;
    b = 0.0;
    tassert_ge(a, b);

    a = 1.0;
    b = 0.0;
    tassert_gt(a, b);

    a = 0.0;
    b = 0.0;
    tassert_le(a, b);

    a = 0.0;
    b = 1.0;
    tassert_le(a, b);

    a = 0.0;
    b = 1.0;
    tassert_lt(a, b);

    a = nan;
    b = nan;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = nan;
    b = 0;
    tassert_ne(a, b);

    a = 1.0/3.0;
    b = 1.0/3.0;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = HUGE_VAL;
    b = HUGE_VAL;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = -HUGE_VAL;
    b = -HUGE_VAL;
    tassert_eq(a, b);

    tassert_eq_almost(a, b, 0);

    a = HUGE_VAL;
    b = HUGE_VAL;
    tassert_ge(a, b);

    a = -HUGE_VAL;
    b = HUGE_VAL;
    tassert_lt(a, b);

    a = HUGE_VAL;
    b = -HUGE_VAL;
    tassert_gt(a, b);

    a = -HUGE_VAL;
    b = HUGE_VAL;
    tassert_ne(a, b);

    a = -HUGE_VAL;
    b = nan;
    tassert_ne(a, b);

    a = HUGE_VAL;
    b = nan;
    tassert_ne(a, b);

    return EOK;
}

test$case(test_tassert_equal_almost)
{
    f32 a = 0.0f;
    f32 b = 0.0f;

    a = 0.01f;
    b = 0.0f;
    tassert_ne(a, b);
    tassert_eq_almost(a, b, 0.01);
    tassert_eq_almost(b, a, 0.01);
    tassert_eq_almost(1, 0, 1);

    return EOK;
}

test$case(test_tassert_error)
{
    tassert_er(EOK, Error.ok);
    tassert_er(Error.argsparse, "ProgramArgsError");
    tassert_eq_ptr(Error.argsparse, Error.argsparse);

    return EOK;
}

test$case(test_tassert_eq_string)
{
    char buf[21] = {"foo"};
    char* s = "foo";
    tassert_eq(s, "foo");
    tassert_ne(s, NULL);
    tassert_eq(buf, "foo");
    tassert_eq(s, buf);
    char* s2 = NULL;
    tassert_eq(s2, NULL);
    tassert_eq("", "");

    return EOK;
}

test$case(test_tassert_eq_slice)
{
    tassert_eq(str.sstr(NULL), (str_s){0});
    tassert_eq(str.sstr(""), str$s(""));
    tassert_eq(str.sstr("foo"), str$s("foo"));
    tassert_eq(str.sstr("foo"), str.sub("foobar", 0, 3));

    return EOK;
}
test$main();
