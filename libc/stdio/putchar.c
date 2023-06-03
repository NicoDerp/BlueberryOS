
#include <stdio.h>

#if defined(__is_libk)

#include <kernel/tty.h>

#else

#include <unistd.h>
#include <sys/syscall.h>

#endif

int putchar(int ic) {

#if defined(__is_libk)

    char c = (char) ic;
    terminal_writechar(c);

#else

    syscall(SYS_write, 1, ic, 1);

#endif

    return ic;
}

