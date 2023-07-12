
#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>

int mvwprintw(WINDOW* win, int y, int x, const char* format, ...) {

    va_list args;
    va_start(args, format);

    int ret = vmvwprintw(win, y, x, format, args);

    va_end(args);

    return ret;
}

