
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
#ASMFLAGS = -fbin


SRC_C = src/test.c
SRC_ASM = src/userfunc.asm

OBJ_C = $(SRC_C:.c=.o)
OBJ_ASM = $(SRC_ASM:.asm=.o)
OBJS = $(OBJ_C) $(OBJ_ASM)

BIN_C = $(SRC_C:.c=)
BIN_ASM = $(SRC_ASM:.asm=)
BINS = $(BIN_C) $(BIN_ASM)

DEPS = $(OBJ:.o=.d)


all: $(BINS) install


# $(CC) -MMD -c $^ -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

#$(CC) -MD -c $^ -o $@ $(CFLAGS) -Wl,--oformat=binary
#%.c.o: %.c
#%.c.mod: %.c
$(OBJ_C): $(SRC_C)
	$(CC) -c -MD $^ -o $@ $(CFLAGS) $(LIBS)

#$(ASM) -f bin $< -o $@
#%.asm.mod: %.asm
$(OBJ_ASM): $(SRC_ASM)
	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

link.o: link.asm
	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

#$(CC) -T linker.ld -o $@ $(CFLAGS) $(LFLAGS) $< $(#LIBS)
$(BINS): $(OBJS) link.o
	$(CC) -o $@ $(CFLAGS) $(LFLAGS) $< link.o $(LIBS)

.PHONY: all clean build rebuild check install
.SUFFIXES: .o .c .asm .d

build: $(BINS)

clean:
	rm -rf $(BINS) $(OBJS) $(DEPS) link.o link.d

rebuild: clean build

initrd: $(BINS)
	mkdir -p initrd/bin
	cp -f --preserve=timestamps $(BINS) initrd/bin/.

initrd.tar: initrd
	tar U -cvf initrd.tar initrd

install: initrd initrd.tar
	mkdir -p ../kernel/isodir/boot
	cp --preserve=timestamps initrd.tar ../kernel/isodir/boot/initrd.tar

-include $(DEPS) link.d

