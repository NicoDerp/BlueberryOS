
#include <stdio.h>

#if defined(__is_libk)
#include <kernel/tty.h>
#endif

int putchar(int ic) {

#if defined(__is_libk)

    char c = (char) ic;
    terminal_writechar(c);

#else

// TODO implement stdio and write system call
#error "System calls aren't implemented yet"

#endif

    return ic;
}

