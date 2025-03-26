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
    CexLexer_c lx = CexLexer_create(code, 0);
    tassert(lx.content == code);
    tassert(lx.cur == code);
    tassert(lx.content_end == code + strlen(code));
    tassert_eqi(lx.line, 0);

    cex_token_s t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 0);
    tassert_eqi(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);

    t = CexLexer_next_token(&lx);
    tassert_eqi(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_new_line_skip)
{
    char* code = " \nfoo ";
    CexLexer_c lx = CexLexer_create(code, 0);

    cex_token_s t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 1);
    tassert_eqi(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);

    t = CexLexer_next_token(&lx);
    tassert_eqi(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_new_empty)
{
    char* code = "";
    CexLexer_c lx = CexLexer_create(code, 0);

    cex_token_s t = CexLexer_next_token(&lx);
    t = CexLexer_next_token(&lx);
    tassert_eqi(t.type, CexTkn__eof);

    return EOK;
}

test$case(test_token_two_indents)
{
    char* code = " \nfoo _Bar$s!";
    CexLexer_c lx = CexLexer_create(code, 0);

    cex_token_s t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 1);
    tassert_eqi(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("foo")), "t.value=%S", t.value);

    t = CexLexer_next_token(&lx);
    tassert_eqi(lx.line, 1);
    tassert_eqi(t.type, CexTkn__ident);
    tassertf(str.slice.eq(t.value, str$s("_Bar$s")), "t.value=%S", t.value);

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
        CexLexer_c lx = CexLexer_create(it.code, 0);
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
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0);
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
    };
    for$each(it, tokens)
    {
        CexLexer_c lx = CexLexer_create(it.code, 0);
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
    CexLexer_c lx = CexLexer_create(code, 0);

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


/*
 *
 * MAIN (AUTO GENERATED)
 *
 */
int
main(int argc, char* argv[])
{
    test$args_parse(argc, argv);
    test$print_header(); // >>> all tests below

    test$run(test_token_ident);
    test$run(test_token_new_line_skip);
    test$run(test_token_new_empty);
    test$run(test_token_two_indents);
    test$run(test_token_numbers);
    test$run(test_token_string);
    test$run(test_token_comment);
    test$run(test_token_comment_and_ident);

    test$print_footer(); // ^^^^^ all tests runs are above
    return test$exit_code();
}
