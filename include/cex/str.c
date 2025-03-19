#pragma once
#include "_sprintf.h"
#include "all.h"
#include <ctype.h>
#include <float.h>
#include <math.h>

static inline bool
str__isvalid(const str_s* s)
{
    return s->buf != NULL;
}

static inline isize
str__index(str_s* s, const char* c, u8 clen)
{
    isize result = -1;

    if (!str__isvalid(s)) {
        return -1;
    }

    u8 split_by_idx[UINT8_MAX] = { 0 };
    for (u8 i = 0; i < clen; i++) {
        split_by_idx[(u8)c[i]] = 1;
    }

    for (usize i = 0; i < s->len; i++) {
        if (split_by_idx[(u8)s->buf[i]]) {
            result = i;
            break;
        }
    }

    return result;
}

static str_s
str_sstr(const char* ccharptr)
{
    if (unlikely(ccharptr == NULL)) {
        return (str_s){ 0 };
    }

    return (str_s){
        .buf = (char*)ccharptr,
        .len = strlen(ccharptr),
    };
}


static str_s
str_sbuf(char* s, usize length)
{
    if (unlikely(s == NULL)) {
        return (str_s){ 0 };
    }

    return (str_s){
        .buf = s,
        .len = strnlen(s, length),
    };
}

static bool
str_eq(const char* a, const char* b)
{
    if (unlikely(a == NULL || b == NULL)) {
        // NOTE: if both are NULL - this function intentionally return false
        return false;
    }
    return strcmp(a, b) == 0;
}

static bool
str__slice__eq(str_s a, str_s b)
{
    if (unlikely(a.buf == NULL || b.buf == NULL)) {
        // NOTE: if both are NULL - this function intentionally return false
        return false;
    }
    if (a.len != b.len) {
        return false;
    }
    return memcmp(a.buf, b.buf, a.len) == 0;
}

static str_s
str__slice__sub(str_s s, isize start, isize end)
{
    slice$define(*s.buf) slice = { 0 };
    if (s.buf != NULL) {
        _arr$slice_get(slice, s.buf, s.len, start, end);
    }

    return (str_s){
        .buf = slice.arr,
        .len = slice.len,
    };
}

static str_s
str_sub(const char* s, isize start, isize end)
{
    str_s slice = str_sstr(s);
    return str__slice__sub(slice, start, end);
}

static Exception
str_copy(char* dest, const char* src, usize destlen)
{
    uassert(dest != src && "buffers overlap");
    if (unlikely(dest == NULL || destlen == 0)) {
        return Error.argument;
    }
    dest[0] = '\0'; // If we fail next, it still writes empty string
    if (unlikely(src == NULL)) {
        return Error.argument;
    }

    char* pend = stpncpy(dest, src, destlen);
    dest[destlen-1] = '\0'; // always secure last byte of destlen

    if (unlikely((pend - dest) >= (isize)destlen)) {
        return Error.overflow;
    }

    return Error.ok;
}

static Exception
str_vsprintf(char* dest, usize dest_len, const char* format, va_list va)
{
    if (unlikely(dest == NULL)){
        return Error.argument;
    }
    if (unlikely(dest_len == 0)){
        return Error.argument;
    }
    uassert(format != NULL);

    dest[dest_len-1] = '\0'; // always null term at capacity

    int result = cexsp__vsnprintf(dest, dest_len, format, va);

    if (result < 0 || (usize)result >= dest_len) {
        dest[0] = '\0';
        return Error.overflow;
    }

    return EOK;
}

static Exception
str_sprintf(char* dest, usize dest_len, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    Exc result = str_vsprintf(dest, dest_len, format, va);
    va_end(va);
    return result;
}


static usize
str_len(char* s)
{
    if (s == NULL) {
        return 0;
    }
    return strlen(s);
}

static char*
str_find(const char* haystack, const char* needle)
{
    if (unlikely(haystack == NULL || needle == NULL || needle[0] == '\0')) {
        return NULL;
    }
    return strstr(haystack, needle);
}

