
#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>


WINDOW* firstwin = NULL;

WINDOW* newwin(unsigned int height, unsigned int width, unsigned int starty, unsigned int startx) {

    WINDOW* win;

    win = (WINDOW*) malloc(sizeof(WINDOW));
    win->curx = 0;
    win->curx = 0;
    win->width = width;
    win->height = height;
    win->startx = startx;
    win->starty = starty;

    win->next = firstwin;

    if (firstwin)
        firstwin->prev = win;

    firstwin = win;

    win->buf = (char*) malloc(width * height);

    return win;
}
