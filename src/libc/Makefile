
SYSROOT = /home/nico/Projects/blueberryos/src/sysroot

# Default
ARCH ?= i386
ARCHDIR = arch/$(ARCH)

CC = i686-elf-gcc --sysroot=$(SYSROOT) -isystem=/usr/local
CFLAGS = -ffreestanding -O2 -Wall -Wextra -g #-std=gnu99
CPPFLAGS = -D__is_libc -Iinclude

PREFIX = /usr/local
INCLUDEDIR = $(PREFIX)/include

#include $(ARCHDIR)/make.config

OBJ = \
string/strlen.o \

all: libk.a

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
	rm -rf $(OBJ)

rebuild: clean all

install-headers:
	mkdir -p $(INCLUDEDIR)
	cp -R --preserve=timestamps ./include/* $(INCLUDEDIR)

-include $(OBJ:.o=.d)

