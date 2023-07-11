
#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>

int wprintw(WINDOW* win, const char* format, ...) {

    move(win->starty + win->cury, win->startx + win->curx);

    // Goofy asf
    int chars = printf(format, *((unsigned int*) ((int) &format + sizeof(unsigned int))));

    win->curx += chars;
    win->cury += win->curx / win->width;
    win->curx %= win->width;
    win->cury %= win->height;

    return OK;
}

