# Building Lua + Lua Module in CEX

1. This example project is standalone project with its own `./cex.c` build commands
2. It's tested on Linux, Windows and MacOS.
3. It builds automatically Lua from source and makes a module for Lua import.

## CEX Capabilities used
* Loading Lua sources via git
* Parsing Lua makefile for C compiler flags
* Parsing Lua makefile for C compiler flags
* Conditional compilation for Lua files (only changed files trigger re-compilation)
* Conditional cross-platform compiler/linker flags
* Creating `compile_flags.txt` for clang LSP work (including Lua sources from git).
* Unit testing of library logic on host without Lua itself

## Getting started 

1. Download the source code
```
cd examples/lua_module
```
3. Make cexy build system  (only once) 
```
cc ./cex.c -o ./cex
```
4. Installing pre-requisites (on Linux and MacOS)
```
> sudo apt install libreadline-dev
> brew install readline
```
5. Compiling Lua console
```
./cex build-lua
```
6. Compiling example module for Lua
```
./cex build-lib
```
7. Running example module in Lua
```
./cex test-lib
```

## Manual launch (after build)
1. Lua executable is placed in `examples/lua_module/build/lua/lua[.exe]`
1. The module should be nearby the Lua file, i.e. `examples/lua_module/build/lua/mylualib[.so|dll]`
1. ```cd examples/lua_module/build/lua/```
1. ```./lua```
1. Enter the following script:
```lua

local mylualib = require "mylualib" 
print(mylualib.myfunction(3, 4))
print(mylualib.version)

-- Expected output
-- 7.0
-- 1.0-cex
```

