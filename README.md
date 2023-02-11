
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
 - (optional) An emulator, like QEMU
 - The source code for BlueberryOS (download by using git clone)

Then head into all sub-directories and run:

```shell
$ make
$ make install
```

Inside the `kernel` directory you will now find a `blueberryos.iso`. 
To emulate:
```shell
$ qemu-system-i386 -cdrom blueberryos.iso
```

## License

Apache License 2.0


