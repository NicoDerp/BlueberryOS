
# Default
CFLAGS ?= -O2 -g
LFLAGS ?= -O2
LIBS ?= -lncurses
INCLUDEDIR = $(PREFIX)/include

# Necessary
CC = i686-blueberryos-gcc
#CFLAGS := $(CFLAGS) -std=gnu99 -ffreestanding -Wall -Wextra
CFLAGS := $(CFLAGS) -std=gnu99 -Wall -Wextra
ASM_CFLAGS =
LFLAGS := $(LFLAGS)
LIBS := $(LIBS)
#LIBS := $(LIBS) -ffreestanding -nostdlib -lc
#LIBS := $(LIBS) -nostdlib -lc -lgcc

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
		src/usr/bin/filetest.c \
		src/usr/bin/vim.c \
		src/usr/bin/pappatest.c \

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
	$(MAKE) -j`nproc` build
	$(MAKE) install

build: $(BINS)

clean:
	rm -rf $(BINS) $(OBJS) $(DEPS) initrd.tar.gz initrd

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
	mkdir -p initrd/home/nico
	cp file.py initrd/home/nico/file.py
	cp vim.c initrd/home/nico/vim.c

initrd.tar.gz: initrd
	tar U --gzip -cvf initrd.tar.gz initrd

install: initrd initrd.tar.gz
	mkdir -p ../kernel/isodir/boot
	cp --preserve=timestamps initrd.tar.gz ../kernel/isodir/boot/initrd.tar.gz

$(BINS): $(OBJS)

%.o: %.c
	$(CC) -c -MD $^ -o $@ $(CFLAGS) $(LIBS)

%.o: %.asm
	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

%: %.o
	$(CC) -o $@ $(CFLAGS) $(LFLAGS) $< $(LIBS)


#link.o: link.asm
#	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

#-include $(DEPS) link.d

