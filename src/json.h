#pragma once
#include "all.h"

#ifndef CEX_MAX_JSON_DEPTH
#    define CEX_MAX_JSON_DEPTH 128
#endif

/// Beginning of json$key_match chain (always first, checks if key is NULL)
#define json$key_invalid(json_iter) if (unlikely((json_iter)->key.buf == NULL))

/// Matches object key by key literal name (compile time optimized string compare)
#define json$key_match(json_iter, key_literal)                                                     \
    else if ((json_iter)->key.len == sizeof(key_literal) - 1 &&                                    \
             memcmp((json_iter)->key.buf, key_literal, sizeof(key_literal) - 1) == 0)

/// Checks if there is unexpected key
#define json$key_unmatched(json_iter) else

typedef enum JsonType_e
{
    JsonType__eos = -2, // end of scope (after json.iter.step_in())
    JsonType__err = -1, // data/integrity error
    JsonType__eof = 0,  // end of file reached
    JsonType__str,      // quoted string
    JsonType__obj,      // {key: value} object
    JsonType__arr,      // [ ... ] array
    JsonType__num,      // floating point num
    JsonType__bool,     // boolean
    JsonType__null,     // null (obj/arr)
    // -----------
    JsonType__cnt,
} JsonType_e;


typedef struct json_iter_c
{
    str_s val;       // string value of the json item
    str_s key;       // associated key of the val (if inside object)
    JsonType_e type; // current json item type (also error, EOF, end-of-scope indication)
    Exc error;       // last iterator error

    struct
    {
        CexParser_c lexer;   // JSON lexer
        bool strict_mode;    // enforces JSON compliant spec
        bool has_items;      // flag is set when at least one item processed in scope
        CexTkn_e prev_token; // JSON previous token
        CexTkn_e curr_token; // JSON current token
        u32 scope_depth;
        char scope_stack[CEX_MAX_JSON_DEPTH];
    } _impl;

} json_iter_c;

typedef struct json_buf_c
{
    sbuf_c buf;
    u32 indent;
    u32 indent_width;
    Exc error;
    u32 scope_depth;
    char scope_stack[CEX_MAX_JSON_DEPTH];
} json_buf_c;

#define _json$buf_var _json_buf_macro_scope

/// Opens JSON buffer scope (json_buf_ptr data is cleared out)
#define json$buf(json_buf_ptr, jsontype_arr_or_obj)                                                \
    _cex_json__buf__clear((json_buf_ptr));                                                         \
    for (json_buf_c * _json$buf_var __attribute__((__cleanup__(_cex__jsonbuf_print_scope_exit))) = \
             _cex__jsonbuf_print_scope_enter((json_buf_ptr), jsontype_arr_or_obj),                 \
                                    *cex$tmpname(jsonbuf_sentinel) = _json$buf_var;                \
         cex$tmpname(jsonbuf_sentinel) && _json$buf_var != NULL;                                   \
         cex$tmpname(jsonbuf_sentinel) = NULL)


/// Add new key: {...} scope into (json$buf)
#define json$kobj(key)                                                                             \
    _cex_json__buf__print(_json$buf_var, "\"%s\": ", key);                                         \
    json$obj()

/// Add new key: [...] scope into (json$buf)
#define json$karr(key)                                                                             \
    _cex_json__buf__print(_json$buf_var, "\"%s\": ", key);                                         \
    json$arr()

/// Add new {...} scope into (json$buf)
#define json$obj()                                                                                 \
    for (json_buf_c * cex$tmpname(jsonbuf_scope)                                                   \
                          __attribute__((__cleanup__(_cex__jsonbuf_print_scope_exit))) =           \
             _cex__jsonbuf_print_scope_enter(_json$buf_var, JsonType__obj),                        \
                          *cex$tmpname(jsonbuf_sentinel) = cex$tmpname(jsonbuf_scope);             \
         cex$tmpname(jsonbuf_sentinel) && cex$tmpname(jsonbuf_scope) != NULL;                      \
         cex$tmpname(jsonbuf_sentinel) = NULL)

/// Add new [...] scope into (json$buf)
#define json$arr()                                                                                 \
    for (json_buf_c * cex$tmpname(jsonbuf_scope)                                                   \
                          __attribute__((__cleanup__(_cex__jsonbuf_print_scope_exit))) =           \
             _cex__jsonbuf_print_scope_enter(_json$buf_var, JsonType__arr),                        \
                          *cex$tmpname(jsonbuf_sentinel) = cex$tmpname(jsonbuf_scope);             \
         cex$tmpname(jsonbuf_sentinel) && cex$tmpname(jsonbuf_scope) != NULL;                      \
         cex$tmpname(jsonbuf_sentinel) = NULL)

/// Append any formatted string, it's for low level printing (json$buf)
#define json$fmt(format, ...) _cex_json__buf__print(_json$buf_var, format, __VA_ARGS__)

