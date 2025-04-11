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

test$case(test_funcs_decl_def)
{
    // clang-format off
    char* code = 
        "int add(int a, int b); \n"
        "int add(int a, int b) { return 0; } \n"
        "__attribute__((inline)) int add(int a, int b) { return 0; } \n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_def);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_def);
        tassert_eq(arr$len(items), 6);
    }
    return EOK;
}

test$case(test_struct_def)
{
    // clang-format off
    char* code = 
        "typedef struct cex_token_s { CexTkn_e type;  str_s value;} cex_token_s;\n"
        "struct cex_token_s { CexTkn_e type;  str_s value;};\n"
        "enum cex_token_s { A, B, C};\n"
        "union cex_token_s { int a; int b;};\n"
        "struct cex_token_s __attribute__((packed, aligned(16), used))) { CexTkn_e type;  str_s value;};\n"
        "struct cex_token_s my_foo(int a);\n"
        "struct B;\n"
        "struct cex_token_s { CexTkn_e type;  str_s value;} foo = {0};\n"
        "extern struct cex_token_s;\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 6);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 4);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 7);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 5);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__typedef);
        tassert_eq(arr$len(items), 3);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_def);
        tassert_eq(arr$len(items), 7);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__var_decl);
        tassert_eq(arr$len(items), 4);
    }
    return EOK;
}

test$case(test_cex_struct_def)
{
    // clang-format off
    char* code = 
        "__attribute__ ((visibility(\"hidden\"))) extern const struct __cex_namespace__io io; // CEX Autogen\n"
        "const struct __cex_namespace__io io = { };"
        "struct __cex_namespace__io { void            (*fclose)(FILE** file); };\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_decl);
        tassert_eq(arr$len(items), 8);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_def);
        tassert_eq(arr$len(items), 8);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__cex_module_struct);
        tassert_eq(arr$len(items), 4);
    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}


test$case(test_funcs_decl_parse)
{
    // clang-format off
    char* code = 
        "/** my doc */ int add(int a, int b); \n"
        "int add(int a, int b) { return 0; } \n"
        "static inline int add(int a, int b) { return 0; } \n"
        "__attribute__((foo)) int add2(int a, int b) { return 0; } \n"
        "extern int add2(int __attribute__((foo)) a, int restrict * b) { return 0; } \n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_decl);
        tassert_eq(arr$len(items), 5);


        cex_decl_s* d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_decl);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->body.buf, NULL);
        tassert_eq(d->docs, str$s("/** my doc */"));
        
        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->body, str$s("{ return 0; }"));
        tassert_eq(d->docs.buf, NULL);
        tassert_eq(d->is_static, false);
        tassert_eq(d->is_inline, false);

        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->body, str$s("{ return 0; }"));
        tassert_eq(d->docs.buf, NULL);
        tassert_eq(d->is_static, true);
        tassert_eq(d->is_inline, true);


        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__func_def);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add2"));
        tassert_eq(d->args, "int a, int b");
        tassert_eq(d->body, str$s("{ return 0; }"));
        tassert_eq(d->docs.buf, NULL);


        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__func_def);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add2"));
        tassert_eq(d->args, "int a, int* b");
        tassert_eq(d->body, str$s("{ return 0; }"));
        tassert_eq(d->docs.buf, NULL);

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_funcs_decl_parse_ret_type)
{
    // clang-format off
    char* code = 
        "int add(int a, int b) { return 0; } \n"
        "int * add(int a, int b) { return 0; } \n"
        "struct foo * add(int a, int b) { return 0; } \n"
        "void add() { return 0; } \n"
        "char * * add() { return 0; } \n"
        "const char * * add() { return 0; } \n"
        "[[nodiscard]] int add() { return 0; } \n"
        "noreturn int add() { return 0; } \n"
        "_Noreturn int add() { return 0; } \n"
        "__int128 add(int a, int b) { return 0; } \n"
        "__attribute__((foo)) int add(int a, int b) { return 0; } \n"
        "int (*func())() {}\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__func_def);

        cex_decl_s* d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");
        
        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int*");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "struct foo*");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "void");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "char**");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "const char**");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");

        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "__int128");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__func_def);
        tassert_eq(d->name, str$s("add"));
        tassert_eq(d->ret_type, "int");


        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d == NULL);

    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}

test$case(test_cex_list_functions)
{
    mem$scope(tmem$, _){
        char* code = io.file.load("src/io.c", _);
        CexParser_c lx = CexParser_create(code, 0, true);
        cex_token_s t;
        arr$(cex_token_s) items = arr$new(items, _);

        while((t = CexParser_next_entity(&lx, &items)).type){
            cex_decl_s* d = CexParser_decl_parse(t, items, _);
            if (d != NULL) {
                log$debug("Decl: type: '%s' name: %S ret: '%s' args: '%s'\n", CexTkn_str[t.type], d->name, d->ret_type, d->args);
            }
        }
        // tassert(false);
    }
    return EOK;
}

test$case(test_funcs_decl_parse_macros)
{
    // clang-format off
    char* code = 
        "/** my doc */ \n#define my$macro() asdadkja \n"
        "/// my doc \n#define my_macro(a, b, ...) a+b\n"
        "#define my$CONST 220981\n"
        "";
    CexParser_c lx = CexParser_create(code, 0, true);
    cex_token_s t;
    mem$scope(tmem$, _){
        arr$(cex_token_s) items = arr$new(items, _);

        t = CexParser_next_entity(&lx, &items);
        log$debug("Entity:  type: %d type_str: '%s' children: %ld\n%S\n", t.type, CexTkn_str[t.type], arr$len(items), t.value);
        tassert_eq(t.type, CexTkn__macro_func);


        cex_decl_s* d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_func);
        tassert_eq(d->name, str$s("my$macro"));
        tassert_eq(d->args, "");
        tassert_eq(d->body.buf, NULL);
        tassert_eq(d->docs, str$s("/** my doc */"));
        
        t = CexParser_next_entity(&lx, &items);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_func);
        tassert_eq(d->name, str$s("my_macro"));
        tassert_eq(d->args, "a, b, ...");
        tassert_eq(d->body.buf, NULL);
        tassert_eq(d->docs, str$s("/// my doc "));


        t = CexParser_next_entity(&lx, &items);
        tassert_eq(t.type, CexTkn__macro_const);
        d = CexParser_decl_parse(t, items, _);
        tassert(d != NULL);
        tassert_eq(d->type, CexTkn__macro_const);
        tassert_eq(d->name, str$s("my$CONST"));
        tassert_eq(d->args, NULL);
        tassert_eq(d->body.buf, NULL);
        tassert_eq(d->docs.buf, NULL);


    }
    tassert_eq(CexParser_next_token(&lx).type, CexTkn__eof);
    return EOK;
}
test$main();
