
# Default
SYSROOT = /home/nico/Projects/blueberryos/sysroot
CFLAGS ?= -O2 -g
LFLAGS ?= -O2
LIBS ?=
DESTDIR = $(SYSROOT)
PREFIX = /usr/local
INCLUDEDIR = $(PREFIX)/include/kernel

# Necessary
#CC = i686-elf-gcc --sysroot=$(SYSROOT)

# Work-around because compiler wasn't configured with --with-sysroot
CC = i686-elf-gcc --sysroot=$(SYSROOT) -isystem=/usr/include
CFLAGS := $(CFLAGS) -std=gnu99 -ffreestanding -Wall -Wextra -Iinclude -D__is_kernel
LFLAGS := $(LFLAGS) -ffreestanding
#LIBS = -nostdlib -lk -lgcc
LIBS = -nostdlib -lgcc

ASM = nasm
ASMFLAGS = -felf32


ISO = blueberryos.iso
BIN = blueberryos.bin


SRC_C = src/kernel.c src/tty.c
SRC_ASM = src/boot.asm

OBJ_C = $(SRC_C:.c=.o)
OBJ_ASM = $(SRC_ASM:.asm=.o)

OBJ = $(OBJ_C) $(OBJ_ASM)

# $(CC) -MMD -c $^ -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

# Compiling C
%.o: %.c
	$(CC) -MD -c $^ -o $@ $(CFLAGS)

# Compiling assembly
%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Linking
$(BIN): $(OBJ)
	$(CC) -T linker.ld -o $(BIN) $(LFLAGS) $(LIBS) $^

# Making iso
$(ISO): $(BIN)
	mkdir -p isodir/boot/grub
	cp $(BIN) isodir/boot/$(BIN)
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

.PHONY: all clean rebuild check run install
.SUFFIXES: .o .c .asm

all: $(ISO)

clean:
	rm -rf $(ISO) $(BIN) $(OBJ) isodir

rebuild: clean all

check: all
	@if grub-file --is-x86-multiboot $(BIN); then\
		echo OS is multiboot;\
	else\
		echo OS is not multiboot!!;\
	fi

run: $(ISO)
	qemu-system-i386 -cdrom $(ISO)

install:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

-include $(OBJS:.o=.d)

