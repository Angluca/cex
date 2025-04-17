#if defined(CEX_NEW)
#if __has_include("cex.c")
    #error "cex.c already exists, CEX project seems initialized, try run `gcc[clang] ./cex.c -o cex`"
#endif

int main(int argc, char** argv) {
    (void)argc;
    (void)argv;
    io.printf("Hello CEX_NEW\n");
    return 0;
}
#endif
