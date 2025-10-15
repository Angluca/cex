
# Proof of the concept for the freestanding compilation of CEX

1. This example project is standalone project with its own `./cex.c` build commands
2. It works only on Linux (simulating freestanding platform without libc via kernel syscalls)
3. It intended for understanding if / how CEX size can be reduced and what minimal functionality required for the architecture.
4. Proof of the concept for embedded use of CEX
5. Concept of static compilation without libc.
6. See `src/platform_*.c` source code for required libc functions you maybe have to implement for your platform.

## CEX Capabilities used
* `#define cex$enable_minimal` allowing to use only bare minimum on CEX functionality
* `#define cex$enable_mem` - enables memory allocators (requires platform `malloc/calloc/realloc/free`)
* `cex$enable_ds` - enables core data structures `arr$` and `hm$`
* `cex$enable_str` - enables CEX `str.` and `sbuf.` namespaces
* `cex$enable_io` - enables CEX `io` namespace
* `cex$enable_os` - enables `os` and `argparse` namespace. 

## Getting started 

1. Download the source code
```
cd examples/freestanding
```
2. Make cexy build system  (only once) 
```
cc ./cex.c -o ./cex
```
3. Build run / examples

```
./cex build src/freestanding_1minimal.c
./cex build src/freestanding_2mem.c
./cex build src/freestanding_3ds.c
./cex build src/freestanding_4strings.c
./cex build src/freestanding_5io.c
./cex build src/freestanding_6os.c

```

## CEX code size

I used non optimized binary size on 64-bit x86 platform, produced by the non optimized clang commands with the following arguments:
```
"clang", "-g", "-I.", "-nostdlib", "-static", "-ffreestanding", 
```

|  Features included |  Binary Size | Change |
| :------- | :------: | :-------: |
| `No CEX (bare C)` | 14k | 0 |
| `cex$enable_minimal` | 58k | +44k |
| `cex$enable_mem` | 101k | +43k |
| `cex$enable_ds` | 132k | +32k |
| `cex$enable_str` | 196k | +64k |
| `cex$enable_io` | 213k | +16k |
| `cex$enable_os` | 288k | +75k |




