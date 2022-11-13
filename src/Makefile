
CXX = i686-elf-gcc
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
ASM = nasm
ASMFLAGS = -felf32

LFLAGS = -ffreestanding -O2 -nostdlib -lgcc
LFILE = linker.ld

ISO = blueberryos.iso
BIN = blueberryos.bin

SRC_C = $(wildcard *.c)
SRC_ASM = $(wildcard *.asm)

OBJ_C = $(SRC_C:.c=.o)
OBJ_ASM = $(SRC_ASM:.asm=.o)

OBJ = $(OBJ_C) $(OBJ_ASM)


all: $(BIN)

%.o: %.c
	$(CXX) -c $^ -o $@ $(CFLAGS)

%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@

$(BIN): $(OBJ)
	$(CXX) -T $(LFILE) -o $(BIN) $(LFLAGS) $^


.PHONY: clean rebuild check iso run

clean:
	rm -rf $(ISO) $(BIN) $(OBJ) isodir

rebuild: clean all

check: all
	@if grub-file --is-x86-multiboot myos.bin; then\
		echo OS is multiboot;\
	else\
		echo OS is not multiboot!!;\
	fi

iso: all
	mkdir -p isodir/boot/grub
	cp $(BIN) isodir/boot/$(BIN)
	cp grub.cfg	isodir/boot/grub/grub.cfg
	grub-mkrescue -o $(ISO) isodir

run: iso
	qemu-system-i386 -cdrom $(OS)


