# CEX.C - Comprehensively EXtended C Language
MOCCA - Make Old C Cexy Again!
![Test suite](https://github.com/alexveden/cex/actions/workflows/main.yml/badge.svg)


CEX is self-contained C language extension, the only dependency is one of gcc/clang compilers.
cex.h contains build system, unit test runner, small standard lib and help system.

## GETTING STARTED

### Existing project (when cex.c exists in the project root directory)
```
1. > cd project_dir
2. > gcc/clang ./cex.c -o ./cex     (need only once, then cex will rebuild itself)
3. > ./cex --help                   get info about available commands
```

### New project / bare cex.h file

1. download [cex.h](https://raw.githubusercontent.com/alexveden/cex/refs/heads/master/cex.h)
```
// 2. Make a project directory 
mkdir project_dir
cd project_dir

// 3. Make a seed program (NOTE: passing header file is ok)
gcc -D CEX_NEW -x c ./cex.h
clang -D CEX_NEW -x c ./cex.h

// 4. Run cex program for project initilization
./cex

// 5. Now your project is ready to go 
./cex test run all
./cex app run myapp
```

## cex tool usage:
```
> ./cex --help
Usage:
cex  [-D] [-D<ARG1>] [-D<ARG2>] command [options] [args]

CEX language (cexy$) build and project management system

help                Search cex.h and project symbols and extract help
process             Create CEX namespaces from project source code
new                 Create new CEX project
config              Check project and system environment and config
test                Test running
app                 App runner

You may try to get help for commands as well, try `cex process --help`
Use `cex -DFOO -DBAR config` to set project config flags
Use `cex -D config` to reset all project config flags to defaults
```

## Supported compilers / platforms
CEX is designed as a stand-alone, single header programming language, without dependencies except gcc/clang compiler.


### Tested compilers
- GCC: 10, 11, 12, 13, 14, 15
- Clang: 13, 14, 15, 16, 17, 18, 19, 20
- MSVC - unsupported, probably never will

### Tested platforms / arch
- Linux - x32 / x64 (gcc + clang)
- Windows (via MSYS2 build) - x64 (mingw64 + clang)
- Macos - x64 / arm64 (clang)

### Test suite
CEX is tested on various platforms, compiler versions, sanitizers, and optimization flags, ensuring future compatibility and stability. Sanitizers verify the absence of memory leaks, buffer overflows, and undefined behavior. Additionally, tests with release flags confirm that compiler optimizations do not interfere with the code logic.

| OS / Build type   |      UBSAN      |  ASAN | Release -O3 | Release -NDEBUG -O2 |
|:----------|:-------------:|:------:| :-----: | :-----: |
| Linux Ubuntu 2204 x64 |  ✅ | ✅ |✅ |✅ |
| Linux Ubuntu 2404 x64 |  ✅ | ✅ |✅ |✅ |
| Linux Ubuntu 2404 x32 |  ✅ | ✅ |✅ |✅ |
| Windows 2019 (Win10) x64 |  ✅ | ✅ |✅ |✅ |
| Windows 2022 (Win10) x64 |  ✅ | ✅ |✅ |✅ |
| Windows 2025 (Win11) x64 |  ✅ | ✅ |✅ |✅ |
| MacOS 13 x64 |  ✅ | ✅ |✅ |✅ |
| MacOS 14 arm64 |  ✅ | ✅ |✅ |✅ |
| MacOS 15 arm64 |  ✅ | ✅ |✅ |✅ |


## Licence
>    MIT License 2023-2025 (c) Alex Veden
>
>    https://github.com/alexveden/cex/
