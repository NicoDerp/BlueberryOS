
#include <ncurses.h>
#include <stdio.h>
#include <stdarg.h>

int mvwprintw(WINDOW* win, int y, int x, const char* format, ...) {

    move(win->starty + y, win->startx + x);

    // Goofy asf
    int chars = printf(format, *((unsigned int*) ((int) &format + sizeof(unsigned int))));

    win->curx += chars;
    win->cury += win->curx / win->width;
    win->curx %= win->width;
    win->cury %= win->height;

    return OK;
}