/// Append string item into array scope (json$buf)
#define json$str(format, ...)                                                                      \
    _cex_json__buf__print_item(_json$buf_var, "\"" format "\"", __VA_ARGS__)

/// Append value item into array scope (json$buf)
#define json$val(format, ...) _cex_json__buf__print_item(_json$buf_var, format "", __VA_ARGS__)

/// Append `"<key>": "<format>"` (with your own format) into object scope (json$buf)
#define json$kstr(key, format, ...)                                                                \
    _cex_json__buf__print_key(_json$buf_var, (key), "\"" format "\"", __VA_ARGS__)

/// Append `"<key>": <format>` into object scope (json$buf)
#define json$kval(key, format, ...)                                                                \
    _cex_json__buf__print_key(_json$buf_var, (key), format, __VA_ARGS__)

void _cex_json__buf__clear(json_buf_c* jb);
void _cex_json__buf__print(json_buf_c* jb, char* format, ...);
void _cex_json__buf__print_item(json_buf_c* jb, char* format, ...);
void _cex_json__buf__print_key(json_buf_c* jb, char* key, char* format, ...);
json_buf_c* _cex__jsonbuf_print_scope_enter(json_buf_c* jb, JsonType_e scope_type);
void _cex__jsonbuf_print_scope_exit(json_buf_c** jbptr);


/**
Low level JSON reader/writer namespace

Making own JSON buffer:

```c
json_buf_c jb;
e$ret(json.buf.create(&jb, 1024, 0, mem$));
json$buf(&jb, JsonType__obj)
{
    json$kstr("foo2", "%d", 1);
    json$kobj("foo3") {
        json$kval("bar", "%s", "3");
    }
    json$karr("foo3") {
        json$val("%d", 8);
        json$str("%d", 9);
    }
}
>> json.buf.get(&jb) ->
>> {"foo2": "1", "foo3": {"bar": 3},"foo3": [8, "9"]}


Reading JSON buffer:
```c
    struct Foo
    {
        struct { u32 baz; u32 fuzz; } foo;
        u32 next;
        u32 baz;
    } data = { 0 };
    str_s content = str$s(
        "{ \"foo\" : {\"baz\": 3, \"fuzz\": 8, \"oops\": 0}, \"next\": 7, \"baz\": 17 }"
    );
    json_iter_c js;
    e$ret(json.iter.create(&js, content.buf, 0, false));
    if (json.iter.next(&js)) { e$ret(json.iter.step_in(&js, JsonType__obj)); }
    while (json.iter.next(&js)) {
        json$key_invalid (&js) {}
        json$key_match (&js, "foo") {
            e$ret(json.iter.step_in(&js, JsonType__obj));
            while (json.iter.next(&js)) {
                json$key_invalid (&js) {}
                json$key_match (&js, "fuzz") { e$ret(str$convert(js.val, &data.foo.fuzz)); }
                json$key_match (&js, "baz") { e$ret(str$convert(js.val, &data.foo.baz)); }
                json$key_unmatched(&js)
                {
                    tassert_eq(js.key, str$s("oops"));
                }
            }
        }
        json$key_match (&js, "next") { e$ret(str$convert(js.val, &data.next)); }
        json$key_match (&js, "baz") { e$ret(str$convert(js.val, &data.baz)); }
    }
    e$assert(js.error == EOK && "No parsing errors");

```

*/
struct __cex_namespace__json
{
    // Autogenerated by CEX
    // clang-format off


    struct {
        /// Create JSON buffer/builder container used with json$buf / json$fmt / json$kstr macros
        Exception       (*create)(json_buf_c* jb, u32 capacity, u8 indent, IAllocator allc);
        /// Destroy JSON buffer instance (not necessary to call if initialized on tmem$ allocator)
        void            (*destroy)(json_buf_c* jb);
        /// Get JSON buffer contents (NULL if any error occurred)
        char*           (*get)(json_buf_c* jb);
        /// Check if there is any error in JSON buffer
        Exception       (*validate)(json_buf_c* jb);
    } buf;

    struct {
        /// Create new JSON reader (it doesn't allocate memory and uses content slicing)
        Exception       (*create)(json_iter_c* it, char* content, usize content_len, bool strict_mode);
        /// Get next JSON item for a scope
        bool            (*next)(json_iter_c* it);
        /// Make step inside JSON object or array scope (json.iter.next() starts emitting this scope)
        Exception       (*step_in)(json_iter_c* it, JsonType_e expected_type);
        /// Early step out from JSON scope (you must immediately break the loop/func after step out)
        Exception       (*step_out)(json_iter_c* it);
    } iter;

    // clang-format on
};
__attribute__((visibility("hidden"))) extern const struct __cex_namespace__json json;
