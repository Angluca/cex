#include <cex/all.c>
#include <cex/cex_parser.c>

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

typedef struct token_cmp_s
{
    char* code;
    char* exp_token;
    CexTkn_e type;
} token_cmp_s;

test$case(test_token_ident)
{
    char* code = " \t foo ";
    CexLexer_c lx = CexLexer_create(code, 0, false);
    tassert(lx.content == code);
    tassert(lx.cur == code);
    tassert(lx.content_end == code + strlen(code));
    tassert_eqi(lx.line, 0);
#define end_brace }

    char* some_brace = "}";
    (void)some_brace;
    // some stuff }
    cex_token_s t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 0);
    tassert_eqi(t.type, CexTkn__ident); /* multiline }        */
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);
#undef end_brace

    t = CexLexer_next_token(&lx);
    tassert_eqi(t.type, CexTkn__eof);

    return EOK;
}


test$case(test_token_new_line_skip)
{
    char* code = " \nfoo ";
    CexLexer_c lx = CexLexer_create(code, 0, false);

    cex_token_s t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 1);
    tassert_eqi(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);
    /* multiline */

    t = CexLexer_next_token(&lx);
    tassert_eqi(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_new_empty)
{
    char* code = "";
    CexLexer_c lx = CexLexer_create(code, 0, false);

    cex_token_s t = CexLexer_next_token(&lx);
    t = CexLexer_next_token(&lx);
    tassert_eqi(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_two_indents)
{
    char* code = " \nfoo _Bar$s(";
    CexLexer_c lx = CexLexer_create(code, 0, false);

    cex_token_s t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 1);
    tassert_eqi(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);

    t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 1);
    tassert_eqi(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("_Bar$s")), "t.value=%S", t.value);
    tassert_eqi(*lx.cur, '(');
    tassert(lx.cur == lx.content_end - 1);

    t = CexLexer_next_token(&lx);
    tassert_eqi(t.type, CexTkn__lparen);
    tassertf(str.slice.eq(t.value, str$s("(")), "t.value='%S'", t.value);
    tassert_eqi(*lx.cur, '\0');
    tassert(lx.cur == lx.content_end);

    t = CexLexer_next_token(&lx);
    tassert_eqi(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_numbers)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { " 0", "0", CexTkn__number },
        { "  0  ", "0", CexTkn__number },
        { "0", "0", CexTkn__number },
        { "1234567890", "1234567890", CexTkn__number },
        { "1234567890   !  ", "1234567890", CexTkn__number },
        { "0x1234567890abcdefABCDEF   !  ", "0x1234567890abcdefABCDEF", CexTkn__number },
        { "0X1234567890abcdefABCDEF   !  ", "0X1234567890abcdefABCDEF", CexTkn__number },
        { "0b01010101   !  ", "0b01010101", CexTkn__number },
        { "0.12", "0.12", CexTkn__number },
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexLexer_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf < it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_string)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { " \"hello world\" ", "hello world", CexTkn__string },
        { " \"hello \\\"world\" ", "hello \\\"world", CexTkn__string },
        { " \"hello \\world\" ", "hello \\world", CexTkn__string },
        { " 'h' ", "h", CexTkn__char },
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexLexer_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf < it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_comment)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { " // hello comment", "// hello comment", CexTkn__comment_single },
        { " // hello comment\n", "// hello comment", CexTkn__comment_single },
        { " /* hello */ foo", "/* hello */", CexTkn__comment_multi },
        { " /* hello\n bar */ foo", "/* hello\n bar */", CexTkn__comment_multi },
        { " /** hello */ foo", "/** hello */", CexTkn__comment_multi },
        { " /* hello */\n foo", "/* hello */", CexTkn__comment_multi },
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexLexer_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf < it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_comment_and_ident)
{
    char* code = " \n /* comm */foo";
    CexLexer_c lx = CexLexer_create(code, 0, false);

    cex_token_s t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 1);
    tassert_eqi(t.type, CexTkn__comment_multi);
    tassertf(str.slice.eq(t.value, str$s("/* comm */")), "t.value=%S", t.value);

    t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 1);
    tassert_eqi(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);

    t = CexLexer_next_token(&lx);
    tassert_eqi(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_preproc)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "#include <foo.h>", "include <foo.h>", CexTkn__preproc },
        { "#   include <foo.h>", "include <foo.h>", CexTkn__preproc },
        { "   #   include <foo.h>", "include <foo.h>", CexTkn__preproc },
        { "#", "", CexTkn__preproc },
        { "   #   include <foo.h> \n bar", "include <foo.h> ", CexTkn__preproc },
        { "   #   include \"foo.h\"", "include \"foo.h\"", CexTkn__preproc },
        { "   #   define my$foo(baz, bar) bar", "define my$foo(baz, bar) bar", CexTkn__preproc },
        { "   #   define my$foo(baz, bar) bar\\another\nstop",
          "define my$foo(baz, bar) bar\\another",
          CexTkn__preproc },
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexLexer_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf <= it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_brace_parens_brackets)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "{{", "{", CexTkn__lbrace },   { "}", "}", CexTkn__rbrace },
        { "[[", "[", CexTkn__lbracket }, { "]", "]", CexTkn__rbracket },
        { "((", "(", CexTkn__lparen },   { ")", ")", CexTkn__rparen },
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexLexer_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf <= it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_paren_block)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "{ foo }", "{ foo }", CexTkn__brace_block },
        { "[ foo ]", "[ foo ]", CexTkn__bracket_block },
        { "( foo )", "( foo )", CexTkn__paren_block },
        { "{( foo )}", "{( foo )}", CexTkn__brace_block },
        { "[{ foo }]", "[{ foo }]", CexTkn__bracket_block },
        { "([ foo ])", "([ foo ])", CexTkn__paren_block },
        { "{}", "{}", CexTkn__brace_block },
        { "[]", "[]", CexTkn__bracket_block },
        { "()", "()", CexTkn__paren_block },
        { "{\n}", "{\n}", CexTkn__brace_block },
        { "[\n]", "[\n]", CexTkn__bracket_block },
        { "(\n)", "(\n)", CexTkn__paren_block },
        { "{ foo ", NULL, CexTkn__error },
        { "{ ((foo }", NULL, CexTkn__error },
        { "{ foo ]", NULL, CexTkn__error },
        { "( foo }", NULL, CexTkn__error },
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0, true);
        tassert(*lx.cur);
        cex_token_s t = CexLexer_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(
            t.value.buf == NULL ||
            (t.value.buf >= it.code && t.value.buf <= it.code + str.len(it.code))
        );
    }

    return EOK;
}

