#include <cex/all.c>

test$case(test_codegen_test)
{
    sbuf_c b = sbuf.create(1024, mem$);
    cg$init(&b);

    tassert(cg$var->buf == &b);
    tassert(cg$var->indent == 0);

    $pn("printf(\"hello world\");");
    $pn("#define GOO");
    $pn("// this is empty scope");
    $scope("", "")
    {
        $pf("printf(\"hello world: %d\");", 2);
    }

    $func("void my_func(int arg_%d)", 2)
    {
        $scope("var my_var = (mytype)", "")
        {
            $pf(".arg1 = %d,", 1);
            $pf(".arg2 = %d,", 2);
        }
        $pa(";\n", "");

        $if("foo == %d", 312)
        {
            $pn("printf(\"Hello: %d\", foo);");
        }
        $elseif("bar == foo + %d", 7)
        {
            $pn("// else if scope");
        }
        $else()
        {
            $pn("// else scope");
        }

        $while("foo == %d", 312)
        {
            $pn("printf(\"Hello: %d\", foo);");
        }

        $for("u32 i = 0; i < %d; i++", 312)
        {
            $pn("printf(\"Hello: %d\", foo);");
            $foreach("it, my_var", "")
            {
                $pn("printf(\"Hello: %d\", foo);");
            }
        }

        $scope("do ", "")
        {
            $pf("// do while", 1);
        }
        $pa(" while(0);\n", "");
    }

    $switch("foo", "")
    {
        $case("'%c'", 'a')
        {
            $pn("// case scope");
        }
        $scope("case '%c': ", 'b')
        {
            $pn("fallthrough();");
        }
        $default()
        {
            $pn("// default scope");
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
