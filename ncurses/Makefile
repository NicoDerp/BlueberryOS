
# Default
CFLAGS ?= -O2 -g
LFLAGS ?= -O2
LIBS ?=

SYSROOT = /home/nico/Projects/blueberryos/sysroot
DESTDIR = $(SYSROOT)
PREFIX = /usr
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(PREFIX)/lib

# Necessary
CC = i686-blueberryos-gcc
CFLAGS := $(CFLAGS) -Wall -Wextra -Iinclude -D__is_ncurses
ASM_CFLAGS = -D__is_ncurses
LFLAGS := $(LFLAGS)
LIBS := $(LIBS)
#LIBS := $(LIBS) -nostdlib -lc -lgcc

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
	ncurses/delwin.o \
	ncurses/init_pair.o \
	ncurses/wattron.o \
	ncurses/wattroff.o \
	ncurses/move.o \
	ncurses/wmove.o \
	ncurses/clear.o \
	ncurses/wclear.o \
	ncurses/erase.o \
	ncurses/werase.o \
	ncurses/delch.o \
	ncurses/wdelch.o \
	ncurses/clrtoeol.o \
	ncurses/wclrtoeol.o \

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

