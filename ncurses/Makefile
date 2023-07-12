

# Default
CFLAGS ?= -O2 -g
LFLAGS ?= -O2
LIBS ?=

SYSROOT = /home/nico/Projects/blueberryos/sysroot
DESTDIR = $(SYSROOT)
PREFIX = /usr/local
EXEC_PREFIX = $(PREFIX)
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(EXEC_PREFIX)/lib

# Necessary
#CC = i686-elf-gcc --sysroot=$(SYSROOT)
#CC = i686-elf-gcc -isystem=/usr/include
#CC = i686-elf-gcc --sysroot=$(SYSROOT) -isystem /usr/include
CC = i686-elf-gcc -isystem $(SYSROOT)$(INCLUDEDIR)
CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra -Iinclude -D__is_ncurses
ASM_CFLAGS = -D__is_ncurses
LFLAGS := $(LFLAGS) -ffreestanding
#LIBS = -nostdlib -lk -lgcc
LIBS := $(LIBS) -nostdlib -L$(SYSROOT)$(LIBDIR) -lc -lgcc

ASM = nasm
ASMFLAGS = -felf32

AR = i686-elf-ar

FREEOBJS =\
	ncurses/initscr.o \
	ncurses/newwin.o \
	ncurses/printw.o \
	ncurses/vmvwprintw.o \
	ncurses/wprintw.o \
	ncurses/mvwprintw.o \
	ncurses/wputcharw.o \
	ncurses/refresh.o \
	ncurses/wrefresh.o \
	ncurses/getch.o \
	ncurses/endwin.o \
	ncurses/move.o \
	ncurses/clear.o \
	ncurses/wclear.o \

OBJS = $(FREEOBJS)

BINARIES = libncurses.a

# $(CC) -MMD -c $^ -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.PHONY: all build clean install install-headers install-libs
.SUFFIXES: .o .ncurses.o .c .asm

all:
	$(MAKE) -j8 build
	$(MAKE) install

build: $(BINARIES)

libncurses.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS) $(LIBS)

.asm.o:
	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

clean:
	rm -rf $(BINARIES) *.a
	rm -rf $(OBJS)	*.o */*.o */*/*.o
	rm -rf $(OBJS:.o=.d) *.d */*.d */*/*.d

install: $(BINARIES) install-headers install-libs

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-libs:
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(BINARIES) $(DESTDIR)$(LIBDIR)

-include $(OBJS:.o=.d)

