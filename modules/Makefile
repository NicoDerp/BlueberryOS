
# Default
SYSROOT = /home/nico/Projects/blueberryos/sysroot
CFLAGS ?= -O2 -g
LFLAGS ?= -O2
LIBS ?=
DESTDIR = $(SYSROOT)
PREFIX = /usr/local
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib

# Necessary
#CC = i686-elf-gcc --sysroot=$(SYSROOT)

# Work-around because compiler wasn't configured with --with-sysroot
#CC = i686-elf-gcc --sysroot=$(SYSROOT) -isystem=/usr/include
#CC = i686-elf-gcc --sysroot=$(SYSROOT) -isystem /usr/include
CC = i686-elf-gcc --sysroot=$(SYSROOT) -isystem $(SYSROOT)/usr/local/include
CFLAGS := $(CFLAGS) -std=gnu99 -ffreestanding -Wall -Wextra -Iinclude
ASM_CFLAGS =
LFLAGS := $(LFLAGS)
#-Wl,--gc-sections
LIBS := $(LIBS) -nostdlib -L$(SYSROOT)$(LIBDIR) -lc -lgcc
#LIBS := $(LIBS) -nostdlib -lgcc

ASM = nasm
ASMFLAGS = -felf32


ISO = blueberryos.iso
BIN = blueberryos.bin


SRC_C = src/test.c
SRC_ASM =

OBJ_C = $(SRC_C:.c=.mod)
OBJ_ASM = $(SRC_ASM:.asm=.mod)

OBJ = $(OBJ_C) $(OBJ_ASM)

# $(CC) -MMD -c $^ -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

#$(CC) -MD -c $^ -o $@ $(CFLAGS) -Wl,--oformat=binary
#%.c.o: %.c
%.mod: %.c
	$(CC) -MD $^ -o $@ $(CFLAGS) $(LIBS)

#$(ASM) -f bin $< -o $@
%.mod: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

.PHONY: all clean build rebuild check run run-bochs install
.SUFFIXES: .o .c .asm

all: build

build: $(OBJ)

clean:
	rm -rf $(OBJ) $(OBJ:.mod=.d)

rebuild: clean build

install:
	mkdir -p ../kernel/isodir/modules
	cp -f --preserve=timestamps $(OBJ) ../kernel/isodir/modules/.

-include $(OBJS:.mod=.d)

