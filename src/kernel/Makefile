

# Default
ARCH ?= i386
ARCHDIR = arch/$(ARCH)

SYSROOT = /home/nico/Projects/blueberryos/src/sysroot

CC = i686-elf-gcc --sysroot=$(SYSROOT) -isystem=/usr/local
CFLAGS = -ffreestanding -O2 -Wall -Wextra -g #-std=gnu99
CPPFLAGS = -D__is_kernel -Iinclude
LIBS = -nostdlib -lk -lgcc

ASM = nasm
ASMFLAGS = -felf32

include make.config

SRC_C = $(shell find . -name "*.c")
SRC_ASM = $(shell find . -name "*.asm")

OBJ_C = $(SRC_C:.c=.o)
OBJ_ASM = $(SRC_ASM:.asm=.o)

OBJ = $(OBJ_C) $(OBJ_ASM)

all: blueberryos.bin

# Compiling C
%.o: %.c
	$(CC) -MMD -c $^ -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

# Compiling assembly
%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Linking
blueberryos.bin: $(OBJ)
	$(CC) -T $(ARCHDIR)/linker.ld -o $@ $(CFLAGS) $(LFLAGS) $^


.PHONY: clean rebuild check

clean:
	rm -rf blueberryos.bin $(OBJ) isodir

rebuild: clean all

check: all
	@if grub-file --is-x86-multiboot myos.bin; then\
		echo OS is multiboot;\
	else\
		echo OS is not multiboot!!;\
	fi


-include $(OBJS:.o=.d)

