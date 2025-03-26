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

typedef struct token_cmp_s {
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
    tassert_eqi(lx.content_len, strlen(code));
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
        {" 0", "0", CexTkn__number},
        {"  0  ", "0", CexTkn__number},
        {"0", "0", CexTkn__number},
    };
    for$each(it, tokens) {
        CexLexer_c lx = CexLexer_create(it.code, 0);
        tassert(*lx.cur);
        cex_token_s t = CexLexer_next_token(&lx);    
        tassertf(t.type == it.type, "code='%s', type_exp=%d, type_act=%d", it.code, it.type, t.type);
        tassertf(str.slice.eq(t.value, str.sstr(it.exp_token)), "code='%s', val_exp='%s' val_value='%S'", it.code, it.exp_token, t.value);
        tassert(t.value.buf >= it.code && t.value.buf < it.code + strlen(it.code));
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
    
    test$run(test_token_ident);
    test$run(test_token_new_line_skip);
    test$run(test_token_new_empty);
    test$run(test_token_two_indents);
    test$run(test_token_numbers);
    
    test$print_footer();  // ^^^^^ all tests runs are above
    return test$exit_code();
}
