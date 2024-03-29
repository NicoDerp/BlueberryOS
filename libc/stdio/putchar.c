
#include <stdio.h>

#if defined(__is_libk)

#include <kernel/tty.h>

#else

#include <unistd.h>
#include <sys/syscall.h>

extern int syscall3(int, int, int, int);

#endif

int putchar(int ic) {

#if defined(__is_libk)

    char c = (char) ic;
    terminal_writechar(c, true);

#else

    syscall3(SYS_write, STDOUT_FILENO, ic, 1);

#endif

    return ic;
}

