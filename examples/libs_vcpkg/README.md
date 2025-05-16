# Building with vcpkg local repo

This example illustrates how to build programs using project specific vcpkg. It contains two simple programs: `cex_download` for downloading `SQLite` zip archive from the internet using `curl` library functions, and `cex_unzip` for extracting the archive's contents into build directory (using  `libzip`). 
 
`./cex` build system of this project, automatically fetch and installs `vcpkg` into `build/` directory and downloads all necessary libs.


## Example highlights
* It works  on Linux, Windows and MacOS. (see [CI file](https://github.com/alexveden/cex/blob/master/.github/workflows/examples.yml))
* Uses project-specific libs, no system dependencies
* `cmd_vcpkg_install` is used for installing all dependencies via `git` and `vcpkg`
* Libs list are set in `#define cexy$pkgconf_libs` 
* `vcpkg` is configured via `cexy$vcpkg_root` - universal path, and `cexy$vcpkg_triplet` - platform specific configuration (includes architecture, platform, and static/dynamic type of linking) 
* All build settings and steps are defined in `./cex.c` file


## Getting started 

1. Download the source code
```
cd examples/libs_vcpkg
```
2. Make cexy build system  (only once) 
```
cc ./cex.c -o ./cex
```
3. Installing pre-requisites
Note: maybe you need to install `pkgconf` on your system.

4. Compiling the project
```
# Installing and configuring vcpkg locally
./cex vcpkg-install

# Ensures all libs are in place
./cex config

On success it should print something like:
.....
Tools installed (optional):
* git                       OK
* cexy$pkgconf_cmd          OK ("pkgconf")
* cexy$vcpkg_root           build/vcpkg/
* cexy$vcpkg_triplet        x64-linux
* cexy$pkgconf_libs         -Lbuild/vcpkg/installed/x64-linux/lib/pkgconfig/../../lib -lcurl -lzip -lssl -lcrypto -ldl -pthread -lz -lbz2 (all found)
.....

# Compiles and run downloader app
./cex app run cex_downloader

# Compiles and run unzip app
./cex app run cex_unzip

```

5. Checking results
```
ls -la ./build/
```

