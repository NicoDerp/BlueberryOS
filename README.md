
# Blueberry OS

## An operating system created from scratch in C and x86 assembly

Made by NicoDerp.

Big thanks to [wiki.osdev.org](wiki.osdev.org) for their wiki and forums.

This project uses a slighly modified version of the [tinf](https://github.com/jibsen/tinf) library for decompressing the initrd.
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
- [ ] Dynamic libraries
- [ ] Graphics manager
- [ ] Kernel modules
- [ ] Multiple CPU cores
- [ ] Package manager (bpm (Blueberryos Package Manager))
- [ ] Sound
- [ ] USB?
- [ ] NVMe
- [ ] Fully adapted Cross-compiler
- [ ] 64-bit
- [ ] UEFI

## Setup

You will need:
 - Linux (not tested on anything other)
 - Custom cross-compiler for i686 (see Cross-compiler section below)
 - (optional) An emulator, like QEMU or Bochs (config file for Bochs is included)
 - The source code for BlueberryOS (download by using `git clone https://github.com/NicoDerp/BlueberryOS.git`)

Inside BlueberryOS, head into the `kernel` folder and run:

```shell
$ make install
```

Then head into `libc` (important that this is first!), `initrd` and lastly `ncurses` (in that order) and run:

```shell
$ make
$ make install
```

Inside the `kernel` directory you will now find a `blueberryos.iso`. 
Now you can emulate using your favourite emulator or even run the BlueberryOS on real hardware (UEFI not supported yet)!
If you wan't to emulate using QEMU you can just use the command:
```shell
$ make run
```
Or, if you like Bochs more (for some reason for me Bochs gets stuck at multiboot2 command in grubcfg):
```shell
$ make run-bochs
```

## Documentation for developers

### Cross-compiler

It is important that you use the `i686-elf-gcc` cross-compiler when developing for this operating system.
This is because the entire standard-library is re-written to work with BlueberryOS.

This ensures portability, so any C code can run with ease.

For now, the cross-compiler is not customized for BlueberryOS, but in will be in the future.
Download from [https://wiki.osdev.org/GCC_Cross-Compiler](https://wiki.osdev.org/GCC_Cross-Compiler), and follow their step-by-step instructions.

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