char*
str_findr(const char* haystack, const char* needle)
{
    if (unlikely(haystack == NULL || needle == NULL || needle[0] == '\0')) {
        return NULL;
    }
    usize haystack_len = strlen(haystack);
    usize needle_len = strlen(needle);
    if (unlikely(needle_len > haystack_len)) {
        return NULL;
    }
    for (const char* ptr = haystack + haystack_len - needle_len; ptr >= haystack; ptr--) {
        if (unlikely(strncmp(ptr, needle, needle_len) == 0)) {
            uassert(ptr >= haystack);
            uassert(ptr <= haystack + haystack_len);
            return (char*)ptr;
        }
    }
    return NULL;
}


static bool str__slice__starts_with(str_s str, str_s prefix)
{
    if (unlikely(!str.buf || !prefix.buf || prefix.len == 0 || prefix.len > str.len)) {
        return false;
    }
    return memcmp(str.buf, prefix.buf, prefix.len) == 0;
}
static bool str__slice__ends_with(str_s s, str_s suffix)
{
    if (unlikely(!s.buf || !suffix.buf || suffix.len == 0 || suffix.len > s.len)) {
        return false;
    }
    return s.len >= suffix.len && !memcmp(s.buf + s.len - suffix.len, suffix.buf, suffix.len);
}

static bool str_starts_with(const char* str, const char* prefix) {
    if (str == NULL || prefix == NULL || prefix[0] == '\0') {
        return false;
    }

    while (*prefix && *str == *prefix) {
        ++str, ++prefix; 
    }
    return *prefix == 0;
}

static bool str_ends_with(const char* str, const char* suffix) {
    if (str == NULL || suffix == NULL || suffix[0] == '\0') {
        return false;
    }
    size_t slen = strlen(str);
    size_t sufflen = strlen(suffix);

    return slen >= sufflen && !memcmp(str + slen - sufflen, suffix, sufflen);
}

static str_s
str__slice__remove_prefix(str_s s, str_s prefix)
{
    if (!str__slice__starts_with(s, prefix)) {
        return s;
    }

    return (str_s){
        .buf = s.buf + prefix.len,
        .len = s.len - prefix.len,
    };
}

static str_s
str__slice__remove_suffix(str_s s, str_s suffix)
{
    if (!str__slice__ends_with(s, suffix)) {
        return s;
    }
    return (str_s){
        .buf = s.buf,
        .len = s.len - suffix.len,
    };
}

static inline void
str__strip_left(str_s* s)
{
    char* cend = s->buf + s->len;

    while (s->buf < cend) {
        switch (*s->buf) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->buf++;
                s->len--;
                break;
            default:
                return;
        }
    }
}

static inline void
str__strip_right(str_s* s)
{
    while (s->len > 0) {
        switch (s->buf[s->len - 1]) {
            case ' ':
            case '\t':
            case '\n':
            case '\r':
                s->len--;
                break;
            default:
                return;
        }
    }
}


static str_s
str__slice__lstrip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_left(&result);
    return result;
}

static str_s
str__slice__rstrip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_right(&result);
    return result;
}

static str_s
str__slice__strip(str_s s)
{
    if (s.buf == NULL) {
        return (str_s){
            .buf = NULL,
            .len = 0,
        };
    }
    if (unlikely(s.len == 0)) {
        return (str_s){
            .buf = "",
            .len = 0,
        };
    }

    str_s result = (str_s){
        .buf = s.buf,
        .len = s.len,
    };

    str__strip_left(&result);
    str__strip_right(&result);
    return result;
}

