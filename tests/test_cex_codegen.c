#include "src/all.c"

test$case(test_codegen_test)
{
    sbuf_c b = sbuf.create(1024, mem$);
    cg$init(&b);

    tassert(cg$var->buf == &b);
    tassert(cg$var->indent == 0);

    cg$pn("printf(\"hello world\");");
    cg$pn("#define GOO");
    cg$pn("// this is empty scope");
    cg$scope("", "")
    {
        cg$pf("printf(\"hello world: %d\");", 2);
    }

    cg$func("void my_func(int arg_%d)", 2)
    {
        cg$scope("var my_var = (mytype)", "")
        {
            cg$pf(".arg1 = %d,", 1);
            cg$pf(".arg2 = %d,", 2);
        }
        cg$pa(";\n", "");

        cg$if("foo == %d", 312)
        {
            cg$pn("printf(\"Hello: %d\", foo);");
        }
        cg$elseif("bar == foo + %d", 7)
        {
            cg$pn("// else if scope");
        }
        cg$else()
        {
            cg$pn("// else scope");
        }

        cg$while("foo == %d", 312)
        {
            cg$pn("printf(\"Hello: %d\", foo);");
        }

        cg$for("u32 i = 0; i < %d; i++", 312)
        {
            cg$pn("printf(\"Hello: %d\", foo);");
            cg$foreach("it, my_var", "")
            {
                cg$pn("printf(\"Hello: %d\", foo);");
            }
        }

        cg$scope("do ", "")
        {
            cg$pf("// do while", 1);
        }
        cg$pa(" while(0);\n", "");
    }

    cg$switch("foo", "")
    {
        cg$case("'%c'", 'a')
        {
            cg$pn("// case scope");
        }
        cg$scope("case '%c': ", 'b')
        {
            cg$pn("fallthrough();");
        }
        cg$default()
        {
            cg$pn("// default scope");
        }
    }

    tassert(cg$is_valid());
    printf("result: \n%s\n", b);

    char c = 'a';

    switch (c) {
        case 'a': {
            printf("a\n");
            fallthrough();
        }
        case 'b': {
            printf("b\n");
            fallthrough();
        }
        default: {
            printf("default\n");
        }
    }

    sbuf.destroy(&b);
    return EOK;
}

test$main();
