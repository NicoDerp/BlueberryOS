
#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>


int printw(char* format, ...) {

    va_list args;
    va_start(args, format);
    int ret = printf(format, args);
    va_end(args);
    return ret;
}