static int
str__slice__cmp(str_s self, str_s other)
{
    if (unlikely(self.buf == NULL)) {
        if (other.buf == NULL) {
            return 0;
        } else {
            return -1;
        }
    }

    if (unlikely(self.len == 0)) {
        if (other.buf == NULL) {
            return 1;
        } else if (other.len == 0) {
            return 0;
        } else {
            return -other.buf[0];
        }
    }

    usize min_len = self.len < other.len ? self.len : other.len;
    int cmp = memcmp(self.buf, other.buf, min_len);

    if (unlikely(cmp == 0 && self.len != other.len)) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

static int
str__slice__cmpi(str_s self, str_s other)
{
    if (unlikely(self.buf == NULL)) {
        if (other.buf == NULL) {
            return 0;
        } else {
            return -1;
        }
    }

    if (unlikely(self.len == 0)) {
        if (other.buf == NULL) {
            return 1;
        } else if (other.len == 0) {
            return 0;
        } else {
            return -other.buf[0];
        }
    }

    usize min_len = self.len < other.len ? self.len : other.len;

    int cmp = 0;
    char* s = self.buf;
    char* o = other.buf;
    for (usize i = 0; i < min_len; i++) {
        cmp = tolower(*s) - tolower(*o);
        if (cmp != 0) {
            return cmp;
        }
        s++;
        o++;
    }

    if (cmp == 0 && self.len != other.len) {
        if (self.len > other.len) {
            cmp = self.buf[min_len] - '\0';
        } else {
            cmp = '\0' - other.buf[min_len];
        }
    }
    return cmp;
}

static str_s*
str__slice__iter_split(str_s s, const char* split_by, cex_iterator_s* iterator)
{
    uassert(iterator != NULL && "null iterator");
    uassert(split_by != NULL && "null split_by");

    // temporary struct based on _ctxbuffer
    struct iter_ctx
    {
        usize cursor;
        usize split_by_len;
        str_s str;
        usize str_len;
    }* ctx = (struct iter_ctx*)iterator->_ctx;
    _Static_assert(sizeof(*ctx) <= sizeof(iterator->_ctx), "ctx size overflow");
    _Static_assert(alignof(struct iter_ctx) <= alignof(usize), "cex_iterator_s _ctx misalign");

    if (unlikely(iterator->val == NULL)) {
        // First run handling
        if (unlikely(!str__isvalid(&s))) {
            return NULL;
        }
        ctx->split_by_len = strlen(split_by);

        if (ctx->split_by_len == 0) {
            return NULL;
        }
        uassert(ctx->split_by_len < UINT8_MAX && "split_by is suspiciously long!");

        isize idx = str__index(&s, split_by, ctx->split_by_len);
        if (idx < 0) {
            idx = s.len;
        }
        ctx->cursor = idx;
        ctx->str = str.slice.sub(s, 0, idx);

        iterator->val = &ctx->str;
        iterator->idx.i = 0;
        return iterator->val;
    } else {
        uassert(iterator->val == &ctx->str);

        if (ctx->cursor >= s.len) {
            return NULL; // reached the end stops
        }
        ctx->cursor++;
        if (unlikely(ctx->cursor == s.len)) {
            // edge case, we have separator at last col
            // it's not an error, return empty split token
            iterator->idx.i++;
            ctx->str = (str_s){ .buf = "", .len = 0 };
            return iterator->val;
        }

        // Get remaining string after prev split_by char
        str_s tok = str.slice.sub(s, ctx->cursor, 0);
        isize idx = str__index(&tok, split_by, ctx->split_by_len);

        iterator->idx.i++;

        if (idx < 0) {
            // No more splits, return remaining part
            ctx->str = tok;
            ctx->cursor = s.len;
        } else {
            // Sub from prev cursor to idx (excluding split char)
            ctx->str = str.slice.sub(tok, 0, idx);
            ctx->cursor += idx;
        }

        return iterator->val;
    }
}


static Exception
str__to_signed_num(str_s self, i64* num, i64 num_min, i64 num_max)
{
    _Static_assert(sizeof(i64) == 8, "unexpected u64 size");
    uassert(num_min < num_max);
    uassert(num_min <= 0);
    uassert(num_max > 0);
    uassert(num_max > 64);
    uassert(num_min == 0 || num_min < -64);
    uassert(num_min >= INT64_MIN + 1 && "try num_min+1, negation overflow");

    if (unlikely(self.len == 0)) {
        return Error.argument;
    }

    char* s = self.buf;
    usize len = self.len;
    usize i = 0;

    for (; s[i] == ' ' && i < self.len; i++) {
    }

    u64 neg = 1;
    if (s[i] == '-') {
        neg = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) {
        return Error.argument;
    }

    if ((len - i) >= 2 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        i += 2;
        base = 16;
        if (unlikely(i >= len)) {
            // edge case when '0x'
            return Error.argument;
        }
    }

    u64 cutoff = (u64)(neg == 1 ? (u64)num_max : (u64)-num_min);
    u64 cutlim = cutoff % (u64)base;
    cutoff /= (u64)base;

    u64 acc = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'F') {
            c = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 10;
        } else {
            break;
        }

        if (unlikely(c >= base)) {
            return Error.argument;
        }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') {
            return Error.argument;
        }
    }

    *num = (i64)acc * neg;

    return Error.ok;
}

