#if !defined(cex$enable_minimal)

#if defined(CEX_NEW)
#    if __has_include("cex.c")
#        error                                                                                     \
            "cex.c already exists, CEX project seems initialized, try run `gcc[clang] ./cex.c -o cex`"
#    endif

int
main(int argc, char** argv)
{
    (void)argc;
    (void)argv;
    e$except (err, cexy.utils.make_new_project(".")) { return 1; }
    io.printf("\n\nMOCCA - Make Old C Cexy Again!\n");
    io.printf("Cex project has been initialized!\n");
    io.printf("See the 'cex.c' building script for more info.\n");
    io.printf("\nNow you can use `./cex` command to do stuff:\n");
    io.printf("- `./cex --help` for getting CEX capabilities\n");
    io.printf("- `./cex test run all` for running all tests\n");
    io.printf("- `./cex app run myapp` for running sample app\n");
    io.printf("- `./cex help --help` for searching projec symbols and examples\n");
    return 0;
}
#endif

#endif
