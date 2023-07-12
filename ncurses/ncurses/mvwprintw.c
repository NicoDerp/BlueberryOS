
#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>

int mvwprintw(WINDOW* win, int y, int x, const char* format, ...) {

    va_list args;
    va_start(args, format);

    move(win->starty + y, win->startx + x);

    int chars = vprintf(format, args);

    win->curx += chars;
    win->cury += win->curx / win->width;
    win->curx %= win->width;
    win->cury %= win->height;

    va_end(args);

    return OK;
}

