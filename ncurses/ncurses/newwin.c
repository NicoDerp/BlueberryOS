
#include <ncurses.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


WINDOW* newwin(unsigned int height, unsigned int width, unsigned int starty, unsigned int startx) {

    WINDOW* win;

    win = (WINDOW*) malloc(sizeof(WINDOW));
    win->curx = 0;
    win->curx = 0;
    win->width = width;
    win->height = height;
    win->startx = startx;
    win->starty = starty;

    win->toclear = 0;

    win->buf = (char*) malloc(width * height);
    win->lineschanged = (char*) malloc(height);

    memset(win->buf, ' ', width * height);
    memset(win->lineschanged, 0, height);

    return win;
}
