
#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>

int wprintw(WINDOW* win, const char* format, ...) {

    va_list args;
    va_start(args, format);

    int ret = vmvwprintw(win, win->cury, win->curx, format, args);

    va_end(args);

    return ret;
}