static Exception
str__to_unsigned_num(str_s self, u64* num, u64 num_max)
{
    _Static_assert(sizeof(u64) == 8, "unexpected u64 size");
    uassert(num_max > 0);
    uassert(num_max > 64);

    if (unlikely(self.len == 0)) {
        return Error.argument;
    }

    char* s = self.buf;
    usize len = self.len;
    usize i = 0;

    for (; s[i] == ' ' && i < self.len; i++) {
    }

    if (s[i] == '-') {
        return Error.argument;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }
    i32 base = 10;

    if (unlikely(i >= len)) {
        return Error.argument;
    }

    if ((len - i) >= 2 && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        i += 2;
        base = 16;

        if (unlikely(i >= len)) {
            // edge case when '0x'
            return Error.argument;
        }
    }

    u64 cutoff = num_max;
    u64 cutlim = cutoff % (u64)base;
    cutoff /= (u64)base;

    u64 acc = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c >= 'A' && c <= 'F') {
            c = c - 'A' + 10;
        } else if (c >= 'a' && c <= 'f') {
            c = c - 'a' + 10;
        } else {
            break;
        }

        if (unlikely(c >= base)) {
            return Error.argument;
        }

        if (unlikely(acc > cutoff || (acc == cutoff && c > cutlim))) {
            return Error.overflow;
        } else {
            acc *= base;
            acc += c;
        }
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') {
            return Error.argument;
        }
    }

    *num = (i64)acc;

    return Error.ok;
}

