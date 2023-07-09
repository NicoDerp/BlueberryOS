
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


SRC_C = src/sbin/loop.c \
		src/bin/sudo.c \
		src/bin/shell.c \
		src/bin/pwd.c \
		src/bin/ls.c \
		src/bin/cat.c \
		src/usr/bin/printenv.c \
		src/usr/bin/echo.c \
		src/usr/bin/whoami.c \
		src/usr/bin/test.c \
		src/usr/bin/test2.c \
		src/usr/bin/mmaptest.c \

SRC_ASM = src/usr/bin/userfunc.asm \

OBJ_C = $(SRC_C:.c=.o)
OBJ_ASM = $(SRC_ASM:.asm=.o)
OBJS = $(OBJ_C) $(OBJ_ASM)

noop=
space = $(noop) $(noop)

BIN_C = $(patsubst %.c,%,$(SRC_C))
BIN_ASM = $(patsubst %.asm,%,$(SRC_ASM))
BINS = $(BIN_C) $(BIN_ASM)
OUT_BINS = $(patsubst src/%,initrd/%,$(BINS))
#OUT = $(join $(BINS),  $(addprefix $(space),$(OUT_BINS)))
#OUT = $(join $(subst $(space),$(space),$(BINS)),$(subst $(space),$(space),$(OUT_BINS)))

DIRS = $(sort $(dir $(OUT_BINS)))

DEPS = $(OBJS:.o=.d)

.PHONY: all clean build rebuild check install
#.SUFFIXES: .o .c .asm .d

all:
	$(MAKE) -j8 build
	$(MAKE) install

build: $(BINS)

clean:
	rm -rf $(BINS) $(OBJS) $(DEPS) link.o link.d initrd.tar.gz initrd

rebuild: clean build

#$(foreach (var1, var2), ($(BINS), $(OUT_BINS)), cp --preserve=timestamps $(var1) $(var2);)
#$(foreach (var1, var2), ($(BINS), $(OUT_BINS)), cp --preserve=timestamps $(var1);)
#cp -f --preserve=timestamps $(BINS) $(OUT_BINS)

initrd: $(BINS)
	rm -f initrd.tar.gz
	mkdir -p $(DIRS)
	$(subst ^, ,$(join $(addprefix cp^,$(BINS)),$(patsubst %,^%;,$(OUT_BINS))))
	chmod 755 initrd/usr/bin/*
	chmod 755 initrd/bin/*
	chmod 4750 initrd/bin/sudo

initrd.tar.gz: initrd
	tar U --gzip -cvf initrd.tar.gz initrd

install: initrd initrd.tar.gz
	mkdir -p ../kernel/isodir/boot
	cp --preserve=timestamps initrd.tar.gz ../kernel/isodir/boot/initrd.tar.gz

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

