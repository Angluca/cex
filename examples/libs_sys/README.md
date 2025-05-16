# Building with system libraries

This example illustrates how to build programs using system-wide libraries. It contains two simple programs: `cex_download` downloads `SQLite` zip source from the internet using `curl` library functions, and `cex_unzip` extracts its contents into build directory (using  `libzip`). 
 
`cexy` build system supports two ways of dealing with package dependencies:
* Using `pkg-config` on system libs
* Using `pkg-config` on `vcpkg` repository


## Example highlights
* It works  on Linux, Windows and MacOS. (see [CI file](https://github.com/alexveden/cex/blob/master/.github/workflows/examples.yml))
* Linux and MacOS use system-wide libs, Windows uses `vcpkg` (system-wide)
* Libs list are set in `#define cexy$pkgconf_libs` 
* `cex_unzip` program is built using custom build command `cmd_build_unzip()` in `cex.c`
* `cex_downloader` program is built using standard cexy build routine (Note: to see its source run `./cex help --source cexy.cmd.simple_app` in your terminal)
* Using `./cex help` on other code (e.g. `SQLite` sources)
* All build settings and steps are defined in `./cex.c` file


## Getting started 

1. Download the source code
```
cd examples/libs_sys
```
2. Make cexy build system  (only once) 
```
cc ./cex.c -o ./cex
```
3. Installing pre-requisites
Note: maybe you also need to install `pkgconf` on your system.

```
# Lunux Ubuntu/Debian
> sudo apt install libcurl4-openssl-dev libzip-dev curl 

# MacOS
> brew install curl unzip libzip      

> Windows: requires fully configured vcpkg!
./vcpkg install --triplet=x64-mingw-static curl
./vcpkg install --triplet=x64-mingw-static libzip

```
4. Compiling the project
```
# Ensures all libs are in place
./cex config

# Compiles and run downloader app
./cex app run cex_downloader

# Custom command for building unzip app
./cex build-unzip

# Note: you can run cex_unzip via ./cex CLI too
./cex app run cex_unzip

```
5. Checking results
```
ls -la ./sqlite-amalgamation-3490200
```

6. Now you will be able to use `./cex help` to query `SQLite` source code
```
>> ./cex help sqlite3_recover

func_def             sqlite3_recover_config         ./sqlite-amalgamation-3490200/shell.c:21464
func_def             sqlite3_recover_errcode        ./sqlite-amalgamation-3490200/shell.c:21457
func_def             sqlite3_recover_errmsg         ./sqlite-amalgamation-3490200/shell.c:21450
func_def             sqlite3_recover_finish         ./sqlite-amalgamation-3490200/shell.c:21548
func_def             sqlite3_recover_init           ./sqlite-amalgamation-3490200/shell.c:21426
func_def             sqlite3_recover_init_sql       ./sqlite-amalgamation-3490200/shell.c:21438
func_def             sqlite3_recover_run            ./sqlite-amalgamation-3490200/shell.c:21533
func_def             sqlite3_recover_step           ./sqlite-amalgamation-3490200/shell.c:21520


>>./cex help sqlite3_recover_init

sqlite3_recover* sqlite3_recover_init (sqlite3* db, const char* zDb, const char* zUri);

```