static Exception
str__to_double(const char* self, double* num, i32 exp_min, i32 exp_max)
{
    _Static_assert(sizeof(double) == 8, "unexpected double precision");
    if (unlikely(self == NULL)) {
        return Error.argument;
    }

    const char* s = self;
    usize len = strlen(s);
    usize i = 0;
    double number = 0.0;

    for (; s[i] == ' ' && i < len; i++) {
    }

    double sign = 1;
    if (s[i] == '-') {
        sign = -1;
        i++;
    } else if (unlikely(s[i] == '+')) {
        i++;
    }

    if (unlikely(i >= len)) {
        return Error.argument;
    }

    if (unlikely(s[i] == 'n' || s[i] == 'i' || s[i] == 'N' || s[i] == 'I')) {
        if (unlikely(len - i < 3)) {
            return Error.argument;
        }
        if (s[i] == 'n' || s[i] == 'N') {
            if ((s[i + 1] == 'a' || s[i + 1] == 'A') && (s[i + 2] == 'n' || s[i + 2] == 'N')) {
                number = NAN;
                i += 3;
            }
        } else {
            // s[i] = 'i'
            if ((s[i + 1] == 'n' || s[i + 1] == 'N') && (s[i + 2] == 'f' || s[i + 2] == 'F')) {
                number = (double)INFINITY * sign;
                i += 3;
            }
            // INF 'INITY' part (optional but still valid)
            // clang-format off
            if (unlikely(len - i >= 5)) {
                if ((s[i + 0] == 'i' || s[i + 0] == 'I') && 
                    (s[i + 1] == 'n' || s[i + 1] == 'N') &&
                    (s[i + 2] == 'i' || s[i + 2] == 'I') &&
                    (s[i + 3] == 't' || s[i + 3] == 'T') &&
                    (s[i + 4] == 'y' || s[i + 4] == 'Y')) {
                    i += 5;
                }
            }
            // clang-format on
        }

        // Allow trailing spaces, but no other character allowed
        for (; i < len; i++) {
            if (s[i] != ' ') {
                return Error.argument;
            }
        }

        *num = number;
        return Error.ok;
    }

    i32 exponent = 0;
    u32 num_decimals = 0;
    u32 num_digits = 0;

    for (; i < len; i++) {
        u8 c = (u8)s[i];

        if (c >= '0' && c <= '9') {
            c -= '0';
        } else if (c == 'e' || c == 'E' || c == '.') {
            break;
        } else {
            return Error.argument;
        }

        number = number * 10. + c;
        num_digits++;
    }
    // Process decimal part
    if (i < len && s[i] == '.') {
        i++;

        for (; i < len; i++) {
            u8 c = (u8)s[i];

            if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
                break;
            }
            number = number * 10. + c;
            num_decimals++;
            num_digits++;
        }
        exponent -= num_decimals;
    }


    number *= sign;

    if (i < len - 1 && (s[i] == 'e' || s[i] == 'E')) {
        i++;
        sign = 1;
        if (s[i] == '-') {
            sign = -1;
            i++;
        } else if (s[i] == '+') {
            i++;
        }

        u32 n = 0;
        for (; i < len; i++) {
            u8 c = (u8)s[i];

            if (c >= '0' && c <= '9') {
                c -= '0';
            } else {
                break;
            }

            n = n * 10 + c;
        }

        exponent += n * sign;
    }

    if (num_digits == 0) {
        return Error.argument;
    }

    if (exponent < exp_min || exponent > exp_max) {
        return Error.overflow;
    }

    // Scale the result
    double p10 = 10.;
    i32 n = exponent;
    if (n < 0) {
        n = -n;
    }
    while (n) {
        if (n & 1) {
            if (exponent < 0) {
                number /= p10;
            } else {
                number *= p10;
            }
        }
        n >>= 1;
        p10 *= p10;
    }

    if (number == HUGE_VAL) {
        return Error.overflow;
    }

    // Allow trailing spaces, but no other character allowed
    for (; i < len; i++) {
        if (s[i] != ' ') {
            return Error.argument;
        }
    }

    *num = number;

    return Error.ok;
}

static Exception
str__convert__to_f32(const char* s, f32* num)
{
    f64 res = 0;
    Exc r = str__to_double(s, &res, -37, 38);
    *num = (f32)res;
    return r;
}

static Exception
str__convert__to_f64(const char* s, f64* num)
{
    return str__to_double(s, num, -307, 308);
}

