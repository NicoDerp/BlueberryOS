

# Default
CFLAGS ?= -O2 -g
LFLAGS ?= -O2
LIBS ?=

SYSROOT = /home/nico/Projects/blueberryos/sysroot
DESTDIR = $(SYSROOT)
PREFIX = /usr
EXEC_PREFIX = $(PREFIX)
INCLUDEDIR = $(PREFIX)/include
LIBDIR = $(EXEC_PREFIX)/lib

# Necessary
CC = i686-blueberryos-gcc
CFLAGS := $(CFLAGS) -ffreestanding -Wall -Wextra -Iinclude -D__is_libc
ASM_CFLAGS = -D__is_libc
LFLAGS := $(LFLAGS) -ffreestanding
#LIBS = -nostdlib -lk -lgcc
LIBS := $(LIBS) -nostdlib -lgcc
LIBK_CFLAGS := $(CFLAGS) -D__is_libk
LIBK_CFLAGS_ASM := -D__is_libk

ASM = nasm
ASMFLAGS = -felf32

AR = i686-elf-ar

FREEOBJS =\
	stdio/vprintf.o \
	stdio/printf.o \
	stdio/fprintf.o \
	stdio/putchar.o \
	stdio/puts.o \
	stdio/getc.o \
	stdio/getchar.o \
	stdio/fopen.o \
	stdio/fdopen.o \
	stdio/fclose.o \
	stdio/stdstreams.o \
	stdio/getline.o \
	stdlib/abort.o \
	stdlib/itoa.o \
	stdlib/uitoa.o \
	stdlib/atoi.o \
	stdlib/exit.o \
	stdlib/getenv_r.o \
	stdlib/setenv.o \
	stdlib/unsetenv.o \
	stdlib/malloc.o \
	stdlib/free.o \
	stdlib/realloc.o \
	stdlib/calloc.o \
	stdlib/abs.o \
	string/memcmp.o \
	string/memcpy.o \
	string/memmove.o \
	string/memset.o \
	string/strlen.o \
	string/strcmp.o \
	string/strncmp.o \
	string/strcpy.o \
	string/strncpy.o \
	string/strtok.o \
	string/strchr.o \
	string/strcat.o \
	string/strrchr.o \
	string/strstr.o \
	string/strrstr.o \
	string/strerror.o \
	unistd/syscall.o \
	unistd/syscall_asm.o \
	unistd/write.o \
	unistd/yield.o \
	unistd/read.o \
	unistd/fork.o \
	unistd/execvp.o \
	unistd/getcwd.o \
	unistd/chdir.o \
	unistd/close.o \
	unistd/getuid.o \
	unistd/setuid.o \
	unistd/ttycmd.o \
	fcntl/open.o \
	sys/wait/wait.o \
	sys/stat/stat.o \
	sys/stat/lstat.o \
	sys/mman/mmap.o \
	sys/mman/munmap.o \
	dirent/opendir.o \
	dirent/readdir.o \
	dirent/getdirentries.o \
	dirent/closedir.o \
	pwd/getpwuid_r.o \
	grp/getgrgid_r.o \
	errno/__errno_location.o \
	shadow/getspnam_r.o \
	ctype/isdigit.o \
	ctype/isspace.o \

HOSTEDOBJS =

OBJS = $(FREEOBJS) $(HOSTEDOBJS)

LIBK_OBJS = $(FREEOBJS:.o=.libk.o)

BINARIES = libc.so libc.a libk.a
LINK_OBJS = link/linkstart.o

# $(CC) -MMD -c $^ -o $@ -std=gnu11 $(CFLAGS) $(CPPFLAGS)

.PHONY: all build clean install install-headers install-libs
.SUFFIXES: .o .libk.o .c .asm

all:
	$(MAKE) -j`nproc` build
	$(MAKE) build
	$(MAKE) install

build: $(BINARIES) $(LINK_OBJS)

libc.so: $(OBJS)
	$(AR) rcs $@ $(OBJS)

libc.a: $(OBJS)
	$(AR) rcs $@ $(OBJS)
 
libk.a: $(LIBK_OBJS)
	$(AR) rcs $@ $(LIBK_OBJS)

.c.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(CFLAGS)

.asm.o:
	$(ASM) $(ASMFLAGS) $< -o $@ $(ASM_CFLAGS)

.c.libk.o:
	$(CC) -MD -c $< -o $@ -std=gnu11 $(LIBK_CFLAGS)

.asm.libk.o:
	$(ASM) $(ASMFLAGS) $< -o $@	$(LIBK_CFLAGS_ASM)


clean:
	rm -rf $(BINARIES) *.so *.a
	rm -rf $(OBJS) $(LIBK_OBJS)	*.o */*.o */*/*.o
	rm -rf $(OBJS:.o=.d) $(LIBK_OBJS:.o=.d) *.d */*.d */*/*.d

install: $(BINARIES) $(LINK_OBJS) install-headers install-libs install-link

install-headers:
	mkdir -p $(DESTDIR)$(INCLUDEDIR)
	cp -R --preserve=timestamps include/. $(DESTDIR)$(INCLUDEDIR)/.

install-libs:
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(BINARIES) $(DESTDIR)$(LIBDIR)

install-link: $(LINK_OBJS)
	mkdir -p $(DESTDIR)$(LIBDIR)
	cp $(LINK_OBJS) $(DESTDIR)$(LIBDIR)

-include $(OBJS:.o=.d)
-include $(LIBK_OBJS:.o=.d)

