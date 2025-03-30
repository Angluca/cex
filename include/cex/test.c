#include "all.h"

enum _cex_test_eq_op_e
{
    _cex_test_eq_op__na,
    _cex_test_eq_op__eq,
    _cex_test_eq_op__ne,
    _cex_test_eq_op__lt,
    _cex_test_eq_op__le,
    _cex_test_eq_op__gt,
    _cex_test_eq_op__ge,
};

static Exc
_check_eq_int(i64 a, i64 b, int line, enum _cex_test_eq_op_e op)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    bool passed = false;
    char* ops = "?";
    switch (op) {
        case _cex_test_eq_op__na:
            unreachable("bad op");
            break;
        case _cex_test_eq_op__eq:
            passed = a == b;
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = a != b;
            ops = "==";
            break;
        case _cex_test_eq_op__lt:
            passed = a < b;
            ops = ">=";
            break;
        case _cex_test_eq_op__le:
            passed = a <= b;
            ops = ">";
            break;
        case _cex_test_eq_op__gt:
            passed = a > b;
            ops = "<=";
            break;
        case _cex_test_eq_op__ge:
            passed = a >= b;
            ops = "<";
            break;
    }
    if (!passed) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            sizeof(_cex_test__mainfn_state.str_buf),
            "%s:%d -> %ld %s %ld",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            ops,
            b
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc
_check_eq_almost(f64 a, f64 b, f64 delta, int line)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    bool passed = false;
    f64 abdelta = a - b;
    if (isnan(a) && isnan(b)) {
        passed = true;
    } else if (isinf(a) && isinf(b)) {
        passed = (signbit(a) == signbit(b));
    } else {
        passed = fabs(abdelta) <= ((delta != 0) ? delta : (f64)FLT_EPSILON);
    }
    if (!passed) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            sizeof(_cex_test__mainfn_state.str_buf),
            "%s:%d -> %f != %f (delta: %f, diff: %f)",
            _cex_test__mainfn_state.suite_file,
            line,
            (a),
            (b),
            delta,
            abdelta
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc
_check_eq_f32(f64 a, f64 b, int line, enum _cex_test_eq_op_e op)
{
    (void)op;
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    bool passed = false;
    char* ops = "?";
    f64 delta = a - b;
    bool is_equal = false;

    if (isnan(a) && isnan(b)) {
        is_equal = true;
    } else if (isinf(a) && isinf(b)) {
        is_equal = (signbit(a) == signbit(b));
    } else {
        is_equal = fabs(delta) <= (f64)FLT_EPSILON;
    }
    switch (op) {
        case _cex_test_eq_op__na:
            unreachable("bad op");
            break;
        case _cex_test_eq_op__eq:
            passed = is_equal;
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = !is_equal;
            ops = "==";
            break;
        case _cex_test_eq_op__lt:
            passed = a < b;
            ops = ">=";
            break;
        case _cex_test_eq_op__le:
            passed = a < b || is_equal;
            ops = ">";
            break;
        case _cex_test_eq_op__gt:
            passed = a > b;
            ops = "<=";
            break;
        case _cex_test_eq_op__ge:
            passed = a >= b || is_equal;
            ops = "<";
            break;
    }
    if (!passed) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            sizeof(_cex_test__mainfn_state.str_buf),
            "%s:%d -> %f %s %f (delta: %f)",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            ops,
            b,
            delta
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc
_check_eq_str(const char* a, const char* b, int line, enum _cex_test_eq_op_e op)
{
    bool passed = false;
    char* ops = "";
    switch (op) {
        case _cex_test_eq_op__eq:
            passed = str.eq(a, b);
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = !str.eq(a, b);
            ops = "==";
            break;
        default:
            unreachable("bad op or unsupported for strings");
    }
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (!passed) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            CEXTEST_AMSG_MAX_LEN - 1,
            "%s:%d -> '%s' %s '%s'",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            ops,
            b
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc
_check_eq_err(const char* a, const char* b, int line)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (!(a == NULL || b == NULL) && !str.eq(a, b)) {
        const char* ea = (a == EOK) ? "Error.ok" : a;
        const char* eb = (b == EOK) ? "Error.ok" : b;
        snprintf(
            _cex_test__mainfn_state.str_buf,
            CEXTEST_AMSG_MAX_LEN - 1,
            "%s:%d -> Exc mismatch '%s' != '%s'",
            _cex_test__mainfn_state.suite_file,
            line,
            ea,
            eb
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}


static Exc
_check_eq_ptr(const void* a, const void* b, int line)
{
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (a != b) {
        snprintf(
            _cex_test__mainfn_state.str_buf,
            CEXTEST_AMSG_MAX_LEN - 1,
            "%s:%d -> %p != %p (ptr_diff: %ld)",
            _cex_test__mainfn_state.suite_file,
            line,
            a,
            b,
            (a - b)
        );
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}

static Exc
_check_eqs_slice(str_s a, str_s b, int line, enum _cex_test_eq_op_e op)
{
    bool passed = false;
    char* ops = "";
    switch (op) {
        case _cex_test_eq_op__eq:
            passed = str.slice.eq(a, b);
            ops = "!=";
            break;
        case _cex_test_eq_op__ne:
            passed = !str.slice.eq(a, b);
            ops = "==";
            break;
        default:
            unreachable("bad op or unsupported for strings");
    }
    extern struct _cex_test_context_s _cex_test__mainfn_state;
    if (!passed) {
        if (str.sprintf(
                _cex_test__mainfn_state.str_buf,
                CEXTEST_AMSG_MAX_LEN - 1,
                "%s:%d -> '%S' %s '%S'",
                _cex_test__mainfn_state.suite_file,
                line,
                a,
                ops,
                b
            )) {
        }
        return _cex_test__mainfn_state.str_buf;
    }
    return EOK;
}
