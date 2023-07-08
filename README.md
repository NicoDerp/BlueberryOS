
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

- [x] Libc with syscalls
- [x] Usermode
- [x] ELF-parsing
- [x] Higher-half kernel
- [x] Multiple processes
- [x] Preemptive os
- [x] Filesystem (with initrd)
- [x] Fancier terminal
- [x] Shell
- [ ] Memory allocator
- [ ] USB?
- [ ] Graphics manager
- [ ] Package manager (bpm (Blueberryos Package Manager))
- [ ] Multiple CPU cores
- [ ] Fully adapted Cross-compiler
- [ ] 64-bit
- [ ] UEFI

## Setup

You will need:
 - Linux (not tested on anything other)
 - Custom cross-compiler for i686 (see Cross-compiler section below)
 - (optional) An emulator, like QEMU or Bochs (config file for Bochs is included)
 - The source code for BlueberryOS (download by using `git clone https://github.com/NicoDerp/BlueberryOS.git`)

Download source

Then head into all sub-directories and run:

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
Return values of syscalls are returned in register `eax`, and the errno is returned in register `ecx`. The rest of the registers are saved.

### Libc

Is pretty POSIX compliant, with a lot of functions implemented.

## License

Apache License 2.0