test$case(test_token_paren_block_overflow_test)
{
    char buf[2048] = { 0 };
    char* pairs[] = { "[]", "{}", "()" };

    for$each(p, pairs)
    {
        memset(buf, 0, sizeof(buf));
        for$eachp(cp, buf)
        {
            usize idx = cp - buf;
            if (idx < sizeof(buf) / 2) {
                *cp = p[0];
            } else {
                *cp = p[1];
            }
        }
        buf[arr$len(buf) - 1] = '\0';
        CexLexer_c lx = CexLexer_create(buf, 0, true);
        cex_token_s t = CexLexer_next_token(&lx);
        tassert_eqi(t.type, CexTkn__error);
        tassert(t.value.buf == NULL);
        tassert(t.value.len == 0);
    }

    return EOK;
}

test$case(test_token_misc)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "*", "*", CexTkn__star }, { ".", ".", CexTkn__dot },   { ",", ",", CexTkn__comma },
        { ";", ";", CexTkn__eos },  { ":", ":", CexTkn__colon }, { "?", "?", CexTkn__question },
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexLexer_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf <= it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_secondary_token)
{
    // <code>, <token txt>, <token_type>
    token_cmp_s tokens[] = {
        { "main.", ".", CexTkn__dot },      { "234*", "*", CexTkn__star },
        { "main*", "*", CexTkn__star },     { "main(", "(", CexTkn__lparen },
        { "main[", "[", CexTkn__lbracket },
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0, false);
        tassert(*lx.cur);
        cex_token_s t = CexLexer_next_token(&lx);
        tassert(t.type);
        // fetch second
        t = CexLexer_next_token(&lx);
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "code='%s', val_exp='%s' val_value='%S'",
            it.code,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= it.code && t.value.buf <= it.code + strlen(it.code));
    }

    return EOK;
}

