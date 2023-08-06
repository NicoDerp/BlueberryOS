
# Blueberry OS

## An operating system created from scratch in C and x86 assembly

Made by NicoDerp.

Big thanks to [wiki.osdev.org](wiki.osdev.org) for their wiki and forums.

This project uses a modified version of the [tinf](https://github.com/jibsen/tinf) library for decompressing the initrd.
This OS uses a memory allocator heavily inspired by [liballoc](https://github.com/blanham/liballoc).

Use BlueberryOS at our own risk. I'm not responsible for any damages or other things that can happen by using my operating system.
This is very in-development and just a hobby-project.

## Minimum requirements

 - x86 (or x86_64) CPU
 - 4 MiB of memory

## Goals

I want BlueberryOS to be simple, small and efficient.

## Features

- [x] Preemptive OS
- [x] C standard-library
- [x] Built-in standard applications (with similar use to GNU) made specifically for BlueberryOS
- [x] Built-in highly-customizable text editor with similar to vim (called vim)
- [x] Built-in terminal-emulator

## Milestones

- [x] Libc with syscalls
- [x] Usermode
- [x] ELF-parsing
- [x] Higher-half kernel
- [x] Multiple processes
- [x] Preemptive os
- [x] Filesystem (with initrd)
- [x] Fancier terminal
- [x] Shell
- [x] Memory allocator
- [x] Ncurses
- [x] Text editor
- [x] Fully adapted cross-compiler
- [ ] Dynamic libraries
- [ ] Graphics manager
- [ ] Kernel modules
- [ ] Multiple CPU cores
- [ ] Package manager (bpm (Blueberryos Package Manager))
- [ ] 64-bit
- [ ] Sound
- [ ] USB
- [ ] NVMe
- [ ] UEFI

## Setup

You will need:
 - Linux (not tested on anything other)
 - Git
 - NASM
 - The source code for BlueberryOS (download by using `git clone https://github.com/NicoDerp/BlueberryOS.git`)
 - Custom cross-compiler for BlueberryOS (see Cross-compiler section below)
 - (optional) An emulator, like QEMU or Bochs (config file for Bochs is included)

### Cross-compiler

It is important that you use the `i686-elf-gcc` cross-compiler when developing for this operating system.
This is because the entire standard-library is re-written to work with BlueberryOS. This ensures portability, so any C code can run with ease.

Compiling the cross-compiler can be tedious and take some time. Be very careful with every command you enter and make sure the paths are correct!
But remember that compiling the cross-compiler will only be done once! After that you can both use BlueberryOS and code your own programs with it!

Tip: For much faster compilation times, specify flag -j<n> for parallel compilation when running `make`.
I've included `-j\`nproc --ignore=2\`` which will automatically use 2 fewer cores than what your computer has, so you can still use it without lagging.
If you want maximum then go ahead and change it yourself. If you don't want multiprocessing, then set CORES="".

1. Donwload BlueberryOS source if you haven't already.
2. Now we will set the enviroment variables for where we will install different things. **All directories _must_ not end in `/`, and all directories _must_ be aboslute paths!** The commands under are an example. `BLUEBERRYOS_SOURCE` is where you donwloaded the BlueberryOS source. `PREFIX` is the location where your cross-compiler will be installed (don't change if you are unsure).
```shell
$ export BLUEBERRYOS_SOURCE=$HOME/blueberryos
$ export BINUTILS=$HOME/crosscompiler/binutils
$ export GCC=$HOME/crosscompiler/gcc
$ export PREFIX="$HOME/opt/cross"
$ export PATH="$PREFIX/bin:$PATH"
$ export CORES="-j`nproc --ignore=2`"
```
3. Download both the latest Binutils and GCC source from [https://sourceware.org/git/binutils-gdb.git](https://sourceware.org/git/binutils-gdb.git) and [https://gcc.gnu.org/git/gcc.git](https://gcc.gnu.org/git/gcc.git) respectivly using `git clone`.
4. Head into `blueberryos/kernel` and run `make install`. Then, also run `make install-headers` in the `blueberryos/libc` folder.
6. Then you want to apply the changes made to the sources.
6. Now we configure and compile first binutils and then GCC. If you set the variables correct you can just relax and copy paste the commands one-by-one. Keep in mind that this will take quite some time. Don't miss any commands!
```shell
$ cd $BINUTILS
$ git apply $BLUEBERRYOS_SOURCE/binutils.patch
$ cd $GCC
$ git apply $BLUEBERRYOS_SOURCE/gcc.patch
$ cd $HOME/crosscompiler
$ mkdir build-binutils
$ cd build-binutils
$ ../binutils/configure --target=i686-blueberryos --prefix="$PREFIX" --with-sysroot=$BLUEBERRYOS_SOURCE/sysroot --enable-languages=c
$ make $CORES && make install $CORES
$ cd $HOME/crosscompiler
$ mkdir build-gcc
$ cd build-gcc
$ ../gcc/configure --target=i686-blueberryos --prefix="$PREFIX" --with-sysroot=$BLUEBERRYOS_SOURCE/sysroot --enable-languages=c
$ make all-gcc $CORES && make install-gcc $CORES
```

Wow! That was a lot. Now you should have the `i686-blueberryos` toolchain. 
You should also be able to run `i686-blueberryos-gcc --version`.

### Compiling BluberryOS with its standard applications

The next step is to finally compile the operating system itself along with the standard applications.

```shell
$ cd $BLUEBERRYOS_SOURCE/kernel
$ make install
$ cd $BLUEBERRYOS_SOURCE/libc
$ make && make install
$ cd $BLUEBERRYOS_SOURCE/ncurses
$ make && make install
$ cd $BLUEBERRYOS_SOURCE/initrd
$ make && make install
$ cd $BLUEBERRYOS_SOURCE/kernel
$ make
```

Now everything is compiled and ready to run!

### Your own programs

This is still work-in-progress. But what you can do is add your program's source (`myprogram.c`) to `initrd/src/usr/bin/`.
Then you need to edit `initrd/Makefile` and simply add the line `    src/usr/bin/myprogram.o \`.
When you recompile initrd and run BlueberryOS, you'll see that your program is available to run!


Developing your own programs is incredibly simple. Let's say you have a project you wan't to port to BlueberryOS, the only thing that you need to change is that you use `i686-blueberryos-gcc` instead of plain `gcc` (probably not true at the moment).
If you only want to test a simple Hello World program, it's as simple as this:

```shell
$ cd blueberryos/initrd
$ cat helloworld.c

#include <stdio.h>

int main(void) {
    printf("Hello world in BlueberryOS!\n");
    return 0;
}

$ i686-blueberryos-gcc helloworld.c -o helloworld
$ mv helloworld initrd/usr/bin/helloworld
$ make install
```

Now, the next time you run BlueberryOS you'll have access to your new program!

(The process of copying to initrd will be simpler in future)

### Emulating

I personally use QEMU. If you wan't the easiest way to emulate, then also just download QEMU and inside the `kernel` directory, run
```shell
$ make run
```
Have fun in you brand-new super-cool operating system!

## Documentation for developers

### Syscalls

Instead of the conventional `int $80` for linux-based operating systems, this os uses the interrupt `$30` or decimal `48`.
Return values of syscalls are returned in register `eax`, and if there is an error, the errno value is returned in register `ecx`.
The rest of the registers are saved.

OS-specific system-calls are found in the `unistd.h` header in BlueberryOS' standard-library;

Custom syscalls:

| ID   | Name          | C call                                     | The function                                                                                                                                                                              |
| --- | --- | --- | --- |
| `23` | `SYS_ttycmd`  | `int ttycmd(int cmd, int* args, unsigned** ret)` | It executes the terminal command `cmd` with arguments `args` and if the command returns anything, it is returned in `ret`. The syscall returns 0 if successful, -1 if there was an error. You can pass NULL to both `args` and `ret` if the command has no arguments or return values. |

### C-Library

The C-library and kernel is pretty POSIX compliant, with a lot of similarities with Linux.
You will find a lot of common functions are implemented, but not all.

### Ncurses

BlueberryOS has it's own Ncurses implementation! This means that you can use programs with text-based user interface.
The OS has it's own version of Vim built-in, and this uses Ncurses.

Though most of the common functions are implemented, there are some minor differences:

 - Terminal is always in no-echo mode
 - Terminal is always in non-canonical mode
 - Ncureses is always accepts keypad / special keys (use `char` instead of `int` if you don't want this)

## License

Apache License 2.0


