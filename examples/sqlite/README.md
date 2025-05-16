# Building SQLite Program From Source

1. This is minimalist `./cex.c` with all `cexy` CLI capabilities disabled by default
2. It works  on Linux, Windows and MacOS. (see [CI file](https://github.com/alexveden/cex/blob/master/.github/workflows/examples.yml))
3. It downloads and builds automatically `SQLite` source code and link it into the program (`sqlite` is shipped as bundled single file, or about 260k or lines (~8Mb)).

## Example highlights
* It's a minimalistic example of using CEX build system directly passing compiler arguments
* Uses system utilities as like shell script
* Uses conditional platform specific compiler flags
* Validates program changes (it tracks `src/main.c` and its includes). You may build with `./cex`, and rerun again, it then won't do anything. If you touch `src/main.c` or `lib/sqlite.h` this should trigger rebuild (only a program, but not SQLite).
* Compiles `SQLite` directly (BTW compilation is pretty fast, for single file 260k LOC)
```
[INFO]    ( cex.c:62 cmd_build_lib() ) Compilation time: sqlite3.c file size: 9016KB compiled in 30.3581 seconds
```
* `src/main.c` - shows error handling patterns for CEX, also resource management and cleanup. It uses minimalist argument parsing and error handling.
* All build settings and steps are defined in `./cex.c` file

## Getting started 

1. Download the source code
```
cd examples/sqlite
```
2. Make cexy build system  (only once) 
```
cc ./cex.c -o ./cex
```
3. Installing pre-requisites (on Linux and MacOS)
```
> sudo apt install curl unzip  # Linux
> brew install curl unzip      # MacOS
> pacman -S curl unzip         # For MSYS2 Windows
```
4. Compiling the project and SQLite
```
./cex
```
5. Running hello world
```
./build/hello_sqlite hello.db
```
6. Testing the result
```
./build/sqlite3 build/hello.db "SELECT * FROM Messages;"

>>> Expected
1|Hello, from CEX!
```