test$case(test_token_code)
{
    // clang-format off
    char* code = 
        "#include <gfoo>\n"
        "int main(char** argv) {\n"
        "    printf(\"hello\"); \n"
        "}";
    // clang-format on
    token_cmp_s tokens[] = {
        { NULL, "include <gfoo>", CexTkn__preproc },
        { NULL, "int", CexTkn__ident },
        { NULL, "main", CexTkn__ident },
        { NULL, "(char** argv)", CexTkn__paren_block },
        { NULL, "{\n    printf(\"hello\"); \n}", CexTkn__brace_block },
    };
    CexLexer_c lx = CexLexer_create(code, 0, true);
    cex_token_s t;
    u32 nit = 0;
    for$each(it, tokens)
    {
        t = CexLexer_next_token(&lx);
        io.printf("step: %d t.type: %d t.value: '%S'\n", nit, t.type, t.value);
        tassert(t.type);
        tassertf(t.type == it.type, "step: %d type_exp=%d, type_act=%d", nit, it.type, t.type);
        tassertf(
            str.slice.eq(t.value, str.sstr(it.exp_token)),
            "step: %d val_exp='%s' val_value='%S'",
            nit,
            it.exp_token,
            t.value
        );
        tassert(t.value.buf >= code && t.value.buf <= code + strlen(code));
        nit++;
    }
    t = CexLexer_next_token(&lx);
    tassertf(
        t.type == CexTkn__eof,
        "step: %d type_exp=%d, t.type=%d t.value='%S'\n",
        nit,
        CexTkn__eof,
        t.type,
        t.value
    );
    return EOK;
}

test$case(test_token_this_file)
{
    char* code = io.file.load(__FILE__, mem$);
    CexLexer_c lx = CexLexer_create(code, 0, true);
    cex_token_s t;
    u32 nit = 0;
    char* prev_tok = lx.cur;
    while ((t = CexLexer_next_token(&lx)).type) {
        /* NOTE:
         * Tokenize this file, and make sure that all test functions are parsed correctly
        */
        tassertf(t.type != CexTkn__unk, "step: %d t.type: %d t.value: '%S'\n", nit, t.type, t.value);
        tassertf(t.type != CexTkn__error, "step: %d token error starts at: ...\n%s\n", prev_tok);
        if (t.type == CexTkn__brace_block) {
            tassert(str.slice.starts_with(t.value, str$s("{")));
            tassert(str.slice.ends_with(t.value, str$s("}")));
        } 
        if (t.type == CexTkn__paren_block) {
            tassert(str.slice.starts_with(t.value, str$s("(")));
            tassert(str.slice.ends_with(t.value, str$s(")")));
        } 
        if (t.type == CexTkn__bracket_block) {
            tassert(str.slice.starts_with(t.value, str$s("[")));
            tassert(str.slice.ends_with(t.value, str$s("]")));
        } 
        nit++;
        prev_tok = lx.cur;
    }
    mem$free(mem$, code);
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
    
    test$run(test_token_ident);
    test$run(test_token_new_line_skip);
    test$run(test_token_new_empty);
    test$run(test_token_two_indents);
    test$run(test_token_numbers);
    test$run(test_token_string);
    test$run(test_token_comment);
    test$run(test_token_comment_and_ident);
    test$run(test_token_preproc);
    test$run(test_token_brace_parens_brackets);
    test$run(test_token_paren_block);
    test$run(test_token_paren_block_overflow_test);
    test$run(test_token_misc);
    test$run(test_token_secondary_token);
    test$run(test_token_code);
    test$run(test_token_this_file);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
