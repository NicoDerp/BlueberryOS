

# Default
SYSROOT = /home/nico/Projects/blueberryos/src/sysroot

#CC = i686-elf-gcc --sysroot=$(SYSROOT)
CC = i686-elf-gcc
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra -Iinclude #-D__is_kernel
LFLAGS = -ffreestanding -O2 -nostdlib -lgcc
#LIBS = -nostdlib -lk -lgcc

ASM = nasm
ASMFLAGS = -felf32


ISO = blueberryos.iso
BIN = blueberryos.bin


SRC_C = src/kernel.c src/tty.c
SRC_ASM = boot.asm

OBJ_C = $(SRC_C:.c=.o)
OBJ_ASM = $(SRC_ASM:.asm=.o)

OBJ = $(OBJ_C) $(OBJ_ASM)

all: $(ISO)

# $(CC) -MMD -c $^ -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

# Compiling C
%.o: %.c
	$(CC) -c $^ -o $@ $(CFLAGS)

# Compiling assembly
%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

# Linking
$(BIN): $(OBJ)
	$(CC) -T linker.ld -o $(BIN) $(LFLAGS) $^

# Making iso
$(ISO): $(BIN)
	mkdir -p isodir/boot/grub
	cp $(BIN) isodir/boot/$(BIN)
	cp grub.cfg isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

.PHONY: clean rebuild check run

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

-include $(OBJS:.o=.d)

