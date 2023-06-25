
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


SRC_C = src/test.c src/shell.c src/loop.c
SRC_ASM = src/userfunc.asm

OBJ_C = $(SRC_C:.c=.o)
OBJ_ASM = $(SRC_ASM:.asm=.o)
OBJS = $(OBJ_C) $(OBJ_ASM)

BIN_C = $(patsubst %.c,%,$(SRC_C))
BIN_ASM = $(patsubst %.asm,%,$(SRC_ASM))
BINS = $(BIN_C) $(BIN_ASM)

DEPS = $(OBJS:.o=.d)

.PHONY: all clean build rebuild check install
#.SUFFIXES: .o .c .asm .d

all: build install

build: $(BINS)

clean:
	rm -rf $(BINS) $(OBJS) $(DEPS) link.o link.d initrd.tar initrd

rebuild: clean build

initrd: $(BINS)
	rm -f initrd.tar
	mkdir -p initrd/bin
	cp -f --preserve=timestamps $(BINS) initrd/bin/.

initrd.tar: initrd
	tar U -cvf initrd.tar initrd

install: initrd initrd.tar
	mkdir -p ../kernel/isodir/boot
	cp --preserve=timestamps initrd.tar ../kernel/isodir/boot/initrd.tar

$(BINS): $(OBJS) link.o

%.o: %.c
	$(CC) -c -MD $^ -o $@ $(CFLAGS) $(LIBS)

%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

%: %.o
	$(CC) -o $@ $(CFLAGS) $(LFLAGS) $< link.o $(LIBS)


#link.o: link.asm
#	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

#-include $(DEPS) link.d

