
# Default
SYSROOT = /home/nico/Projects/blueberryos/sysroot
CFLAGS ?= -O2 -g
LFLAGS ?= -O2
LIBS ?=
DESTDIR = $(SYSROOT)
PREFIX = /usr
INCLUDEDIR = $(PREFIX)/include

CC = i686-blueberryos-gcc
CFLAGS := $(CFLAGS) -std=gnu99 -ffreestanding -Wall -Wextra -Iinclude -D__is_kernel
ASM_CFLAGS = -D__is_kernel
LFLAGS := $(LFLAGS)
LIBS := $(LIBS) -nostdlib -lk
#LIBS := $(LIBS) -nostdlib -lk -lgcc

ASM = nasm
ASMFLAGS = -felf32


ISO = blueberryos.iso
BIN = blueberryos.bin


SRC_C = src/tinf/tinflate.c \
		src/tinf/tinfgzip.c \
		src/tinf/crc32.c \
		src/kernel.c \
		src/tty.c \
		src/gdt.c \
		src/idt.c \
		src/errors.c \
		src/io.c \
		src/memory.c \
		src/paging.c \
		src/file.c \
		src/usermode.c \
		src/logging.c \

SRC_ASM = src/boot.asm \
		  src/gdt.asm \
		  src/idt.asm \
		  src/errors.asm \
		  src/paging.asm \
		  src/usermode.asm \

OBJ_C = $(SRC_C:.c=.c.o)
OBJ_ASM = $(SRC_ASM:.asm=.asm.o)

OBJ = $(OBJ_C) $(OBJ_ASM)

# $(CC) -MMD -c $^ -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)


all:
	$(MAKE) -j8 $(ISO)

# Compiling C
%.c.o: %.c
	$(CC) -MD -c $^ -o $@ $(CFLAGS)

# Compiling assembly
%.asm.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

# Linking
$(BIN): $(OBJ)
	$(CC) -T linker.ld -o $@ $(CFLAGS) $(LFLAGS) $^ $(LIBS)

# Making iso
$(ISO): $(BIN) modules
	mkdir -p isodir/boot/grub
	cp $(BIN) isodir/boot/$(BIN)
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

.PHONY: all clean rebuild check run run-bochs install modules
.SUFFIXES: .o .c .asm

clean:
	rm -rf $(ISO) $(BIN) $(OBJ) $(OBJ:.o=.d) isodir initrd.tar

rebuild: clean all

check: all
	@if grub-file --is-x86-multiboot2 $(BIN); then\
		echo OS is multiboot2;\
	else\
		echo OS is not multiboot2!!;\
	fi

#qemu-system-i386 -cdrom $(ISO) -display gtk,zoom-to-fit=on -s
run: all
	qemu-system-i386 -cdrom $(ISO) -s

run-bochs: $(ISO)
	bochs

install:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

-include $(OBJS:.o=.d)

