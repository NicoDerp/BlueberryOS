

# Default
CFLAGS ?= -O2 -g
LFLAGS ?= -O2
LIBS ?=

DESTDIR = /home/nico/Projects/blueberryos/sysroot
PREFIX = /usr/local
EXEC_PREFIX = $(PREFIX)
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(EXEC_PREFIX)/lib

# Necessary
#CC = i686-elf-gcc --sysroot=$(SYSROOT)
CC = i686-elf-gcc -isystem=/usr/include
CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra -Iinclude -D__is_kernel
LFLAGS := $(LFLAGS) -ffreestanding
#LIBS = -nostdlib -lk -lgcc
LIBS := $(LIBS) -nostdlib -lgcc
LIBK_CFLAGS := $(CFLAGS) -D__is_libk
LIBK_CFLAGS_ASM := -D__is_libk

ASM = nasm
ASMFLAGS = -felf32

SRC_C = src/kernel.c src/tty.c

FREEOBJS =\
	stdio/printf.o \
	stdio/putchar.o \
	stdio/puts.o \
	stdlib/abort.o \
	string/memcmp.o \
	string/memcpy.o \
	string/memmove.o \
	string/memset.o \
	string/strlen.o \

HOSTEDOBJS =

OBJS = $(FREEOBJS) $(HOSTEDOBJS)

LIBK_OBJS = $(FREEOBJS:.o=.libk.o)

#BINARIES = libc.a libk.a # Not ready for libc yet.
BINARIES = libk.a

# $(CC) -MMD -c $^ -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

# Compiling C
.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS)

# Compiling assembly
.asm.o:
	$(ASM) $(ASMFLAGS) $< -o $@

.c.libk.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(LIBK_CFLAGS)

.asm.libk.o:
	$(ASM) $(ASMFLAGS) $< -o $@	$(LIBK_FLAGS_ASM)

.PHONY: all clean install install-headers install-libs

all: $(ISO)

clean:
	rm -rf $(BINARIES) *.a
	rm -rf $(OBJS) $(LIBK_OBJS)	*.o */*.o */*/*.o
	rm -rf $(OBJS:.o=.d) $(LIBK_OBJS:.o=.d) *.d */*.d */*/*.d

install: install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve-timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-libs:
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(BINARIES) $(DESTDIR)$(LIBDIR)

-include $(OBJS:.o=.d)
-include $(LIBK_OBJS:.o=.d)