static Exception
str__convert__to_i8(str_s self, i8* num)
{
    i64 res = 0;
    Exc r = str__to_signed_num(self, &res, INT8_MIN, INT8_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_i16(str_s self, i16* num)
{
    i64 res = 0;
    var r = str__to_signed_num(self, &res, INT16_MIN, INT16_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_i32(str_s self, i32* num)
{
    i64 res = 0;
    var r = str__to_signed_num(self, &res, INT32_MIN, INT32_MAX);
    *num = res;
    return r;
}


static Exception
str__convert__to_i64(str_s self, i64* num)
{
    i64 res = 0;
    // NOTE:INT64_MIN+1 because negating of INT64_MIN leads to UB!
    var r = str__to_signed_num(self, &res, INT64_MIN + 1, INT64_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_u8(str_s self, u8* num)
{
    u64 res = 0;
    Exc r = str__to_unsigned_num(self, &res, UINT8_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_u16(str_s self, u16* num)
{
    u64 res = 0;
    Exc r = str__to_unsigned_num(self, &res, UINT16_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_u32(str_s self, u32* num)
{
    u64 res = 0;
    Exc r = str__to_unsigned_num(self, &res, UINT32_MAX);
    *num = res;
    return r;
}

static Exception
str__convert__to_u64(str_s self, u64* num)
{
    u64 res = 0;
    Exc r = str__to_unsigned_num(self, &res, UINT64_MAX);
    *num = res;

    return r;
}

static char*
str__fmt_callback(const char* buf, void* user, u32 len)
{
    (void)buf;
    cexsp__context* ctx = user;
    if (unlikely(ctx->has_error)) {
        return NULL;
    }

    if (unlikely(
            len >= CEX_SPRINTF_MIN && (ctx->buf == NULL || ctx->length + len >= ctx->capacity)
        )) {

        if (len > INT32_MAX || ctx->length + len > INT32_MAX) {
            ctx->has_error = true;
            return NULL;
        }

        uassert(ctx->allc != NULL);

        if (ctx->buf == NULL) {
            ctx->buf = mem$calloc(ctx->allc, 1, CEX_SPRINTF_MIN * 2);
            if (ctx->buf == NULL) {
                ctx->has_error = true;
                return NULL;
            }
            ctx->capacity = CEX_SPRINTF_MIN * 2;
        } else {
            ctx->buf = mem$realloc(ctx->allc, ctx->buf, ctx->capacity + CEX_SPRINTF_MIN * 2);
            if (ctx->buf == NULL) {
                ctx->has_error = true;
                return NULL;
            }
            ctx->capacity += CEX_SPRINTF_MIN * 2;
        }
    }
    ctx->length += len;

    // fprintf(stderr, "len: %d, total_len: %d capacity: %d\n", len, ctx->length, ctx->capacity);
    if (len > 0) {
        if (ctx->buf) {
            if (buf == ctx->tmp) {
                uassert(ctx->length <= CEX_SPRINTF_MIN);
                memcpy(ctx->buf, buf, len);
            }
        }
    }


    return (ctx->buf != NULL) ? &ctx->buf[ctx->length] : ctx->tmp;
}

static str_s
str__fmtva(IAllocator allc, const char* format, va_list va)
{
    cexsp__context ctx = {
        .allc = allc,
    };
    cexsp__vsprintfcb(str__fmt_callback, &ctx, ctx.tmp, format, va);
    va_end(va);

    if (unlikely(ctx.has_error)) {
        mem$free(allc, ctx.buf);
        return (str_s){ 0 };
    }

    if (ctx.buf) {
        uassert(ctx.length < ctx.capacity);
        ctx.buf[ctx.length] = '\0';
        ctx.buf[ctx.capacity - 1] = '\0';
    } else {
        uassert(ctx.length <= arr$len(ctx.tmp) - 1);
        ctx.buf = mem$malloc(allc, ctx.length + 1);
        if (ctx.buf == NULL) {
            return (str_s){ 0 };
        }
        memcpy(ctx.buf, ctx.tmp, ctx.length);
        ctx.buf[ctx.length] = '\0';
    }
    return (str_s){ .buf = ctx.buf, .len = ctx.length };
}

static char*
str_fmt(IAllocator allc, const char* format, ...)
{
    va_list va;
    va_start(va, format);
    str_s result = str__fmtva(allc, format, va);
    va_end(va);
    return result.buf;
}

static char*
str_tfmt(const char* format, ...)
{
    va_list va;
    va_start(va, format);
    str_s result = str__fmtva(tmem$, format, va);
    va_end(va);
    return result.buf;
}

static char*
str_tnew(str_s s)
{
    if (s.buf == NULL) {
        return NULL;
    }
    char* result = mem$malloc(tmem$, s.len + 1);
    if (result) {
        memcpy(result, s.buf, s.len);
        result[s.len] = '\0';
    }
    return result;
}

static char*
str_tcopy(char* s)
{
    if (s == NULL) {
        return NULL;
    }
    usize slen = strlen(s);
    uassert(slen < PTRDIFF_MAX);

    char* result = mem$malloc(tmem$, slen + 1);
    if (result) {
        memcpy(result, s, slen);
        result[slen] = '\0';
    }
    return result;
}

static arr$(char*) str_tsplit(char* s, const char* split_by)
{
    str_s src = str_sstr(s);
    if (src.buf == NULL || split_by == NULL) {
        return NULL;
    }
    arr$(char*) result = arr$new(result, tmem$);
    if (result == NULL) {
        return NULL;
    }

    for$iter(str_s, it, str__slice__iter_split(src, split_by, &it.iterator))
    {
        char* tok = str_tnew(*it.val);
        arr$push(result, tok);
    }

    return result;
}

static char*
str_tjoin(arr$(char*) str_arr, const char* join_by)
{
    if (str_arr == NULL || join_by == NULL) {
        return NULL;
    }

    usize jlen = strlen(join_by);
    if (jlen == 0) {
        return NULL;
    }

    char* result = NULL;
    usize cursor = 0;
    for$arr(s, str_arr)
    {
        if (s == NULL) {
            return NULL;
        }
        usize slen = strlen(s);
        if (result == NULL) {
            result = mem$malloc(tmem$, slen + jlen + 1);
        } else {
            result = mem$realloc(tmem$, result, cursor + slen + jlen + 1);
        }
        if (result == NULL) {
            return NULL; // memory error
        }

        if (cursor > 0) {
            memcpy(&result[cursor], join_by, jlen);
            cursor += jlen;
        }

        memcpy(&result[cursor], s, slen);
        cursor += slen;
    }
    if (result) {
        result[cursor] = '\0';
    }

    return result;
}

const struct __module__str str = {
    // Autogenerated by CEX
    // clang-format off
    .sstr = str_sstr,
    .sbuf = str_sbuf,
    .eq = str_eq,
    .sub = str_sub,
    .copy = str_copy,
    .vsprintf = str_vsprintf,
    .sprintf = str_sprintf,
    .len = str_len,
    .find = str_find,
    .findr = str_findr,
    .starts_with = str_starts_with,
    .ends_with = str_ends_with,
    .fmt = str_fmt,
    .tfmt = str_tfmt,
    .tnew = str_tnew,
    .tcopy = str_tcopy,
    .tsplit = str_tsplit,
    .tjoin = str_tjoin,

    .slice = {  // sub-module .slice >>>
        .eq = str__slice__eq,
        .sub = str__slice__sub,
        .starts_with = str__slice__starts_with,
        .ends_with = str__slice__ends_with,
        .remove_prefix = str__slice__remove_prefix,
        .remove_suffix = str__slice__remove_suffix,
        .lstrip = str__slice__lstrip,
        .rstrip = str__slice__rstrip,
        .strip = str__slice__strip,
        .cmp = str__slice__cmp,
        .cmpi = str__slice__cmpi,
        .iter_split = str__slice__iter_split,
    },  // sub-module .slice <<<

    .convert = {  // sub-module .convert >>>
        .to_f32 = str__convert__to_f32,
        .to_f64 = str__convert__to_f64,
        .to_i8 = str__convert__to_i8,
        .to_i16 = str__convert__to_i16,
        .to_i32 = str__convert__to_i32,
        .to_i64 = str__convert__to_i64,
        .to_u8 = str__convert__to_u8,
        .to_u16 = str__convert__to_u16,
        .to_u32 = str__convert__to_u32,
        .to_u64 = str__convert__to_u64,
    },  // sub-module .convert <<<
    // clang-format on
};
