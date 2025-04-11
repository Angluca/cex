#include "src/all.c"

// test$setup_case() {return EOK;}
// test$teardown_case() {return EOK;}
// test$setup_suite() {return EOK;}
// test$teardown_suite() {return EOK;}

test$case(test_token_next_entity)
{
    // clang-format off
    char* code = 
        "#include <gfoo>\n"
        "// foo\n"
        "#define BAR 2\n"
        "#define ca$ma(a, b, c) 2\n"
        "#define ca$ma() 2\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);
        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__preproc);
        tassert_eq(arr$len(items), 1);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__macro_const);
        tassert_eq(arr$len(items), 2);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__macro_func);
        tassert_eq(arr$len(items), 1);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__macro_func);
        tassert_eq(arr$len(items), 1);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__eof);
        tassert_eq(arr$len(items), 0);
    }
    return EOK;
}

test$case(test_extern_vars)
{
    // clang-format off
    char* code = 
        "extern int \nglobal_var;\n"
        "extern int add(int a, int b);  // Optional 'extern'\n"
        "extern int __attribute__\n((visibility(\"hidden\"))) internal_var;\n"
        "int \nglobal_var = 2;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_decl);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 5);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_decl);
        tassert_eq(arr$len(items), 7);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: %s children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_def);
        tassert_eq(arr$len(items), 4);
    }
    return EOK;
}

test$case(test_extern_funcs)
{
    // clang-format off
    char* code = 
        "int add(int a, int b); \n"
        "int __attribute__\n((foo))\nglobal_var;\n"
        "extern int __attribute__\n((foo)) add(int a, int b); \n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__global_misc);
        tassert_eq(arr$len(items), 5);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d children: %ld\n%S\n", t.type, arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 7);
    }
    return EOK;
}

test$main();
