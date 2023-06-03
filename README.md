
# Bluberry OS

## An operating system created from scratch in C and x86 assembly

Made by NicoDerp.

Big thanks to wiki.osdev.org for their wiki and forums.

Btw use at our own risk. I'm not responsible for any damages or other things that can happen by using my operating system.
This is very in-development and just a hobby-project.

## Setup

You will need:
 - Linux (not tested on anything other)
 - A [cross-compiler](https://wiki.osdev.org/GCC_Cross-Compiler) for i686
 - (optional) An emulator, like QEMU or Bochs (config file for Bochs is included)
 - Git
 - The source code for BlueberryOS (download by using `git clone https://github.com/NicoDerp/BlueberryOS.git`)

Download source

Then head into all sub-directories and run:

```shell
$ make
$ make install
```

Inside the `kernel` directory you will now find a `blueberryos.iso`. 
Now you can emulate using your favourite emulator or even run the BlueberryOS on real hardware!
If you wan't to emulate using QEMU you can just use the command:
```shell
$ make run
```
Or, if you like Bochs more:
```shell
$ make run-bochs
```

## Documentation for developers

### Cross-compiler

It is important that you use the `i686-elf-gcc` cross-compiler when developing for this operating system.
This is because the entire standard-library is re-written to work with BlueberryOS.

This ensures portability, so any C code can run with ease.

### Syscalls

Instead of the conventional `int $80` for linux-based operating systems, this os uses the interrupt `$30` or decimal `48`.


## License

Apache License 2.0


