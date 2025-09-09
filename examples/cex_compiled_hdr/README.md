# Improving CEX compiling times

When building CEX it takes about 0.5s to build `Hello, world` app without sanitizer, and 1.5s to build with ASAN enabled. This may be a bottleneck for development workflow. To improve compilation times it's possible to compile `cex.h` as `obj` file and link it. 

This example, illustrates the concept of linking `cex.obj` for speed-up, take following steps:
1. Add special pre-build step in `cex.c` (see `prebuild_cex()` function)
2. Eliminate `#define CEX_IMPLEMENTATION` in `src/myapp.c`
3. Eliminate `#define CEX_IMPLEMENTATION` in `tests/test_myapp.c`
4. Consider using `-DCEX_IMPLEMENTATION` as compiler flag for compiling the program.

> [!NOTE]
> 
> CEX was designed with unity-build approach in mind, which requires all `cex.h` source code to be exposed to the compiler at compilation stage. When we link `cex.obj` we may add performance overhead for namespace functions, i.e. `io.printf()` call will lead to additional pointer dereferencing to take address of a function.
>
> It's recommended to use pre-compiled `cex.obj` only in debug builds, use conventional approach in release builds.


