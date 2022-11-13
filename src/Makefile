
CXX = i686-elf-gcc
CFLAGS = -std=gnu99 -ffreestanding -O2 -Wall -Wextra
ASM = nasm
ASMFLAGS = -felf32

LINKFLAGS = -ffreestanding -O2 -nostdlib -lgcc
LINKFILE = linker.ld

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

clean:
	rm -rf $(BIN) $(OBJ)


.PHONY: clean

